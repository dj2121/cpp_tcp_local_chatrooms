#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netdb.h>
#include<signal.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(){

    int sockfd, portno, n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent *server; 
    struct sockaddr_in serv_addr;

    printf("Enter server port: ");
    scanf("%d", &portno);

    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        error("ERROR opening socket");
    }

    server = gethostbyname("localhost");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
        error("ERROR connecting");
    }

    //Get input string  
    bzero(buffer, 256);

    char *qStr = "/quit";
    char *ttt;
    size_t tttt;
    getline(&ttt, &tttt, stdin);

    char target[5] = "recv";

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    int pid = fork();

    if(pid > 0){
        //server process that reads incoming messages
        while(1){

            int k = write(sockfd, target, strlen(target));
            //Read the response  
            bzero(buffer, 256);
            n = read(sockfd, buffer, 255);   

            if(strlen(buffer) > 1)             
                printf("\nMessage Received: %s\n\n\n",buffer);

        }
    }

    else if(pid == 0){

        while(1){   

            char buffer[256];
            char *b = buffer;
            size_t bufsize = 255; 
            getline(&b, &bufsize, stdin);

            if(strstr(buffer, qStr) == buffer){
                printf("Quit request received. Quitting.");

                //Write to the connection
                n = write(sockfd, buffer, strlen(buffer));
                if (n < 0){
                    error("ERROR writing to socket");
                }

                kill(pid, SIGKILL);
                break;
            }

            //Write to the connection
            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0){
                error("ERROR writing to socket");
            }

        }

    }

    close(sockfd);    
    return 0;

}