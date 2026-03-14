#include "history.h"

struct history* new_history(int max){
	struct history *history = (struct history*)malloc(sizeof(struct history));
	history->i = 0;
	history->max = max;
	history->stack = (char**)malloc(sizeof(char*) * max);
	return history;
}

void insert_record(struct history* h, char* record){
	h->stack[h->i++] = record;
}

char* get_history_all(struct history* h){
	char *output = (char*)malloc(sizeof(char) * (h->i * PATH_MAX));
	output[0] = '\0';
	for(int i=0;i<h->i;i++){
		char *line = (char*)malloc(sizeof(char)*(strlen(h->stack[i])+20));
		sprintf(line,"\t%d %s\n", i+1, h->stack[i]);
		strcat(output, line);
	}
	return output;
}

char* get_history_limit(struct history* h, int limit){
	char *output = (char*)malloc(sizeof(char) * (limit * PATH_MAX));
	output[0] = '\0';
	int i = h->i - limit - 1;
	if (i < 0 ){
		i = 0;
	}
	for(;i<h->i;i++){
		char *line = (char*)malloc(sizeof(char)*(strlen(h->stack[i])+20));
		sprintf(line,"\t%d %s\n", i+1, h->stack[i]);
		strcat(output, line);
	}
	return output;
}
