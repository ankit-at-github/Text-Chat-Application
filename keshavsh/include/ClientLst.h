#include<iostream>
#include<stdlib.h>
#include<string.h>
#include <sstream>
#include<stdio.h>
#include<list>
using namespace std;
#define INTMIN (1<<31)
void getMsgAftrCmnd(char *a, char *b){
  int i=0;
  for(;i<strlen(a);i++){
    if(a[i] == ' '||a[i] == '\0'){
       break;
    }
  }
  i++;
  strcpy(b,(a+i));
}

int strToInt(char *str){
  int s=0;
  int len = strlen(str);
  for(int i = 0;i<len;i++){
    s = s*10+str[i]-'0';
  }
  return s;
}

void BreakInTwo(char *x, char *b, char *c){
  char a[1024];
  strcpy(a,x);
  char *temp;
  temp = strtok(a, " ");
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
void BreakInThree(char *x, char *b, char *c, char *d){
  char a[1024];
  strcpy(a,x);
  char *temp;
  temp = strtok(a, " ");
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
  if( temp != NULL )
  {
    strcpy(d,temp);
  }
  int len = strlen(d);
  if(d[len-2]=='\n'){
    d[len-2]='\0';
  }
}
void BreakInFour(char *x, char *b, char *c, char *d, char *e){
  char a[1024];
  strcpy(a,x);
  char *temp;
  temp = strtok(a, " ");
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
  if( temp != NULL )
  {
    strcpy(d,temp);
    temp = strtok(NULL, " ");
  }
  if( temp != NULL )
  {
    strcpy(e,temp);
    temp = strtok(NULL, " ");
  }
  int len = strlen(e);
  if(e[len-2]=='\n'){
    e[len-2]='\0';
  }
}

void BreakInFive(char *x, char *b, char *c, char *d, char *e, char *f){
  char a[1024];
  strcpy(a,x);
  char *temp;
  temp = strtok(a, " ");
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
  if( temp != NULL )
  {
    strcpy(d,temp);
    temp = strtok(NULL, " ");
  }
  if( temp != NULL )
  {
    strcpy(e,temp);
    temp = strtok(NULL, " ");
  }
  if( temp != NULL )
  {
    strcpy(f,temp);
    temp = strtok(NULL, " ");
  }
  int len = strlen(f);
  if(f[len-2]=='\n'){
    f[len-2]='\0';
  }

}

struct sockinfo{
  int fd;
  char IP[14];
  int PORT;
  int SntCnt;
  int RcvCnt;
  char host[1024];
  char status[255];
  list<string> pendMsgBuf;
  list<string> blocked;
};

struct sockinfo *initsockadd(int fd, char *host, char *IP, int PORT, char *status,char *msg){
  sockinfo *si = new sockinfo();
  si->fd = fd;
  strcpy(si->IP,IP);
  si->PORT = PORT;
  si->SntCnt = 0;
  strcpy(si->host,host);
  strcpy(si->status,status);
  if(strlen(msg)!=0){
    string s(msg);
   (si->pendMsgBuf).push_back(s);
    si->RcvCnt = 1;
  }
  else{
    si->RcvCnt = 0;
  }
  return si;
}


int cmpequal(char *x, char *y){
  int len1 = strlen(x);
  int len2 = strlen(y);
  for(int i = 0;i<len1&& i<len2;i++){
    if(x[i]!=y[i]){
      return 0;
    }
  }
  return 1;

}

template <class T>
class listClients {
    list<sockinfo*> ls;
  public:

    void add(char *IP, char *msg=(char *)"", int fd=-1, char *host=(char *)"", int PORT=0, char *status=(char *)"OFFLINE"){
      list<sockinfo*>::iterator it = getObjByIP(IP);
      if(it==ls.end()){
            sockinfo *si = initsockadd(fd,host,IP,PORT,status,msg);
            ls.push_back(si);
       }
       else{
         (*it)->fd = fd;
         strcpy((*it)->status,status);
       }

    }
    list<sockinfo*>::iterator getLastAdd(){
      return ls.end();
    }
    list<sockinfo*> getObjList(){
      return ls;
    }

    int getfdByIP(char *IP){
      list<sockinfo*>::iterator it = getObjByIP(IP);
      if(it!=ls.end()){
        return (*it)->fd;
      }
      return INTMIN;
    }

    // iterative search for corresponding fd
    char *getIPByfd(int fd){
      list<sockinfo*>::iterator it = getObjByfd(fd);
      if(it!=ls.end()){
        return (*it)->IP;
      }
      return NULL;
    }

    void setOffline(int fd){
      list<sockinfo*>::iterator it = getObjByfd(fd);
      if(it!=ls.end()){
        strcpy(((*it)->status),"OFFLINE");
      }

    }

    void incSndCnt(int fd){
      list<sockinfo*>::iterator it = getObjByfd(fd);
      if(it!=ls.end()){
        ((*it)->SntCnt)+=1;
        return ;
      }
    }

    list<sockinfo*>::iterator getObjByfd(int fd){
      list<sockinfo*>::iterator it = ls.begin();
      for(;it!=ls.end();++it){
        if((*it)->fd==fd)
          return it;
      }
      return ls.end();
    }

    void addPORT(int fd, char *PORT){
      list<sockinfo*>::iterator it = getObjByfd(fd);
      if(it!=ls.end())
        (*it)->PORT=strToInt(PORT);
    }

    list<sockinfo*>::iterator getObjByIP(char *IP){
      list<sockinfo*>::iterator it = ls.begin();
      for(;it!=ls.end();++it){
        if(cmpequal(((*it)->IP),IP)){
          return it;
        }
      }
      return ls.end();
    }

    void incRcvCnt(int fd){
      list<sockinfo*>::iterator it = getObjByfd(fd);
      if(it!=ls.end()){
          ((*it)->RcvCnt)+=1;
      }
    }

    void block(int fd,char *blockIP){
      list<sockinfo*>::iterator it = getObjByfd(fd);
      if(it!=ls.end()){
        string block(blockIP);
        ((*it)->blocked).push_back(block);
      }
    }

    void unBlock(int sendrfd, char *blckdOrNotIP){
      list<sockinfo*>::iterator it = getObjByfd(sendrfd);
      list<sockinfo*>::iterator blckit = getObjByIP(blckdOrNotIP);

      if(it!=ls.end()){
        ((*it)->blocked).remove((char *)((*blckit)->IP));
      }
    }


    int isIPInBlockLst(int sendrfd, int blckdOrNotfd){
      list<sockinfo*>::iterator it = getObjByfd(sendrfd);
      list<sockinfo*>::iterator blckit = getObjByfd(blckdOrNotfd);
      if(it!=ls.end()){
        for(list<string>::iterator bit = ((*it)->blocked).begin();bit!=((*it)->blocked).end();++bit){
          if(cmpequal((char *)(*bit).c_str(),(char *)((*blckit)->IP))){
            return 1;
          }
        }
      }
      if(blckit!=ls.end()){
        for(list<string>::iterator bit = ((*blckit)->blocked).begin();bit!=((*blckit)->blocked).end();++bit){
          if(cmpequal((char *)(*bit).c_str(),(char *)((*it)->IP))){
            return 1;
          }
        }
      }
      return 0;
    }

    list<string> getBlockedList(int fd){
      list<sockinfo*>::iterator it = getObjByfd(fd);
      if(it!=ls.end()){
        return ((*it)->blocked);
      }
      return list<string>();
    }

    list<string> getPendMsg(char *IP){
      list<sockinfo*>::iterator it = getObjByIP(IP);
      if(it!=ls.end()){
        list<string> temp(((*it)->pendMsgBuf));
        ((*it)->pendMsgBuf).clear();
        return temp;
      }
      return list<string>();
    }

    int available(char *IP){
      list<sockinfo*>::iterator it = getObjByIP(IP);

      if(it!=ls.end()){
        if(strcmp(((*it)->status),"ONLINE")==0)
          return 1;
      }
      return 0;
    }

    void addInRecpBuff(char *IP,char *msg){
      list<sockinfo*>::iterator it = getObjByIP(IP);
      if(it!=ls.end()){
        ((*it)->RcvCnt)+=1;
        string s(msg);
        ((*it)->pendMsgBuf).push_back(s);
        return;
      }
      add(IP,msg);
    }

    list<string> getAllLogedIn(){
      list<sockinfo*>::iterator it = ls.begin();
      int iter = 1;
      list<string> res;
      for(;it!=ls.end();++it){
        if(strcmp((*it)->status,"OFFLINE")==0){
          continue;
        }
        string s;
        ostringstream convertP;
        ostringstream convertfd;
        convertfd<<iter;
        iter++;
        s += convertfd.str();
        s += " ";
        s += (*it)->host;
        s += " ";
        s += (*it)->IP;
        s += " ";
        convertP<<(*it)->PORT;
        s += convertP.str();
        res.push_back(s);
      }
      return res;

    }

    list<string> getList(){
      list<sockinfo*> sortdls;
      sortdls.assign(ls.begin(),ls.end());
      sortdls.sort();
      list<string> res;
      list<sockinfo*>::iterator it = sortdls.begin();
      int iter = 1;
      for(;it!=sortdls.end();++it){
        string s;
        ostringstream convertP;
        ostringstream convertfd;
        convertfd<<iter;
        iter++;
        s += convertfd.str();
        s += " ";
        s += (*it)->host;
        s += " ";
        s += (*it)->IP;
        s += " ";
        convertP<<(*it)->PORT;
        s += convertP.str();
        res.push_back(s);
      }
      return res;
    }
    list<string> getStats(){
      list<sockinfo*> sortdls;
      sortdls.assign(ls.begin(),ls.end());
      sortdls.sort();
      list<string> res;
      list<sockinfo*>::iterator it = sortdls.begin();
      int iter = 1;
      for(;it!=sortdls.end();++it){
        string s;
        ostringstream convertsnt;
        ostringstream convertrcv;
        ostringstream convertfd;
        convertfd<<iter;
        iter++;
        s += convertfd.str();
        s += " ";
        s += (*it)->host;
        s += " ";
        convertrcv<<(*it)->RcvCnt;
        s += convertrcv.str();
        s += " ";
        convertsnt<<(*it)->SntCnt;
        s += convertsnt.str();
        s += " ";
        s += (*it)->status;
        res.push_back(s);
      }
      return res;
    }


};
