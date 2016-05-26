#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <list>
#include <string>
#define STDIN 0
#define MAXDATASIZE 1024

using namespace std;
class HOST{
protected:
  listClients<sockinfo> clients;
  char lastcmd[500];
  char input[1024];
  char IP[256];
  char ListenPORT[1024];
  int listener;
  int yes;        // for setsockopt() SO_REUSEADDR, below
  int rv;
  struct addrinfo hints, *ai, *p;

public:
  HOST(){
    yes = 1;
  }
  char *getHost(char *IP){
    struct hostent *he;
    struct in_addr stIP;
    if(!inet_aton(IP,&stIP)){
      return NULL;
    }

    if ((he = gethostbyaddr((const void *)&stIP,strlen(IP),AF_INET) )== NULL) {
        herror("gethostbyname");
    }
    return he->h_name;
  }


  // get sockaddr, IPv4 or IPv6:
  void *get_in_addr(struct sockaddr *sa)
  {
      if (sa->sa_family == AF_INET) {
          return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }


  int sendall(int sendrSockFd, int s, char *buf, int *len, listClients<sockinfo> clients)
  {
    if(clients.isIPInBlockLst(sendrSockFd,s)){
      return 0;
    }

    int total = 0; // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
    while(total < *len) {
      n = send(s, buf+total, bytesleft, 0);
      if (n == -1) { break; }
      total += n;
      bytesleft -= n;
    }
    *len = total; // return number actually sent here
    return n==-1?-1:0; // return -1 on failure, 0 on success
  }
  void getadd(){

      struct hostent *he;
      struct in_addr **addr_list;
      char hostname[100];
      gethostname(hostname,1024); ///fills ip add

      if ((he = gethostbyname(hostname) )== NULL) {  // get the host info
          herror("gethostbyname");
      }

      // save IP of this host:
      addr_list = (struct in_addr **)he->h_addr_list;
      for(int i = 0; addr_list[i] != NULL; i++) {
          strcpy(IP,inet_ntoa(*addr_list[i]));
      }
  }
  void PrintError(char *cmmnd){
    cse4589_print_and_log("[%s:ERROR]\n", cmmnd);
    cse4589_print_and_log("[%s:END]\n", cmmnd);
  }
  void PrintSuccess(char *cmmnd){
    cse4589_print_and_log("[%s:SUCCESS]\n", cmmnd);
    cse4589_print_and_log("[%s:END]\n", cmmnd);
  }
  void PrintLIST(){
    list<sockinfo*> ls = clients.getObjList();
    int iter = 1;
    for(list<sockinfo*>::iterator it = ls.begin(); it != ls.end(); ++it){
      cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", iter++, (*it)->host, (*it)->IP, (*it)->PORT);
    }
  }
  void PrintSTATS(){
    list<sockinfo*> ls = clients.getObjList();
    int iter = 1;
    for(list<sockinfo*>::iterator it = ls.begin(); it != ls.end(); ++it){
      cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", iter++, (*it)->host, (*it)->SntCnt, (*it)->RcvCnt, (*it)->status);
    }
  }

  int isValidPort(char *PRT){
    if (strlen(PRT)>4) {
      return 0;
    }
    for(int i = 0;i < 4;i++){
      if(!isdigit(PRT[i])){
        return 0;
      }
    }
    return 1;
  }
  int isValidIP(char *cIP){
    return ((!nonExistntIP(cIP))&&(!inValidIP(cIP)));
  }
  int nonExistntIP(char *IP){
    if(clients.getObjByIP(IP)==clients.getLastAdd()){
      return 1;
    }
    return 0;
  }
  int inValidIP(char *cIP){
    int lnth = strlen(cIP);
    if(lnth>16)
      return 1;
    int len = 0,numofDig = 0,numOfDelim = 0;
    for(;len<lnth;len++){
      if(cIP[len]=='.'){ // IF DELIMITER FOUND
        if(numofDig<1||numofDig>3){
          return 1;
        }
        numofDig=0;
        numOfDelim++;
      }
      else if((!isdigit(cIP[len]))){ // IF NOT DELIMITER AND NOT A DIGIT
        return 1;
      }
      else{ // IF IT IS A DIGIT
        numofDig++;
      }
    }
    if(numOfDelim<3){
      return 1;
    }
    return 0;
  }
  void RegListenPort(){

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    // GET INFORMATION OF THE SYSTEM PORT AND ADDRESS
    if ((rv = getaddrinfo(NULL, ListenPORT, &hints, &ai)) != 0) {
      //cse4589_print_and_log(stderr, "selectserver: %s\n", gai_strerror(rv));
      exit(1);
    }
    for(p = ai; p != NULL; p = p->ai_next) {
      listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (listener < 0) {
        continue;
      }
      // lose the pesky "address already in use" error message
      setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
      if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
          close(listener);
          continue;
      }
      break;
    }
    // if we got here, it means we didn't get bound
    if (p == NULL) {
      //cse4589_print_and_log(stderr, "selectserver: failed to bind\n");
      exit(2);
    }
    freeaddrinfo(ai); // all done with this
    // listen
    if (listen(listener, 10) == -1) {
      perror("listen");
      exit(3);
    }
  }
  void PrintAuthor(){
    cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "AUTHOR");
    cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", "keshavsh");
    cse4589_print_and_log((char *)"[%s:END]\n", "AUTHOR");
  }
  void PrintPort(){
    cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "PORT");
    cse4589_print_and_log("PORT:%s\n", ListenPORT);
    cse4589_print_and_log((char *)"[%s:END]\n", "PORT");
  }
  void PrintIP(){
    cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "IP");
    cse4589_print_and_log("IP:%s\n", IP);
    cse4589_print_and_log((char *)"[%s:END]\n", "IP");
  }

};



class RegisServer : public HOST{
public:
  void setPORT(char *port){
    strcpy(ListenPORT,port);
  }
  RegisServer(char *port){
    //FILLS SYSTEMS IP ADDRESS
    getadd();
    setPORT(port);
    RegListenPort();
    connectServer();
  }
void connectServer(){
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax,newfd;
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[256], remoteIP[INET6_ADDRSTRLEN];
    int nbytes, i, j;
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
    FD_SET(listener, &master);
    FD_SET(STDIN, &master);
    fdmax = listener; // so far, it's this one

    for(;;) {
      read_fds = master;
      if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
          perror("select");
          exit(4);
      }
      for(i = 0; i <= fdmax; i++) {
        memset( (void *)&buf, '\0', sizeof(buf));

        if (FD_ISSET(i, &read_fds)) { // we got one!!
          if(i==STDIN){
            read(STDIN,buf, MAXDATASIZE);
            buf[strlen(buf)-1]='\0';
            if(strcmp(buf,"AUTHOR")==0){
              PrintAuthor();
            }
            else if(strcmp(buf,"IP")==0){
              PrintIP();
            }
            else if(strcmp(buf,"PORT")==0){
              PrintPort();
            }
            else if((strncmp(buf,"BLOCKED",7)==0)){
              char clientIP[100],cmmnd[100];
              BreakInTwo(buf,cmmnd,clientIP);
              if(!isValidIP(clientIP)){
                PrintError(cmmnd);
                continue;
              }
              cse4589_print_and_log("[%s:SUCCESS]\n", buf);
              list<string> blockedLst = clients.getBlockedList(clients.getfdByIP(clientIP));
              for(list<string>::iterator blocked = blockedLst.begin(); blocked!=blockedLst.end();++blocked){
                if (send(i, (*blocked).c_str(), (*blocked).length(), 0) == -1) {
                  perror("send");
                }
              }
              cse4589_print_and_log("[%s:END]\n", buf);
            }
            else if((strcmp(buf,"STATISTICS")==0)){
              cse4589_print_and_log("[%s:SUCCESS]\n", buf);
              PrintSTATS();
              cse4589_print_and_log("[%s:END]\n", buf);
            }
            else if((strcmp(buf,"LIST")==0)){
              cse4589_print_and_log("[%s:SUCCESS]\n", buf);
              PrintLIST();
              cse4589_print_and_log("[%s:END]\n", buf);
            }

          }
          else if (i == listener) {
                // handle new connections
                addrlen = sizeof remoteaddr;
                newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen);
                if (newfd == -1) {
                    perror("accept");
                } else {
                    FD_SET(newfd, &master); // add to master set
                    if (newfd > fdmax) {    // keep track of the max
                        fdmax = newfd;
                    }
                    sockaddr_in *clientadd = (sockaddr_in *)get_in_addr((struct sockaddr*)&remoteaddr);
                    inet_ntop(remoteaddr.ss_family, clientadd, remoteIP, INET6_ADDRSTRLEN);
                    // ADD INFORMATION OF NEWLY ADDED CLIENT
                    char *hostnme =getHost(remoteIP);
                    clients.add(remoteIP, (char *)"", newfd, hostnme, -1,(char *)"ONLINE");
                    cse4589_print_and_log("connection IP port: %s, %d\n", remoteIP, clientadd->sin_port);

                }


            } else {
                // handle data from a client
                if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                    // got error or connection closed by client
                    if (nbytes == 0) {
                        // connection closed
                        clients.setOffline(i);
                    } else {
                        perror("recv");
                    }
                    close(i); // bye!
                    FD_CLR(i, &master); // remove from master set
                } else {
                  char cmmnd[256];
                  memset(cmmnd, 0, sizeof cmmnd);
                  char recvIP[256];
                  char msg[256];
                  BreakInThree(buf,cmmnd,recvIP,msg);
                  //ADDYING LAST ADDED CLIENTS PORT NUMBER IN LIST
                  if((strncmp(buf,"SETCLNTPORT",11)==0)){
                    clients.addPORT(i,recvIP); //RECVIP WILL CONTAIN PORT NUMBER HERE WHICH WILL BE ADDED WHEN CLIENT LOGS IN

                    //SEND THE LIST OF ALL THE LOGGEDIN CLIENTS
                    list<string> currOnline = clients.getAllLogedIn();
                    if(!currOnline.empty()){
                      for(list<string>::iterator clnt = currOnline.begin(); clnt!=currOnline.end();++clnt){
                        int len = ((*clnt).length());
                        int ret = sendall(i, newfd, (char *)((*clnt).c_str()), &len,clients);
                        if (ret == -1) {
                            perror("send");
                        }
                      }
                    }
                    if (send(i, "\n", 1, 0) == -1) {
                      perror("send");
                    }
                  }else if((strcmp(cmmnd,"BUFFERMSG")==0)){
                    list<string> msgs = clients.getPendMsg(remoteIP);
                    if(!msgs.empty()){
                      for(list<string>::iterator msg = msgs.begin(); msg!=msgs.end();++msg){
                        int len = ((*msg).length());
                        int ret = sendall(i, newfd, (char *)((*msg).c_str()), &len,clients);
                        if (ret == -1) {
                            perror("send");
                        }
                      }
                    }

                  }else if((strcmp(cmmnd,"SEND")==0)){
                    if(!isValidIP(recvIP)){
                      PrintError(cmmnd);
                      continue;
                    }
                    strcpy(cmmnd,"RELAYED");
                    cse4589_print_and_log("[%s:SUCCESS]\n", cmmnd);
                    clients.incSndCnt(i);
                    char sender[32];
                    strcpy(sender,clients.getIPByfd(i));
                    char msgs[256];
                    strcpy(msgs,msg);
                    cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",sender,recvIP,msgs);
                    //ADDYING IP ADDRESS OF SENDER TO THE MSG STRING FOR CLIENT
                    string msgStr(sender);
                    msgStr += " ";
                    msgStr += msgs;
                    strcpy(msg,msgStr.c_str());
                    if(clients.available(recvIP)){
                      int fd = clients.getfdByIP(recvIP);
                      clients.incRcvCnt(fd);
                      int len = strlen(msg);
                      int ret = sendall(i,fd,msg,&len,clients);
                      if (ret == -1) {
                          perror("send");
                      }
                    }
                    else{
                      clients.addInRecpBuff(recvIP,msg);
                    }
                    cse4589_print_and_log("[%s:END]\n", cmmnd);
                  }
                  else if((strcmp(cmmnd,"BLOCK")==0)){
                    cse4589_print_and_log("[%s:SUCCESS]\n", cmmnd);
                    clients.block(i,recvIP);
                    cse4589_print_and_log("[%s:END]\n", cmmnd);
                  }
                  else if((strcmp(cmmnd,"UNBLOCK")==0)){
                    cse4589_print_and_log("[%s:SUCCESS]\n", cmmnd);
                    clients.unBlock(i,recvIP);
                    cse4589_print_and_log("[%s:END]\n", cmmnd);
                  }
                  else if((strcmp(buf,"REFRESH")==0)){
                    list<string> clntsLst = clients.getList();
                    if (FD_ISSET(i, &master)){
                      for(list<string>::iterator clnts = clntsLst.begin(); clnts!=clntsLst.end();++clnts){
                        if (send(i, (*clnts).c_str(), (*clnts).length(), 0) == -1) {
                          perror("send");
                        }
                      }
                    }
                  }
                  else if((strcmp(cmmnd,"BROADCAST")==0)){
                    strcpy(cmmnd,"RELAYED");
                    cse4589_print_and_log("[%s:SUCCESS]\n", cmmnd);
                    clients.incSndCnt(i);
                    char msgs[256];
                    //BreakInTwo(buf,cmmnd,msgs);
                    strcpy(msgs,recvIP);
                    for(j = 0; j <= fdmax; j++) {
                      if (FD_ISSET(j, &master)) {

                      if (j != listener && j != i  && j!=STDIN) {
                        char recvr[1024],sendr[1024];
                        strcpy(recvr,clients.getIPByfd(j));
                        strcpy(sendr,clients.getIPByfd(i));
                        cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", sendr, recvr, msgs);
                        if(clients.available(recvr)){
                          clients.incRcvCnt(j);
                          string msgStr(sendr);
                          msgStr += " ";
                          msgStr += msgs;
                          strcpy(msgs,msgStr.c_str());
                          int len = strlen(msgs);
                          int ret = sendall(i, j, msgs, &len,clients);
                          if (ret == -1) {
                            perror("send");
                          }
                        }
                        else{
                          clients.addInRecpBuff(clients.getIPByfd(j),msg);
                        }

                      }
                    }
                    }
                    cse4589_print_and_log("[%s:END]\n", cmmnd);
                  }
/*                  else {
                    PrintError(cmmnd);
                  }
*/               }

              } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
 }

};
/*int main(int argc, char *argv[])
{

  if(argc<2)
    return 0;
  if(*argv[1]=='s')
    RegisServer SRVR(argv[2]);
  return 0;
}
*/
