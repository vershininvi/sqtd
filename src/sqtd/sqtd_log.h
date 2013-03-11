#ifndef TRACON_LOG
#define TRACON_LOG
#include <list>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;
struct tmessage{
  int level;
  string message;

};

class tracon_log{
 private:
  int _level;
  int _target;
  streambuf *_cout; 
  ofstream _logfile;
  list<tmessage> _messages;
  
  
 public:
  tracon_log(){ _cout = cout.rdbuf();_target=0;}
  ~tracon_log(){cout.rdbuf(_cout);}
  void print (){
    switch (_target){
    case 0: //cout
      for (list<tmessage>::iterator i=_messages.begin();i!=_messages.end();++i){
	if(i->level<=_level)  cout << i->message << endl;
      }
    case 1: //file
      _logfile.flush();
      break;
    case 2: //syslog
      
      break;
    }
    _messages.clear();
  }

  void put(int level,string message){
    if( level <= _level){
      tmessage m;
      m.level=level;
      m.message=message;
      _messages.push_back(m);
     }
  }

  void setLevel(int level){_level=level;}

  int getLevel(){return _level;} 

  bool setTarget(string target){ 
    if(target.compare("syslog")==0){
      _target=2;
      cout.rdbuf(_cout);
      return true;
    }
    else {
      _logfile.open(target.c_str(),ios::app);
      if(!_logfile.fail()){  
        cout.rdbuf(_logfile.rdbuf());
        target=1;
      }
      else return false;
    }
    return true;
  }
  
};
tracon_log tlog;
#endif  /*TRACON_LOG*/
