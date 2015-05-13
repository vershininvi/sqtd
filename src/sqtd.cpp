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

//Common used functions

//Convert username to user id 
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

//Convert group name to group id
gid_t getGid(string* groupname){
  struct group  *group=getgrnam(groupname->c_str());
  if(!group) return -1;
  else return group->gr_gid;
}

//Change file mode bits 
void chMod(string* filename,string* mode ){
  if (mode->compare("")==0) return;
  int i= strtol(mode->c_str(),0,8);
  chmod(filename->c_str(),i);
}

//Set global terminate flag 
void my_terminate (int param){
  canwork=false;
}


//Socket error handling class
class sock_exception: public exception{
  virtual const char* what() const throw()  {
    return _("Error reading socket");
  }
};


//Socket functions

//Write an integer to socket
size_t write_int(int sock,int message){
  size_t result=write(sock,&message,sizeof(int));
  if(result<=0) throw  sock_exception(); 
  return result;
}

//Write  a string to socket
size_t write_string(int sock,string message){
  size_t length=message.size()+1;
  write_int(sock,length);
  size_t result=write(sock,message.c_str(),length);   
  if(result<=0) throw  sock_exception(); 
}

//Read an integer from  socket
size_t read_int(int sock,int* output){
  size_t result= recv(sock,output, sizeof(int),0);
  if (result<=0) throw  sock_exception();
  return result;   
}

//Read string from  socket
size_t read_string(int sock,int length,string* output ){
  if (length>0){
    char* text=(char*)malloc(length);
    size_t result= recv(sock,text,length,0);
    if (result<=0){
      free(text);
      throw  sock_exception(); 
    }
    *output=text;
    free(text);
    return result;
  }
  else return true;
}

// Socket reaction functions
//return  user status to the socket  
void write_User(int sock,int length){
  string username;
  size_t result=read_string(sock,length,&username); 
  string  response;
  if (counter.checkUser(&username))response="OK";  
  else response="ERR";
  write_string(sock,response);
}

//Write sqtd config file to the socket
void write_Conf(int sock,string* configFile){
  size_t result;
  ifstream conf(configFile->c_str());
  string confline;
  if(conf){
    while (getline(conf,confline)) result=write_string(sock,confline); 
  }
  else {
    confline=_("Can not open configuration file")+ ' ' + *configFile;
    result=write_string(sock,confline);
  } 
  result=write_int(sock,0); 
}

//Write user(s) limits  to the socket
void write_Map(int sock, map < string, map<string, long long > >* limits){
  int length;
  string username;
  ostringstream os;
  size_t result= read_int(sock,&length);
  result=read_string(sock,length,&username);
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
}

//Listen socket input 
void* response (void*  client_sock){
  int sock=*((int*)client_sock);
  free(client_sock);
  int length=0;
  string text;     
  ssize_t result;
  try {
    while(canwork){
      result= read_int(sock,&length);
      if (length>0) write_User(sock,length);
      else switch(-length){
	case 1:
          write_Conf(sock,counter.getConf()->getCommandLine()->getConfigFileName());
	  break;
        case 2:
          write_Map(sock,counter.getConf()->getLimits());	   
	  break;
        case 3:
          write_Map(sock, counter.getTraf()); 
	  break;
	}
    }
  }
  catch (sock_exception& e){ 
    close(sock);
    return NULL;
  }
  close(sock);
  return NULL;
}

//Create socket and start responce function  
void* keep_connection(void* unused){
  while(canwork) {
    int* client_sock;
    client_sock=(int*)malloc(sizeof(int));
    socklen_t client_len;
    struct  sockaddr_un client_addr;
    *client_sock = accept (serv_sock,(struct sockaddr *)&client_addr, &client_len);
    //Starting socket connection keeper thread
    pthread_t response_thread;
    pthread_create(&response_thread,NULL,&response,client_sock);
  } 
  return NULL;
}


//main finction
int main(int argc,char**argv){
   setlocale( LC_ALL, "" );
   bindtextdomain( TEXTDOMAIN, LOCALE_DIR);
   textdomain( TEXTDOMAIN );
  //Parse command line parameters 
  command_line cmdl(argc,argv);
  //Configuring sqtd
  config_file conf(&cmdl);
  conf.reconfig();
  logger.setTarget(conf.getLogFile(),cmdl.getNoDaemonMode());
  logger.put(0,_("Starting") +' '+ string(program_name));  
  ostringstream os; 
  pid_t pid,sid;
  bool configured;
  int iret;  
  //Registering signal handlers;
  void (*prev_fn)(int);
  canwork= true; 
  prev_fn = signal (SIGTERM,my_terminate);
  prev_fn = signal (SIGABRT,my_terminate);
  //Starting as daemon
  if(!cmdl.getNoDaemonMode()){
    logger.put(1,_("Go to daemon mode"));    
    pid=fork();
    if (pid > 0)  {
      exit(0); 
    }
    if(pid<0){
      logger.put(0, _("Can not start as a daemon"));
      exit(1);
    } 
    //Write  pidfile 
    logger.put(2,_("Opening pid file")+ string(" '")+ *conf.getPidFile()+string("'"));
    ofstream pidfile(conf.getPidFile()->c_str(),ios_base::out);
    if (!pidfile) {
      logger.put(0, _("Can not open pid file")+string(" '")+*conf.getPidFile()+string("'") ) ; 
    }
    else {
      logger.put(2,_("Write pid to the pid file") +string(" '")+*conf.getPidFile()+ string("'") );
      pidfile << getpid()<<endl;
      pidfile.close();
    }
  }
  logger.put(2,_("Set session id"));
  sid=setsid();
  logger.put(2,_("Changing working directory"));
  iret=chdir("/");
  //Configuring counter 
  counter.setConf(&conf);
  //Open socket
  unlink(conf.getSockFile()->c_str());
  serv_sock=socket(PF_LOCAL,SOCK_STREAM,0);
  struct sockaddr_un  serv_addr;  
  serv_addr.sun_family=AF_LOCAL;
  strcpy(serv_addr.sun_path,conf.getSockFile()->c_str()) ;
  if (bind(serv_sock,(struct sockaddr *)&serv_addr,SUN_LEN(&serv_addr))){
    logger.put(0,_("Can not create  socket") +string(" '")+ *conf.getSockFile()+string("'") );
    logger.put(0,_("Error starting") + ' '+ string(program_name));
    logger.put(0,_("Exit"));  
    exit(1);
  }
  int ret =chown(conf.getSockFile()->c_str(),getUid(conf.getSockUser()),getGid(conf.getSockGroup()));
  chMod(conf.getSockFile(),conf.getSockMod());
  listen(serv_sock,10);
  //Starting thread for socket listening 
  pthread_t connection_thread;
  pthread_create(&connection_thread,NULL,&keep_connection,NULL);
  // Calculate traffic  
  while (canwork){
    if(!counter.calc(&canwork)) {
      logger.put(0,_("Can not calculate user traffic"));
    }
    logger.put(2, _("Reconfiguring sqtd")) ; 
    configured=conf.reconfig();
    if (!configured){
        logger.put(0,_("Can not reconfigure sqtd"));
    }
    os<< _("Waiting") <<" "<<conf.getCheckInterval() <<_("s");
    logger.put(1, os.str());
    os.str("");
    sleep(conf.getCheckInterval());
  }
  close(serv_sock);
  unlink(conf.getPidFile()->c_str());
  unlink(conf.getSockFile()->c_str());
  logger.put(0,_("Exit")+string(" ")+  string(program_name));
  return 0;  
}

