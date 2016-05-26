/**
 * @ubitname_assignment1
 * @author  Fullname <ubitname@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../include/global.h"
#include "../include/logger.h"
//#include "logger.cpp"
#include "../include/ClientLst.h"
#include "../include/server.h"
#define STDIN 0
#define MAXDATASIZE 1024
using namespace std;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
class RegisClnt : public HOST{
public:
    void setPORT(char *port){
        strcpy(ListenPORT,port);
    }
    RegisClnt(char *port){
        setPORT(port);
        getadd();
        RegListenPort();
        while(strcmp(input,(char *)"EXIT")!=0){
            if(strcmp(input,(char *)"AUTHOR")==0){
                PrintAuthor();
             } else if(strcmp(input,(char *)"IP")==0){
               PrintIP();
             } else if(strcmp(input,(char *)"PORT")==0){
               PrintPort();
             } else if(strncmp(input,"LOGIN",5)==0){
                char cmmnd[16],serverIP[32],serverPort[32];
                BreakInThree(input,cmmnd,serverIP,serverPort);
                if((!inValidIP(serverIP)) && isValidPort(serverPort)){
                  // SAVING THE LAST COMMAND TO SAVE THE LIST RETURNED FROM THE SERVER AFTER LOGGING
                  strcpy(lastcmd,cmmnd);
                  LogInServer(serverIP,serverPort);
                  if(strcmp(lastcmd,"EXIT")==0) // IN CASE THE EXIT IS CALLED WHEN CLIENT WAS LOGGED IN
                    break;
                }
                else{
                  PrintError((char *)"LOGIN");
                }
            }
            string in;
            getline (cin,in);
            strcpy(input,in.c_str());
        }
        if((strcmp(input,(char *)"EXIT")!=0)||(strcmp(lastcmd,"EXIT")==0)){
          cse4589_print_and_log((char *)"[%s:SUCCESS]\n", (char*)"EXIT");
          cse4589_print_and_log((char *)"[%s:END]\n", (char*)"EXIT");
        }
    }

    int LogInServer(char *serverIP, char *serverPort){
        //  list<sockinfo*> socklist;
        int sockfd, numbytes;
        char buf[MAXDATASIZE];
        struct addrinfo hints, *servinfo, *p;
        int rv;
        char s[INET6_ADDRSTRLEN];
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        // GET SERVER ADDRESS INFORMATION
        if ((rv = getaddrinfo(serverIP, serverPort, &hints, &servinfo)) != 0) {
          PrintError((char *)"LOGIN");
          return 1;
        }

        // FIND THE FIRST VALID SERVER IP AND CONNECT
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
                continue;
            }
            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                continue;
            }
            break;
        }
        // IF NO VALID SERVER IP ADDRESS FOUND RETURN TO THE CONSTRUCTOR OF CLASS
        if (p == NULL) {
          PrintError((char *)"LOGIN");
          return 2;
        }

        // FREE THE ADDRESS INFORMATION OF THE SERVER
        freeaddrinfo(servinfo); // all done with this structure
        struct timeval tv;
        fd_set readfds;
        tv.tv_sec = 2;
        tv.tv_usec = 500000;

        strcpy(buf,(char*)"SETCLNTPORT");
        strcat(buf," ");
        strcat(buf,ListenPORT);
        if(send(sockfd, buf, strlen(buf), 0) < 0){
          PrintError((char *)"LOGIN");
          return 1;
        }
        cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "LOGIN");
        cse4589_print_and_log((char *)"[%s:END]\n", "LOGIN");
        //SOCKFD SOCKET OF THE SERVER
        for(;;) {
            FD_ZERO(&readfds);
            FD_SET(STDIN, &readfds);
            FD_SET(sockfd, &readfds);
            int newfd,fdmax = sockfd;
            // LATER NEED TO ADD THE LISTENER AND ADD MORE CONNECTIONS
            if (select(fdmax+1, &readfds, NULL, NULL, &tv) == -1) {
                PrintSuccess((char *)"LOGOUT");//perror((char *)"select");
                return 4;
            }
            // IF IT IS THE STANDARD INPUT
            if(FD_ISSET(STDIN, &readfds)){
                memset( (void *)&buf, '\0', sizeof(buf));
                read(STDIN,buf, MAXDATASIZE);

                buf[strlen(buf)-1]='\0';
                strcpy(lastcmd,buf);
                fflush(stdin);
                if(strcmp(buf,"LOGOUT")==0){
                    cse4589_print_and_log((char *)"[%s:SUCCESS]\n", buf);
                    close(sockfd);
                    cse4589_print_and_log((char *)"[%s:END]\n", buf);
                    return 0;
                }
                if(strcmp(buf,"AUTHOR")==0){
                    cse4589_print_and_log((char *)"[%s:SUCCESS]\n", buf);
                    PrintAuthor();
                    cse4589_print_and_log((char *)"[%s:END]\n", buf);
                }
                else if(strcmp(buf,"IP")==0){
                    cse4589_print_and_log((char *)"[%s:SUCCESS]\n", buf);
                    PrintIP();
                    cse4589_print_and_log((char *)"[%s:END]\n", buf);
                }
                else if(strcmp(buf,"PORT")==0){
                    cse4589_print_and_log((char *)"[%s:SUCCESS]\n", buf);
                    PrintPort();
                    cse4589_print_and_log((char *)"[%s:END]\n", buf);
                }
                else if(strcmp(buf,"REFRESH")==0){
                  cse4589_print_and_log((char *)"[%s:SUCCESS]\n", buf);
                  if(send(sockfd, "REFRESH", strlen((char *)"REFRESH"), 0) < 0){
                     perror((char *)"send");
                  }
                  cse4589_print_and_log((char *)"[%s:END]\n", buf);
                }
                else if(strcmp(buf,"LIST")==0){
                  PrintLIST();
                }
                else if(strcmp(buf,"EXIT")==0){
                  strcpy(lastcmd,"EXIT");
                  close(sockfd);
                  return 0;
                }
                else if((strncmp(buf,"SEND",4)==0)||(strncmp(buf,"BROADCAST",9)==0)){
                  char cmmnd[500], client_ip[500], msg[500];
                  BreakInThree(buf,cmmnd,client_ip,msg);
                  if(((strcmp(cmmnd,"SEND")==0)&&(!isValidIP(client_ip)))||(strcmp(cmmnd,"BROADCAST")!=0)){
                    PrintError(cmmnd);
                    continue;
                  }

                  if(send(sockfd, buf, strlen(buf), 0) < 0){
                     PrintError(cmmnd);//perror((char *)"send");
                  }else{
                    PrintSuccess(cmmnd);
                  }
                }


            }

            // MESSAGE FROM THE SERVER
            else  if (FD_ISSET(sockfd, &readfds)&&((numbytes = recv(sockfd, buf, sizeof buf, 0)) <= 0)) {
                if (numbytes == 0) {
                    PrintSuccess((char *)"LOGOUT");//cse4589_print_and_log((char *)"selectserver: socket %d hung up\n", sockfd);
                    close(sockfd);
                } else {
                    perror((char *)"recv");
                }
            } else if(FD_ISSET(sockfd, &readfds)){
                // WHEN LOGGING IN, TAKING UPDATED LIST OF ALL THE OTHER CURRENTLY LOGGEDIN CLIENTS
                if(strcmp(lastcmd,"REFRESH")==0||strcmp(lastcmd,"LOGIN")==0){
                    if(strncmp(buf,"\n",strlen(buf))==0){
                      memset( (void *)&buf, '\0', sizeof(buf));
                      // ASK SERVER TO SEND ANY BUFFER MESSAGES
                      strcpy(buf,"BUFFERMSG");
                      if(send(sockfd, buf, strlen(buf), 0) < 0){
                        perror((char *)"send");
                      }
                      continue;
                    }
                    char idd[500],hostname[500],ip_addr[500],prt[500];
                    BreakInFour(buf,idd,hostname, ip_addr, prt);
                    clients.add(ip_addr, (char *)"", -1, hostname, strToInt(prt));
                }
                else{
                  //RECIEVE MSGS AND SEND PORT ADDRESS
                    char client_ip[500],msg[500];
                    BreakInTwo(buf,client_ip,msg);
                    char cmmnd[50];
                    strcpy(cmmnd,"RECEIVED");
                    if(isValidIP(client_ip)){
                      cse4589_print_and_log((char *)"[%s:SUCCESS]\n", cmmnd);
                      cse4589_print_and_log((char *)"msg from:%s\n[msg]:%s\n", client_ip, msg);
                    }
                    else{
                      cse4589_print_and_log((char *)"[%s:ERROR]\n", cmmnd);
                    }
                    cse4589_print_and_log((char *)"[%s:END]\n", cmmnd);
                    lastcmd[0]='\0';
                }
            } // END handle data from client
        } // END for(;;)--and you thought it would never end!


    }



};

int main(int argc, char **argv)
{
    if(argc<=2)
        return 0;
    /*Init. Logger*/
   	cse4589_init_log(argv[2]);

   	/* Clear LOGFILE*/
    fclose(fopen(LOGFILE, "w"));
    if(*argv[1]=='c')
        RegisClnt CLNT(argv[2]);
    else if(*argv[1]=='s')
        RegisServer SRVR(argv[2]);

    return 0;
}
