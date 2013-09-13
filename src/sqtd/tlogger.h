#ifndef TLOGGER
#define TLOGGER
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <syslog.h>
#include "sqtd_cmdl.h"

using namespace std;


class tlogger{
 private:
  bool _filterOn;            //Не принимать сообщения с уровнем лога выше _level, по умолчанию false
  int _level;                //Уровень отображаемых сообщений, по умолчанию 0 
  int _target;               //Приемник сообщений 0 - cout, 1 - syslog,2 - файл
  ofstream _logfile;         //Файловый поток  
  int _logLevels[3];
  streambuf  *_backup;
 public:

  tlogger(){
    _filterOn = false;
    _level=0;
    _target=0;
    _logLevels[0] =LOG_ERR;
    _logLevels[1] =LOG_WARNING;
    _logLevels[2] =LOG_INFO;
    _backup=cout.rdbuf();  
  }

  ~tlogger(){
    cout.rdbuf(_backup);
    if(_logfile.is_open()) _logfile.close();  
  }

  void put(int level,string message){
    time_t tis = time(NULL);
    struct tm  buff;
    localtime_r(&tis,&buff);
    ostringstream timestamp;
    timestamp << buff.tm_year+1900 <<"-"<<buff.tm_mon+1<<"-"<<buff.tm_mday<<" " << 
      buff.tm_hour<<":"<<buff.tm_min<<":"<<buff.tm_sec;
    if (_filterOn &&(level > _level)) return; 
      switch (_target){
      case 0: //cout
	cout << timestamp.str() << " " << message << endl;
	break;
      case 1: //syslog
	if(level>2)level=2;
	if(level<0)level=0;
	syslog(_logLevels[level],"%s",message.c_str());
	break;
      case 2: //file
	_logfile << timestamp.str() << " "<< message << endl;      
	_logfile.flush();
	break;
      } 
   }

  void setLevel(int level){_level=level;}

  int getLevel(){return _level;} 

  bool getFilterOn(){return _filterOn;}

  void setFilterOn(bool filteron){ _filterOn=filteron;}

  void setTarget(string* logFile, int noDaemon){
    if(noDaemon)  _target=0;
    else {
      if(logFile->compare("")==0) _target=1;
      else{
	_logfile.open(logFile->c_str(),ios::app);
	if(_logfile.is_open()){
          _target=2;
          cout.rdbuf(_logfile.rdbuf());
        }
	else{
	  cout << "Can not open log file " << *logFile << endl;
	  exit(1);
	}
      }
    }
  }

  int getTarget(){return _target;}
};

tlogger logger;

#endif  /*TLOGGER*/
