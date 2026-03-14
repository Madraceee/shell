#include "history.h"
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct history* new_history(int max){
	struct history *history = (struct history*)malloc(sizeof(struct history));
	history->i = -1;
	history->ptr = 0;
	history->max = max;
	history->stack = (char**)malloc(sizeof(char*) * max);
	history->last_appended_history = 0;
	history->history_file_path = NULL;


	char* path = getenv("HISTFILE");
	if(path != NULL){
		history->history_file_path = path;
		history_load(history, path);
	}

	return history;
}

void insert_record(struct history* h, char* record){
	h->i++;
	h->stack[h->i] = record;
	h->ptr = h->i+1;
}

char* get_history_all(struct history* h){
	char *output = (char*)malloc(sizeof(char) * (h->i * PATH_MAX));
	output[0] = '\0';
	for(int i=0;i<=h->i;i++){
		char *line = (char*)malloc(sizeof(char)*(strlen(h->stack[i])+20));
		sprintf(line,"\t%d %s\n", i+1, h->stack[i]);
		strcat(output, line);
	}
	return output;
}

char* get_history_limit(struct history* h, int limit){
	char *output = (char*)malloc(sizeof(char) * (limit * PATH_MAX));
	output[0] = '\0';
	int i = h->i - limit + 1;
	if (i < 0 ) {
		i = 0;
	}
	for(;i<=h->i;i++){
		char *line = (char*)malloc(sizeof(char)*(strlen(h->stack[i])+20));
		sprintf(line,"\t%d %s\n", i+1, h->stack[i]);
		strcat(output, line);
	}
	return output;
}

void history_up(struct history* h, char* input){
	if(h->i == -1  || h->ptr == 0){
		return;
	}
	h->ptr--;
	printf("\r\033[2K$ %s", h->stack[h->ptr]);
	strcpy(input, h->stack[h->ptr]);
	fflush(stdout);
}


void history_down(struct history* h, char* input){
	if(h->i == 0 || h->ptr == h->i){
		return;
	}
	h->ptr++;
	printf("\r\033[2K$ %s", h->stack[h->ptr]);
	strcpy(input, h->stack[h->ptr]);
	fflush(stdout);
}

void history_load(struct history* h, char* path){
	FILE* file = fopen(path, "r");

	char* output = (char*)malloc(sizeof(char)*PATH_MAX);
	output[0]='\0';
	fgets(output, PATH_MAX, file);
	while(output[0] != '\0'){
		output[strlen(output)-1] = '\0';
		insert_record(h, output);

		output = (char*)malloc(sizeof(char)*PATH_MAX);
		output[0] = '\0';
		fgets(output, PATH_MAX, file);
	}
	fclose(file);
	free(output);
}

void history_save(struct history* h, char* path, char mode){
	FILE* file = fopen(path,&mode);

	for(int i=h->last_appended_history;i<=h->i;i++){
		fprintf(file,"%s\n", h->stack[i]);
	}
	if(mode == 'a'){
		h->last_appended_history = h->i+1;
	}
	fclose(file);
}
