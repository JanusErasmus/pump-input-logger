#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pump_client.h"

int main(int argc, char ** argv)
{
    printf("Logger Daemon (%s %s) started\n", __DATE__, __TIME__);

//    /*
//     * check command line arguments
//     */
//    if (argc != 2) {
//      fprintf(stderr, "usage: %s <port>\n", argv[0]);
//      return (1);
//    }
    int portno = 60000;

    int parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if (parentfd < 0)
    {
        printf("ERROR opening socket\n");
        return 1;
    }

    int optval = 1; /* flag value for setsockopt */
    setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    struct sockaddr_in serveraddr; /* server's addr */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)portno);

    /*
     * bind: associate the parent socket with a port
     */
    if (bind(parentfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    {
        printf("ERROR on binding\n");
        return 1;
    }

    if(listen(parentfd, 5) < 0)
    {
        printf("ERROR on listen\n");
        return 1;
    }

    fd_set readfds;
    struct timeval waitThreshold;
    waitThreshold.tv_sec = 5000;
    waitThreshold.tv_usec = 50;


    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(parentfd, &readfds);
        int ret = select(parentfd + 1, &readfds, NULL, NULL, &waitThreshold);
        if(ret <0)
        {
            printf("\nSelect thrown an exception\n");
            break;
        }
        else if(FD_ISSET(parentfd, &readfds))
        {
            struct sockaddr_in clientaddr; /* client addr */
            int clientlen = sizeof(clientaddr);
            int childfd = accept(parentfd, (struct sockaddr *) &clientaddr, (socklen_t *)&clientlen);
            if (childfd < 0)
            {
                printf("ERROR on accept");
                break;
            }

            PumpClient(childfd, &clientaddr);
        }
    }

    close(parentfd);

    printf("DONE\n");
    return 0;
}
