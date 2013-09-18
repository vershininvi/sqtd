#ifndef SQTD_CONF
#define SQTD_CONF
#include "tlogger.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <grp.h>




using namespace std;

class config_file{
 private:
  command_line *                        _cmdl;
  string                                _accessLogFile;   
  long                                  _checkInterval;
  int                                   _debug_level;
  string                                _log_file;
  string                                _pid_file;
  string                                _sock_file;
  string                                _sock_user;
  string                                _sock_group;
  string                                _sock_mod;
  string                                _systemDomainDelimiter;
  string                                _squidDomainDelimiter;

  map <string,int> _keyValues;
  map< string, map <string,long long> > _limits; 

  bool canReadFile(const char* filename){
    ifstream ifs;
    ifs.open(filename);
    if(ifs.is_open()){
      logger.put(2,  _("Reading ") + string(filename) + _("... OK")); 	    
      ifs.close();
      return true;
    }   
    else {
      logger.put(0,  _("Error reading ") + string(filename)); 	
      return false;
    }	
  }

  bool canWriteFile(string filename){
    ofstream ofs;
    ofs.open(filename.c_str(),ios::app);
    if(ofs.is_open()){
      logger.put(2,  _("Writing filename ") + filename +_("... OK")); 	    ofs.close();
      return true;
    }   
    else {
      logger.put(0,_("Error reading ") + filename); 	
      return false;
    }	
  }
  
  long long tokenToLong(string token ){
    long long result;
    result=atoll(token.c_str());
    if(token[token.length()-1]=='K')      result *= 1024;
    else if(token[token.length()-1]=='M') result *= 1048776;
    else if(token[token.length()-1]=='G') result *= 1073741824;
    return result;    
  }

  bool checkKeyValue(string key ){
    logger.put(2, _("Checking configuration key ") + key);
    switch(_keyValues[key]){
    case 2:
      if  (_accessLogFile.compare("")==0){
	logger.put(1, _("The squid access log file not specified") );
	_accessLogFile="/var/log/squid/access.log";
      } 
      return canReadFile(_accessLogFile.c_str());	 
    case 3:
      if(_checkInterval<=0){
	logger.put(1,_("Check interval is not specified"));
	_checkInterval = 300;
      }  
      return true;
    case 4:
      logger.setLevel(_debug_level);
      return true;
    case 5:
      if(_log_file.compare("")==0){
	logger.put(1,_("Log file is not specified"))  ;
      }  
      return true;
    case 6:
      if(_pid_file.compare("")==0) { 
	logger.put(1, _("No pid file specified "));
	_pid_file="/var/lib/sqtd/sqtd.pid";
      }
      return true;	 
    case 7:
      if(_sock_file.compare("")==0) { 
	logger.put(1, _("No sock file specified "));
	_sock_file="/var/lib/sqtd/sqtd.sock";
      }
      return true; 
    case 8:
      if(_sock_user.compare("")==0) logger.put(1, _("The SockOwner not specified"));
      return true; 
    case 9:
      if(_sock_group.compare("")==0) logger.put(1, _("The SockGroup not specified "));
      return true; 
    case 10:
      if(_sock_mod.compare("")==0) logger.put(1, _("The SockMod not specified") );
      return true;
    case 11:
      if(_systemDomainDelimiter.compare("")==0) logger.put(1, _("The SystemDomainDelimiter not specified") );
      return true;
    case 12:
      if(_squidDomainDelimiter.compare("")==0) logger.put(1, _("The SquidDomainDelimiter  not specified") );
      return true;
    default:
      return  true;
    }
  }  
   
  bool addLimit(string limit){
    logger.put (2, _("Adding limit"));
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
	  logger.put(0,_("Can not add limit"));
	  logger.put(0,_("There are two types of limit (g -for group or  u for singe user))")) ;
	  return false;
        }
	break;
      case 2:
	lname = token;
	break;
      case 3: 
	lperiod=token;
	if (lperiod.length()!=1){
	  logger.put(0,_("Error in limit's period  (h - hour,d - day,m - month), period='") + lperiod + "'");
	  return false;
        }
        if (periods.find(lperiod)==string::npos) {
	  logger.put(0,_("Error in limit's period  (h - hour,d - day,m - month), period='") + lperiod + "'");
	  return false;
	}
	break;
      case 4:
	lcount=tokenToLong(token);
	if (lcount==0);
	break;
      default:
	logger.put(0, _("Ignoring all other fields of limit")) ;
	break;
      }  
    }  
    if (ltype==1){
      transform(lname.begin(),lname.end(),lname.begin(),::tolower);
      ostringstream os;
      os<<lcount;  
      logger.put(2,_("Adding user limit ") + lperiod + " "+ os.str() + "\t " +lname);
      _limits[lname][lperiod]=lcount; 
    }
    else {
      grp=  getgrnam(lname.c_str()); 
      logger.put(2,_("Adding group limit "));
      int usercount=0;
      if (grp!=NULL) {
	for (current=grp->gr_mem; (*current!= NULL); current++) {
	  string t (*current);  
          ostringstream os;
          os<<lcount;  
	  transform(t.begin(),t.end(),t.begin(),::tolower);
	  logger.put(2,_("Adding user limit ") + lperiod  + " " +  os.str() +  "\t" +  t); 
	  _limits[t][lperiod]=lcount;  
	  ++usercount;
        }
	if (usercount==0){
	  _limits["-"][lperiod]=lcount;
	  logger.put(0, _("The group is empty: ") + lname);
        } 
        else {
	  ostringstream os;
	  os<<usercount; 
	  logger.put(2,_("The count of user in group  ") + lname +" : " + os.str());
        }
      }
      else {
	logger.put(0,_("Can not get the group ingormation ") + lname); 
	return false;
      }
    } 
    return true;  
  }
  
  void  reinit(){
    _checkInterval=0;
    _debug_level=0;
    _limits.clear();
  } 

 public:
  config_file(command_line* cmdl){
    _cmdl=cmdl;
    _accessLogFile="";   
    _log_file="";
    _pid_file="";
    _sock_file="";
    _sock_user="";
    _sock_group="";
    _sock_mod="";
    _systemDomainDelimiter="";
    _squidDomainDelimiter="";
    _keyValues[""]=1;
    _keyValues["ACCESSLOGFILE"]=2;
    _keyValues["CHECKINTERVAL"]=3;   
    _keyValues["DEBUGLEVEL"]=4;
    _keyValues["LOGFILE"]=5;
    _keyValues["PIDFILE"]=6;
    _keyValues["SOCKFILE"]=7;
    _keyValues["SOCKUSER"]=8;
    _keyValues["SOCKGROUP"]=9;
    _keyValues["SOCKMOD"]=10;
    _keyValues["SYSTEMDOMAINDELIMITER"]=11;
    _keyValues["SQUIDDOMAINDELIMITER"]=12;
    _keyValues["LIMIT"]=13;
  }
  


  bool reconfig(){
    reinit(); 
    string* configFileName=_cmdl->getConfigFileName(); 
    logger.put(2,_("Opening ") + *configFileName);
    ifstream conf(configFileName->c_str());
    if(!conf) {
      logger.put(0, _("Can not open  '") + *configFileName +"'");
      exit(1) ;
    }	
    int lineNom=0;
    string confline;
    logger.put(1,_("Reading configuration file ... ") );
    while(getline(conf,confline)){
      ++lineNom;
      ostringstream os;
      os <<lineNom; 
      if(confline[0]=='#') {
        logger.put(2,_("Skip comment line # ") + os.str());
	continue;
      }       
      stringstream ss(confline);
      string key;
      string value; 
      ss >> key;
      transform(key.begin(),key.end(),key.begin(),::toupper);
      switch (_keyValues[key]) {
      case 1:
	logger.put(2,_("Skip empty line ") +os.str());
	break;
      case 2:
	ss >> _accessLogFile;
	break;	
      case  3:
	ss >> _checkInterval;
	break;
      case 4:
	ss >> _debug_level; 
	break;
      case 5:
	ss>>_log_file;
	break;
      case 6:
	ss>>_pid_file;
	break;
      case 7:	
	ss>> _sock_file;
        break; 
      case 8:	
	ss>> _sock_user;
        break; 
     case 9:	
	ss>> _sock_group;
        break; 
     case 10:	
	ss>> _sock_mod;
        break; 
     case 11:	
	ss>> _systemDomainDelimiter;
        break; 
      case 12:	
	ss>> _squidDomainDelimiter;
        break; 
      case 13:
	ss >> value;
	if(!addLimit(value)){
	  logger.put(0, _("Error adding limit in config file line  ") + os.str()) ; 
	  return false;
	}
	break;
      default :
	logger.put(0,_("Error in config file line ") +os.str()); 
        logger.put(0,_("Unknown key "));
        logger.put(0,confline);
	return false;
      }
      if (!checkKeyValue(key)) {
	logger.put(0,_("Error in config file line ") +os.str()); 
	logger.put(0,confline);
        return false;
      }   
    }
    return true;
  }  

  bool check() {
    bool result=true;
    logger.put (2, _("Checking configuration... "));
    for( map<string,int>::iterator i=_keyValues.begin();i!=_keyValues.end();++i)
      result=checkKeyValue(i->first) && result;
    return result;
  }

  string* getAccessLogFile(){return  & _accessLogFile ;} 
  long getCheckInterval(){return    _checkInterval;}
  long getDebugLevel(){return    _debug_level;} 
  string* getLogFile(){return  & _log_file;}
  string* getPidFile(){return  & _pid_file;}
  string* getSockFile(){return  & _sock_file;}
  string* getSockUser(){return  & _sock_user;}
  string* getSockGroup(){return  & _sock_group;}
  string* getSockMod(){return  & _sock_mod;}
  string* getSquidDomainDelimiter(){return  & _squidDomainDelimiter;}
  string* getSystemDomainDelimiter(){return  & _systemDomainDelimiter;}
  map <string, map<string, long long> >* getLimits(){return  &  _limits;}
  command_line* getCommandLine    () {return _cmdl;};  
};
#endif

