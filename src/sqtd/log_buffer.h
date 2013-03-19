#ifndef LOG_BUFFER
#define LOG_BUFFER
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
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
  streambuf  *_backup;
  ostringstream _cout_buf;
                 
 public:
  
  log_buffer(){
    _filterOn = false;
    _level=0;
    _target=0;
    _logLevels[0] =LOG_ERR;
    _logLevels[1]= LOG_WARNING;
    _logLevels[2]=LOG_INFO;
    _backup=cout.rdbuf();  
  }

  ~log_buffer(){
    cout.rdbuf(_backup);
    if(_logfile.is_open()) _logfile.close();  
  }

  bool setTarget(string target){ 
    string co="console";
    string sy="syslog";

    if(target.compare(co)==0){
      _target=0;
      cout.rdbuf(_backup);
      return true;
    }
    if(target.compare(sy)==0){
      _target=1;
      cout.rdbuf(_cout_buf.rdbuf());
      return true;
    }
    //Открыть файл
    _logfile.open(target.c_str(),ios::app);
    if(_logfile.is_open()){
      _target=2;
      cout.rdbuf(_logfile.rdbuf());
      return true;
    }
    _target=0; 
    cout.rdbuf(_backup);
    return false;
  }

  int getTarget(){return _target;}

  void print (){
    int level; 
    
    time_t tis = time(NULL);
    struct tm  buff;
    localtime_r(&tis,&buff);
    ostringstream timestamp;
    timestamp << buff.tm_year+1900 <<"-"<<buff.tm_mon+1<<"-"<<buff.tm_mday<<" " << 
      buff.tm_hour<<":"<<buff.tm_min<<":"<<buff.tm_sec;
 
    for (list<tmessage>::iterator i=_messages.begin();i!=_messages.end();++i)
      
    if(i->level<=_level)  
      switch (_target){
      case 0: //cout
	cout << timestamp.str() << " " << i->message << endl;
	break;
      case 1: //syslog
	if(i->level>2)i->level=2;
	if(i->level<0)i->level=0;
	syslog(_logLevels[i->level],"%s",i->message.c_str());
	if(_cout_buf.str().length()){
    	   syslog(_logLevels[i->level],"%s",_cout_buf.str().c_str());
	   _cout_buf.flush();
	}  
	break;
      case 2: //file
	_logfile << timestamp.str() << " " << i->message << endl;      
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
