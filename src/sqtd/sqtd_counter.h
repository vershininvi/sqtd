#ifndef SQTD_COUNTER
#define  SQTD_COUNTER
#include "log_buffer.h"
#include "squid_accesslog.h"
#include "sqtd_conf.h"
#include <map>
#include <vector>
#include <algorithm>


class sqtd_counter {
 private:
  time_t _mbeg; 
  time_t _dbeg; 
  time_t _hbeg;
  time_t _mbeg_old;
  time_t _dbeg_old;
  time_t _hbeg_old;  
  access_log _al;
  log_buffer* _tlog;
  sqtd_conf* _conf;
  map < string, map <string, long long> > _traf;

  void settime(){
    time_t tis =time(NULL);
    struct tm tbd;
    localtime_r(&tis,&tbd);
    tbd.tm_sec=0 ; 
    tbd.tm_min=0 ;  _hbeg=mktime(&tbd);
    tbd.tm_hour=0 ; _dbeg=mktime(&tbd);
    tbd.tm_mday=1 ; _mbeg=mktime(&tbd);
  } 
   
  void clean() {
    if(_hbeg!=_hbeg_old) {for(map<string, map<string,long long> >::iterator i=_traf.begin();i!=_traf.end();++i) 
	i->second["h"]=0;_hbeg_old=_hbeg;}
    if(_dbeg!=_dbeg_old) {for(map<string, map<string,long long> >::iterator i=_traf.begin();i!=_traf.end();++i) 
	i->second["d"]=0;_hbeg_old=_hbeg;}
    if(_mbeg!=_mbeg_old) {for(map<string, map<string,long long> >::iterator i=_traf.begin();i!=_traf.end();++i) 
	i->second["m"]=0;_hbeg_old=_hbeg;}
  } 
  
  void replace(string *source ,string pattern ,string replacement){
    size_t pos = source->find(pattern);
    if ( pos != string::npos ) 
      source->replace( pos, pattern.length(),replacement  );  
  }
 
 public:
  
  bool calc(bool *canwork){   
    ostringstream os;
    _tlog->put(2,"Рассчет траффика");
    settime();
    clean();
    if (int iret=_al.open()){
      if (iret==2) _traf.clear();
      while (_al.next()){
    	if(!*canwork){
    	  _al.close();
    	  return true;
    	}
    	try {
    	  vector<string> rec= _al.getFields();
          if (rec.size()!= 10) throw 0;
    	  stringstream field0(rec[0]);
    	  time_t logtime;
    	  field0>>logtime;
          long long bytes= atoll(rec[4].c_str());
    	  string username= rec[7];
    	  string result=rec[3];
    	  if (result.compare("TCP_MISS/200")!=0) continue;
    	  transform(username.begin(),username.end(),username.begin(),::tolower);
          replace(&username,"\\\\","\\");
    	  if (logtime >=_mbeg) {
    	    _traf[username]["m"]+=bytes;
    	  }
    	  if (logtime >=_dbeg){
    	    _traf[username]["d"]+=bytes;
    	  }
    	  if (logtime >=_hbeg){
    	    _traf[username]["h"]+=bytes;
    	  }
        }
    	catch(...){
          os.str("");
    	  os << "Wrong record " << _al.getPos() << endl << _al.getRecord();
    	  _tlog->put(1,os.str());
    	  _tlog->print();
    	}
     }
     _al.close();
    }
    else {
      _tlog->put(0,"Can not open  squid acces log file" );
      return false;
    }
    return true;
  }
  
  map < string, map <string, long long> > *getTraf(){return &_traf;}
  map < string, map <string, long long> > *getLimits(){return _conf->getLimits();}
  
  bool checkUser(string* username){
    bool pass=false;
    if(_conf->getLimits()->count(*username)==0) return false; //Нет в лимитах  - не пускать 
    map <string, long long> limit= _conf->getLimits()->at(*username); //Лимиты пользователя
    for ( map<string,long long>::iterator i=limit.begin();i!=limit.end();++i){
      if(i->second==0) continue; //Не ограничен
      if (i->second<=_traf[*username][i->first]) return false; 
    } 
    return true;//Пользователь в списке, траффик либо не ограничен либо не указан 
  }
  

  void setConf(sqtd_conf* conf){_conf=conf;_al.setConf(_conf); }
  sqtd_conf* getConf(){return _conf;}
  void setLog (log_buffer* log){_tlog=log;_al.setLog(_tlog);}


  
};

#endif /*SQTD_COUNTER*/
