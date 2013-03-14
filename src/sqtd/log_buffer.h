#ifndef LOG_BUFFER
#define LOG_BUFFER
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <syslog.h>
using namespace std;
struct tmessage{
  int level;
  string message;

};

class log_buffer{
 private:
  bool _filterOn;            //Не принимать сообщения с уровнем лога выше _level, по умолчанию false
  int _level;                //Уровень отображаемых сообщений, по умолчанию 0 
  list<tmessage> _messages;  //Буффер сообщений
  int _target;               //Приемник сообщений 0 - cout, 1 - syslog,2 - файл
  ofstream _logfile;         //Файловый поток  
  int _logLevels[3];
 public:
  
  log_buffer(){
    _filterOn = false;
    _level=0;
    _target=0;
    _logLevels[0] =LOG_ERR;
    _logLevels[1]= LOG_WARNING;
    _logLevels[2]=LOG_INFO;
  }

  ~log_buffer(){
    //Закрыть открытый файл
    if(_logfile.is_open()) _logfile.close();  
  }

  bool setTarget(string target){ 
    string co="";
    string sy="syslog";
    if(target.compare(co)==0){
      _target=0;
      return true;
    }
    if(target.compare(sy)==0){
      _target=1;
      return true;
    }
    //Открыть файл
    _logfile.open(target.c_str(),ios::app);
    if(_logfile.is_open()){
      _target=2;
      return true;
    }
    _target=0; 
    return false;
  }

  int getTarget(){return _target;}

  void print (){
    int level; 
    for (list<tmessage>::iterator i=_messages.begin();i!=_messages.end();++i)
    if(i->level<=_level)  
      switch (_target){
      case 0: //cout
	cout << i->message << endl;
	break;
      case 1: //syslog
	if(i->level>2)i->level=2;

	if(i->level<0)i->level=0;

	syslog(_logLevels[i->level],"%s",i->message.c_str());
	break;
      case 2: //file
	_logfile << i->message << endl;      
	_logfile.flush();
	break;
      }
    _messages.clear();
  }

  void put(int level,string message){
    if (_filterOn &&(level > _level)) return; 
    tmessage m;
    m.level=level;
    m.message=message;
    _messages.push_back(m);
  }

  void setLevel(int level){_level=level;}

  int getLevel(){return _level;} 

  bool getFilterOn(){return _filterOn;}

  void setFilterOn(bool filteron){ _filterOn=filteron;}

  int count(){ 
    int ret=0;
    for (list<tmessage>::iterator i=_messages.begin();i!=_messages.end();++i) ret++;
    return ret;
  }
};
log_buffer  tlog;
#endif  /*LOG_BUFFER*/
