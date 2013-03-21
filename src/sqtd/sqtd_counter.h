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
  map < string, map <string, long long> > _traf;
  time_t _mbeg; 
  time_t _dbeg; 
  time_t _hbeg;
  time_t _mbeg_old;
  time_t _dbeg_old;
  time_t _hbeg_old;  
  access_log _al;
  sqtd_conf _conf;
  vector <string> _dl; 

 private:
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
    if(_hbeg!=_hbeg_old) _traf["h"].clear();_hbeg_old=_hbeg;
    if(_dbeg!=_dbeg_old) _traf["d"].clear();_dbeg_old=_dbeg;
    if(_mbeg!=_mbeg_old) _traf["m"].clear();_mbeg_old=_mbeg;
    _dl.clear();
  } 


 public:
  
  vector < string > *getDenyList(){ return &_dl;} 
  sqtd_conf * getConfig(){return &_conf;}
    
  bool calc(bool *canwork){   
    ostringstream os;
    tlog.put(2,"Рассчет траффика\n");
    settime();
    clean();
    if (_al.open()){
      while (_al.next()){
	if(!*canwork){
	  _al.close();
	  return true;
	}
    	vector<string> rec= _al.getFields();
	 
	stringstream field0(rec[0]);  
	time_t logtime; 
	field0>>logtime;
	long long bytes= atoll(rec[4].c_str()); 
	string username= rec[7];
        string result=rec[3];

	if (result.compare("TCP_MISS/200")!=0) continue;  

	transform(username.begin(),username.end(),username.begin(),::tolower);
	if (logtime >=_mbeg) {
	  _traf["m"][username]+=bytes;
	}
	if (logtime >=_dbeg){
	  _traf["d"][username]+=bytes;
	}
	if (logtime >=_hbeg){
	  _traf["h"][username]+=bytes;
	}
      }
     _al.close();
     tlog.put(2,"Получение списка отключаемых пользователей\n");
     map< string, map <string,long long> > * limits= _conf.getLimits();
     tlog.put(2,"Список отключаемых пользователей");
     for (map <string, map<string,long long> >::iterator i=_traf.begin();i!=_traf.end();++i)
       for (map<string,long long>::iterator j= i->second.begin();j!=i->second.end();++j){
	 os.str("");
         os <<   "Траффик пользователя (" << i->first << ") "  <<  j->first << ": " <<  j->second;
	 tlog.put(2,os.str());
         if((*limits)[i->first][j->first]==0) {
           tlog.put(2, "Траффик пользователя (" +i->first+") "  + j->first +" не ограничен");
	    continue;
         }  
     	 if((*limits)[i->first][j->first] <=j->second){
	   if (find(_dl.begin(),_dl.end(),j->first)==_dl.end()) {
             os.str("");  
	     os << "Отключение " << j->first << "\tлимит (" << i->first <<") :"    << (*limits)[i->first][j->first] << "\t\tтраффик:" <<   j->second;    
	     _dl.push_back(j->first);
             tlog.put(1, os.str()); 
	   }
         }
	 
       }
    }
    else {
      tlog.put(0,"Ошибка открытия файла лога доступа squid '" + _conf.getAccessLogFile()+ "'");
      return false;
    }
    return true;
  }
  
  map < string, map <string, long long> > *getTraf(){return &_traf;}
  
  access_log *getAccessLog(){return &_al;};
};

#endif /*SQTD_COUNTER*/
