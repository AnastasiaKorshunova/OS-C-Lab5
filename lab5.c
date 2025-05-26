#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

typedef struct {
    int num_processes;
    int exec_index;
    char **exec_args;
} Args;

void read_command_interactively(char *line, size_t size);
void build_exec_args(char *line, Args *args);
void create_children(Args *args, pid_t *children);
void child_loop(int index);
void run_exec(char **exec_args);
void terminate_children(pid_t *children, int count);
void wait_for_children(pid_t *children, int count);
void take_screenshot(const char *prefix, char **exec_args);

int main(int argc, char *argv[]) {
    Args args;

    if (argc < 4) {
        printf("Enter the number of child processes: ");
        scanf("%d", &args.num_processes);

        printf("Enter the index of the process that will run exec (from 0 to %d): ", args.num_processes - 1);
        scanf("%d", &args.exec_index);
        getchar(); // remove newline

        char line[256];
        read_command_interactively(line, sizeof(line));
        build_exec_args(line, &args);
    } else {
        args.num_processes = atoi(argv[1]);
        args.exec_index = atoi(argv[2]);

        if (args.exec_index < 0 || args.exec_index >= args.num_processes) {
            fprintf(stderr, "Invalid exec_index: %d (should be from 0 to %d)\n", args.exec_index, args.num_processes - 1);
            exit(EXIT_FAILURE);
        }

        args.exec_args = &argv[3];
    }

    pid_t *children = malloc(sizeof(pid_t) * args.num_processes);
    if (!children) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    create_children(&args, children);

    sleep(2);
    take_screenshot("start", args.exec_args);

    sleep(3);
    terminate_children(children, args.num_processes);
    wait_for_children(children, args.num_processes);

    free(children);
    printf("Parent process finished.\n");
    fflush(stdout);
    usleep(200000); 

    take_screenshot("end", args.exec_args);

    return 0;
}

void read_command_interactively(char *line, size_t size) {
    printf("Enter the command to run via exec (e.g., ls, ps aux, time ls):\n> ");
    fgets(line, size, stdin);
    line[strcspn(line, "\n")] = 0;
}

void build_exec_args(char *line, Args *args) {
    if (strcmp(line, "time") == 0) {
        strcpy(line, "time true");
    }

    args->exec_args = malloc(4 * sizeof(char *));
    args->exec_args[0] = strdup("bash");
    args->exec_args[1] = strdup("-c");
    args->exec_args[2] = strdup(line);
    args->exec_args[3] = NULL;
}

void create_children(Args *args, pid_t *children) {
    for (int i = 0; i < args->num_processes; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            if (i == args->exec_index) {
                run_exec(args->exec_args);
            } else {
                child_loop(i);
            }
        } else {
            children[i] = pid;
        }
    }
}

void child_loop(int index) {
    while (1) {
        printf("Child #%d (PID %d) is running...\n", index, getpid());
        sleep(1);
    }
}

void run_exec(char **exec_args) {
    printf("Running command: ");
    for (int i = 0; exec_args[i] != NULL; i++) {
        printf("%s ", exec_args[i]);
    }
    printf("\n");
    fflush(stdout);

    execvp(exec_args[0], exec_args);
    perror("exec failed");
    exit(EXIT_FAILURE);
}

void terminate_children(pid_t *children, int count) {
    for (int i = 0; i < count; i++) {
        kill(children[i], SIGKILL);
        printf("Terminated child with PID %d\n", children[i]);
    }
}

void wait_for_children(pid_t *children, int count) {
    for (int i = 0; i < count; i++) {
        waitpid(children[i], NULL, 0);
    }
}

void take_screenshot(const char *prefix, char **exec_args) {
    char command[256];
    char filename[128];

    snprintf(filename, sizeof(filename), "screenshot_%s_%s.png", prefix, exec_args[0]);
    snprintf(command, sizeof(command), "scrot %s", filename);

    int result = system(command);
    if (result == 0) {
        printf("Screenshot saved as: %s\n", filename);
    } else {
        fprintf(stderr, "Failed to take screenshot (is scrot installed?)\n");
    }
}
