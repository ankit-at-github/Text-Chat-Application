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
#include "../include/packUnpack.h"
#include <vector>
#include <map>
#include <string>
#include <queue>
#include <fstream>
#define STDIN 0
#define MAXDATASIZE 1024
#define INF ((1<<16)-1)
#define resetTime(x) x->Time = x->period//time(NULL);
#define remainTime(x) x->Time//((x->Time) + (x->period)) - time(NULL)

using namespace std;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */


 #define sendPacket(newpack, newfd) \
 { \
     if(send(newfd, newpack, sizeof(newpack), 0) < 0) \
     { \
         cout<<"FAILED :: Sending ::"<<(newpack)<<endl; \
     }else \
     { \
         cout<<"SUCCESS :: Sending ::"<<(newpack)<<endl; \
     } \
 }



class HOST{
protected:
    char IP[256];
    int newfd;
    int routrListener;
    int dataListener;

public:
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

    int createListenPort(char *ListenPORT, int connectType){
      cout<<"Creating Socket Listener Now with Port ::"<<ListenPORT<<endl;
      int listener;
      int yes = 1;        // for setsockopt() SO_REUSEADDR, below
      int rv;
      struct addrinfo hints, *ai, *p;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = connectType;
        hints.ai_flags = AI_PASSIVE;

        if ((rv = getaddrinfo(NULL, ListenPORT, &hints, &ai)) != 0) {
            cout<<"FAILED :: Cannot get address info for New Listener"<<endl;
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
              cout<<"FAILED :: Cannot Bind New Listener"<<endl;
                close(listener);
                continue;
            }
            break;
        }

        if (p == NULL) {
            exit(2);
        }
        freeaddrinfo(ai); // all done with this
        if (connectType != SOCK_DGRAM && listen(listener, 10) == -1) {
            cout<<"FAILED :: Cannot listen with New Listener"<<endl;
            perror("listen");
            exit(3);
        }
        cout<<"SUCCESS :: Creating Socket Listener Now"<<endl;
        return listener;
    }

};


struct IDInfo
{
  unsigned int nextHopID, DestID, DestRouterPort, DestDataPort, cost;
  unsigned long DestIP;
  long int Time, period;
  int sockfd, missedCnt;
  char DestIPstr[40];
};

  struct min_comparator {
    bool operator()(IDInfo* i, IDInfo* j) {
      return (*i).Time > (*j).Time;
    }
  };

  struct max_comparator {
    bool operator()(IDInfo* i, IDInfo* j) {
      return (*i).Time < (*j).Time;
    }
  };
/************* Store neighbours and the cost ****************/
class neighbours {
  public :
   unsigned int numOfNeigh, UpdateInterval;
   struct IDInfo* OwnIDinfo;
   map<long, struct IDInfo*> DestbyIDInfoMap;
   map<long, struct IDInfo*> DestbyIPInfoMap;
   unsigned char lastSentPack[1000];
   unsigned char secLastSentPack[1000];

  neighbours()
  {
     memset(lastSentPack, 0, 1000);
     memset(secLastSentPack, 0, 1000);
     OwnIDinfo = new (struct IDInfo)();
     cout<<"//************* Allocated space :: OwnIDInfo ****************//"<<endl;
  }
  priority_queue<IDInfo*, vector<IDInfo*>, min_comparator> MinHeap_MinTimeFor_Timer_Restrt;
  priority_queue<IDInfo*, vector<IDInfo*>, max_comparator> MaxHeap_To_set_Router_INF;

  int connectUDP(char *IP, int port)
  {
    cout<<"//************* Connecting UDP connection for IP ::"<<IP<<" *****************//"<<endl;
    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_DGRAM;

    char prt[100];
    sprintf(prt, "%d", port);
    cout<<"With IP Port Number ::"<<prt<<endl;
    if ((getaddrinfo((const char*)IP, prt, &hints, &res)) != 0) {
        return -1;
    }
    if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
    {
       return -1;
    }
    connect(sockfd, res->ai_addr, res->ai_addrlen);
    return sockfd;
  }
// *************************  Work Done by Reset Timer *************************
/* 1. If not a neighbour Don't Check
   2. Renew Time remaining for each Node
   3. Send Routing Table If Self TimeOut
   4. Check for Neighbour Router Not sending regular update
   5. Restart Timer if Time Exceeded
*/
  void resetTimers(int lastTimer)
  {
    for(map<long, struct IDInfo*>::iterator it = DestbyIDInfoMap.begin(); it != DestbyIDInfoMap.end(); ++it)
    {
        IDInfo* temp = ((*it).second);
        if(temp->Time >= INF)
        {
           continue;
        }
        temp->Time -= lastTimer;
        if(temp->cost == 0) ifTimeToSendRouting();

        if(!MaxHeap_To_set_Router_INF.empty()) checkNeighTimer();

        if(temp->Time <= 0)
        {
           cout<<"Resetting Time of the Node ::"<<(*temp).DestID<<" to Time ::"<<(temp->period)<<endl;
           temp->Time = temp->period;
           ((*it).second) = temp;
        }
    }
  }

  //*************** Timer Interrupt 1. If need to send routing info 2. If any neighbours timer expire ******************//
  int TimerInterrupt(int lastTimer)
  {
     resetTimers(lastTimer);
     if(OwnIDinfo->Time == 0)
     {
        cout<<"// *********** OwnIDinfo not yet initialized ************ //"<<endl;
        return 4204283;
     }
     return getMinTimer();
  }

  void ifTimeToSendRouting()
  {
     if( remainTime(OwnIDinfo) <= 0 )
     {
        sendRoutingTableToNeigh();
        cout<<" Remain Time of the OwnIDinfo after Reset ::"<<remainTime(OwnIDinfo)<<endl;
     }
  }

  //******************* If time exceeded by 3 times update cost to infinity ********************//
  // TODO :: initialize missedCnt to 0 if received an input
  void checkNeighTimer()
  {
     IDInfo* ID = MaxHeap_To_set_Router_INF.top();
     if( remainTime(ID) <= 0)
     {
        if( (*ID).missedCnt == 3)
        {
           (*ID).cost = INF;
           cout<<"Timer Missed 3 Times Setting cost to INF for ID ::"<<(*ID).DestID<<endl;
        }
        else
        {
            (*ID).missedCnt++;
//            resetTime(ID);
            cout<<" Timer missed for ID ::"<<(*ID).DestID<<" with count ::"<<(*ID).missedCnt<<endl;
        }
     }
  }

  //****************** Get the timer of IDInfo object with minimum time remaining to set select Timer *********************//
  long int getMinTimer()
  {
     cout<<"// *********** trying to get the minimum time ************ //"<<endl;
     if(MinHeap_MinTimeFor_Timer_Restrt.empty())
     {
        cout<<"|| ************** No Object in min heap ***************** ||"<<endl;
        return 0;
     }
     IDInfo* ID = MinHeap_MinTimeFor_Timer_Restrt.top();
     unsigned int remain = remainTime(ID);
     if( remain >= INF )
     {
        cout<<"|| ==> Remaining is INFINITY for ID ::"<<(*ID).DestID<<" see "<<(*ID).Time<<endl;
        return 0;
     }
     cout<<" Remain Time ::"<<remain<<endl;
/*     if( remain <= 0)
     {
        resetTime(ID);
        remain = remainTime(ID);
        cout<<" Remain Time after update ::"<<remain<<endl;
     }
*/     return remain;
  }

  //************* Get Next hop ID to reach Destination IP **************//
  unsigned long nextHopID(unsigned long ID)
  {
     return DestbyIDInfoMap[ID]->nextHopID;
  }

  // ************ Create Object for each ID and map them all **** Done only once initially ******** //
  // 1. Create Object of Each Node in network
  // 2. Start Timer for self
  // 3. If Cost is INF set nextHop to INF else to self
  // 4. Make UDP connection for each neighbour and store sockfd in object, Done Once
  // 5. Add in minHeap to find minimum time Object to help start Timer accordingly
  // 6. Initially timer will be set to the time period of Self Timer.
  // 7. Initially all other Objects will have INF Time
  // 8. Initialize missedCnt to 0 for all neighbours
  void CreateNMapID(char *payload, unsigned int UpdateInterval)
  {
     unsigned long ID = 0, RPort = 0, DPort = 0, Cost = 0, IP = 0;
     unpack((unsigned char*)payload, (char*)"HHHHL", &ID, &RPort, &DPort, &Cost, &IP);

     cout<<"//************** Creating Routing Information and storing in mapping ***************//"<<endl;
     struct IDInfo* IInfo = (struct IDInfo*)malloc(sizeof(struct IDInfo));
     IInfo->DestID = ID;
     sprintf(IInfo->DestIPstr,"%d.%d.%d.%d", ((IP>>24)&((1<<8)-1)), ((IP>>16)&((1<<8)-1)), ((IP>>8)&((1<<8)-1)), (IP&((1<<8)-1)));
     cout<<"IP address by byte wise conversion ::"<<(IInfo->DestIPstr)<<endl;
     IInfo->DestRouterPort = RPort;
     IInfo->DestDataPort = DPort;
     IInfo->cost = Cost;
     IInfo->DestIP = IP;
     cout<<"value in Cost :"<<Cost<<endl;
     IInfo->Time = INF;
     if(Cost == INF)
     {
        IInfo->nextHopID = INF;
     }
     else if(Cost == 0)
     {
        IInfo->nextHopID = ID;
        IInfo->period = UpdateInterval;
        OwnIDinfo = IInfo;
        resetTime(OwnIDinfo);
        addInMinHeap(IInfo);
     }
     else
     {
        IInfo->nextHopID = ID;
        IInfo->sockfd = connectUDP(IInfo->DestIPstr, RPort);
        addInMinHeap(IInfo);
     }
     DestbyIDInfoMap[ID] = IInfo;
     DestbyIPInfoMap[IP] = IInfo;
     cout<<"//************** SUCCESS :: Stored Routing Information ***************//"<<endl;
  }

  void addInMinHeap(struct IDInfo* IInfo)
  {
      MinHeap_MinTimeFor_Timer_Restrt.push(IInfo);
  }

  void sendRoutingTable(unsigned long controlIP, int newfd)
  {
     cout<<"//************** Packing Routing Table and Sending to Controller ***************//"<<endl;
     unsigned char newpack[1000];
     const unsigned long zeropad = 0;
     //****************** number of 2 byte strings *******************//
     unsigned long lenOfPayLoad = DestbyIDInfoMap.size() * 4 * 2;
     cout<<"size of Payload :"<<lenOfPayLoad<<endl;
     int size = pack(newpack, (char*)"LHH", controlIP, zeropad, lenOfPayLoad);
     //sendPacket(newpack, newfd);
     int sz = 0;
     for(map<long, struct IDInfo*>::iterator it = DestbyIDInfoMap.begin(); it != DestbyIDInfoMap.end(); ++it)
     {
       size += sz;
       unsigned char *packet = newpack + size;
       cout<<"Sending content ::"<<(((*it).second)->DestID)<< " "<<zeropad<< " "<<(((*it).second)->nextHopID)<<" "<< (((*it).second)->cost)<<endl<<endl;
       sz = pack(packet, (char*)"HHHH", ((*it).second)->DestID, zeropad, ((*it).second)->nextHopID, ((*it).second)->cost);
//       sendPacket(packet, newfd);
       cout<<"Content of Packet containing Forwarding Row :"<<endl;
       printf("%x %x %x %x\n", *packet, *(packet + 1), *(packet + 2), *(packet + 3));
       printf("%x %x %x %x\n", *(packet + 4), *(packet + 5), *(packet + 6), *(packet + 7));
     }
     if(!DestbyIDInfoMap.empty())
        sendPacket(newpack, newfd);
     cout<<"//************** Completed :: Sending Routing Table ***************//"<<endl<<endl;
  }

  void sendRoutingTableToNeigh()
  {
    cout<<"//************** SEND :: Sending Routing Table to Neighbours ***************//"<<endl<<endl;
    unsigned char newpack[10000];
    cout<<"//************** Packing Routing Table and Sending to all Neighbours ***************//"<<endl<<endl;

    unsigned long lenOfPayLoad = DestbyIDInfoMap.size() * 4 * 2;

    cout<<"UDP :: size of Payload :"<<lenOfPayLoad<<endl<<endl;
    int size = pack(newpack, (char*)"LHH", (*OwnIDinfo).DestID, UpdateInterval, lenOfPayLoad);
    //sendPacket(newpack, newfd);
    int sz = 0;
    for(map<long, struct IDInfo*>::iterator it = DestbyIDInfoMap.begin(); it != DestbyIDInfoMap.end(); ++it)
    {
      size += sz;
      unsigned char *packet = newpack + size;

      sz = pack(packet, (char*)"HH", ((*it).second)->DestID, ((*it).second)->cost);

    }
    if(!DestbyIDInfoMap.empty())
    {
      save_The_Packet(newpack);
      for(map<long, struct IDInfo*>::iterator it = DestbyIDInfoMap.begin(); it != DestbyIDInfoMap.end(); ++it)
      {
         IDInfo *temp = ((*it).second);
         if(temp->cost != 0 && temp->cost != INF)
         {
            cout<<"// ************ Sending Routing Table to IP ::"<<temp->DestIPstr<<" "<<temp->cost<<" ************ //"<<endl<<endl;
        //    UDPsend( temp->DestIP,  temp->DestRouterPort,  (char*)newpack,  temp->sockfd);
            sendPacket(newpack, temp->sockfd);
         }
      }
    }

    cout<<"//************** COMPLETE :: Sending Routing Table to Neighbours ***************//"<<endl<<endl;

  }

  void UDPsend(int IP, int port, char *pack, int fd)
  {
    struct hostent *hp;     /* host information */
    struct sockaddr_in servaddr;    /* server address */

    /* fill in the server's address and data */
    memset((char*)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = port;


    /* put the host's address into the server address structure */
    memcpy((void *)&servaddr.sin_addr, (const void *)&IP, 32);
//    cout<<"ADDRESS PACKET SENT TO ::"<<(char*)(servaddr.sin_addr)<<endl;
    /* send a message to the server */
    if (sendto(fd, pack, strlen(pack), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
      perror("sendto failed");
      return ;
    }
  }

  void save_The_Packet(unsigned char *newpack)
  {
     if(strlen((const char*)lastSentPack) != 0)
     {
        memcpy(secLastSentPack, lastSentPack, 1000);
     }
     memcpy(lastSentPack, newpack, 1000);
  }

  unsigned char* getLastSent()
  {
     return lastSentPack;
  }

  unsigned char* getSecLastSent()
  {
     return secLastSentPack;
  }

// ************* Set timer for the Routing table update for the Router ************* //
  void setTimer(long senderID, unsigned long timer)
  {
     cout<<"// ************* Set timer for the Routing table update for the Router ************* //"<<endl;
     if(DestbyIDInfoMap.find(senderID) != DestbyIDInfoMap.end())
     {
       struct IDInfo* NodeInfo = DestbyIDInfoMap[senderID];
       NodeInfo->Time = timer;
     }
  }

// **************** Check if any node is reachable with lower cost through this sender **************** //
  void findShortest(unsigned char *payload, long senderID)
  {
    cout<<"// **************** Check if any node is reachable with lower cost through this sender **************** //"<<endl;
    int costToSender;
    if(DestbyIDInfoMap.find(senderID) != DestbyIDInfoMap.end())
    {
       cout<<"UDP :: CHECK :: Sender ID "<<endl;
       struct IDInfo* newNextHopInfo = DestbyIDInfoMap[senderID];
       costToSender = (newNextHopInfo->cost);
    }
    else
    {
       cout<<"NOT FOUND ::"<<senderID<<endl;
       return;
    }

    for(int i = 0; i < numOfNeigh; i++)
    {
        unsigned int DestID, destCost;
        cout<<"UDP :: UNPACK :: PAYLOAD "<<endl;
        unpack(payload, (char*)"HH", &DestID, &destCost);
        payload += 4;
        cout<<"UDP :: CHECK :: Destination ID "<<endl;
        int oldCost;
        struct IDInfo* NodeInfo;
        if(DestbyIDInfoMap.find(DestID) != DestbyIDInfoMap.end())
        {
          NodeInfo = DestbyIDInfoMap[DestID];
          oldCost = NodeInfo->cost;
        }
        else
        {
           cout<<"NOT FOUND ::"<<DestID<<endl;
           continue;
        }
        cout<<"UDP :: CHECK :: After Sender ID "<<endl;
        int newCost = destCost + costToSender;
        cout<<"UDP :: CHECK :: After Sender ID newCost "<<endl;
        cout<<"UDP :: CHECK :: After Sender ID oldCost "<<endl;
        if(newCost < oldCost)
        {
            cout<<"Cost Updated for ID ::"<<DestID<<" Previous Cost ::"<<oldCost<<" New Cost ::"<<newCost<<endl;
            NodeInfo->nextHopID = senderID;
            NodeInfo->cost = newCost;
        }
    }

  }

  //************* Initialize for first time **************//
  void initTable(char *payload)
  {
    unpack((unsigned char*)payload, (char*)"HH",  &numOfNeigh, &UpdateInterval);
    payload += 4;
    cout<<endl<<"//************** Start :: Routing Table Contents ***************//"<<endl;
    cout<<endl<<"Number of Neighbours :"<<numOfNeigh<<endl;
    cout<<"Update Interval :"<<UpdateInterval<<endl<<endl;

    for(int i = 0; i < numOfNeigh; i++)
    {
       CreateNMapID(payload, UpdateInterval);
       payload += 12;
    }
    printNeigh();
  }

  void updatePath(char *payload)
  {
    cout<<endl<<"//************** Start :: Updating Path ***************//"<<endl;
    unsigned int ID = 0, pathCost = 0;
    unpack((unsigned char*)payload, (char*)"HH",  &ID, &pathCost);
    if(DestbyIDInfoMap.find(ID)==DestbyIDInfoMap.end())
    {
       cout<<"Exception :: Cannot Find the Destination ID Information !!!!!!!!!!!"<<endl;
       cout<<"//************** UnSUCCESS :: Update ***************//"<<endl;
       return;
    }
    DestbyIDInfoMap[ID]->cost = pathCost;
    cout<<endl<<"//************** Completed :: Update ***************//"<<endl;
  }




  void printNeigh( )
  {
    for(map<long, struct IDInfo*>::iterator it = DestbyIDInfoMap.begin(); it != DestbyIDInfoMap.end(); ++it)
    {
      cout << "ID :" <<((*it).second)-> DestID<< endl;
      cout << "Router Port :" << ((*it).second)->DestRouterPort << endl;
      cout << "Data Port :" << ((*it).second)->DestDataPort << endl;
      cout << "Path Cost :"  << ((*it).second)->cost << endl;
      cout << "IP Address :"<<((*it).second)->DestIPstr<<endl<<endl;
    }
    cout<<endl<<"//************** END :: Routing Table Contents ***************//"<<endl;
  }
};

struct transferInfo {
   unsigned int TransferID, TTL;
   char FileName[1000];
   long destRouterIP;
   vector<unsigned int> SeqNum;
};

class controlHeader {
public :
   long controlIPAdd, payLength;
   unsigned int controlCode, responseCode;
   char pload[12000];

   controlHeader()
   {
     controlIPAdd = 0;
     controlCode = 0;
     responseCode = 0;
   }

   void print()
   {
      cout << endl << "//************Control Header Information ************//"<<endl;
      cout <<"Controller IP Address :"<<controlIPAdd<<endl;
      cout <<"Control Code :"<<controlCode<<endl;
      cout <<"Response Code :"<<responseCode<<endl;
      cout <<"Payload Packet packed Content :"<<pload<<endl<<endl;
   }
};

class RegisServer : public HOST{
  public :
    controlHeader CHeader;
    neighbours n;
    int newfd;
    int controListener;
    char remoteIP[INET6_ADDRSTRLEN];
     int OwnID, OwnRoutrPrt, OwnDataPrt, OwnRouterFd, OwnDataFd;
    fd_set master;
    struct timeval timeout;
    int fdmax;
    int lastTimer;
    long OwnIPAdd;

    vector<struct transferInfo *> StoredTransInfo;
    RegisServer(char *port){
        //FILLS SYSTEMS IP ADDRESS
        OwnRouterFd = -1;
        OwnDataFd = -1;
        getadd();
        controListener = createListenPort(port, SOCK_STREAM);
        connectServer();
        timeout.tv_sec = 0;
        lastTimer = 0;
    }
//**************** Added new port for Router or Data connection to Router ******************//
    int addSocketPort(int port, int connectionType)
    {
        cout<<"Trying to create socket Listener of Type :"<<connectionType<<endl;
        char newPrt[40];
        sprintf(newPrt,"%d",port);
        int listener = createListenPort(newPrt, connectionType);
        cout<<"//************ Addying the new socket in the fdset *************//"<<endl;
        FD_SET(listener, &master);
        if (listener > fdmax)
        {
            fdmax = listener;
        }
        cout<<"//************ Connection complete *************//"<<endl;
        return listener;
    }

    unsigned int *connectNRecv(int listener)
    {
      cout<<"// ************** LISTEN :: Connect and Receive the Packet ************** //"<<endl;
      unsigned int *chunk = (unsigned int *)malloc(sizeof(unsigned int)*256);
      struct sockaddr_storage remoteaddr;
      socklen_t addrlen;
      addrlen = sizeof remoteaddr;

      newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen);
      if (newfd == -1)
      {
          perror("accept");
      }
      else
      {
          FD_SET(newfd, &master);
          if (newfd > fdmax)
          {
              fdmax = newfd;
          }
          /************** Get socket information of the connection **************/
          sockaddr_in *clientadd = (sockaddr_in *)get_in_addr((struct sockaddr*)&remoteaddr);
          /************* Convert IP address to Human Readable Form **************/
          inet_ntop(remoteaddr.ss_family, clientadd, remoteIP, INET6_ADDRSTRLEN);
          /***************** Work on the packet from Controller *****************/
          cout<<"START :: Receiving the packet"<<endl<<endl;
          if(recv(newfd , chunk , 1000 , 0)  < 0)
          {
              cout<< "FAILED :: Packet Not Received"<<endl<<endl;
          }
          else{
              cout<<"SUCCESS :: Packet Received"<<endl<<endl;
          }
          unsigned int incominPrt = (*clientadd).sin_port;
          cout<<"SENDER :: IP Address ::"<<remoteIP<<endl<<endl;
          cout<<endl<<"SENDER :: Port Address ::"<<incominPrt<<endl;
          return chunk;
        }
        cout<<"// ************** SUCCESS :: Trying to connect and Receive the Packet ************** //"<<endl;
    }

    void UnPackDoBellmanFord()
    {
       // ******* start timer for the neighbour to receive timely Routing Update *********** //
       unsigned long timer = (CHeader.controlCode << 16) | ((CHeader.responseCode)&((1<<16)-1));
       n.setTimer(CHeader.controlIPAdd, timer);
       // ************** Here controlIPAdd = SenderID ******************* //
       n.findShortest((unsigned char*)CHeader.pload, CHeader.controlIPAdd);
    }


    void connectServer(){
        /********** Declarations for adding and keeping TCP connections ***********/
        fd_set read_fds;
        fdmax = controListener;
        unsigned char buf[256];
        int nbytes, i, j;
        FD_ZERO(&master);    // clear the master and temp sets
        FD_ZERO(&read_fds);
        FD_SET(controListener, &master);

        for(;;) {
            read_fds = master;
            int ret;
            if ((ret = select(fdmax+1, &read_fds, NULL, NULL, &timeout)) < 0) {
                perror("Select");
                exit(4);
            }
            if(ret == 0)
            {
                cout<<"Socket Closed"<<endl;
                close(newfd);
            }

            cout<<endl<<"TIME OUT :: "<< timeout.tv_sec<<endl<<endl;
            memset( (void *)&buf, '\0', sizeof(buf));
            // ************** Check if Any timer Exceeds ****************** //
            timeout.tv_sec = n.TimerInterrupt(lastTimer);
            lastTimer = timeout.tv_sec;
            cout<<"RESET TIMEOUT ::"<< timeout.tv_sec<<endl<<endl;

            // *********** Check if Any Socket FD read is set ************* //
//            if (FD_ISSET(OwnRouterFd, &read_fds)) {
            unsigned int chunk[1000];
            struct sockaddr_storage their_addr;
            socklen_t addr_len = sizeof their_addr;
/*            cout << "\\--\\ *********** UDP Router Port :: Check If Packet Incoming ********** /--/" << endl;
            if (recvfrom(OwnRouterFd, chunk, 1000, 0, (struct sockaddr *)&their_addr, &addr_len) >= 0)
            // if (recv(OwnRouterFd, chunk, 1000, 0)>=0)//, (struct sockaddr *)&their_addr, &addr_len) >= 0)
            {
                // TODO: verify the packet is an acknowledgement
                // of the packet sent above and not something else...
                cout << "\\--\\ *********** UDP :: Packet received ********** /--/" << endl;
                cout<<"UDP :: Connect & Receive Packet"<<endl;
            //  unsigned int *chunk = connectNRecv();
                cout<<"UDP :: UNPACK :: Header"<<endl;
                unpackHeader(chunk);
                cout<<"UDP :: UNPACK :: Payload Apply BellmenFord"<<endl;
                UnPackDoBellmanFord();
                FD_CLR(OwnRouterFd, &master);
            }
*/            // TODO :: DECREASE TTL AND FORWARD IS NECESSARY
            cout << "\\--\\ *********** TCP DATA PORT :: If Packet Incoming, OWN_DATA_FD = "<<OwnDataFd<<" ********** /--/" << endl;
            if (OwnDataFd != -1 && FD_ISSET(OwnDataFd, &read_fds)) {
                cout << "\\--\\ *********** TCP DATA PORT :: FD SET ********** /--/" << endl;
               unsigned int *chunk = connectNRecv(OwnDataFd);
               cout << "\\--\\ *********** TCP DATA PORT :: Packet received ********** /--/" << endl;
               unpackHeader((unsigned int *)chunk);
               cout<<"TCP DATA PORT :: IP ADDRESS OF THE DESTINATION IN THE PACKET ::"<<CHeader.controlIPAdd<<" SELF IP ADDRESS ::"<<OwnIPAdd<<endl;
               if(CHeader.controlIPAdd == OwnIPAdd)
               {
                  cout<<"File :: Destined to ME"<<endl<<endl;
                  recvFile(CHeader.pload);
               }
               else
               {
                  cout<<"DATA PORT :: File :: Not Destined to ME so Forward"<<endl<<endl;
                  int nextHopID = (n.DestbyIPInfoMap[CHeader.controlIPAdd])->nextHopID;
                  int nextHopDprt = (n.DestbyIDInfoMap[nextHopID])->DestDataPort;
                  char* nextHopIP = (n.DestbyIDInfoMap[nextHopID])->DestIPstr;
                  TCPSendPack((unsigned char*)CHeader.pload, nextHopDprt, nextHopIP);
                  //sendPacket((char*)chunk, nextSockFd);
                  /*
                  struct IDInfo
                  {
                    unsigned int nextHopID, DestID, DestRouterPort, DestDataPort, cost;
                    unsigned long DestIP;
                    long int Time, period;
                    int sockfd, missedCnt;
                    char DestIPstr[40];
                  };

                  */
               }
               exit(0);
               cout<<"COMPLETE :: DATA PORT"<<endl<<endl;
               close(newfd);
            }
            if (FD_ISSET(controListener, &read_fds)) {
                /********* Listen on the Controller Port **********/
                    unsigned int *chunk = connectNRecv(controListener);
                    cout<<"// ************* CONTROLLER :: START :: Controller Packet Check *********** //"<<endl<<endl;
                    unpackHeader((unsigned int *)chunk);

                    cout<<endl<<"UNPACK :: CONTROLLER :: Check Control Code, Unpack Payload and Respond"<<endl;
                    unpackPayload();
                    cout<<endl<<"SUCCESS :: CONTROLLER :: Payload Checked"<<endl;

                    cout<<endl<<"// ******* COMPLETE :: CONTROLLER :: Packet Check ********* //"<<endl<<endl;

            }
            FD_CLR(newfd, &master);

        } // END handle data from client

    } // END for(;;)--and you thought it would never end!

    void recvFile(char *payload)
    {
      char fileName[1000];
      int i = 0;
      char content[12000];
      BreakInTwo(payload, fileName, content);
      cout<<" ---- FileName Found :: "<<fileName<<" ---- "<<endl<<endl;
      ofstream myfile;
      myfile.open(fileName);
      myfile<<(&payload[i]);
      myfile.close();
    }

    void BreakInTwo(char *x, char *b, char *c)
    {
        char a[1024];
        strcpy(a,x);
        char *temp;
        temp = strtok(a, "##");
        if( temp != NULL )
        {
            strcpy(b,temp);
            temp = strtok(NULL, " ");
        }
        if( temp != NULL )
        {
            strcpy(c,temp);
            temp = strtok(NULL, " ");
        }
        int len = strlen(c);
        if(c[len-2]=='\n'){
            c[len-2]='\0';
        }

    }

    void unpackHeader(unsigned int *chunk)
    {
        /********** Unpack the packet form the controller ***********/
        cout<<endl<<"UNPACK :: Controller Header"<<endl;
        if(strlen((char*)chunk) < 6)
        {
          cout<<"FAILED :: SMALL PACK SIZE OF THE CONTROLLER HEADER"<<endl;
          return;
        }
        unpack((unsigned char *)chunk, (char*)"LCCs", &(CHeader.controlIPAdd), &(CHeader.controlCode), &(CHeader.responseCode), CHeader.pload);
        cout<<"SUCCESS :: Unpacked the Controller Header"<<endl;
        CHeader.print();
    }

    void sendFile(char *file, int Dport, char* IP, struct transferInfo* transIP)
    {

        unsigned long zeroPad = 0;
        cout<<"\\************** PACKING FILE FOR SENDFILE ************** //"<<endl;
        unsigned char newpack[12000];

        int sz = pack(newpack, (char*)"LCC", transIP->destRouterIP, transIP->TransferID, transIP->TTL, (transIP->SeqNum).back());
        cout<<"PACK :: HEADER FOR SENDFILE ::"<<file<<" DESTINED TO ::"<<CHeader.controlIPAdd<<" NEXT HOP IP ::"<<IP<<endl;
        int size = pack(newpack+sz, (char*)"L", zeroPad);
        cout<<"PACKED FIN :: FOR SENDFILE ::"<<file<<endl;
        size += sz;
        *(newpack+size) |= (1<<7);
        size += 4;

        cout<<"APPEND FILE CONTENTS :: Opening File ::"<<file<<endl;

        string line;
        ifstream myfile(file);
        cout<<"OPEN :: Opening File ::"<<file<<endl;
        if (myfile.is_open())
        {
          cout<<"OPENED :: Opened File ::"<<file<<endl;
           string s(file);
           s.append((char*)"##");
           while ( getline (myfile,line) )
           {
              cout<<"GETLINE :: Get Line from File ::"<<file<<endl;
              int len = line.length();
              strcpy((char*)(newpack+size), line.c_str());
              size += len;
           }
           myfile.close();
        }
        cout<<"PACKED :: SEND PACKET NOW"<<endl;
        TCPSendPack(newpack, Dport, IP);
//        sendPacket(newpack, peerfd);
    }

    void TCPSendPack(unsigned char* newpack, int port, char *IP)
    {
        int sockfd, numbytes;
        struct addrinfo hints, *servinfo, *p;
        int rv;
        char s[INET6_ADDRSTRLEN];

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        unsigned char PORT[2];
        sprintf((char*)PORT,"%d",port);
        cout<<"//**************** TCP DATA CONNECTION ON IP ::"<<IP<<" PORT ::"<<PORT<<" WHILE WHAT I RECEIVED WAS IP ::"<<IP<<" PORT ::"<<port<<endl;
//        unsigned char ip[100];
//        pack(ip,(char*)"L",IP);
        if ((rv = getaddrinfo(IP, (char*)PORT, &hints, &servinfo)) != 0) {
            fprintf(stderr, "TCP DATA GET_ADDR_INFO: %s\n", gai_strerror(rv));
            return ;
        }

        // loop through all the results and connect to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
              p->ai_protocol)) == -1) {
                fprintf(stderr, "FAILED :: TCP DATA PORT SOCKET CONNECTION\n");
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                fprintf(stderr, "FAILED :: TCP DATA PORT CONNECTION\n");
                continue;
            }

            break;
        }

        if (p == NULL) {
            fprintf(stderr, "FAILED :: TCP DATA PORT CANNOT BE CONNECTED\n");
            return ;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
           s, sizeof s);
        printf("TCP DATA CLIENT :: connecting to %s\n", s);

        freeaddrinfo(servinfo); // all done with this structure

        sendPacket(newpack, sockfd);

        cout<<"COMPLETE :: FILE DATA CONTENT SENT"<<endl;
        close(sockfd);


    }

    void unpackPayload()
    {
      unsigned char newpack[1000];
      unsigned long zeroPad = 0;
      switch(CHeader.controlCode)
      {
        /*************** Create Packet for the Author **************/
         case 0:
         {
           cout<<"//************ ControlCode :: 0 Author *************//"<<endl;
           char *payload = (char*)"I, keshavsh, have read and understood the course academic integrity policy.";
           cout<<"Payload Size :"<<strlen(payload)<<endl;
           pack(newpack, (char*)"LHs", CHeader.controlIPAdd, zeroPad, payload);
           sendPacket(newpack, newfd);
           break;
         }
         /************* Create Routing Table and send empty Packet to Controller ****************/
        case 1:
        {
             cout<<"//************ ControlCode :: 1 INIT Routing Table *************//"<<endl;
             n.initTable(CHeader.pload);
             pack(newpack, (char*)"LL", CHeader.controlIPAdd, zeroPad);
             sendPacket(newpack, newfd);
             OwnRoutrPrt = (n.OwnIDinfo)->DestRouterPort; OwnDataPrt = (n.OwnIDinfo)->DestDataPort;
             OwnID = (n.OwnIDinfo)->DestID;
             OwnIPAdd = (n.OwnIDinfo)->DestIP;
            // TODO :: Check For Routing Updates .. It's Doutful
             //************* Add Socket Port for Own Router Port **************//
             if(OwnRouterFd == -1)
                OwnRouterFd = addSocketPort(OwnRoutrPrt, SOCK_DGRAM);
             //************* Add Socket Port for Own Data Port **************//
             if(OwnDataFd == -1)
                OwnDataFd = addSocketPort(OwnDataPrt, SOCK_STREAM);
             cout<<"Router Address of current router :"<<OwnRoutrPrt<<endl;
             cout<<"Data Address of current router :"<<OwnDataPrt<<endl;
             timeout.tv_sec = n.getMinTimer();
             break;
        }
        /************* Request for Forwarding Table from Controller ****************/
        case 2:
        {
             cout<<"//************ ControlCode :: 2 Send Routing Table to Controller *************//"<<endl;
             n.sendRoutingTable(CHeader.controlIPAdd, newfd);
             break;
        }
        /************* Update Cost and send empty Packet to Controller ****************/
        case 3:
        {
          cout<<"//************ ControlCode :: 3 Update Cost  *************//"<<endl;
             n.updatePath(CHeader.pload);
             pack(newpack, (char*)"LL", CHeader.controlIPAdd, zeroPad);
             sendPacket(newpack, newfd);
             break;
        }
        /********** Exit to show Crash and send empty Packet to Controller *************/
        case 4:
        {
             cout<<"//************ ControlCode :: 4 Crash *************//"<<endl;
             pack(newpack, (char*)"LL", CHeader.controlIPAdd, zeroPad);
             sendPacket(newpack, newfd);
             exit(0);
             break;
        }
        // ********** Sendfile to the Router ID and send empty Packet to Controller ************* //
        // ************** TODO :: Send File to Neighbour **************** //
        // ************* TODO :: send the response message after the packet with the FIN bit set (last packet) has been sent to the next hop.
        case 5:
        {
             cout<<"//************ ControlCode :: 5 SendFile *************//"<<endl;
             struct transferInfo *transIT = createTransPack();
             cout<<"//************ Storing Transfer info *************//"<<endl;
             vector<struct transferInfo*>::iterator it = StoredTransInfo.begin();
             for(; it != StoredTransInfo.end(); ++it)
             {
                 if((*it)->TransferID == transIT->TransferID)
                 {
                    ((*it)->SeqNum).push_back((transIT->SeqNum)[0]);
                    break;
                 }
             }
             if(it == StoredTransInfo.end())
             {
                 StoredTransInfo.push_back(transIT);
             }


             int nextHopID = (n.DestbyIPInfoMap[transIT->destRouterIP])->nextHopID;
             int nextHopDPrt = (n.DestbyIDInfoMap[nextHopID])->DestDataPort;
             char* nextHopIP = (n.DestbyIDInfoMap[nextHopID])->DestIPstr;
             int destIP = (transIT->destRouterIP);
             cout<<"DESTINATION IP :: "<<destIP<<endl<<endl;
             cout<<"NEXT HOP IP :: "<<nextHopIP<<endl<<endl;
             cout<<"DESTINATION DATAPORT :: "<<nextHopDPrt<<endl<<endl;
             // *************** Send to nexthop for the Destination Address *************** //
             sendFile(transIT->FileName, nextHopDPrt, nextHopIP, transIT);

             // ************** Send Receipt of Sending to Controller ************** //
             sendPacket(newpack, newfd);
             cout<<"//****** SUCCESS :: Sent File And Replied to Controller *******//"<<endl;
             exit(0);
             break;
        }
        /********** SENDFILE-STATS*************/
        case 6 :
        {
             cout<<"//************ ControlCode :: 6 SENDFILE-STATS *************//"<<endl;
             unsigned int transferID = 0;
             unpack((unsigned char*)CHeader.pload, (char*)"C", &transferID);
             vector<struct transferInfo*>::iterator it = StoredTransInfo.begin();
             for(; it != StoredTransInfo.end(); ++it)
             {
                 if(((*it)->TransferID) == transferID)
                 {
                    cout<<"Packing up Header inside"<<endl;
                    int size = pack(newpack, (char*)"LH", CHeader.controlIPAdd, zeroPad);////////////change address to controller IP Address
                    //********************** Pack TransferID TTL padding SeqNum **************************//
                    unsigned char tempPack[1000];
                    int sz = pack(tempPack, (char*)"CCH", ((*it)->TransferID), ((*it)->TTL), zeroPad);
                    vector<unsigned int> x = ((*it)->SeqNum);
                    unsigned char *temp = (tempPack+sz);
                    for(vector<unsigned int>::iterator sit = x.begin(); sit != x.end(); ++(sit) )
                    {
                        cout<<"Putting Sequence Number in the packet :"<<(*sit)<<endl;
                        *temp++ = (*sit)>>8;
                        *temp++ = (*sit);
                        sz += 2;
                    }
                    cout<<"Total Size of Header :"<<size<<endl;
                    cout<<"Total Size of Payload :"<<sz<<endl;
                    *(newpack+size) = sz>>8;
                    *(newpack+size + 1) = sz;
//                    strcpy((newpack + size + 2), tempPack, sz);
//                    pack((newpack+size), "s", tempPack);
                    sendPacket(newpack, newfd);
                    sendPacket(tempPack, newfd);
                    break;
                 }
             }
             break;
        }
        case 7:
        {
           cout<<"// ************** ControlCode :: 7 Last Sent ******************* //"<<endl;
           unsigned char* newpack = n.getLastSent();
           unsigned long lenOfPayLoad = strlen((const char*)newpack);
           cout<<"size of Payload :"<<lenOfPayLoad<<endl;
           unsigned char packet[1000];
           int size = pack(packet, (char*)"LHH", OwnID, zeroPad, lenOfPayLoad);
           sendPacket(packet, newfd);
           sendPacket(newpack, newfd);
           break;
        }
        case 8:
        {
           cout<<"// ************** ControlCode :: 8 Second Last Sent ******************* //"<<endl;
           unsigned char* newpack = n.getSecLastSent();
           unsigned long lenOfPayLoad = strlen((const char*)newpack);
           cout<<"size of Payload :"<<lenOfPayLoad<<endl;
           unsigned char packet[1000];
           int size = pack(packet, (char*)"LHH", OwnID, zeroPad, lenOfPayLoad);
           sendPacket(packet, newfd);
           sendPacket(newpack, newfd);
           break;
        }
        default :
        {
             cout<<"Something Wrong came in default with ControlCode :"<<CHeader.controlCode<<endl;
        }
      }
    }

    struct transferInfo *createTransPack()
    {
      unsigned long destRouterIP = 0;
      unsigned int initTTL = 0, transferID = 0, initSeqNo = 0;
      char FileName[1000];
      unpack((unsigned char*)CHeader.pload, (char*)"LCCH", &destRouterIP, &transferID, &initTTL, &initSeqNo);
      char *tempPoint = CHeader.pload;
      tempPoint += 8;
      strcpy(FileName, tempPoint);

      cout<<"FileName :"<<FileName<<endl;
      cout<<"Destination Router IP :"<<destRouterIP<<endl;
      cout<<"Init TTL :"<<initTTL<<endl;
      cout<<"TransferID :"<<transferID<<endl;
      cout<<"Init Seq. Num. :"<<initSeqNo<<endl;

      struct transferInfo *ti = new (struct transferInfo)();
      ti->TransferID = transferID;
      ti->TTL = initTTL;
      ti->destRouterIP = destRouterIP;
      strcpy(ti->FileName, FileName);
      (ti->SeqNum).push_back(initSeqNo);
      return ti;
    }

};

int main(int argc, char **argv)
{
    if(argc<=1)
        return 0;

    RegisServer SRVR(argv[1]);

    return 0;
}
