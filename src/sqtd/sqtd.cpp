#include "sqtd_log.h"
#include "sqtd_cmdl.h"
#include "sqtd_conf.h"
#include "squid_accesslog.h"
#include "sqtd_counter.h"
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>

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
  traf_counter trc;
  tracon_conf * conf = trc.getConfig();
  configured=conf->reconfig(cmd.getConfigFile())&&conf->check();
  if(!configured) {
    tlog.print();
    exit(1);
  }

  if(daemonMode){
    rlimit rl;
    tlog.put(1,"Переход в режим демона");
    tlog.put(2,"Получение списка открытых файлов");
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
      tlog.put(0, "Невозможно получить список открытых файлов");
      tlog.print(); 
      exit (1);  
    } 
    
    tlog.put(2,"Запуск демона");
    pid=fork();

    if(pid<0){
      tlog.put(0, "Запуск в режиме демона невозможен");
      tlog.print(); 
      exit(1);
    } 

    if (pid > 0){
       tlog.put(2,"Открытие файла идентификатора процесса");
       ofstream pidfile(conf->getPidFile().c_str(),ios_base::out);
       if (!pidfile) {
         tlog.put(0, "Невозможно открыть файл идентификатора процесса" +conf->getPidFile() ) ; 
         tlog.print();
       }
       tlog.put(2,"Вывод идентификатора процесса в файл");
       pidfile << pid<<endl;
       tlog.put(2,"Закрытие файла идентификатора процесса");
       pidfile.close();
       exit(0);
    }

    tlog.put(2,"Установка идентификатора сессии");
    sid=setsid();

    tlog.put(2,"Закрытие открытых файлов");
    for (unsigned int i = 0; i < rl.rlim_max; i++) close(i);     

    tlog.put(2,"Открытие файла лога");
    if (!tlog.setTarget(conf->getLogFile())) {
      tlog.put(0, "Невозможно открыть файл лога"+conf->getLogFile() ) ; 
      tlog.print();
      exit(1);

    }
     
  

    tlog.put(2,"Смена текущей директории");

    iret=chdir("/");
  }       

 
  access_log *al= trc.getAccessLog();
  al->setFileName(conf->getAccessLogFile());
  
  while (canwork){
    tlog.print();
    if(!configured){
      tlog.put(0,"Ошибка конфигурации");
      break;   
    }
    //Расчет траффика
    trc.calc(&canwork);
    int ret = system(conf->getActionScript().c_str());   
    
    ofstream denyfile(conf->getDenyFile().c_str(),ios_base::out);
    if(!denyfile){
      tlog.put(0,"Ошибка открытия файла запрета доступа " + conf->getDenyFile());
      break;
    }
    denyfile<< "-" << endl;    
    printUserList(trc.getDenyList(),&denyfile); 
    denyfile.close();   

    ofstream allowfile(conf->getAllowFile().c_str(),ios_base::out);
    if(!allowfile){
      tlog.put(0,"Ошибка открытия файла разрешения доступа  " + conf->getAllowFile());
      break;
    }
    allowfile << "-" << endl;


    printUserList(conf->getAllowList(),&allowfile);
    allowfile.close();
    

    iret=system(conf->getActionScript().c_str());
    configured=conf->reconfig(cmd.getConfigFile())&&conf->check();

    
    if (!configured){
      if (++confErrCount> 10) exit(-1);
    } else  if(confErrCount!=0) confErrCount=0;


    ostringstream os;
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

      os << "Список доступа" << endl;
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
    tlog.put(2, os.str());
    tlog.print();
    sleep(conf->getCheckInterval());
  }
 
  return 0;  
}
