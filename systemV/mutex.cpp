#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <fcntl.h>
#include "semun.h"
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <iostream>

using namespace std;

const int BUFFER_SIZE = 8192;

void fillBuffer(char *buffer, const char c, const int size);
void processingChild(const char c, int sem);
static int semOp(int sem, int value);
int wait(int sem);
int signal(int sem);

int
main(int argc, const char *argv[]) {

  int mutex;

  if ((mutex = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL
		      | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1) {

    cerr << "Error openning PRIVATE semaphore "
	 << " error: " << errno << " "
	 << strerror(errno) << endl;
    _exit(EXIT_FAILURE);
  }

  union semun arg;

  arg.val = 1;

  if (semctl(mutex, 0, SETVAL, arg) == -1) {
    cerr << "Error setting value for semaphore " << mutex
	 << " error: " << errno << " "
	 << strerror(errno) << endl;
    _exit(EXIT_FAILURE);
  }

  pid_t child1;
  pid_t child2;

  if ((child1 = fork()) == 0) { // First child
    processingChild('a', mutex);
  }
  else {

    if ((child2 = fork()) == 0) { // Second child
      processingChild('X', mutex);
    }
  }

  int status;

  waitpid(child1, &status, 0);
  waitpid(child2, &status, 0);

  return EXIT_SUCCESS;
}

void usage(const char *program) {
  cerr << "Usage: " << program << " <semkey>" << endl;
  _exit(EXIT_FAILURE);
}


void fillBuffer(char *buffer, const char c, const int size) {

  for (int i = 0; i < size - 2; i++) buffer[i] = c;

  buffer[size-2] = '\n';
  buffer[size-1] = '\0';
}

void processingChild(const char c, int mutex) {
  char *buffer = new char[BUFFER_SIZE];

  fillBuffer(buffer, c, BUFFER_SIZE);

  for (;;) {

    if (wait(mutex) == -1) {
      cerr << "Error operating over semaphore " << mutex
	   << " error: " << errno << " "
	   << strerror(errno) << endl;
      _exit(EXIT_FAILURE);
    }

    cout << buffer;

    if (signal(mutex) == -1) {
      cerr << "Error operating over semaphore " << mutex
	   << " error: " << errno << " "
	   << strerror(errno) << endl;
	_exit(EXIT_FAILURE);
    }
  }
}

int semOp(int sem, int value) {
  struct sembuf semCtrl;

  semCtrl.sem_num = 0;
  semCtrl.sem_op  = value;
  semCtrl.sem_flg = 0;

  return semop(sem, &semCtrl, 1);
}

int wait(int sem) {
  return semOp(sem, -1);
}

int signal(int sem) {
  return semOp(sem, 1);
}
