#include "lib/unp.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>

int count = 0;

void parent(int pid) {
    printf("Parent spawned child PID %d \n", pid);
}

void child() {
    srand(time(NULL) ^ (getpid()<<16));
    int sleep_sec = rand() % 5;
    printf("Child PID %d dying in %d seconds.\n", getpid(), sleep_sec);

    sleep(sleep_sec);

    printf("Child PID %d terminating.\n", getpid());
}

void sig_child(int signo) {
    pid_t pid;
    int stat;

    while((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("Parent sees child PID %d has terminated.\n", pid);
        count += 1;
    }
}

int main()
{
    int child_num;
    printf("Number of children to spawn: ");  
    scanf("%d", &child_num);  
    printf("Told to spawn %d children\n", child_num);
    fflush( stdout );

    signal(SIGCHLD, &sig_child);
    for (int i = 0; i < child_num; i++) {
        int pid = fork();
        if (pid) {
            parent(pid);
        }
        else {
            child();
            return 0;
        }
    }

    while(count < child_num);
    
    return 0;
}

