#include "tlogger.h"
#include "commandline.h"
#include "configfile.h"
#include "tlogparser.h"
#include "tcounter.h"
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sstream>
#include <pthread.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <string>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

using namespace std;

bool canwork;
int  serv_sock;
tcounter counter;

const char* const OK="OK";
const char* const ERR="ERR";


uid_t getUid(string* username){
  struct passwd   pwd;
  struct passwd*  result;
  char* buf;
  size_t bufsize;
  int s;
  bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (bufsize == -1)          
      bufsize = 16384;        
  buf = (char*)malloc(bufsize);
  if (buf == NULL) return -1;
  s = getpwnam_r(username->c_str(), &pwd, buf, bufsize, &result);
  if (result == NULL) {
    free(buf); 
    return -1;
  }
  return pwd.pw_uid;
}

gid_t getGid(string* groupname){
  struct group  *group=getgrnam(groupname->c_str());
  if(!group) return -1;
  else return group->gr_gid;
}

void chMod(string* filename,string* mode ){
  if (mode->compare("")==0) return;
  int i= strtol(mode->c_str(),0,8);
  chmod(filename->c_str(),i);
}

void my_terminate (int param){
  canwork=false;
}

bool write_int(int sock,int message){
  return (write(sock,&message,sizeof(int)) !=-1);
}

bool  read_string(int sock,int length,string* result ){
  if (length>0){
      char* text=(char*)malloc(length);
      size_t res= recv(sock,&length,sizeof(length),0);
      if (res==0) return false;
      if (res==-1){
	free(text);
	return false;
      }
      *result=text;
      free(text);
      return res;
  }
  else return true;
}

bool write_string(int sock,string message){
    int length=message.size()+1;
    if(!write_int(sock,length)) return false;
    return (write(sock,message.c_str(),length)!=-1);   
}

void* responce (void*  client_sock){
  int sock=*((int*)client_sock);
  free(client_sock);
  /*Чтение запроса клиента*/
  while(canwork){
    int length=0;
    char* text=NULL;     
    ssize_t result;
    result= recv(sock,&length, sizeof(length),0);
    if (result<=0)  break; //0 - сокет закрыт, -1 ошибка чтения  
    if (length>0){
      text=(char*)malloc(length);
      result= recv(sock,text, length,0);
      if (result<=0){
	free(text);
	break;
      }
      string t = text;
      free(text);
      string  responce;
      if (counter.checkUser(&t))responce="OK";  
      else responce="ERR";
      if (!write_string(sock,responce)) break;
    }
    else {
      string configFile=*(counter.getConf()->getCommandLine()->getConfigFileName());
      ifstream conf(configFile.c_str());
      string confline;
      map < string, map<string, long long > >* limits;
      string username;
      ostringstream os;
      switch(-length){
      case 1:
	if(conf) while (getline(conf,confline))	 write_string(sock,confline); 
	write_int(sock,0); 
	break;
      case 2:
        result= recv(sock,&length,sizeof(length),0);
	//result=read(sock,&length,sizeof(length));
	if(result<=0) goto closesock;
	result=read_string(sock,length,&username);
	if(result<=0) goto closesock;
          limits=  counter.getConf()->getLimits();
	if (username.compare("")==0){
	  for( map< string, map <string,long long> >::iterator i=limits->begin(); i!=limits->end();++i)
	    for(map< string,long long>::iterator j=i->second.begin(); j!=i->second.end();++j){ 
	      os.str("");
	      os <<i->first << "\t\t" << j->first << "\t" <<j->second;
  	      write_string(sock,os.str());
	    }
	}
	else if(limits->count(username)!=0){
	  map<string, long long> userlimit=limits->at(username);
	  for (map<string,long long>::iterator i=userlimit.begin();i!=userlimit.end();++i){
	    os.str("");
	    os<<username<<"\t\t"<<i->first<<"\t"<< i->second; 
	    write_string(sock,os.str());
	  }
	}
        write_int(sock,0);
	break;
      case 3:
        result= recv(sock,&length,sizeof(length),0);
	//result=read(sock,&length,sizeof(length));
	if(result<=0) goto closesock;
	result=read_string(sock,length,&username);
	if(result<=0) goto closesock;
        limits=  counter.getTraf();
	if (username.compare("")==0){
	  for( map< string, map <string,long long> >::iterator i=limits->begin(); i!=limits->end();++i)
	    for(map< string,long long>::iterator j=i->second.begin(); j!=i->second.end();++j){ 
	      os.str("");
	      os <<i->first << "\t\t" << j->first << "\t" <<j->second;
  	      write_string(sock,os.str());
	    }
	}
	else if(limits->count(username)!=0){
	  map<string, long long> userlimit=limits->at(username);
	  for (map<string,long long>::iterator i=userlimit.begin();i!=userlimit.end();++i){
	    os.str("");
	    os<<username<<"\t\t"<<i->first<<"\t"<< i->second; 
	    write_string(sock,os.str());
	  }
	}
        write_int(sock,0);
	break;
      }
    }
  }
 closesock:
  close(sock);
}

void* keep_connection(void* unused){
  while(canwork) {
    int* client_sock;
    client_sock=(int*)malloc(sizeof(int));
    socklen_t client_len;
    struct  sockaddr_un client_addr;
    *client_sock = accept (serv_sock,(struct sockaddr *)&client_addr, &client_len);
    /*Создание треда обработки соединения*/
    pthread_t responce_thread;
    pthread_create(&responce_thread,NULL,&responce,client_sock);
  } 
  return NULL;
}

int main(int argc,char**argv){
  //Разбор параметров командной строки
  command_line cmdl(argc,argv);
  //Конфигурация программы
  config_file conf(&cmdl);
  if (!conf.reconfig())  exit(1);
  logger.setTarget(conf.getLogFile(),cmdl.getNoDaemonMode());

  logger.put(2,"Starting" + *(cmdl.getProgrammName()));  
  ostringstream os; 
  pid_t pid,sid;
  bool configured;
  int confErrCount;
  int iret;  
  //Регистрация обработчиков сигналов;
  void (*prev_fn)(int);
  canwork= true; 
  prev_fn = signal (SIGTERM,my_terminate);
  prev_fn = signal (SIGABRT,my_terminate);
  //Демонизация процессв
  if(!cmdl.getNoDaemonMode()){
      logger.put(1,"Starting in daemon mode");    
      pid=fork();
      if (pid > 0)  {
	exit(0); 
      }
      if(pid<0){
	logger.put(0, "Can not start as daemon");
	exit(1);
      } 
  }
   //Запись pid в файл
  logger.put(2,"Opening pid file");
  ofstream pidfile(conf.getPidFile()->c_str(),ios_base::out);
  if (!pidfile) {
       logger.put(0, "Can not open pid file " +*(conf.getPidFile()) ) ; 
       exit(1);
  }

  else {
    logger.put(2,"Priting pid into the pid file");
     pidfile << getpid()<<endl;
     pidfile.close();
  }
  
  logger.put(2,"Set session id");
  sid=setsid();
  logger.put(2,"Changing working dir");
  iret=chdir("/");
  //Настройка счетчика
  counter.setConf(&conf);
  
  /*Открытие сокета*/
  unlink(conf.getSockFile()->c_str());
  serv_sock=socket(PF_LOCAL,SOCK_STREAM,0);
  struct sockaddr_un  serv_addr;  
  serv_addr.sun_family=AF_LOCAL;
  strcpy(serv_addr.sun_path,conf.getSockFile()->c_str()) ;
  if (bind(serv_sock,(struct sockaddr *)&serv_addr,SUN_LEN(&serv_addr))){
    logger.put(2,"Can not create  socket " + *conf.getSockFile() );
    exit(1);
  }
  int ret =chown(conf.getSockFile()->c_str(),getUid(conf.getSockUser()),getGid(conf.getSockGroup()));
  chMod(conf.getSockFile(),conf.getSockMod());
  listen(serv_sock,10);
  /*Запуск треда  процесса прослушивания сокета*/ 
  pthread_t connection_thread;
  pthread_create(&connection_thread,NULL,&keep_connection,NULL);
  /* Рассчет траффика */ 
  while (canwork){
    //Расчет траффика
    if(!counter.calc(&canwork)) {
      logger.put(0,"Can not calculate user traffic");
      exit(1);
    }
    logger.put(2, "Reconfiguring ") ; 
    configured=conf.reconfig();
    if (!configured){
      if (++confErrCount> 10){
        logger.put(0,"Can not reconfig sqtd");
	exit(1);
      }
    } else  if(confErrCount!=0) confErrCount=0;
    os<< "Waiting " <<conf.getCheckInterval() <<"s";
    logger.put(1, os.str());
    os.str("");
    sleep(conf.getCheckInterval());
  }
  close(serv_sock);
  unlink(conf.getPidFile()->c_str());
  unlink(conf.getSockFile()->c_str());
  logger.put(0,"Exit..." + string(*cmdl.getProgrammName()));
  return 0;  
}

