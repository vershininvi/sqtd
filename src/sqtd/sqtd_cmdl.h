#ifndef COMMAND_LINE
#define COMMAND_LINE
#define VERSION "0.0.0"
#include "sqtd_log.h"
#include <map>
#include <cstdlib>

using namespace std;
class command_line{
  string _config_file;
  string _pid_file;
  int debug_level;

  map <string,int> _params; 
 private:
  void usage (char *progname){

  cout << "Использование\n" << progname <<   
" КЛЮЧ]… [ФАЙЛ]… \n \
Анализирует файл access.log прокси сервера squid.\n \
Формирует списки доступа и списки отключения.\n\n \
-c имя_конигурационного_файла   \t использовать файл конфигурации\n \
-d уровень_отладочных_сообщений \t установить уровень отладочных сообщений 0-2\n \
-h                              \t вывод этой справки\n \
-v                              \t вывод версии программы\n \
-t                              \t запуск в консоли \n\n \
Коды выхода:\n\n \
 0  нормальное завершение работы\n \
 1  небольшие проблемы\n \
 2  серьёзная проблема\n\n \
Об ошибках  сообщайте по адресу Vershinin_VI@vlg-gaz.ru\n  \
Домашняя страница:\n  \
Справка по работе с программой:\
man tracon\n" << endl;
}
 
 public:
  command_line(){
    string config_file="";
    string pid_file="";
    int debug_level=0;
    _params["-c"]=1;
    _params["-d"]=2;
    _params["-h"]=3;
    _params["-v"]=4;
    _params["-t"]=5; 
  }
  
  bool get(int argc,char**argv){
    bool result;
    result=true;
    tlog.

    tlog.put(3,"Анализ параметров командной строки");
    if (argc<2){
      tlog.setLevel(3);  
      tlog.print();
      cout << "Ошибка: Недостаточное количестово аргументов"<<endl ;
      usage(argv[0]);
      exit(1);
    }  
    for (int i=1;i<argc;i++){
      switch(_params[argv[i]]){
      case 1:
	if (++i < argc)  {
          tlog.put(3, "Установка имени файла конфигурации '" +  _config_file  + "'");
	  _config_file=argv[i];
          tlog.put(3, "Файла конфигурации '" +  _config_file  + "' установлен");
	  break;
	} 
	else{
          tlog.setLevel(3);  
          tlog.print();
          cout << "Не указано имя файла конфигурации"<< endl;  
          cout <<"Пример:"<<  argv[0] << " " <<  argv[i-1]  <<  " /etc/tracon/tracon.conf" << endl;
	  exit(1);
	}
      case 2:
	if (++i < argc)  {
          tlog.put(3, "Установка уровня отладочных сообщений (0-только ошибки,1 - предупреждения,  2 - все )") ;
	  debug_level=atoi(argv[i]);
          tlog.setLevel(debug_level);  
	  break;
	} 
	else{
          tlog.setLevel(3);  
          tlog.print();
	  cout << "Не указан уровень отладочных соощений" << endl;  
cout <<"Пример:"<<  argv[0] << " " <<  argv[i-1]  <<  " 2 " << endl;
	  usage(argv[0]);
	  exit(1);
	}
      case 3:
	usage(argv[0]);
	exit(0);
     
      case 4:
	cout <<argv[0] << " версия " <<   VERSION << endl;
	exit(0);
      case 5:
	result=false;
	break;
      default:
	usage(argv[0]);
	exit(0);
      }
    }
    if(_config_file.compare("")==0){
      tlog.setLevel(3);  
      tlog.print();
      cout << "Не задано имя конфигурационного файла" << endl;
      cout <<"Пример:"<<  argv[0] <<  "  -c /etc/tracon/tracon.conf" << endl;
      usage(argv[0]);
      exit(1);
    }  
    return result;
  }
  string getConfigFile(){return _config_file;}

};
#endif  /*COMMAND_LINE */
