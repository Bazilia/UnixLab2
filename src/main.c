#include <labHeader.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const struct option opts[] = {
  {"logfile", required_argument, NULL, 0},
  {"execute", required_argument, NULL, 1},
  {"multiplex", required_argument, NULL, 2}
};

int main(int argc, char *argv[]){
  int index = 0;
  int currentOpt = 0;
  int multiplexVal = 1;
  char* commandsAndArgs = NULL;
  char* logFileName = NULL;

  currentOpt = getopt_long( argc, argv, "", opts, &index );
  while (currentOpt != -1) {
   if (index == 0) {
     logFileName = optarg;
   }else if (index == 1){
     commandsAndArgs = optarg;
   }else if(index == 2){
      multiplexVal = atoi(optarg);
   }
   currentOpt = getopt_long( argc, argv, "", opts, &index );
 }

 printf("Логгируем сюда: %s\n",logFileName);
 printf("В режиме: %d\n",multiplexVal);

 char* command = strtok(commandsAndArgs," ");
 printf("Выполняем её: %s\n",command);

 char** arguments = (char**)malloc(strlen(commandsAndArgs)*sizeof(char*));
 char* currentArgument = strtok(NULL," ");
 arguments[0] = command;
 arguments[1] = currentArgument;
 printf("С аргументом: %s\n",arguments[1]);
 int i=2;
 while (currentArgument!= NULL) {
   currentArgument = strtok(NULL," ");
   arguments[i] = currentArgument;
   printf("С аргументом: %s\n",arguments[i]);
   i++;
 }


 if(multiplexVal == 1){
   doSelect(logFileName, command, arguments);
 }else{
   doSignals(logFileName, command, arguments);
  }

}
