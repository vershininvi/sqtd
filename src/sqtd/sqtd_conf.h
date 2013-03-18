#ifndef TRACON_CONF
#define TRACON_CONF
#include "log_buffer.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <grp.h>


using namespace std;

class sqtd_conf{
 private:
  string     _accessLogFile ;   
  long       _checkInterval;
  string     _allowFile;
  string     _actionScript;
  string     _denyFile;
  string     _logFile;
  string     _pidFile;
  map <string,int> _keyValues;
  map< string, map <string,long long> > _limits; 
  vector <string> _allowList;

  bool canReadFile(string filename){
    ifstream ifs;
    ifs.open(filename.c_str());
      if(ifs.is_open()){
	tlog.put(1,  "Файл доступен для чтения " + filename); 	
        ifs.close();
	return true;
      }   
      else {
	tlog.put(0,  "Невозможно открыть файл " + filename); 	
	return false;
      }	
  }

  bool canWriteFile(string filename){
      ofstream ofs;
      ofs.open(filename.c_str(),ios::app);
      if(ofs.is_open()){
	tlog.put(1,  "Файл доступен для записи " + filename); 	
        ofs.close();
	return true;
      }   
      else {
	tlog.put(0,  "Невозможно открыть файл для записи " + filename); 	
	return false;
      }	
    }
  
  long long tokenToLong(string token ){
    long long result;
      result=atoll(token.c_str());
      if(token[token.length()-1]=='K')    result *= 1024;
      else if(token[token.length()-1]=='M') result *= 1048776;
      else if(token[token.length()-1]=='G') result *= 1073741824;
      return result;    
   }

  bool checkKeyValue(string key ){
    tlog.put(1, "Проверка параметра конфигурации " + key);
    switch(_keyValues[key]){
    case 2:
      if  (_accessLogFile.compare("")==0){
	tlog.put(1,  "Не задан файл лога squid\n \
Пример :  AccesLogFile /var/log/squid/access.log\n \
Установлено значение по умолчанию  '/var/log/squid/access.log'");
	 _accessLogFile="/var/log/squid/access.log";
      } 
      return canReadFile(_accessLogFile);	 
      case 3:
	if(_actionScript.compare("")==0){
	    tlog.put(1,"Не задан файл скрипта \n  \
Пример :  ActionScript /etc/sqtd/action.sh  \
Имя файла скрипта установлено по умолчанию в /etc/sqtd/action.sh");
	    _actionScript= "/etc/sqtd/action.sh";}
	return canReadFile(_actionScript);
      case 4:
	if(_checkInterval<=0){
	  tlog.put(1,"Не задан интервал проверок (c)\n \
Пример :  CheckInterval 300\n \
Интервал проверок установлен по умолчанию на 5 мин (300с))");
	  _checkInterval = 300;
	}  
	
	return true;
      case 5:
	if(_allowFile.compare("")==0){
	  tlog.put(1, "Не задан файл групы  доступа\n \
Пример :  AllowFile   /var/lib/sqtd/allow.lst");
	  _allowFile= "/var/lib/sqtd/allow.lst";
	} 
	 return canWriteFile(_allowFile);
      case 6:
	if(_limits.empty()) { 
	tlog.put(0, "Не заданы лимиты \n \
Пример : Limit    g:MyDomain\\GroupName:d:20M");
	return false;
	 }
	 return true;	 
      case 7: 
	 if (_denyFile.compare("")==0){ 
	   tlog.put(1, "Не задано имя файла группы запрета доступа \n\
Пример :  DenyFile /var/lib/sqtd/deny.lst\n \
Установлено значение по умолчанию /var/lib/sqtd/deny.lst\n");		    
	   _denyFile = "/var/lib/sqtd/deny.lst";
	 }
	 return canWriteFile(_denyFile);
      case 8: 
        if (_logFile.compare("")==0){ 
	  tlog.put(1,"Не задано имя файла лога\n \
Пример :  logFile /var/log/sqtd.log)\n \
Установлено значение по умолчанию /var/log/sqtd.log\n");
	  _logFile="/var/log/sqtd.log";
        }
        return canWriteFile(_logFile);
      case 9: 
	 if(_pidFile.compare("")==0){
      	  tlog.put(1, "Не задано имя файла идентификатора процесса \n \
Пример :  pidFile /var/lib/sqtd.pid\n \
Установлено значение по умолчанию /var/lib/sqtd/sqtd.pid");
	  _pidFile="/var/lib/sqtd/sqtd.pid";
         }
        return canWriteFile(_pidFile);
      default:
	return  true;
      }
    }  
   
  bool addLimit(string limit){
    tlog.put (1,"Добавление лимита ");
    stringstream ss(limit);
    int pos=0,ltype=0 ;
    string token,lname,lperiod;
    long long lcount;
    struct group *grp;
    char **current;
    string periods="hdm"; 
    while (getline(ss,token,':')){
      pos ++;
      switch(pos){
      case 1:
	if (token.compare("u")==0) ltype=1;
        else if(token.compare("g")==0) ltype=2;
        else {
	  tlog.put(0,"Ошибка добавления лимита");
	  tlog.put(0, "Тип лимита может быть либо g (группа) либо u (пользователь) ") ;
	  return false;
        }
	break;
      case 2:
	lname = token;
	break;
      case 3: 
	lperiod=token;
	if (lperiod.length()!=1){
	  tlog.put(0,"Ошибка периода лимита (h - час,d-день,m-месяц), период='" + lperiod + "'");
	  return false;
        }
        
        if (periods.find(lperiod)==string::npos) {
	  tlog.put(0,"Ошибка периода лимита (h - час,d-день,m-месяц),период='"+ lperiod + "'") ;
	  return false;
	}
	break;
      case 4:
	lcount=tokenToLong(token);
	break;
      default:
	tlog.put(0, "Игнорирование лишних полей лимита") ;
	break;
      }  
    }  
    if (ltype==1){
      transform(lname.begin(),lname.end(),lname.begin(),::tolower);
      ostringstream os;
      os<<lcount;  
      tlog.put(2,"Добавление лимита пользователя " + lperiod + " "+ os.str() + " " +lname);
      _limits[lperiod][lname]=lcount; 
      _allowList.push_back(lname);
      
    }
    else {
      grp=  getgrnam(lname.c_str()); 
      tlog.put(2,"Добавление группового лимита");
      int usercount=0;
      if (grp!=NULL) {
	for (current=grp->gr_mem; (*current!= NULL); current++) {
	  string t (*current);  
          ostringstream os;
          os<<lcount;  
	  transform(t.begin(),t.end(),t.begin(),::tolower);
	  tlog.put(2,"Добавлен лимит пользователя " + lperiod  + " " +  os.str() +  " " +  t); 
          _allowList.push_back(t);  
	  _limits[lperiod][t]=lcount;  
	  ++usercount;
        }
	if (usercount==0){
	  _limits[lperiod]["-"]=lcount;
	  tlog.put(0, "Группа " + lname +"  не содержит ни одного пользователя");
        } 
        else {
         ostringstream os;
         os<<usercount; 
         tlog.put(2,"Количество пользователей в группе " + lname +" " + os.str());

        }
      }
      else {
	tlog.put(0,"Невозможно получение информации о группе " + lname); 
	return false;
      }

    } 
    return true;  
  }
  
  void  reinit(){
    _accessLogFile="";   
    _checkInterval=0;
    _allowFile="";
    _actionScript="";
    _denyFile="";
    _limits.clear();
  } 

 public:
  sqtd_conf(){
    _keyValues[""]=1;
    _keyValues["ACCESSLOGFILE"]=2;
    _keyValues["ACTIONSCRIPT"]=3;
    _keyValues["CHECKINTERVAL"]=4;
    _keyValues["ALLOWFILE"]=5;
    _keyValues["LIMIT"]=6;
    _keyValues["DENYFILE"]=7;
    _keyValues["LOGFILE"]=8;
    _keyValues["PIDFILE"]=9;
    reinit();
  }

  bool reconfig(string filename){
    reinit();    
    tlog.put(2,"Открытие конфигурационного файла " + filename);
    ifstream conf(filename.c_str());
    if(!conf) {
      tlog.put(0, "Ошибка открытия конфигурационного файла '" + filename +"'");
      exit(1) ;
    }	
    int lineNom=0;
    string confline;
    tlog.put(2,"Конфигурация программы");
    while(getline(conf,confline)){
      ++lineNom;
      ostringstream os;
      os <<lineNom; 
      stringstream ss(confline);
      string key;
      string value; 
      ss >> key;
      transform(key.begin(),key.end(),key.begin(),::toupper);
      switch (_keyValues[key]) {
      case 1:
	tlog.put(1,"Пропуск пустой строки " +os.str());
	break;
      case 2:
	ss >> _accessLogFile;
	break;	
      case  3:
	ss >> _actionScript;
	break;

      case  4:
	ss >> _checkInterval;
	break;
      case  5:
	ss >> _allowFile;
	break;
      case 6:
	ss >> value;
	if(!addLimit(value)){
	  tlog.put(0, "Ошибка в строке конфигуационного файла " + os.str()) ; 
	  return false;
	}
	break;
      case 7:
	ss >> _denyFile;    
	break;
      case 8:
	ss >> _logFile;    
	break;
      case 9:
	ss >> _pidFile;    
	break;

      default :
	tlog.put(0,"Ошибка в строке конфигуационного файла " +os.str()); 
        tlog.put(0,"Неизвестное значение ключа");
        tlog.put(0,confline);
	return false;
      }
      if (!checkKeyValue(key)) {
	tlog.put(0,"Ошибка в строке конфигуационного файла " +os.str()); 
	tlog.put(0,confline);
        return false;
      }   
    }
    return true;
  }  

  bool check() {
    bool result=true;
    tlog.put (2, "Проверка конфигурации");
    for( map<string,int>::iterator i=_keyValues.begin();i!=_keyValues.end();++i)
      result=checkKeyValue(i->first) && result;
    return result;
  }

  map <string, map<string, long long> > *getLimits()          {return &_limits;}
  string                                 getAccessLogFile  () {return  _accessLogFile ;} 
  long                                   getCheckInterval  () {return  _checkInterval;}
  string                                 getAllowFile      () {return  _allowFile;} 
  string                                 getActionScript   () {return _actionScript;}
  string                                 getDenyFile       () {return _denyFile;}
  vector<string>                        *getAllowList      () {return &_allowList;}
  string                                 getLogFile        () {return _logFile;}
  string                                 getPidFile        () {return _pidFile;}
};
#endif

