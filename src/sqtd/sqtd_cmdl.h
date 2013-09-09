#ifndef COMMAND_LINE
#include "../../config.h" 
#define COMMAND_LINE
#include "log_buffer.h"
#include <map>
#include <cstdlib>
#include <sstream>
#include <getopt.h>

using namespace std;
const char*  const  short_options="c:hnv"; 
const struct option long_options[]={
    {"config"        , 0, NULL, 'c'},  
    {"help"          , 0, NULL, 'h'},   
    {"no-daemon"     , 0, NULL, 'n'}, 
    {"version"       , 0, NULL, 'v'},
    {NULL            , 0, NULL, 0  }
}; 

class command_line{
 private:
  string _programm_name;      
  string _config_file;
  int     _no_daemon;

  void print_usage (ostream *dest,int exit_code){
    *dest <<  "Usage: " << _programm_name << " [OPTIONS] " << endl
	  <<  " -c --config filename      full path to configuration file" << endl
	  <<  " -h --help                 Display this usage information." << endl
      	  <<  " -n --no-daemon            do not start as daemon, output log to console" << endl
	  <<  " -v --version              Print version messages."<<endl<<endl 
	  <<  "The more information about programm:\n\t man sqtd" << endl;
    exit (exit_code);
  }
  
  void print_version(){
    cout <<_programm_name <<  endl 
         << "Version : "  <<   VERSION << endl;
    exit(0);
  }

 public:
  command_line(int argc, char** argv){
    _programm_name=argv[0];      
    _config_file="";
    _no_daemon=0;
    int next_option;
      do{
	next_option = getopt_long (argc, argv, short_options, long_options, NULL);
	switch (next_option) {
	case 'c':   
	  _config_file  = optarg;
	  break;
	case 'h':  
	  print_usage (&cout, 0);
	case 'n':   
	  _no_daemon=1;
	  break;
	case 'v':  
	  print_version();
	case '?': 
	  print_usage (&cerr, 1);
	case -1:    
	  break;
	default:    
	  abort ();
	}
      }
      while (next_option != -1);
      if (_config_file.compare("")==0) _config_file="/etc/sqtd/sqtd.conf";
  }

  string*  getProgrammName()  {return &_programm_name;}
  string*  getConfigFileName(){return &_config_file; }
  int    getNoDaemonMode()    {return _no_daemon;}
  
};
#endif  /*COMMAND_LINE */
