#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct history{
	char **stack;
	int i;
	int max;
};


struct history* new_history(int max);
void insert_record(struct history* h, char* record);
char* get_history_all(struct history* h);
char* get_history_limit(struct history* h, int limit);
