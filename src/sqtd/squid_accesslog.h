#ifndef ACCESS_LOG
#define ACCESS_LOG
#include "log_buffer.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>



using namespace std;

class access_log{
private:
  ifstream * _file;
  ifstream::pos_type _pos;
  string _record; 
  string _filename;

public:
  access_log(){
    _pos=0;
    _record="";
    _file=NULL;
  };
    

  ~access_log(){
    close();
  };
  
  void setFileName(string name){_filename=name;};

  void setPos(ifstream::pos_type position){ _pos=position;}
  ifstream::pos_type getPos(){return _pos;} 

  void setRecord(string record){_record=record;}  
  string getRecord(){return _record;} 

  int open(){
    tlog.put(2,"Открытие файла " + _filename );
    _file= new  ifstream(_filename.c_str());
    if(!_file){
      tlog.put(0,"Ошибка открытия файла " +_filename);
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
	    tlog.put(2,"Обработка будет продолжена с  позиции: " + os.str() );
	   return 1;
	 }  
         else{	  
            os<<_pos;  
	    tlog.put(2,"Запись на позиции "+ os.str()+ "  не соответсвует последней обработанной записи");
	    tlog.put(2, "Последняя запись : " + _record);
	    tlog.put(2, "Запись на позиции: " + newrec );
	    tlog.put(2, "Обработка с начала файла");
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
  
  vector<string> getFields(){
    try {
      vector <string> tokens;
      stringstream ss(_record); 
      string token;
      while (ss >> token)  tokens.push_back(token);
      return tokens;
    }
    catch(...){
      throw 1;
    }  
  };
};
#endif  /* ACCESS_LOG */
