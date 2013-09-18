#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <map>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "../../config.h"
#include <libintl.h>
#include <locale.h>
#define _(STRING)    gettext(STRING)


using namespace std;

const char* program_name;
int interactive;
string lightmode;
ofstream debug_stream;


void print_version(){
    cout <<program_name <<  endl 
         << _("Version : ")  <<   VERSION << endl;
    exit(0);
  }

string url_decode(string input){
  ostringstream os;
  for (string::iterator i= input.begin();i!=input.end();++i){
    if(*i=='%'){
      stringstream ss; 
      string nom;  
      if(++i!=input.end()) nom=*i;
      if(++i!=input.end()) nom+=*i;
      ss<<std::hex<< nom;
      int inom ;
      ss >> inom; 
      os << (char) inom;
    }
    else os << *i;
  }
  return os.str();
}


void print_usage (ostream *dest , int exit_code){
  *dest <<  _("Usage: ") << program_name <<_(" options ") << endl;
  *dest <<  _(" -i --interactive             Ineractive mode.")<<endl 
        <<  _(" -h --help                    Display this usage information.") << endl
        <<  _(" -l --light-mode              Return OK on error.")              << endl
        <<  _(" -s --socket   filename       The sqtd socket file to connect (default /var/lib/sqtd/sqtd.sock).") << endl
        <<  _(" -d --debug    filename       The output debug info to specified file. ") << endl;
        
  exit (exit_code);
}

void help(){
  cout << _("Enter command") << endl
       << _("The avaiable commands is") << endl
       << _("help               - show this help")  << endl
       << _("config             - show current sqtd config") << endl
       << _("limits [USERNAME]  - show limits  for all or for one user")   << endl       
       << _("traf   [USERNAME]  - show traffic for all or for one user")   << endl       
       << _("user    USERNAME   - check user status (OK -can access , ERR -can not acess)")<< endl       
       << _("quit               - exit programm")   << endl
       << _("Format of USERNAME : domain\\login, where \\ is domain delimiter") << endl;  
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
  filesock_client(string socket_file,int verbose,string default_responce){
    _isConnected=false;
    _sock_fd = socket (PF_LOCAL, SOCK_STREAM, 0);
    _serv_addr.sun_family = AF_LOCAL;
    strcpy (_serv_addr.sun_path, socket_file.c_str()); 
    _verbose=verbose;
    _default=default_responce;
  }
  
  bool Connect(){  
    if(_isConnected) return _isConnected;
    if ( connect (_sock_fd, (struct sockaddr *)&_serv_addr, SUN_LEN (&_serv_addr))==0) _isConnected=true;
    else if(_verbose) cerr << _("Can not connect to the socket ") << _serv_addr.sun_path  << endl
                           << _("Check the sqtd is started and socket file exists")<< endl;
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
      if(_verbose)cout <<_("Return default value (can be changed with --lightmode option)") << endl; 
      cout <<  _default << endl;
      return;
    } 
    if(!put(username.c_str())) {
      if(_verbose)cout <<_("Return default value (can be changed with --lightmode option)") << endl; 
      cout <<_default<<endl;
      return;
    }
    char* responce=NULL;
    responce=get(&responce);
    if(responce){
      if(debug_stream.is_open()) debug_stream << _("\t\t\t\tResponce :")<< responce << endl;
	  cout << responce <<endl;
	  free (responce); 
    }
    else {
      cout <<_("Return default value (can be changed with --lightmode option)") << endl; 
      cout << lightmode << endl;
    }
    
 }

};

int main(int argc,char** argv){
   program_name=argv[0];
   setlocale( LC_ALL, "" );
   bindtextdomain( "sqtd", "/usr/share/locale" );
   textdomain( "sqtd" );


  /*Анализ параметров командной строки*/
  const char*  const  short_options="d:ihls:v"; 
  const struct option long_options[]={
    {"debug"         , 1, NULL, 'd'  },
    {"interactive"   , 0, NULL, 'i'  },
    {"help"          , 0, NULL, 'h'  },
    {"light-mode"    , 1, NULL, 'l'  },
    {"socket"        , 1, NULL, 's'  },
    {"version"       , 1, NULL, 'v'  },
    {NULL            , 0, NULL, 0    }
  };
  
  string socket_file;
  char* debug_file=NULL;
  interactive=0;
  lightmode="ERR";
  program_name=argv[0];
  int next_option; 
  do{
    next_option = getopt_long (argc, argv, short_options, long_options, NULL);
    switch (next_option){
      case 'd':   
	debug_file = optarg;
	break;
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
      case 'v':   
	print_version();
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
  if (socket_file.compare("")==0)  socket_file="/var/lib/sqtd/sqtd.sock";

  if(debug_file){
    debug_stream.open(debug_file,ios::app);
    if (!debug_stream.is_open()) debug_file==NULL;
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
  else while (getline(cin , input)){
      if (debug_file) debug_stream <<_("Query :")<< input; 
      con.showUser(url_decode(input));
  }
  
  con.Disconnect();
  if(debug_stream.is_open())debug_stream.close();
  return 0;
}


