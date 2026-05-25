#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int hub_mon_pipe[2];

void hub_mon() {
    // child

    close(hub_mon_pipe[0]);    // close the read end of the pipe in the child
    dup2(hub_mon_pipe[1], 1);  // redirect stdout to the write end of the pipe
    close(hub_mon_pipe[1]);    // close the write end of the pipe in the child

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

int create_scorer_process(const char* district) { // returns the read and of the pipe
    int fd[2];

    pipe(fd);

    int pid = fork();
    if (pid < 0) {
        printf("Error foking");
        exit(1);
    }

    if (pid == 0) {
        // child

        close(fd[0]);       // close the read pipe
        dup2(fd[1], 1);     // stdout to the write pipe
        close(fd[1]);       // close the write end

        execlp("./scorer", "./scorer", district, NULL);
    }

    //parent
    close(fd[1]);
    return fd[0];
}

void handle_district_scorer(char* district) {
    int fd = create_scorer_process(district);

    if (fd == -1) {
        printf("Failed to create scorer process for district %s.\n", district);
        return;
    }

    char buf[512];
    int n = 0;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
        fflush(stdout);
    }
    
    close(fd);
}

void handle_calculate_scores(char* districts) {
    printf("=== WORKLOAD REPORT ===\n");
    char* token = strtok(districts, " ");
    while (token != NULL) {
        handle_district_scorer(token);
        token = strtok(NULL, " ");
    }
    printf("=== END WORKLOAD REPORT ===\n");
}



int main() {
    char buf[512];

    while (1) {
        if (fgets(buf, 512, stdin) == NULL) return 0;

        buf[strcspn(buf, "\n")] = '\0'; // replace newline with \0

        if (strcmp("start_monitor", buf) == 0) {
            start_hub_mon();
        } else if (strncmp("calculate_scores", buf, strlen("calculate_scores")) ==
                 0) {

            char *args = buf + strlen("calculate_scores") + 1; // skip the command and the space
            handle_calculate_scores(args);
        } else {
            printf("Unknown command %s\n", buf);
        }
    };

    return 0;
}