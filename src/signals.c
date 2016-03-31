#define _GNU_SOURCE
#include <labHeader.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

int gotChildSignal = 0, gotIoSignal = 0;
siginfo_t info;
void helpHandler(int sigNumber, siginfo_t *siginfo, void *context){
  if (sigNumber == SIGCHLD){
    gotChildSignal = 1;
  }
  if(sigNumber == SIGIO){
    gotIoSignal = 1;
    info = *siginfo;
  }
}

void doSignals(char* logFileName, char* command, char** arguments){
  int fd0[2], fd1[2], fd2[2];
  printf("Signals пошёл\n" );
  if(pipe(fd0) != 0){
    perror("Не могу открыть первую пайпу");
    return;
  }
  if(pipe(fd1) != 0){
    perror("Не могу открыть вторую пайпу");
    return;
  }
  if(pipe(fd2) != 0){
    perror("Не могу открыть третью пайпу");
    return;
  }
  printf("Пайпы создались \n");
  pid_t child = fork();
    if(child == -1){
      perror("Не могу форкнуться: ");
    }else if(child == 0){
      if(close(fd0[1])){
        perror("Не могу закрыть писателя в потомке на stdin: ");
        return;
      }
      if(close(fd1[0])){
        perror("Не могу закрыть читателя в потомке на stdout: ");
        return;
      }
      if(close(fd2[0])){
        perror("Не могу закрыть читателя в потомке на stderr: ");
        return;
      }
      if (dup2(fd0[0],0) == -1){
        perror("Не могу заменить дескриптор stdin: ");
        return;
      }
      if (dup2(fd1[1],1) == -1){
        perror("Не могу заменить дескриптор stdout: ");
        return;
      }
      if (dup2(fd2[1],2) == -1){
        perror("Не могу заменить дескриптор stderr: ");
        return;
      }
      if (execvp  (command,arguments) == -1){
        perror("Не могу выполнить команду: ");
        return;
      }
      return;
    }else{
      struct sigaction act;
      act.sa_sigaction = &helpHandler;
      act.sa_flags = SA_SIGINFO;
      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGCHLD);
      sigaddset(&mask, SIGIO);
      act.sa_mask = mask;
      if(sigaction(SIGCHLD, &act, NULL) == -1) {
        perror("Не могу поменять обработчик SIGCHILD: ");
        return;
      }
      if(sigaction(SIGIO, &act, NULL) == -1) {
        perror("Не могу поменять обработчик SIGCHILD: ");
        return;
      }
      if(close(fd0[0])){
        perror("Не могу закрыть читателя в предке на stdin: ");
        return;
      }
      if(close(fd1[1])){
        perror("Не могу закрыть писателя в предке на stdout: ");
        return;
      }
      if(close(fd2[1])){
        perror("Не могу закрыть писателя в предке на stderr: ");
        return;
      }
      fcntl(0,F_SETFL,O_ASYNC);
      fcntl(fd1[0],F_SETFL,O_ASYNC);
      fcntl(fd2[0],F_SETFL,O_ASYNC);

      if(fcntl(0,F_SETSIG,SIGIO) == -1){
        perror();
      }
      if(fcntl(fd1[0],F_SETSIG,SIGIO) == -1){
        perror();
      }
      if(fcntl(fd2[0],F_SETSIG,SIGIO) == -1){
        perror();
      }
      while(!gotChildSignal){
        sleep(1);
        if(){

        }
      }
}
