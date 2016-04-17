#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "pump_client.h"
#include "pump_event.h"
#include "pump_config.h"

PumpClient::PumpClient(int fd, struct sockaddr_in * client) : mFD(fd)
{
    mRSSI = 0;
    mMACstring[0] = 0;

    char * hostaddrp = inet_ntoa(client->sin_addr);
    if (hostaddrp == NULL)
    {
        puts("ERROR on inet_ntoa\n");
        return;
    }

    printf("server established connection with PumpClient: %s \n", hostaddrp);

    /*
     * read: read input string from the client
     */
    char buf[256]; /* message buf */
    bzero(buf, 256);
    int n = read(mFD, buf, 256);
    if (n < 0)
    {
        printf("ERROR reading from socket");
        return;
    }
    printf("server received %d bytes\n", n);//: %s", n, buf);

    if(mDB.connect("localhost", "root", "VictorHanny"))
    {
        puts("Connected to pumplog DB");
    }

    int ackEvents = handleClient(buf);
    if(ackEvents)
    {
        sprintf(buf, "A%d\r\n", ackEvents);
        writeClient(buf);
    }

    if(mMACstring[0])
    {
        mDB.loggerLogin(mMACstring, mRSSI);

        PumpConfig cfg;
        //printf("\nCfg: %d %d %d %d %d\n", cfg.interval, cfg.startHour, cfg.stopHour, cfg.runTime, cfg.restTime);

        sprintf(buf, "Fp%d\r\n", cfg.interval);
        writeClient(buf);
        sprintf(buf, "Fs%d\r\n", cfg.startHour);
        writeClient(buf);
        sprintf(buf, "Fe%d\r\n", cfg.stopHour);
        writeClient(buf);
        sprintf(buf, "Fu%d\r\n", cfg.runTime);
        writeClient(buf);
        sprintf(buf, "Fr%d\r\n", cfg.restTime);
        writeClient(buf);

        sprintf(buf, "T%d\r\n", (int)time(0));
        writeClient(buf);
    }
}

int PumpClient::handleClient(char * buf)
{
    int ackEvents = 0;
    int argc = 16;
    char * argv[16];
    argc = parseArgs(buf, argc, argv, '\n', '\r');
    if(argc)
    {
        for(int k = 0; k < argc; k++)
          {
            if(handleLine(argv[k]))
                ackEvents++;
          }
    }

    return ackEvents;
}

int PumpClient::parseArgs(char * string, int argc, char ** argv, char c1, char c2)
{
    int found = 0;
    argv[found++] = string;
    while(string && (argc--))
    {
        char * i = strchr(string, c1);
        if(i)
        {
            *i = 0;
            argv[found++] = i + 1;
            string = i + 1;
        }
        else
            break;

        while((i[0] == c1) || (i[0] == c2))
                i++;
    }

    //printf("Found %d\n", found);
    return found;
}

bool PumpClient::handleLine(char * line)
{
    //printf("%s\n", line);

    if(!strncmp(line, "RSSI", 4))
    {
        int argc = 2;
        char * argv[2];
        argc = parseArgs(line, argc, argv, ':', ' ');
        if(argc > 0)
        {
            mRSSI = atoi(argv[1]);
            printf("RSSI(%d)\n" , mRSSI);
        }

        return false;
    }

    if(strchr(line, ':'))
    {
        int argc = 6;
        char * argv[6];
        argc = parseArgs(line, argc, argv, ':', ':');
        //printf("GOT mac %d\n", argc);

        if(argc > 5)
        {
            mMACstring[0] = 0;
            for(int k = 0; k < 6; k++)
            {
                //printf(argv[k]);

                char hex[4];
                int val = strtoul(argv[k], 0, 16);
                sprintf(hex, "%02X", val);
                strcat(mMACstring, hex);

                if(k < 5)
                    strcat(mMACstring, ":");
            }
            printf("MAC %s\n", mMACstring);
        }


        return false;
    }

    if(line[0] == 'R')
    {
        int port = atoi(&line[1]);
        printf("Port %d RESET\n", port);

        return false;
    }

    if(line[0] == 'S')
    {
        int port = atoi(&line[1]);
        printf("Port %d SET\n", port);

        return false;
    }

    if(strchr(line,','))
    {
        int argc = 6;
        char * argv[6];
        argc = parseArgs(line, argc, argv, ',', ',');
        //printf("GOT event %d\n", argc);
        if(argc > 2)
        {
            time_t stamp = atoi(argv[0]);
            uint8_t port = atoi(argv[1]);
            bool state = atoi(argv[2]);

            PumpEvent evt(stamp, port, state);
            evt.print();
            mDB.insertEvent(&evt);

            return true;
        }
    }

    return false;
}

bool PumpClient::writeClient(char * buf)
{
    if(!mFD)
        return false;

    int n = write(mFD, buf, strlen(buf));
    if (n < 0)
    {
        printf("ERROR writing to socket");
        return false;
    }

    return true;
}

PumpClient::~PumpClient()
{
    if(mFD)
        close(mFD);
}

