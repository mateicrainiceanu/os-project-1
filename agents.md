# AI TOOLS USED
- Claude Code - for implementing filtering and creation of test
- Claude - for quick information and code snippets to implement
- Copilot Inline Suggestions

# Original Prompt - for implementing the ai allowed feature
## Prompt
in filter_requirement.txt, you have the requirement for implementing the filter functionality. this function is allowed to be implemented with ai. also document in agents.md the communication with the ai. respect and thest that function. add more reports to the test_district and test them.

## Changes and AI Code

### 1. Extended the `Usage` struct to hold filter conditions

```c
typedef struct {
    char role[50];
    char inspector_name[50];
    char district[50];
    OperationType operation;
    int report_id;
    int value;
    char conditions[10][100];  // added
    int condition_count;        // added
} Usage;
```

### 2. Initialised `condition_count` and parsed conditions from argv

At the top of `parse_command_line_arguments`:
```c
usage.condition_count = 0;
```

In the `--filter` branch:
```c
} else if (strcmp(argv[5], "--filter") == 0) {
    usage.operation = FILTER;
    for (int i = 7; i < argc && usage.condition_count < 10; i++) {
        strncpy(usage.conditions[usage.condition_count], argv[i], 99);
        usage.conditions[usage.condition_count][99] = '\0';
        usage.condition_count++;
    }
}
```

### 3. AI-generated: `parse_condition`

```c
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
```

### 4. AI-generated: `match_condition`

```c
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
```

### 5. Manually written: `handle_filter_reports`

```c
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
```

### 6. Wired up the handler in `handle_action`

```c
case FILTER:
    handle_filter_reports(usage);
    break;
```

# AI Usage Log — Filter Feature

## Tool Used
Claude Sonnet 4.6 (via Claude Code CLI), session date 2026-05-02.

## Context Provided to the AI

The Report struct and its fields were described:

```c
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
```

Supported filter fields: `severity` (int), `category` (string), `inspector` (string), `timestamp` (time_t).  
Supported operators: `==`, `!=`, `<`, `<=`, `>`, `>=`.  
Condition format: `field:operator:value`.

## Prompts Given

1. "Generate a function `int parse_condition(const char *input, char *field, char *op, char *value)` that splits a `field:operator:value` string into its three parts. Return 1 on success, 0 if the string is malformed."

2. "Generate a function `int match_condition(Report *r, const char *field, const char *op, const char *value)` that returns 1 if the record satisfies the condition and 0 otherwise. Fields are: severity (int), category (string), inspector (string), timestamp (time_t)."

## What Was Generated

### parse_condition
Used `strchr` twice to locate the first and second colons. Computed lengths by pointer arithmetic and used `strncpy`/`strcpy` to fill the three output buffers. Returns 0 early if either colon is missing.

### match_condition
Dispatched on `field` with a chain of `strcmp` calls. For numeric fields (`severity`, `timestamp`) it used `atoi`/`atol` to convert `value` and compared with the standard operators. For string fields (`category`, `inspector`) it used `strcmp` and only applied `==` and `!=`.

## Changes Made After Review

- No logic changes were needed; both functions were correct as generated.
- Added `\0` terminator safety (`field[field_len] = '\0'`) — the AI included this already.
- Confirmed that `atol` is appropriate for `time_t` (which is a long on this platform).
- Decided to leave unsupported operator+field combinations (e.g. `<` on a string field) returning 0 silently rather than erroring, matching the generated behaviour — this is acceptable because the requirement does not specify an error for invalid combinations.

## Filter Logic

The filter handler `handle_filter_reports` was written without AI assistance:
- Opens `reports.dat` with `open(O_RDONLY)`.
- Calls `parse_condition` for each condition stored in `usage.conditions`.
- Reads reports one at a time with `read()` in a loop.
- For each report, calls `match_condition` against every parsed condition; prints with `print_report_summary` only if all return 1.
- Prints a "no results" message if no report matched.

# Testing suite
### Prompts:
Create me a bash script for testing the city_manager.c executable

### Output
test_city_manager.sh

# City Manager

## print_file permissions

### Propmt
Create me a function to print file permissions

### Response
```c
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
```

## create_district_reports_symlink
### Propmt
how do i create a symlink in C
also add checking if the symlink exists or is dangling

### Response
Using the symlink call
```c
char link_name[100];
char target[100];

snprintf(link_name, sizeof(link_name), "active_reports-%s", district);
snprintf(target, sizeof(target), "%s/reports.dat", district);

if (symlink(target, link_name) == -1) {
    perror("symlink");
}
```

```c
struct stat st;

// check if symlink already exists
if (lstat(link_name, &st) == 0) {
    if (S_ISLNK(st.st_mode)) {
        // symlink exists — check if it's dangling
        if (stat(link_name, &st) == -1) {
            printf("Warning: dangling symlink detected at %s\n", link_name);
            unlink(link_name); // remove it and recreate below
        } else {
            printf("Symlink %s already exists.\n", link_name);
            return; // valid symlink already in place
        }
    }
}
```
### Implementation

```c
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
```

# Monitor Reports

## Signal handling
### Propmt
how do i configure a signal handler in c
how do i wait for multiple signals
### Response
```c
struct sigaction sa;
sa.sa_handler = your_handler_function;
sigemptyset(&sa.sa_mask);
sa.sa_flags = 0;
sigaction(SIGINT, &sa, NULL); // replace with you signal
```

```c
sigset_t wait_mask;
sigfillset(&wait_mask);          // block all signals...
sigdelset(&wait_mask, SIGUSR1);  // ...except SIGUSR1
sigdelset(&wait_mask, SIGUSR2);  // ...except SIGUSR2
sigdelset(&wait_mask, SIGINT);   // ...and SIGINT

while (1) {
    sigsuspend(&wait_mask);      // sleep until one of them arrives
}
```
### Implementation

```c

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
```

# CityHub
## Command line interface
### Prompt
The first command does not do anything, commands are only processed from the second command downwards

```c
    char buf[32];

    while (scanf("%s\n", buf)) {
        if (strcmp("start_monitor", buf) == 0) {
            start_hub_mon();
        } else if (strncmp("calculate_scores", buf,
                           strlen("calculate_scores")) == 0) {
            printf("calculating scores\n");
        } else {
            printf("Unknown command");
        }
    };

    return 0;
```

### Response 

The issue is with scanf("%s\n", buf) — %s stops reading at the first whitespace, and the \n in the format string doesn't do what you'd expect. Also scanf returns the number of items read, so it will loop but may not match correctly.

Use fgets instead:

```c
char buf[32];

while (1) {
    printf("> ");
    fflush(stdout);

    if (fgets(buf, sizeof(buf), stdin) == NULL) break;
    buf[strcspn(buf, "\n")] = '\0'; // strip newline

    if (strcmp(buf, "start_monitor") == 0) {
        start_hub_mon();
    } else if (strcmp(buf, "calculate_scores") == 0) {
        printf("calculating scores\n");
    } else {
        printf("Unknown command: %s\n", buf);
    }
}
```

strcspn(buf, "\n") finds the position of the newline that fgets leaves in the buffer and replaces it with \0 — without this, strcmp("start_monitor", buf) would be comparing against "start_monitor\n" and never match.

<!-- 
## function_name
### Propmt
### Response
### Implementation
 -->