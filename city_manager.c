#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Data structures
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

typedef enum { ADD, LIST, REMOVE, FILTER, VIEW, UPDATE_THRESHOLD } OperationType;

typedef struct {
    char role[50];
    char inspector_name[50];
    char district[50];
    OperationType operation;
    int report_id; // used for view and remove operations
    int value; // used for threshold updates operation
} Usage;

// END: Data structures

// Command line argument parsing and utility functions

Usage parse_command_line_arguments(int argc, char* argv[]) {
    Usage usage;
    if (argc < 5) {
        printf(
            "Usage: %s --role <role> --user <inspector_name> --[operation] "
            "<district>\n",
            argv[0]);
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
        usage.report_id = atoi(argv[7]);
    } else if (strcmp(argv[5], "--filter") == 0) {
        usage.operation = FILTER;
    } else if (strcmp(argv[5], "--view") == 0) {
        usage.operation = VIEW;
        usage.report_id = atoi(argv[7]); 
    } else if (strcmp(argv[5], "--update-threshold") == 0) {
        usage.operation = UPDATE_THRESHOLD;
        usage.value = atoi(argv[7]);
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
        case VIEW:
            printf("View Report\n");
            break;
        case UPDATE_THRESHOLD:
            printf("Update Threshold\n");
            printf("New Threshold Value: %d\n", u.value);
            break;
        case REMOVE:
            printf("Remove Report\n");
            break;
        case FILTER:
            printf("Filter Reports\n");
            break;
    }
}

// END: Command line argument parsing and utility functions

// File and directory management functions

void create_dir_if_not_exists(char* dir_name) {
    struct stat st;
    if (stat(dir_name, &st) == 0 && S_ISDIR(st.st_mode)) {  // found directory
        return;
    }

    mkdir(dir_name, 0750);
}

bool create_file_in_directory(const char* dir_name,
                              const char* file_name,
                              int permissions) {
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, file_name);

    int fd = open(file_path, O_CREAT | O_EXCL | O_WRONLY, permissions);

    if (fd == -1) {  // a file already exists
        return false;
    }

    close(fd);
    return true;
}

void print_file_permissions(char *file_path) {
    struct stat st;
    if (lstat(file_path, &st) == -1) {
        printf("Failed to get file permissions: %s\n", file_path);
        return;
    }
    
    printf("Permissions for %s: ", file_path);
    printf((st.st_mode & S_IRUSR) ? "r" : "-");
    printf((st.st_mode & S_IWUSR) ? "w" : "-");
    printf((st.st_mode & S_IXUSR) ? "x" : "-");
    printf((st.st_mode & S_IRGRP) ? "r" : "-");
    printf((st.st_mode & S_IWGRP) ? "w" : "-");
    printf((st.st_mode & S_IXGRP) ? "x" : "-");
    printf((st.st_mode & S_IROTH) ? "r" : "-");
    printf((st.st_mode & S_IWOTH) ? "w" : "-");
    printf((st.st_mode & S_IXOTH) ? "x" : "-");
    printf("\n");
}

// END: File and directory management functions

// Functions

void print_report_full(Report report) {
    printf("ID: %d\n", report.id);
    printf("Inspector: %s\n", report.inspector_name);
    printf("Location: (%f, %f)\n", report.latitude, report.longitude);
    printf("Category: %s\n", report.issue_category);
    printf("Severity: %d\n", report.severity_level);
    printf("Description: %s\n", report.description);
    printf("Timestamp: %s\n", ctime(&report.timestamp));
}

void print_report_summary(Report report) {
    printf("ID: %d | Inspector: %s | Location: (%f, %f) | Category: %s | "
           "Severity: %d\n",
           report.id, report.inspector_name, report.latitude, report.longitude,
           report.issue_category, report.severity_level);
}

char* operation_to_string(OperationType op) {
    switch (op) {
        case ADD:
            return "ADD";
        case LIST:
            return "LIST";
        case REMOVE:
            return "REMOVE";
        case FILTER:
            return "FILTER";
        case VIEW:
            return "VIEW";
        case UPDATE_THRESHOLD:
            return "UPDATE_THRESHOLD";
        default:
            return "UNKNOWN";
    }
}

// END: Functions

// File operations for reports and configuration

void update_threshold_in_config(char* distrct_name, int new_threshold) {
    char config_path[100];
    snprintf(config_path, sizeof(config_path), "%s/district.cfg", distrct_name);

    FILE* config_file = fopen(config_path, "w");
    if (config_file == NULL) {
        printf("Failed to open config file");
        return;
    }

    struct stat st;
    if (stat(config_path, &st) == -1) {
        printf("Failed to get file permissions: %s\n", config_path);
    } else {
       // verify that the permissions are 0640
        if ((st.st_mode & 0777) != 0640) {
            printf("Warning: Config file permissions are not 0640\n");
            return;
        } 
    }

    fprintf(config_file, "escalation_threshold=%d\n", new_threshold);

    fclose(config_file);
}

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
    scanf(" ");
    fgets(report.description, sizeof(report.description), stdin);
    report.timestamp = time(NULL);  // current time
    return report;
}

void save_report_to_file(char* district, Report report) {
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", district);

    int fd = open(file_path, O_RDWR, 0644);
    if (fd == -1) {
        printf("Failed to open file for writing: %s\n", file_path);
        exit(-1);
    }

    Report last_report;

    if (lseek(fd, 0, SEEK_END) == 0) {  // file is empty
        report.id = 1;
    } else {
        lseek(fd, -sizeof(Report), SEEK_END);

        if (read(fd, &last_report, sizeof(Report)) == sizeof(Report)) {
            report.id = last_report.id + 1;
        } else {
            report.id = 1;
        }
    }

    lseek(fd, 0, SEEK_END);

    write(fd, &report, sizeof(Report));

    close(fd);
}

void list_reports_for_district(char* district) {
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", district);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        printf("Failed to open file for reading: %s\n", file_path);
        exit(-1);
    }

    print_file_permissions(file_path);

    Report report;
    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        print_report_summary(report);
    }

    close(fd);
}

void view_report_by_id(char* district, int report_id) {
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", district);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        printf("Failed to open file for reading: %s\n", file_path);
        exit(-1);
    }

    Report report;
    bool found = false;
    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        if (report.id == report_id) {
            print_report_full(report);
            found = true;
            break;
        }
    }

    if (!found) {
        printf("Report with ID %d not found in district %s.\n", report_id,
               district);
    }

    close(fd);
}

void delete_report_by_id(char* district, int report_id) {
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", district);

    int fd = open(file_path, O_RDWR);
    if (fd == -1) {
        printf("Failed to open file for reading/writing: %s\n", file_path);
        exit(-1);
    }

    Report report;
    bool found = false;

    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        if (report.id == report_id) {
            found = true;
            continue;
        }

        if (found) {
            lseek(fd, -sizeof(Report) * 2, SEEK_CUR);
            write(fd, &report, sizeof(Report));
            lseek(fd, sizeof(Report), SEEK_CUR);
        }
    }

    if (!found) {
        printf("Report with ID %d not found in district %s.\n", report_id,
               district);
    } else {
        printf("Report with ID %d deleted from district %s.\n", report_id,
               district);
        ftruncate(fd, lseek(fd, -sizeof(Report), SEEK_CUR));
    }

    close(fd);
}

void log_operation(Usage usage) {
    char log_path[100];
    snprintf(log_path, sizeof(log_path), "%s/logged_district", usage.district);

    FILE* log_file = fopen(log_path, "a");
    if (log_file == NULL) {
        printf("Failed to open log file: %s\n", log_path);
        return;
    }  

    time_t now = time(NULL);

    fprintf(log_file, "Role: %s | Inspector: %s | Operation: %s | Timestamp: "
                      "%s\n",
            usage.role, 
            usage.inspector_name, 
            operation_to_string(usage.operation),
            ctime(&now));
    fclose(log_file);
}

// END: File operations for reports and configuration


// Logic handlers for different operations

void handle_add_report(Usage usage) {
    Report report = read_report_data_from_stdin();

    report.id = 1;  // TODO: generate unique id based on last index in file;
    strcpy(report.inspector_name, usage.inspector_name);

    save_report_to_file(usage.district, report);
}

void handle_list_reports(Usage usage) {
    list_reports_for_district(usage.district);
}

void handle_view_report(Usage usage) {
    view_report_by_id(usage.district, usage.report_id);
}

void handle_threshold_update(Usage usage) {
    update_threshold_in_config(usage.district, usage.value);
}

void handle_remove_report(Usage usage) {
    delete_report_by_id(usage.district, usage.report_id);
}

// END: Logic handlers for different operations

// Main action handler

void handle_action(Usage usage) {
    // creating district and files if not exists
    create_dir_if_not_exists(usage.district);
    create_file_in_directory(usage.district, "reports.dat", 0664);

    bool created =
        create_file_in_directory(usage.district, "district.cfg", 0640);
    if (created) {
        update_threshold_in_config(usage.district, 2);
    }

    create_file_in_directory(usage.district, "logged_district", 0644);
    // END - creating district and files if not exists

    switch (usage.operation) {
        log_operation(usage);
        case ADD:
            handle_add_report(usage);
            break;
        case LIST:
            handle_list_reports(usage);
            break;
        case REMOVE:
            handle_remove_report(usage);
            break;
        case FILTER:
            break;
        case VIEW:
            handle_view_report(usage);
            break;
        case UPDATE_THRESHOLD:
            handle_threshold_update(usage);
            break;
        default:
            printf(
                "Unimplemented operation in switch statement - handleAction\n");
            break;

    }
}

int main(int argc, char* argv[]) {
    Usage usage = parse_command_line_arguments(argc, argv);
    print_usage(usage);
    handle_action(usage);

    return 0;
}