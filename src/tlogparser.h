#ifndef TLOGPARSER
#define TLOGPARSER
#include "tlogger.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

class tlogparser{
private:
  ifstream * _file;
  ifstream::pos_type _pos;
  string _record; 
  config_file*   _conf;
  vector <string> _tokens;
public:
  tlogparser(){
    _pos=0;
    _record="";
    _file=NULL;
  };
    

  ~tlogparser(){
    close();
  };

  void setConf(config_file * conf){_conf=conf;}
  void setPos(ifstream::pos_type position){ _pos=position;}
  ifstream::pos_type getPos(){return _pos;} 
  void setRecord(string record){_record=record;}  
  string getRecord(){return _record;} 

  int open(){
    string filename= _conf->getAccessLogFile()->c_str();
    logger.put(2,_("Opening the file") +string(" ")+ filename );
    _file= new  ifstream(filename.c_str());
    if(!_file){
      logger.put(0,_("Can not open file") +string(" ")+filename);
      return 0;
    }
    if (_pos!=0){
       _file->seekg(_pos);
       string newrec;
       ostringstream os;
       if (getline(*_file,newrec)){ 
	 if (newrec.compare(_record)==0){
	    _pos=_file->tellg();
            os<<_pos;  
	    logger.put(2,_("Start from position") + string(": ")+ os.str() );
	   return 1;
	 }  
         else{	  
            os<<_pos;  
	    logger.put(2,_("The  position is")+ string(": ")+os.str());
	    logger.put(2,_( "The last read record")+string(": ") + _record);
	    logger.put(2,_( "The record at position")+string(": ") + newrec );
	    logger.put(2,_( "Starting read from begin of the file"));
	    _file->seekg(0);
	    _pos=0;
	    _record="";
	    return 2;
          } 
       }
    }
    return true; 
  };

  void close(){
    if(_file) _file->close();
    delete _file;
    _file=NULL;
  };

  bool next(){
    if(_file){
      string newrec;
      ifstream::pos_type pos;
      pos=_file->tellg();
      if (getline(*_file,newrec)){ 
          _record=newrec;
	  _pos=pos;
          ostringstream os;
          return true;
      }
      else return false;
    }
    else return false;
  };
  
  vector<string>* getFields(){
    _tokens.clear();
      stringstream ss(_record); 
      string token;
      while (ss >> token)  _tokens.push_back(token);
      return &_tokens;
  };
};
#endif  /* TLOGPARSER */
