#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


int hub_mon_pipe[2];

void hub_mon() {
    // child

    close(hub_mon_pipe[0]);  // close the read end of the pipe in the child
    dup2(hub_mon_pipe[1], 1); // redirect stdout to the write end of the pipe
    close(hub_mon_pipe[1]); // close the write end of the pipe in the child

    // start the monitor
    pid_t mon_pid = fork();
    if (mon_pid == 0) {
        // grandchild — becomes monitor_reports
        execlp("./monitor_reports", "monitor_reports", NULL);
        perror("execlp");
        exit(1);
    }
}

void parent() {
    close(hub_mon_pipe[1]);  // Close the write end of the pipe in the parent

    char buf[512];
    int n = 0;
    // open the read end of the pipe to read output from the monitor
    while ((n = read(hub_mon_pipe[0], buf, sizeof(buf))) > 0) {
        // read output from the monitor and process it
        buf[n] = '\0';
        printf("Received from monitor: %s", buf);
        fflush(stdout);
    }
    
}

void start_hub_mon() {
    pipe(hub_mon_pipe);
    
    int pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    
    if (pid == 0) { 
        // child process
        hub_mon();
        exit(0);
    }
    
    // parent process continues
    parent();
}


int main() {

    start_hub_mon();


    return 0;
}