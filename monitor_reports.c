#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>

void init_mon_file() {
    int f = open(".monitor_pid", O_RDONLY);
    if (f != -1) {
        char buf[32];
        int sz = read(f, buf, 31);
        close(f);

        buf[sz] = '\0';

        int running_pid = atoi(buf);

        printf("Another monitor is already running with pid: %d\n", running_pid);
        fflush(stdout);

        exit(-1);
    }

    int mon_file = open(".monitor_pid", O_CREAT | O_WRONLY, 0644);

    if (mon_file == -1) {
        printf("Failed to create or open .monitor_pid file.\n");
        fflush(stdout);
        exit(-1);
    }
    
    // write pid to file
    char buf[32];
    int len = snprintf(buf, 32, "%d", getpid());
    write(mon_file, buf, len);

    close(mon_file);
}

void close_and_delete_mon_file() {
    remove(".monitor_pid");
}

void handle_sigusr1(int sig) {
    printf("SIGUSR1: new report generated.\n");
    fflush(stdout);
}

void handle_sigint(int sig) {
    close_and_delete_mon_file();
    printf("Monitor process ended and .monitor_pid file deleted.\n");
    fflush(stdout);

    exit(0);
}

void start_monitor() {
    init_mon_file();
    printf("Monitor process started and .monitor_pid file created.\n");
    fflush(stdout);
    
    struct sigaction sa_usr1, sa_int;

    // SIGUSR1
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    // SIGINT
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    // configure signal waiting;
    sigset_t wait_mask;
    sigfillset(&wait_mask);
    sigdelset(&wait_mask, SIGUSR1);
    sigdelset(&wait_mask, SIGINT);

    // main loop to keep the monitor process running
    while (1) {
        sigsuspend(&wait_mask); // wait until either one of the signals is recieved
    }
}

int main() {
    start_monitor();

    return 0;
}