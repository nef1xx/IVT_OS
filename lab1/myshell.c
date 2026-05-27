
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "builtins.h"

#define MAX_LINE_LEN 1024
#define MAX_ARGS     64


static int split_line(char *line, char **argv, int max_args)
{
    int count = 0;
    char *token = strtok(line, " \t\n");
    while (token != NULL && count < max_args - 1) {
        argv[count++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[count] = NULL;  
    return count;
}


static void run_external(char **argv, const char *shell_path)
{
    if (setenv("parent", shell_path, 1) != 0) {
        perror("setenv parent");
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "myshell: %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
    }
}

static void process_line(char *line, const char *shell_path)
{
    char *argv[MAX_ARGS];
    int argc = split_line(line, argv, MAX_ARGS);

    if (argc == 0) {
        return;
    }

    if (try_run_builtin(argv)) {
        return;
    }

    run_external(argv, shell_path);
}

static void shell_loop(FILE *input, int interactive, const char *shell_path)
{
    char line[MAX_LINE_LEN];

    while (1) {
        if (interactive) {
            printf("myshell> ");
            fflush(stdout);
        }

        if (fgets(line, sizeof(line), input) == NULL) {
            if (interactive) {
                printf("\n");
            }
            break;
        }

        process_line(line, shell_path);
    }
}

static char *resolve_shell_path(const char *argv0)
{
    char *resolved = realpath(argv0, NULL);
    if (resolved != NULL) {
        return resolved;
    }
    return strdup(argv0);
}

int main(int argc, char *argv[])
{
    char *shell_path = resolve_shell_path(argv[0]);
    if (shell_path == NULL) {
        fprintf(stderr, "myshell: не удалось определить путь к оболочке\n");
        return 1;
    }фы

    if (setenv("shell", shell_path, 1) != 0) {
        perror("setenv shell");
    }

    if (argc == 1) {
        shell_loop(stdin, 1, shell_path);
    } else if (argc == 2) {
        FILE *batch = fopen(argv[1], "r");
        if (batch == NULL) {
            fprintf(stderr, "myshell: %s: %s\n", argv[1], strerror(errno));
            free(shell_path);
            return 1;
        }
        shell_loop(batch, 0, shell_path);
        fclose(batch);
    } else {
        fprintf(stderr, "Использование: %s [batchfile]\n", argv[0]);
        free(shell_path);
        return 1;
    }

    free(shell_path);
    return 0;
}
