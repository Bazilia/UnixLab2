#include <labHeader.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>

int gotSignal = 0;
siginfo_t info;
void childHandler(int sigNumber, siginfo_t *siginfo, void *context){
  gotSignal = 1;
  info = *siginfo;
}

void doSelect(char* logFileName,char* command,char* arguments){
  int fd0[2], fd1[2], fd2[2];
  fd_set fds;

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
      if (execlp(command,arguments,NULL) == -1){
        perror("Не могу выполнить команду: ");
        return;
      }
      return;
    }else{
      struct sigaction act;
      act.sa_sigaction = &childHandler;
      act.sa_flags = SA_SIGINFO;
      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGCHLD);
      act.sa_mask = mask;
      if (sigaction(SIGCHLD, &act, NULL) == -1) {
        fprintf(stderr, "Не могу поменять обработчик SIGCHILD");
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
      while(!gotSignal){
        struct timeval waitTime;
        char currentRead[1000];

        waitTime.tv_sec = 1;
        waitTime.tv_usec = 0;

        FD_ZERO(&fds);
        FD_SET(0,&fds);
        FD_SET(fd1[0],&fds);
        FD_SET(fd2[0],&fds);

        int res = select(FD_SETSIZE, &fds, NULL, NULL,&waitTime);
        if(res == -1){
          perror ("Ошибка в селекте: ");
          return;
        }
        if(FD_ISSET(0,&fds)){
          int readSize = read(0, currentRead, sizeof(currentRead));
          if(readSize == -1){
            perror("Не могу считать из stdin: ");
            return;
          }
          if(strcmp(currentRead,"exit\n")){
            kill(SIGKILL,child);
          }
          write(fd0[1],currentRead,readSize);
        }
        if(FD_ISSET(fd1[0],&fds)){
          int readSize = read(fd1[0], currentRead, sizeof(currentRead));
          if(readSize == -1){
            perror("Не могу считать из пайпы для stdout: ");
            return;
          }
          write(1,currentRead,readSize);
        }
        if(FD_ISSET(fd2[0],&fds)){
          int readSize = read(fd2[0], currentRead, sizeof(currentRead));
          if(readSize == -1){
            perror("Не могу считать из пайпы для stderr: ");
            return;
          }
          write(2,currentRead,readSize);
        }
      }
    }
}
