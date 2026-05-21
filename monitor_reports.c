#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

FILE* init_mon_file() {
    FILE* mon_file = fopen(".monitor_pid", "w");
    if (mon_file == NULL) {
        printf("Failed to open .monitor_pid file.\n");
        fflush(stdout);
        exit(-1);
    }
    fprintf(mon_file, "%d", getpid());
    fflush(mon_file);
    fclose(mon_file);

    return mon_file;
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
    
    // register signal handlers
    struct sigaction sa_usr1, sa_int;

    // SIGUSR1 for new report notification
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    // SIGINT for graceful shutdown
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
        sigsuspend(&wait_mask);
    }
}

int main() {
    start_monitor();

    return 0;
}