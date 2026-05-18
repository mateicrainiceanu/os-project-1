#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

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

typedef enum { ADD, LIST, REMOVE, FILTER, VIEW, UPDATE_THRESHOLD, REMOVE_DISTRICT } OperationType;

typedef struct {
    char role[50];
    char inspector_name[50];
    char district[50];
    OperationType operation;
    int report_id; // used for view and remove operations
    int value; // used for threshold updates operation
    char conditions[10][100];
    int condition_count;
} Usage;

// END: Data structures

// Command line argument parsing and utility functions

Usage parse_command_line_arguments(int argc, char* argv[]) {
    Usage usage;
    usage.condition_count = 0;
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
        for (int i = 7; i < argc && usage.condition_count < 10; i++) {
            strncpy(usage.conditions[usage.condition_count], argv[i], 99);
            usage.conditions[usage.condition_count][99] = '\0';
            usage.condition_count++;
        }
    } else if (strcmp(argv[5], "--view") == 0) {
        usage.operation = VIEW;
        usage.report_id = atoi(argv[7]); 
    } else if (strcmp(argv[5], "--update_threshold") == 0) {
        usage.operation = UPDATE_THRESHOLD;
        usage.value = atoi(argv[7]);
    } else if (strcmp(argv[5], "--remove_district") == 0) {
        usage.operation = REMOVE_DISTRICT;
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
        case REMOVE_DISTRICT:
            printf("Remove District\n");
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

// AI-generated: parse_condition splits "field:op:value" into its three parts.
// Returns 1 on success, 0 if the string is malformed.
int parse_condition(const char *input, char *field, char *op, char *value) {
    const char *first_colon = strchr(input, ':');
    if (!first_colon) return 0;

    int field_len = first_colon - input;
    strncpy(field, input, field_len);
    field[field_len] = '\0';

    const char *second_colon = strchr(first_colon + 1, ':');
    if (!second_colon) return 0;

    int op_len = second_colon - (first_colon + 1);
    strncpy(op, first_colon + 1, op_len);
    op[op_len] = '\0';

    strcpy(value, second_colon + 1);
    return 1;
}

// AI-generated: match_condition returns 1 if report r satisfies field op value.
int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int v = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity_level == v;
        if (strcmp(op, "!=") == 0) return r->severity_level != v;
        if (strcmp(op, "<")  == 0) return r->severity_level <  v;
        if (strcmp(op, "<=") == 0) return r->severity_level <= v;
        if (strcmp(op, ">")  == 0) return r->severity_level >  v;
        if (strcmp(op, ">=") == 0) return r->severity_level >= v;
    } else if (strcmp(field, "category") == 0) {
        int cmp = strcmp(r->issue_category, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
    } else if (strcmp(field, "inspector") == 0) {
        int cmp = strcmp(r->inspector_name, value);
        if (strcmp(op, "==") == 0) return cmp == 0;
        if (strcmp(op, "!=") == 0) return cmp != 0;
    } else if (strcmp(field, "timestamp") == 0) {
        time_t ts = (time_t)atol(value);
        if (strcmp(op, "==") == 0) return r->timestamp == ts;
        if (strcmp(op, "!=") == 0) return r->timestamp != ts;
        if (strcmp(op, "<")  == 0) return r->timestamp <  ts;
        if (strcmp(op, "<=") == 0) return r->timestamp <= ts;
        if (strcmp(op, ">")  == 0) return r->timestamp >  ts;
        if (strcmp(op, ">=") == 0) return r->timestamp >= ts;
    }
    return 0;
}

// END: Functions

// File operations for reports and configuration
void create_district_reports_symlink(char* district) {
    char target_path[100];
    snprintf(target_path, sizeof(target_path), "%s/reports.dat", district);

    char link_path[100];
    snprintf(link_path, sizeof(link_path), "active-reports-%s", district);

    if (symlink(target_path, link_path) == -1) {
        printf("Failed to create symbolic link for reports.dat in %s\n",
               district);
    }

   struct stat lst;
    if (lstat(link_path, &lst) == 0) {
        if (S_ISLNK(lst.st_mode)) {
            // check if the file exists
            struct stat st;
            if (stat(link_path, &st) == -1) {
                printf("Warning: dangling symlink detected at %s\n", link_path);
            }
        } else {
            printf("Warning: %s expected symlink, but is not\n", link_path);
            return;
        }
    }
}

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

void initialize_district_files(char* district) {
    // creating district and files if not exists
    create_dir_if_not_exists(district);
    create_file_in_directory(district, "reports.dat", 0664);

    bool created = create_file_in_directory(district, "district.cfg", 0640);
    if (created) {
        update_threshold_in_config(district, 2);
        create_district_reports_symlink(district);
    }

    create_file_in_directory(district, "logged_district", 0644);

    // END - creating district and files if not exists
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

void remove_district(char* district) {
    char dir_path[100];
    snprintf(dir_path, sizeof(dir_path), "%s", district);

    char link_path[100];
    snprintf(link_path, sizeof(link_path), "active-reports-%s", district);

    pid_t pid = fork();

    if (pid < 0) {
        printf("Failed to fork process for removing district.\n");
        return;
    } else if (pid == 0) {
        if(unlink(link_path) == -1) {
            printf("Failed to remove symlink: %s\n", link_path);
            exit(-1);
        } else {
            printf("Symlink %s removed successfully.\n", link_path);
        }

        execlp("rm", "rm", "-rf", dir_path, NULL);
        printf("Failed to execute rm command.\n");
        exit(-1);
    }

    int status;
    wait (&status);
    if (WEXITSTATUS(status) != 0) {
        printf("Failed to remove district %s.\n", district);
    } else {
        printf("District %s removed successfully.\n", district);
    }
}

// END: File operations for reports and configuration

// Signal sending

void notify_monitor_of_new_report() {
    FILE* mon_file = fopen(".monitor_pid", "r");
    if (mon_file == NULL) {
        printf("Monitor process is not running. Cannot send notification.\n");
        return;
    }

    int monitor_pid;
    fscanf(mon_file, "%d", &monitor_pid);
    fclose(mon_file);

    kill(monitor_pid, SIGUSR1);
}

// END - Signal sending

// Logic handlers for different operations

void handle_add_report(Usage usage) {
    Report report = read_report_data_from_stdin();

    report.id = 1;  // TODO: generate unique id based on last index in file;
    strcpy(report.inspector_name, usage.inspector_name);

    save_report_to_file(usage.district, report);

    notify_monitor_of_new_report();
}

void handle_list_reports(Usage usage) {
    list_reports_for_district(usage.district);
}

void handle_view_report(Usage usage) {
    view_report_by_id(usage.district, usage.report_id);
}

void handle_threshold_update(Usage usage) {

    if (strcmp(usage.role, "manager") != 0) {
        printf("Only managers users can remove a district.\n");
        return;
    }

    update_threshold_in_config(usage.district, usage.value);
}

void handle_remove_report(Usage usage) {
    if (strcmp(usage.role, "manager") != 0) {
        printf("Only managers users can remove a district.\n");
        return;
    }

    delete_report_by_id(usage.district, usage.report_id);
}

void handle_remove_district(Usage usage) {

    if (strcmp(usage.role, "manager") != 0) {
        printf("Only managers users can remove a district.\n");
        return;
    }

    remove_district(usage.district);
}

void handle_filter_reports(Usage usage) {
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", usage.district);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        printf("Failed to open file for reading: %s\n", file_path);
        exit(-1);
    }

    char fields[10][50], ops[10][10], values[10][100];
    for (int i = 0; i < usage.condition_count; i++) {
        if (!parse_condition(usage.conditions[i], fields[i], ops[i], values[i])) {
            printf("Invalid condition: %s\n", usage.conditions[i]);
            close(fd);
            exit(-1);
        }
    }

    Report report;
    int found = 0;
    while (read(fd, &report, sizeof(Report)) == sizeof(Report)) {
        int match = 1;
        for (int i = 0; i < usage.condition_count; i++) {
            if (!match_condition(&report, fields[i], ops[i], values[i])) {
                match = 0;
                break;
            }
        }
        if (match) {
            print_report_summary(report);
            found++;
        }
    }

    if (!found) {
        printf("No reports match the given conditions.\n");
    }

    close(fd);
}

// END: Logic handlers for different operations

// Main action handler

void handle_action(Usage usage) {
   initialize_district_files(usage.district);

    log_operation(usage);
    switch (usage.operation) {
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
            handle_filter_reports(usage);
            break;
        case VIEW:
            handle_view_report(usage);
            break;
        case UPDATE_THRESHOLD:
            handle_threshold_update(usage);
            break;
        case REMOVE_DISTRICT:
            handle_remove_district(usage);
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