#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <map>
#include <sstream>
#include <algorithm>

using namespace std;

const char* program_name;
int interactive;
string lightmode;

void print_usage (ostream *dest , int exit_code){
  *dest <<  "Usage: " << program_name << " options " << endl;
  *dest <<  " -i --interactive             ineractive mode"<<endl 
        <<  " -h --help                    Display this usage information." << endl
        <<  " -l --light-mode              Return OK on error"              << endl
        <<  " -s --socket   filename       the sqtd socket file to connect" << endl;
        
  exit (exit_code);
}

void help(){

  cout << "Enter command" << endl
       << "The avaiable commands is" << endl
       << "help               - show this help"  << endl
       << "config             - show current sqtd config" << endl
       << "limits [USERNAME]  - show limits  for all or for one user"   << endl       
       << "traf   [USERNAME]  - show traffic for all or for one user"   << endl       
       << "user    USERNAME   - check user status (OK -can access , ERR -can not acess)"<< endl       
       << "quit               - exit programm"   << endl
       << "Format of USERNAME : domain\\login, where \\ is domain delimiter" << endl;  
}

void prompt(){
  cout << "cmd>";
}


class filesock_client{
private:
  bool               _isConnected;
  int                _sock_fd;
  struct sockaddr_un _serv_addr ;
  int _verbose;
  string _default;
public: 
  filesock_client(char* socket_file,int verbose,string default_responce){
    _isConnected=false;
    _sock_fd = socket (PF_LOCAL, SOCK_STREAM, 0);
    _serv_addr.sun_family = AF_LOCAL;
    strcpy (_serv_addr.sun_path, socket_file); 
    _verbose=verbose;
    _default=default_responce;
  }
  
  bool Connect(){  
    if(_isConnected) return _isConnected;
    if ( connect (_sock_fd, (struct sockaddr *)&_serv_addr, SUN_LEN (&_serv_addr))==0) _isConnected=true;
    else if(_verbose) cerr << "Can not connect to the socket " << _serv_addr.sun_path 
                           << "check the sqtd is started and socket file exists"<< endl;
    return _isConnected;
  }

  void Disconnect(){close (_sock_fd);_isConnected=false;};
  bool isConnected(){return _isConnected;}

  bool put(int message){
    if(!Connect()) return _isConnected;
    if(write (_sock_fd, &message, sizeof (int))==-1) Disconnect();
    return _isConnected;
  }

  bool put(const char* message){
    int length = strlen (message) + 1;
    ssize_t sended;
    if (!put(length)) return false;
    if(write (_sock_fd, message, length)==-1) return false; 
    return true;
  }

  char* get(char **responce){
    int length=0;
    ssize_t getted;
    if (read (_sock_fd, &length, sizeof (length)) == 0) return NULL;
    if (!length>0)return NULL;
    *responce = (char*) malloc (length);
    getted=read (_sock_fd, *responce, length);
    if (getted!=-1) return *responce;
    else {
      Disconnect();
      return NULL;
    }  
  }

  void showConfig(){ 
    char* responce;
    if(put(-1)) while(get(&responce)){
      cout <<responce << endl;
      free(responce);
    }	  
  }
    
  
  void showLimit(string username){ 
    char* responce;
    if(put(-2)&& put(username.c_str())) while(get(&responce)){
      cout <<responce << endl;
      free(responce);
    }	  
  }

  void showTraf(string username){ 
  char* responce;
  if(put(-3)&& put(username.c_str()))  while(get(&responce)){
      cout <<responce << endl;
      free(responce);
    }	
  }
  
  void showUser(string username){ 
    transform(username.begin(),username.end(),username.begin(),::tolower);
    if(!Connect()) {
      if(_verbose)cout <<"Return default value (can be changed with --lightmode option)" << endl; 
      cout <<  _default << endl;
      return;
    } 
    if(!put(username.c_str())) {
      if(_verbose)cout <<"Return default value (can be changed with --lightmode option)" << endl; 
      cout <<_default<<endl;
      return;
    }
    char* responce=NULL;
    responce=get(&responce);
    if(responce){
	  cout << responce <<endl;
	  free (responce); 
    }
    else {
      cout <<"Return default value (can be changed with --lightmode option)" << endl; 
      cout << lightmode << endl;
    }
 }

};

int main(int argc,char** argv){
  /*Анализ параметров командной строки*/
  const char*  const  short_options="ihls:"; 
  const struct option long_options[]={
    {"interactive"   , 0, NULL, 'h'  },
    {"help"          , 0, NULL, 'h'  },
    {"light-mode"    , 1, NULL, 'l'  },
    {"socket"        , 1, NULL, 's'  },
    {NULL            , 0, NULL, 0    }
  };
  
  char* socket_file= NULL;
  interactive=0;
  lightmode="ERR";
  program_name=argv[0];
  int next_option; 
  do{
    next_option = getopt_long (argc, argv, short_options, long_options, NULL);
    switch (next_option){
      case 'i':   
	interactive = 1;
	break;
      case 'h':   
	print_usage (&cout, 0);
      case 'l':   
	lightmode= "OK";
      break;
      case 's':   
	socket_file  = optarg;
	break;
      case '?': 
	print_usage (&cerr, 1);
      case -1:    
	break;
      default:    
	abort ();
      }
  }
  while (next_option != -1);
  if (!socket_file){
    cerr << "Socket file not specified";
    print_usage (&cerr, 1);
  } 
  filesock_client con(socket_file,interactive,lightmode);
  string input;
  /*Iteractive mode*/
  if(interactive){
    map <string, int> keyValue;
    keyValue[""]=1; 
    keyValue["HELP"]=2;
    keyValue["CONFIG"]=3;
    keyValue["LIMITS"]=4;
    keyValue["TRAF"]=5;
    keyValue["USER"]=6;
    keyValue["QUIT"]=7;
    help();
    prompt();
    while(getline(cin,input)){
      stringstream ss(input);
      string key;
      string value; 
      ss >> key;
      ss >>value;
      transform(key.begin(),key.end(),key.begin(),::toupper);
      if (keyValue.count(key)){
	switch(keyValue[key]){
  	 case 1:
	   prompt();
	   break;
	 case 2:
	  help();
	  break;
	case 3:
	  con.showConfig();
	  break;
	case 4:
	  con.showLimit(value);
	  break;
	case 5:
	  con.showTraf(value);
	  break;
	case 6:
	  con.showUser(value);
	  break;
	case 7:
	  exit(0);
	  break;
	}
      }
      prompt(); 
    }  
  }
  //Not Interactive mode
  else while (getline(cin , input)) con.showUser(input);
  con.Disconnect();
  return 0;
}


