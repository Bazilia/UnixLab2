#include <labHeader.h>
#include <sys/select.h>
#include <unistd.h>

void doSelect(char* logFileName,char* command,char* arguments){
  int fd[2], fd1[2], fd2[2];
  fd_set fds;

  if(pipe(fd) != 0){
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
  printf("Пайпы создались %s\n");
  pid_t child = fork();
    if(child == -1){
      perror("Не могу форкнуться: ");
    }else if(child == 0){
      if(close(fd[1])){
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
      if (execlp(command,arguments,NULL) == -1){
        perror("Не могу выполнить команду: ");
      }
    }else{
      if(close(fd[0])){
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
      while(1){
        struct timeval waitTime;
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

      }
    }
}
