#include <labHeader.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

int gotSignal = 0;
siginfo_t childInfo;
void childHandler(int sigNumber, siginfo_t *siginfo, void *context){
	gotSignal = 1;
	childInfo = *siginfo;
}

void doSelect(char* logFileName, char* command, char** arguments){
	int fd0[2], fd1[2], fd2[2];
	fd_set fds;
	printf("Select пошёл\n");
	if (pipe(fd0) != 0){
		perror("Не могу открыть первую пайпу");
		return;
	}
	if (pipe(fd1) != 0){
		perror("Не могу открыть вторую пайпу");
		return;
	}
	if (pipe(fd2) != 0){
		perror("Не могу открыть третью пайпу");
		return;
	}
	printf("Пайпы создались \n");
	pid_t child = fork();
	if (child == -1){
		perror("Не могу форкнуться: ");
	}
	else if (child == 0){
		if (close(fd0[1])){
			perror("Не могу закрыть писателя в потомке на stdin: ");
			return;
		}
		if (close(fd1[0])){
			perror("Не могу закрыть читателя в потомке на stdout: ");
			return;
		}
		if (close(fd2[0])){
			perror("Не могу закрыть читателя в потомке на stderr: ");
			return;
		}
		if (dup2(fd0[0], 0) == -1){
			perror("Не могу заменить дескриптор stdin: ");
			return;
		}
		if (dup2(fd1[1], 1) == -1){
			perror("Не могу заменить дескриптор stdout: ");
			return;
		}
		if (dup2(fd2[1], 2) == -1){
			perror("Не могу заменить дескриптор stderr: ");
			return;
		}
		if (execvp(command, arguments) == -1){
			perror("Не могу выполнить команду: ");
			return;
		}
		return;
	}
	else{

		int logFD = 2;
		if (logFileName != NULL) {
			logFD = open(logFileName, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
			if (logFD == -1) {
				perror("Не могу открыть/создать файл: ");
				return;
			}
		}
		struct sigaction act;
		act.sa_sigaction = &childHandler;
		act.sa_flags = SA_SIGINFO;
		sigset_t mask;
		sigemptyset(&mask);
		sigaddset(&mask, SIGCHLD);
		act.sa_mask = mask;
		if (sigaction(SIGCHLD, &act, NULL) == -1) {
			perror("Не могу поменять обработчик SIGCHILD: ");
			return;
		}
		if (close(fd0[0])){
			perror("Не могу закрыть читателя в предке на stdin: ");
			return;
		}
		if (close(fd1[1])){
			perror("Не могу закрыть писателя в предке на stdout: ");
			return;
		}
		if (close(fd2[1])){
			perror("Не могу закрыть писателя в предке на stderr: ");
			return;
		}
		while (!gotSignal){
			struct timeval waitTime;
			char currentRead[1000];

			waitTime.tv_sec = 1;
			waitTime.tv_usec = 0;

			FD_ZERO(&fds);
			FD_SET(0, &fds);
			FD_SET(fd1[0], &fds);
			FD_SET(fd2[0], &fds);

				if(fd1[0]>fd2[0]){
					int maxFd = fd1[0];
				}
				else{
					int maxFd = fd2[0];
				}
			int res = select(maxFd, &fds, NULL, NULL, &waitTime);
			if (res == -1){
				perror("Ошибка в селекте: ");
				return;
			}

			char timeStr[18];
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			strftime(timeStr, 18, "%D %H:%M:%S", &tm);
			if (res == 0) {
				write(logFD, timeStr, strlen(timeStr));
				write(logFD, " / NO IO\n", 9);
			}
			if (FD_ISSET(0, &fds)){
				int readSize = read(0, currentRead, sizeof(currentRead)-1);
				currentRead[readSize]='\0';
				if (readSize == -1){
					perror("Не могу считать из stdin: ");
					return;
				}
				if (strncmp(currentRead, "exit", 4) == 0){
					kill(SIGKILL, child);
					break;
				}
				write(fd0[1], currentRead, readSize);

				printf("%d / >0 / %s", child, currentRead);

				write(logFD, timeStr, strlen(timeStr));
				write(logFD, " / >0 / ", 8);
				write(logFD, currentRead, readSize);
			}
			if (FD_ISSET(fd1[0], &fds)){
				int readSize = read(fd1[0], currentRead, sizeof(currentRead)-1);
				currentRead[readSize++]='\0';
				if (readSize == -1){
					perror("Не могу считать из пайпы для stdout: ");
					return;
				}

				printf("%d / <1 / %s", child, currentRead);

				write(logFD, timeStr, strlen(timeStr));
				write(logFD, " / <1 / ", 8);
				write(logFD, currentRead, readSize);

			}
			if (FD_ISSET(fd2[0], &fds)){
				int readSize = read(fd2[0], currentRead, sizeof(currentRead)-1);
				currentRead[readSize++]='\0';
				if (readSize == -1){
					perror("Не могу считать из пайпы для stderr: ");
					return;
				}
				printf("%d / <2 / %s", child, currentRead);
				write(logFD, timeStr, strlen(timeStr));
				write(logFD, " / <2 / ", 8);
				write(logFD, currentRead, readSize);
			}
		}
		printf("%d TERMINATED WITH CODE %d\n", child, childInfo.si_code);
	}
}
