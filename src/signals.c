#define _GNU_SOURCE
#include <labHeader.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>

int gotChildSignal = 0, gotInSignal = 0, gotOutSignal = 0, gotErrorSignal = 0;
int fd0[2], fd1[2], fd2[2];
siginfo_t info;
siginfo_t childInfo;
void helpHandler(int sigNumber, siginfo_t *siginfo, void *context){
	if (sigNumber == SIGCHLD){
		gotChildSignal = 1;
		childInfo = *siginfo;
	}
	if (sigNumber == SIGIO){
		info = *siginfo;
		if (info.si_fd == 0){
			gotInSignal = 1;
		}
		if (info.si_fd == fd1[0]){
			gotOutSignal = 1;
		}
		if (info.si_fd == fd2[0]){
			gotErrorSignal = 1;
		}
	}
}

void doSignals(char* logFileName, char* command, char** arguments){
	printf("Signals пошёл\n");
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
		act.sa_sigaction = &helpHandler;
		act.sa_flags = SA_SIGINFO;
		sigset_t mask;
		sigemptyset(&mask);
		sigaddset(&mask, SIGCHLD);
		sigaddset(&mask, SIGIO);
		act.sa_mask = mask;
		if (sigaction(SIGCHLD, &act, NULL) == -1) {
			perror("Не могу поменять обработчик SIGCHILD: ");
			return;
		}
		if (sigaction(SIGIO, &act, NULL) == -1) {
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
		fcntl(0, F_SETFL, O_ASYNC);
		fcntl(fd1[0], F_SETFL, O_ASYNC);
		fcntl(fd2[0], F_SETFL, O_ASYNC);

		if (fcntl(0, F_SETSIG, SIGIO) == -1){
			perror("Не могу повесить сигнал на stdin: ");
			return;
		}
		fcntl(0, F_SETOWN, getpid());
		if (fcntl(fd1[0], F_SETSIG, SIGIO) == -1){
			perror("Не могу повесить сигнал на чтение из пайпы: ");
			return;
		}
		fcntl(fd1[0], F_SETOWN, getpid());
		if (fcntl(fd2[0], F_SETSIG, SIGIO) == -1){
			perror("Не могу повесить сигнал на чтение ошибок из пайпы:");
			return;
		}
		fcntl(fd2[0], F_SETOWN, getpid());

		while (!gotChildSignal){
			char currentRead[1000];
			sleep(1);
			char timeStr[18];
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			strftime(timeStr, 18, "%D %H:%M:%S", &tm);
			if (gotInSignal == 1){
				gotInSignal = 0;
				int readSize = read(0, currentRead, sizeof(currentRead));
				if (readSize == -1){
					perror("Не могу считать из stdin: ");
					return;
				}
				if (strcmp(currentRead, "exit\n")){
					kill(SIGKILL, child);
				}
				write(fd0[1], currentRead, readSize);

				printf("%d / >0 / %s\n", child, str);

				write(logFD, timeStr, strlen(timeStr));
				write(logFD, " / >0 / ", 8);
				write(logFD, str, strlen(str));
			}
			else if (gotOutSignal == 1){
				gotOutSignal = 0;

				int readSize = read(fd1[0], currentRead, sizeof(currentRead));
				if (readSize == -1){
					perror("Не могу считать из пайпы для stdout: ");
					return;
				}
				printf("%d / <1 / %s\n", child, str);

				write(logFD, timeStr, strlen(timeStr));
				write(logFD, " / <1 / ", 8);
				write(logFD, str, strlen(str));
			}
			else if (gotErrorSignal == 1){
				gotErrorSignal = 0;

				int readSize = read(fd2[0], currentRead, sizeof(currentRead));
				if (readSize == -1){
					perror("Не могу считать из пайпы для stderr: ");
					return;
				}
				printf("%d / <2 / %s\n", child, str);

				write(logFD, timeStr, strlen(timeStr));
				write(logFD, " / <2 / ", 8);
				write(logFD, str, strlen(str));
			}
			else{
				write(logFD, timeStr, strlen(timeStr));
				write(logFD, " / NO IO", 8);
			}
		}
		printf("%d TERMINATED WITH CODE %d", child, childInfo.si_code);
	}

}
