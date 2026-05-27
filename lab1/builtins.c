#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

extern char **environ;

#include "builtins.h"

int builtin_cd(char **argv)
{
    if (argv[1] == NULL) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("cd: getcwd");
            return -1;
        }
        printf("%s\n", cwd);
        return 0;
    }

    if (chdir(argv[1]) != 0) {
        fprintf(stderr, "cd: %s: %s\n", argv[1], strerror(errno));
        return -1;
    }

    char new_cwd[1024];
    if (getcwd(new_cwd, sizeof(new_cwd)) != NULL) {
        if (setenv("PWD", new_cwd, 1) != 0) {
            perror("cd: setenv PWD");
        }
    }

    return 0;
}

int builtin_clr(char **argv)
{
    (void)argv;
    printf("\033[H\033[2J");
    fflush(stdout);
    return 0;
}

int builtin_dir(char **argv)
{
    const char *path = (argv[1] != NULL) ? argv[1] : ".";

    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "dir: %s: %s\n", path, strerror(errno));
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return 0;
}

int builtin_environ(char **argv)
{
    (void)argv;
    for (char **env = environ; *env != NULL; env++) {
        printf("%s\n", *env);
    }
    return 0;
}

int builtin_echo(char **argv)
{
    for (int i = 1; argv[i] != NULL; i++) {
        if (i > 1) {
            printf(" ");
        }
        printf("%s", argv[i]);
    }
    printf("\n");
    return 0;
}

int builtin_help(char **argv)
{
    (void)argv;
    printf(
        "myshell — простая UNIX-оболочка.\n"
        "\n"
        "Встроенные команды:\n"
        "  cd <dir>      сменить текущий каталог (без аргумента — показать его)\n"
        "  clr           очистить экран\n"
        "  dir <dir>     показать содержимое каталога\n"
        "  environ       показать все переменные среды\n"
        "  echo <text>   вывести текст и перейти на новую строку\n"
        "  help          показать это сообщение\n"
        "  pause         пауза до нажатия Enter\n"
        "  quit          выйти из оболочки\n"
        "\n"
        "Любая другая команда запускается как внешняя программа через fork/exec.\n"
        "Запуск с аргументом-файлом: ./myshell <batchfile> — исполнить команды из файла.\n"
    );
    return 0;
}

int builtin_pause(char **argv)
{
    (void)argv;
    printf("Нажмите Enter для продолжения...");
    fflush(stdout);

    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
    }
    return 0;
}

int builtin_quit(char **argv)
{
    (void)argv;
    exit(0);
}

int try_run_builtin(char **argv)
{
    if (argv == NULL || argv[0] == NULL) {
        return 0;
    }

    struct builtin_entry {
        const char *name;
        int (*func)(char **);
    };

    static const struct builtin_entry table[] = {
        { "cd",      builtin_cd      },
        { "clr",     builtin_clr     },
        { "dir",     builtin_dir     },
        { "environ", builtin_environ },
        { "echo",    builtin_echo    },
        { "help",    builtin_help    },
        { "pause",   builtin_pause   },
        { "quit",    builtin_quit    },
    };
    const int table_size = sizeof(table) / sizeof(table[0]);

    for (int i = 0; i < table_size; i++) {
        if (strcmp(argv[0], table[i].name) == 0) {
            table[i].func(argv);
            return 1;
        }
    }

    return 0;
}
