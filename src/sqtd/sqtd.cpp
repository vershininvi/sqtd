#include "log_buffer.h"
#include "sqtd_cmdl.h"
#include "sqtd_conf.h"
#include "squid_accesslog.h"
#include "sqtd_counter.h"
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sstream>

using namespace std;
bool canwork;
bool daemonMode;

void my_terminate (int param){
  tlog.put(0,"Завершение программы...");
  tlog.print();
  canwork=false;
}

void printUserList(map<string, map<string,long long> > *inmap,ostream *outfile){
  for (map <string, map<string,long long> >::iterator i=inmap->begin();i!=inmap->end();++i){
    for (map<string,long long>::iterator j= i->second.begin();j!=i->second.end();++j) *outfile << j->first<<endl;
  }
} 

void printUserList(vector<string> *invect,ostream *outfile){
  for (vector <string>::iterator i=invect->begin();i!=invect->end();++i) *outfile << *i<<endl;
} 

void printMap(map <string, map<string,long long> > *inmap,ostream *outfile ){
  for (map <string, map<string,long long> >::iterator i=inmap->begin();i!=inmap->end();++i)
    for (map<string,long long>::iterator j= i->second.begin();j!=i->second.end();++j) 
      *outfile << i->first << " " << j->first << " " << j->second  <<endl;
}

int main(int argc,char**argv){
  tlog.put(1,"Запуск программы");  
  ostringstream os; 
  pid_t pid,sid;
  
  bool configured;
  int confErrCount;
  int iret;  

  //Регистрация обработчиков сигналов;
  void (*prev_fn)(int);
  canwork= true; 
  prev_fn = signal (SIGTERM,my_terminate);
  prev_fn = signal (SIGABRT,my_terminate);
  prev_fn = signal (SIGKILL,my_terminate);

  //Разбор параметров командной строки
  command_line cmd;
  daemonMode=cmd.get(argc,argv);

  //Чтение конфигурационного файла
  sqtd_counter trc;
  sqtd_conf * conf = trc.getConfig();
  configured=conf->reconfig(cmd.getConfigFile())&&conf->check();
  if(!configured) {
    tlog.print();
    exit(1);
  }
  
  if(daemonMode){
    tlog.put(2,"Открытие файла лога");
    if (!tlog.setTarget(conf->getLogFile())) {
      tlog.put(0, "Невозможно открыть файл лога "+conf->getLogFile() ) ; 
      tlog.print();
      exit(1);
    }
    tlog.setFilterOn(true);
    tlog.put(1,"Переход в режим демона");    
    tlog.put(2,"Запуск демона");
    pid=fork();
    if(pid<0){
      tlog.put(0, "Запуск в режиме демона невозможен");
      tlog.print(); 
      exit(1);
    } 
    if (pid > 0)  {
      exit(0); 
    }
    //sleep(5);
    tlog.put(2, "Продолжение child  процесса") ; 
    tlog.put(2,"Открытие файла идентификатора процесса");
    ofstream pidfile(conf->getPidFile().c_str(),ios_base::out);
    if (!pidfile) {
      tlog.put(0, "Невозможно открыть файл идентификатора процесса " +conf->getPidFile() ) ; 
    }
    else {
      tlog.put(2,"Вывод идентификатора процесса в файл");
      pidfile << getpid()<<endl;
    }
        
    tlog.put(2,"Установка идентификатора сессии");
    sid=setsid();


     
    tlog.put(2,"Смена текущей директории");
    iret=chdir("/");
  }  else tlog.setTarget("console");

  access_log *al= trc.getAccessLog();
  
  tlog.put(2,"Открытие файла лога squid");
  al->setFileName(conf->getAccessLogFile());
  tlog.print(); 
  while (canwork){
    tlog.put(2, "Рассчет данных") ; 
    //Расчет траффика
    if(!trc.calc(&canwork)) {
      tlog.put(0,"Ошибка рассчета");
      tlog.print();
      exit(1);
    }
    tlog.put(2, "Заполнение файла запрета доступа " + conf->getDenyFile()) ;  
    vector<string> * v;
    ofstream denyfile(conf->getDenyFile().c_str(),ios_base::out);
    if(!denyfile.is_open()){
      tlog.put(0,"Ошибка открытия файла запрета доступа " + conf->getDenyFile());
      tlog.print();
      exit (1);
    }

    v=trc.getDenyList();
    if(v->size()>0){
       for (vector <string>::iterator i=v->begin();i!=v->end();++i) 
       denyfile << "acl SQTD_DENY proxy_auth -i " <<*i<<endl;
       denyfile<< "http_access deny SQTD_DENY" << endl;      
    }
    else  tlog.put(2, "Список запрета доступа пуст") ;
    denyfile.close();   

    tlog.put(2, "Заполнение файла разрешения доступа " +  conf->getAllowFile()) ;  
    ofstream allowfile(conf->getAllowFile().c_str(),ios_base::out);
    if(!allowfile.is_open()){
      tlog.put(0,"Ошибка открытия файла разрешения доступа  " + conf->getAllowFile());
      tlog.print();
      exit(1);
    }
    
    v=conf->getAllowList();
    if(v->size()>0){
      for (vector <string>::iterator i=v->begin();i!=v->end();++i){ 
	 allowfile << "acl SQTD_ACCESS proxy_auth -i " << *i <<endl;
      }   
      allowfile << "http_access allow SQTD_ACCESS " << endl;      
    }
    else  tlog.put(2, "Список разрешения доступа пуст") ;
    allowfile.close();
    
    tlog.put(2, "Выполнение скрипта") ;
    iret=system(conf->getActionScript().c_str());
   
    //tlog.put(2, "Реконфигурация программы") ; 
    //configured=conf->reconfig(cmd.getConfigFile())&&conf->check();

   
    //if (!configured){
    //  if (++confErrCount> 10) exit(-1);
    //} else  if(confErrCount!=0) confErrCount=0;

    if(tlog.getLevel()>=2){
      os.flush();
      os << "Лимиты пользователей" << endl;
      printMap(conf->getLimits(),&os);
      tlog.put(2, os.str());
      os.flush();
      tlog.print();
     
      os << "Трафик  пользователей" << endl;
      printMap(trc.getTraf(),&os);
      tlog.put(2, os.str());
      os.flush();
      tlog.print();

      os << "Список разрешения доступа" << endl;
      printUserList(conf->getAllowList(),&os);
      tlog.put(2, os.str());
      os.flush();
      tlog.print();
      
      os << "Список запрета доступа" << endl;
      printUserList(trc.getDenyList(),&os);
      tlog.put(2, os.str());
      os.flush();
      tlog.print();     

    }
    
    os<< "Ожидание " <<conf->getCheckInterval() <<"с" << endl;
    tlog.put(1, os.str());
    tlog.print();
    sleep(conf->getCheckInterval());
  }
 
  return 0;  
}
