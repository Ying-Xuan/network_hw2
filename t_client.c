#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<pthread.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>

#define PORT 8080

int client_sock;

char path[100];

char port[100];


void input_send(int socketfd)
{
    char input_str[10];

    fgets(input_str, 10, stdin);
    input_str[strlen(input_str)-1]='\0';
    send(socketfd, input_str, sizeof(input_str), 0);    
}

void recieve_print(int socketfd)
{
    char receive[100];

    recv(socketfd, receive, sizeof(receive), 0);
    printf("%s\n", receive);
    if( strcmp(receive, "Your password is wrong!!") == 0 )
	exit(1);
}


//send
void *sendsocket(void *arg)
{
    int st = *(int *)arg;
    char sendbuffer[100];
    char writebuffer[100];

    while(1){
        
        memset(sendbuffer, 0, sizeof(sendbuffer));

        scanf("%s",sendbuffer);

	if( strcmp(sendbuffer, "logout") == 0 ){

        	send(st, sendbuffer, strlen(sendbuffer), 0);
		close(st);
		exit(0);
	}

        send(st, sendbuffer, strlen(sendbuffer), 0);
    }

    return NULL;
}

//recieve
void *recvsocket(void *arg)
{
    int st = *(int *)arg;
    char receivebuffer[100];
    char writebuffer[100];
    int n;
    while(1){

        memset(receivebuffer, 0, sizeof(receivebuffer));
        n = recv(st, receivebuffer, sizeof(receivebuffer), 0);

        //whether the connect is end
        if(n<=0){
			printf("error");
        	return NULL;
		}

        printf("%s\n", receivebuffer);
    }
    return NULL;
}

int main() {

    char receive[100];

    //build socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(PORT);

    //connect
    client_sock = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(client_sock == 0)
        printf("connect waiting~~~!\n");	
    else{
	printf("can not connect , exit!\n");
	close(sock);
        exit(1);  
    }
/*-------------------------------------------------------------------------------------*/
    recieve_print(sock);  //print //enter account

    input_send(sock);  //enter account to ask for log in 
    
    recieve_print(sock);  //print //enter passwd

    input_send(sock);  //enter passwd

    recieve_print(sock);  //print result of log in 

/*-------------------------------------------------------------------------------------*/

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));

    int len = sizeof(client_addr);
    int ti = getsockname(sock, (struct sockaddr*)&client_addr, &len);

/*    
    sprintf(port,"%d",client_addr.sin_port);
    strcat(path,"./usernote/");
    strcat(path,port);
*/


    //build two thread, one for recieve, one for send 
    pthread_t thrd1, thrd2;
    pthread_create(&thrd1, NULL, sendsocket, &sock);
    pthread_create(&thrd2, NULL, recvsocket, &sock);
    pthread_join(thrd1, NULL);
    pthread_join(thrd2, NULL);

    //open file
    //close(fd);

    close(sock);

    return 0;
}
