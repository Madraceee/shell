#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct history{
	char **stack;
	int i;
	int max;
	int ptr;
};


struct history* new_history(int max);
void insert_record(struct history* h, char* record);
char* get_history_all(struct history* h);
char* get_history_limit(struct history* h, int limit);
void history_up(struct history* h, char* input);
void history_down(struct history* h, char* input);
void history_load(struct history* h, char* path);
