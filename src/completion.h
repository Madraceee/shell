#pragma once
#include <ctype.h>
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
#include "built-in.h"
#include "trie.h"

char* handle_tab(char *input, int *input_count, int *is_tab_pressed,struct trie *cmd_completion);
