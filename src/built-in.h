#pragma once
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/wait.h>
#include <ctype.h>
#include "history.h"

enum STATE {
	NORMAL,
	SINGLE,
	DOUBLE,
	REDIRECT,
};

void cd(int argc, char **argv, char** output);
void type(int argc, char **argv,char *cmds[], int no_of_cmds, char** output);
char *get_inbuilt_cmd_path(char *input);
void pwd(int argc, char **argv, char** output);
int history_cmd(int argc, char **argv, char** output, struct history* history);
void exec_cmd(int argc, char* cmd,char **argv, char** output);
void echo(int argc, char **argv,char **output);
