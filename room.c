#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <signal.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<pthread.h>
#include<fcntl.h>
#include <sys/syscall.h> 
#include<sys/types.h>
#include<sys/stat.h>

#define PORT 8080

int serv_sock, client_sock;

typedef struct{
    char *name;
    int passwd;
    int client_fd;
    int gaming;
}account;

account users[5]={{"aaa",1, -1, 0}, {"bbb", 2, -1, 0}, {"ccc",3, -1, 0}, {"ddd", 4, -1, 0}, {"eee", 5, -1, 0}};

int chessboard[3][3]={0};

struct threadinfo
{
    int my_sock;
    int my_port;
};

//number of client
int usernum = 0;

int sock_array[10];

int name_find_idx(char account[])
{
    int i=0, j=0;
    for(i=0; i<5; i++){
    	if( strcmp(users[i].name, account) == 0 )
	    return i;
    }

    return -1;
}

int fd_find_idx(int fd)
{
    int i=0, j=0;
    for(i=0; i<5; i++){
    	if( users[i].client_fd == fd )
	    return i;
    }

    return -1;
}

void print_online()
{
    int i=0, empty=1;

    printf("\n");
    for(i=0; i<5; i++){
    	if(users[i].client_fd != -1){
	    printf("---%s is online.\tclient fd is %d.---\n", users[i].name, users[i].client_fd);
	    empty=0;
	}
    }
    
    if(empty)
	printf("---nobody is online!\n\n");
    else
	printf("\n\n");

}

void print_board()
{
	int i=0, j=0;
	for(i=0; i<3; i++){
		for(j=0; j<3; j++)
			printf("%d ", chessboard[i][j]);
		printf("\n");
	}
	printf("\n");
}

int check_result()
{
	int i=0, j=0;
	int tie=1;  //no result

	for(i=0; i<3; i++){
		if( chessboard[i][0]==chessboard[i][1] && chessboard[i][1]==chessboard[i][2] && chessboard[i][0]!=0 )
			return chessboard[i][0];
		if( chessboard[0][i]==chessboard[1][i] && chessboard[1][i]==chessboard[2][i] && chessboard[0][i]!=0 )
			return chessboard[0][i];
	}
	
	if( chessboard[0][0]==chessboard[1][1] && chessboard[1][1]==chessboard[2][2] && chessboard[0][0]!=0 )
		return chessboard[0][0];
	if( chessboard[0][2]==chessboard[1][1] && chessboard[1][1]==chessboard[2][0] && chessboard[0][2]!=0 )
		return chessboard[0][2];

	for(i=0; i<3; i++){
		for(j=0; j<3; j++){
			if(	chessboard[i][j]==0 ){
				tie=0;
				break;
			}
		}
	}

	if( tie==1 )
		return 0;

	return -1;
}

void *recvsocket(void *arg)
{
    struct threadinfo *thread = arg;
    int st = thread->my_sock, idx=fd_find_idx(st);
    int port = thread->my_port;
    char receivebuffer[100], portbuffer[100], st_buff[100], fd_buff[100], turn_mess[]="\nIt your turn.";
    int i, n;
    int length = sizeof(sock_array)/sizeof(sock_array[0]);
    char sendbuffer[100];
    int idx_challenged, fd_challenged;
    int result=0, row=0, col=0;  

    while(1){	

		recv(st, portbuffer, sizeof(portbuffer), 0);
		
		if( strcmp(portbuffer, "ask") == 0 ){  //ask for challenge
			strcpy(portbuffer, "Who you want to challenge?");
			send(st, portbuffer, sizeof(portbuffer), 0);

			memset(portbuffer, 0, sizeof(portbuffer));
			recv(st, portbuffer, sizeof(portbuffer), 0);  //receive the name challenged

			idx_challenged=name_find_idx(portbuffer);
			fd_challenged=users[idx_challenged].client_fd;

			strcpy(portbuffer, users[idx].name);
			strcat(portbuffer, " ask for challenging you.\nDo you want to accept?");

			//ask for challenging
			if( send(fd_challenged, portbuffer, sizeof(portbuffer), 0) == -1 ){  //if people challenged is not online
				strcpy(portbuffer, "people you want to challeng is not online.");
				send(st, portbuffer, strlen(portbuffer), 0);
				continue;
			}else {
				memset(portbuffer, 0, sizeof(portbuffer));
				recv(fd_challenged, portbuffer, sizeof(portbuffer), 0);  //receive the answer
				if( strcmp(portbuffer, "no")==0 ){
					strcpy(portbuffer, "He/She don't want to play with you.");
					send(st, portbuffer, strlen(portbuffer), 0);
					continue;
				}else{
					strcpy(portbuffer, "He/She accept your challenge!");
					send(st, portbuffer, strlen(portbuffer), 0);
					strcpy(portbuffer, "\n-------Game will begin!-------");
					send(fd_challenged, portbuffer, strlen(portbuffer), 0);
					send(st, portbuffer, strlen(portbuffer), 0);	
					users[idx].gaming=1;
					users[idx_challenged].gaming=1;
				}
			}
		
			printf("Game is begining!\n");
			
			while( 1 ){

				send(st, turn_mess, strlen(turn_mess), 0);
				memset(st_buff, 0, sizeof(st_buff));
				recv(st, st_buff, sizeof(st_buff), 0);
				row=st_buff[0]-'0';  col=st_buff[2]-'0';
				chessboard[row][col]=st;

				memset(fd_buff, 0, sizeof(fd_buff));
				strcpy(fd_buff, users[idx].name);
				strcat(fd_buff, " occupies ");
				strcat(fd_buff, st_buff);
				send(fd_challenged, fd_buff, strlen(fd_buff), 0);
				printf("%s\n", fd_buff);
				print_board();
				
				result=check_result();
				//printf("%d\n", result);
				if( result != -1 ){

					printf("%s is winner!\n", users[fd_find_idx(result)].name);

					if( result==st ){

						strcpy(st_buff, "You win the game!");

						send(st, st_buff, strlen(st_buff), 0);
						strcpy(fd_buff, "You lose the game QQ");
						send(fd_challenged, fd_buff, strlen(fd_buff), 0);
					}else if( result==fd_challenged ){
						strcpy(fd_buff, "You win the game!");
						send(fd_challenged, fd_buff, strlen(fd_buff), 0);
						strcpy(st_buff, "You lose the game QQ");
						send(st, st_buff, strlen(st_buff), 0);

					}else{  //it's a tie
						strcpy(fd_buff, "It's a tie.");
						send(fd_challenged, fd_buff, strlen(fd_buff), 0);
						strcpy(st_buff, "It's a tie.");
						send(st, st_buff, strlen(st_buff), 0);
					}

					break;
				}
				/*------------------------------------------------------------------*/
				send(fd_challenged, turn_mess, strlen(turn_mess), 0);
				memset(fd_buff, 0, sizeof(fd_buff));
				recv(fd_challenged, fd_buff, sizeof(fd_buff), 0);
				row=fd_buff[0]-'0';  col=fd_buff[2]-'0';
				chessboard[row][col]=fd_challenged;
				
				memset(st_buff, 0, sizeof(st_buff));
				strcpy(st_buff, users[idx_challenged].name);
				strcat(st_buff, " occupies ");
				strcat(st_buff, fd_buff);
				send(st, st_buff, strlen(st_buff), 0);
				printf("%s\n", st_buff);
				print_board();

				result=check_result();
				//printf("%d\n", result);
				if( result != -1 ){
					printf("%s is winner!\n", users[fd_find_idx(result)].name);

					if( result==st ){
						strcpy(st_buff, "You win the game!");
						send(st, st_buff, strlen(st_buff), 0);
						strcpy(fd_buff, "You lose the game QQ");
						send(fd_challenged, fd_buff, strlen(fd_buff), 0);
					}else if( result==fd_challenged ){
						strcpy(fd_buff, "You win the game!");
						send(fd_challenged, fd_buff, strlen(fd_buff), 0);
						strcpy(st_buff, "You lose the game QQ");
						send(st, st_buff, strlen(st_buff), 0);
					}else{  //it's a tie
						strcpy(fd_buff, "It's a tie.");
						send(fd_challenged, fd_buff, strlen(fd_buff), 0);
						strcpy(st_buff, "It's a tie.");
						send(st, st_buff, strlen(st_buff), 0);
					}
					break;
				}
			
	
			}
			

		}/*else if( strcmp(portbuffer, "logout") == 0 ){

			printf("%s log out!\n", users[idx].name);
			users[idx].client_fd=-1;
			print_online();
			break;
		
		}*/

    }

    return NULL;
}

void SIG_handler(int signo)
{
	close(client_sock);
    close(serv_sock);
	exit(0);
}

int main() {

    struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;
    pthread_t thrd[10];
    int len = sizeof(client_addr);
    struct threadinfo my_thread;
    struct threadinfo thread_array[10];
    int i;
    char portbuffer[100];


	signal(SIGINT, SIG_handler);

    //build socket
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT);

    //bind
    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind failed!");
        exit(1);
    }
    printf("bind succesfully\n");

    //begin to listen
    listen(serv_sock, 5);

    while(1){

        //accept() return value is new socket of client
        client_sock = accept(serv_sock, (struct sockaddr*)&client_addr, &len);

        if(client_sock < 0) {
            perror("connect failed!");
            exit(1);
        }

        printf("client %d want to connect.\n", client_addr.sin_port);

/*--------------------------------deal for "log in"-------------------------------------------*/
		strcpy(portbuffer, "please enter your name:");
		send(client_sock, portbuffer, sizeof(portbuffer), 0);

		memset(portbuffer, 0, sizeof(portbuffer));
		recv(client_sock, portbuffer, sizeof(portbuffer), 0);  //recieve name
		printf("%s ask for log in\n", portbuffer);
		
		int idx_acc=name_find_idx(portbuffer);
		memset(portbuffer, 0, sizeof(portbuffer));
		strcpy(portbuffer, "please enter your password:");
		send(client_sock, portbuffer, sizeof(portbuffer), 0);
		
		memset(portbuffer, 0, sizeof(portbuffer));
		recv(client_sock, portbuffer, sizeof(portbuffer), 0);  //recieve password
		
		if( atoi(portbuffer) == users[idx_acc].passwd ){  //log in successfully

			printf("%s log in successfully.\n", users[idx_acc].name);
			strcpy(portbuffer, "~~~Log in successfully.~~~\nEnter 'ask' to challenge other people or enter 'logout' to log out");
			send(client_sock, portbuffer, sizeof(portbuffer), 0);  //send successful message

			users[idx_acc].client_fd=client_sock;
			print_online();


		/*-------------------------------------------------------*/
			my_thread.my_sock = client_sock;
			my_thread.my_port = client_addr.sin_port;
			thread_array[usernum] = my_thread;

			sock_array[usernum] = client_sock;

			pthread_create(&thrd[usernum], NULL, recvsocket, &thread_array[usernum]);

			usernum++;
		
		/*-------------------------------------------------------*/

		}else{  //fail to log in

			printf("%s enter wrong password!\n", users[idx_acc].name);
			print_online();
			strcpy(portbuffer, "Your password is wrong!!");
			send(client_sock, portbuffer, sizeof(portbuffer), 0);
		}
/*-------------------------------------------------------------------------------------*/

    }

    //clode socket
    close(client_sock);
    close(serv_sock);

    return 0;
}
