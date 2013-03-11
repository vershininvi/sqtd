#ifndef TRAF_COUNTER
#define  TRAF_COUNTER

#include "squid_accesslog.h"
#include "sqtd_conf.h"
#include <map>
#include <vector>
#include <algorithm>
#include "sqtd_log.h"


class traf_counter {
 private:
  map < string, map <string, long long> > _traf;
  time_t _mbeg; 
  time_t _dbeg; 
  time_t _hbeg;
  time_t _mbeg_old;
  time_t _dbeg_old;
  time_t _hbeg_old;  
  access_log _al;
  tracon_conf _conf;
  vector <string> _dl; 

 private:
  void settime(){
    time_t tis =time(NULL);
    struct tm tbd= *gmtime(&tis);
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
  tracon_conf * getConfig(){return &_conf;}
    
  void calc(bool *canwork){   
    settime();
    clean();
    if (_al.open()){
       
      while (_al.next()){
	if(!*canwork){
	  _al.close();
	  return;
	}

    	vector<string> rec= _al.getFields();
	stringstream field0(rec[0]);  
	time_t logtime; 
	field0>>logtime;
	long long bytes= atoll(rec[4].c_str()); 
	string username= rec[7];
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
     map< string, map <string,long long> > * limits= _conf.getLimits();
     tlog.put(2,"Список отключаемых полтзователей");
     for (map <string, map<string,long long> >::iterator i=_traf.begin();i!=_traf.end();++i)
       for (map<string,long long>::iterator j= i->second.begin();j!=i->second.end();++j){
     	 if((*limits)[i->first][j->first] <=j->second )  _dl.push_back(j->first);
	 tlog.put(2, j->first);
       }
    }
  }
  
  map < string, map <string, long long> > *getTraf(){return &_traf;}
  
  access_log *getAccessLog(){return &_al;};
};

#endif /*TRAF_COUNTER*/
