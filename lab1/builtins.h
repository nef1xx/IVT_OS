#ifndef BUILTINS_H
#define BUILTINS_H

int builtin_cd(char **argv);

int builtin_clr(char **argv);

int builtin_dir(char **argv);

int builtin_environ(char **argv);

int builtin_echo(char **argv);

int builtin_help(char **argv);

int builtin_pause(char **argv);

int builtin_quit(char **argv);

int try_run_builtin(char **argv);

#endif
