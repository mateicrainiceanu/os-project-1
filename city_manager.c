#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>   
#include <fcntl.h>
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
    int escalation_threshold;
} Configuration;

typedef struct {
    char action[50];
    char district[50];
    time_t timestamp;
    char role[50];
    char inspector_name[50];
} OperationLogEntry;

Report read_report_data_from_stdin() {
    /* Returns a report without id and inspector data */
    Report report;
    printf("Enter report details:\n");
    printf("Latitude (X): ");
    scanf("%f", &report.latitude);
    printf("Longitude (Y): ");
    scanf("%f", &report.longitude);
    printf("Issue Category (road/lightning/flooding/other): ");
    scanf("%s", report.issue_category);
    printf("Severity Level (1-3): ");
    scanf("%d", &report.severity_level);
    printf("Description: ");
    scanf("%s", report.description);
    report.timestamp = time(NULL); // current time
    return report;
}

typedef enum { ADD, LIST, REMOVE, FILTER } OperationType;

typedef struct {
    char role[50];
    char inspector_name[50];
    char district[50];
    OperationType operation;
} Usage;

Usage parse_command_line_arguments(int argc, char *argv[]) {
    Usage usage;
    if (argc < 5) {
        printf("Usage: %s --role <role> --user <inspector_name> --[operation] <district>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    strncpy(usage.role, argv[2], sizeof(usage.role) - 1);
    strncpy(usage.inspector_name, argv[4], sizeof(usage.inspector_name) - 1);
    strncpy(usage.district, argv[6], sizeof(usage.district) - 1);
    
    if (strcmp(argv[5], "--add") == 0) {
        usage.operation = ADD;
    } else if (strcmp(argv[5], "--list") == 0) {
        usage.operation = LIST;
    } else if (strcmp(argv[5], "--remove_report") == 0) {
        usage.operation = REMOVE;
    } else if (strcmp(argv[5], "--filter") == 0) {
        usage.operation = FILTER;
    } else {
        printf("Invalid operation: %s\n", argv[5]);
        exit(EXIT_FAILURE);
    }
    return usage;
}

void print_usage(Usage u) {
    printf("Role: %s\n", u.role);
    printf("Inspector Name: %s\n", u.inspector_name);
    printf("District: %s\n", u.district);
    printf("Operation: ");
    switch (u.operation) {
        case ADD:
            printf("Add Report\n");
            break;
        case LIST:
            printf("List Reports\n");
            break;
        case REMOVE:
            printf("Remove Report\n");
            break;
        case FILTER:
            printf("Filter Reports\n");
            break;
    }
}

void create_dir_if_not_exists(char *dir_name) { 
    struct stat st;
    if (stat(dir_name, &st) == 0 && S_ISDIR(st.st_mode)) { // found directory
        return;
    }

    mkdir(dir_name, 0750);
}

void create_file_in_directory(char *dir_name, char* file_name, int permissions) {
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, file_name);
    
    int fd = open(file_path, O_CREAT | O_WRONLY | O_TRUNC, permissions);
    close(fd);
}

void handle_action(Usage usage) {
    create_dir_if_not_exists(usage.district);
    create_file_in_directory(usage.district, "reports.dat", 0664);
    create_file_in_directory(usage.district, "district.cfg", 0640);
    create_file_in_directory(usage.district, "logged_district", 0644);

    switch (usage.operation) {
        case ADD:
            break;
        default:
            printf("Unknown operation\n");
            break;
    }
}

int main(int argc, char *argv[]) {

    Usage usage = parse_command_line_arguments(argc, argv);
    print_usage(usage);
    handle_action(usage);

    return 0;
}