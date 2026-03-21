#pragma once
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>
#include <termios.h>
#include <ctype.h>

enum STATE {
	NORMAL,
	SINGLE,
	DOUBLE,
	REDIRECT_SUCCESS,
	REDIRECT_FAILURE,
	PIPE,
};

char* trim(char *str);
int get_cmd_and_args(char **raw_arg, char **cmd, char *args[], enum STATE *state);
