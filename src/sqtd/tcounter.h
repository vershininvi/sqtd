#ifndef TCOUNTER
#define  TCOUNTER
#include "tlogger.h"
#include "tlogparser.h"
#include "sqtd_conf.h"
#include <map>
#include <vector>
#include <algorithm>


class tcounter {
 private:
  time_t _mbeg; 
  time_t _dbeg; 
  time_t _hbeg;
  time_t _mbeg_old;
  time_t _dbeg_old;
  time_t _hbeg_old;  
  tlogparser  _parser;
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
  
  void replace(string *source ,string* pattern ,string* replacement){
    size_t pos = source->find(*pattern);
    if ( pos != string::npos ) 
      source->replace( pos, pattern->length(),*replacement  );  
  }
 
 public:
  
  bool calc(bool *canwork){   
    ostringstream os;
    logger.put(2,"Рассчет траффика");
    settime();
    clean();
    if (int iret=_parser.open()){
      if (iret==2) _traf.clear();
      while (_parser.next()){
    	if(!*canwork){
    	  _parser.close();
    	  return true;
    	}
    	try {
    	  vector<string>* rec= _parser.getFields();
	  if (rec->at(3).compare("TCP_MISS/200")!=0) continue;
          if (rec->size()!= 10) throw 0;
    	  stringstream ints(rec->at(0));
    	  time_t logtime;
    	  ints >>logtime;
	  ints.str(rec->at(4));
          long long bytes;
	  ints >> bytes;
          string username= rec->at(7);
    	  transform(username.begin(),username.end(),username.begin(),::tolower);
          if(_conf->getSquidDomainDelimiter()->compare("")!=0 && _conf->getSystemDomainDelimiter()->compare("")!=0)
          replace(&username,_conf->getSquidDomainDelimiter(),_conf->getSystemDomainDelimiter());
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
    	  os << "Wrong record " << _parser.getPos() << endl << _parser.getRecord();
    	  logger.put(1,os.str());
    	}
     }
     _parser.close();
    }
    else {
      logger.put(0,"Can not open  squid acces log file" );
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

  void setConf(sqtd_conf* conf){_conf=conf;_parser.setConf(_conf); }
  sqtd_conf* getConf(){return _conf;}

};

#endif /*SQTD_COUNTER*/
