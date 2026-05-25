#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    int id;
    char inspector_name[50];
    float latitude;
    float longitude;
    char issue_category[50];
    int severity_level;
    time_t timestamp;
    char description[500];
} Report;

typedef struct {
    char name[50];
    int score;
} InspectorScore;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ./scorer <district>\n");
        fflush(stdout);
        exit(1);
    }

    char path[100];
    snprintf(path, sizeof(path), "%s/reports.dat", argv[1]);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("District '%s': reports.dat not found.\n", argv[1]);
        fflush(stdout);
        exit(1);
    }

    InspectorScore scores[30];

    int count = 0;

    Report report;

    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        // search for inspector in scores
        int found = -1;

        for (int i = 0; i < count && i < 30; i++) {
            if (strcmp(scores[i].name, report.inspector_name) == 0) {
                found = i;
                break;
            }
        }

        // handle found or not found
        if (found == -1) {  // found
            strcpy(scores[count].name, report.inspector_name);
            scores[count].score = report.severity_level;
            count++;
        } else {  // not found
            scores[found].score += report.severity_level;
        }
    }

    close(fd);

    printf("------ District %s ------\n", argv[1]);
    fflush(stdout);

    if (count == 0) {
        printf("No reports found \n");
        fflush(stdout);
    }

    for (int i = 0; i < count; i++) {
        printf("\tInspector [%s] \t Score: %d\n", scores[i].name,
               scores[i].score);
        fflush(stdout);
    }

    return 0;
}