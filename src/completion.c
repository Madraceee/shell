#include "completion.h"
#include "trie.h"
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* complete_cmd(char *input,struct trie *cmd_list, int *tab_pressed);
char* complete_args(char *input, int *tab_pressed,char *cmd, char *args[100], int no_of_args);

char* handle_tab(char *input, int *input_count, char **output, int *is_tab_pressed,struct trie *cmd_completion_list){
	if(*input_count <= 0){
		return input;
	}
	char **completions;
	input[*input_count] = '\0';
	char *original_input = strdup(input);

	char *cmd;
	char *args[100];
	enum STATE state;
	char *input_copy = strdup(input);
	int no_of_args = get_cmd_and_args(&input_copy, &cmd, args, &state);

	int no_of_completions = 0;

	printf("\r\033[2K$ ");
	if(no_of_args == 0){
		input = complete_cmd(input, cmd_completion_list, is_tab_pressed);
		*input_count = strlen(input);
	}else{
		input = complete_args(input, is_tab_pressed, cmd, args, no_of_args);
		*input_count = strlen(input);
	}
	printf("%s", input);
	printf("\a");
	return input;
}

char* complete_cmd(char *input,struct trie *cmd_list, int *tab_pressed){
	char *original_input = strdup(input);
	char **completions;
	int no_of_completions = get_completion(cmd_list, input, &completions);
	if(no_of_completions == 0){
		free(completions);
		return input;
	}

	input[0] = '\0';
	if(no_of_completions == 1){
		strcat(input, completions[0]);
		strcat(input, " \0");
	}else{
		if(*tab_pressed == 0){
			*tab_pressed += 1;
			int is_all_matching = 1;
			int i=0;
			while(1){
				if(i >= strlen(completions[0])){
					break;
				}
				char c = completions[0][i];
				for(int j=0;j<no_of_completions;j++){
					if(i >= strlen(completions[j]) || completions[j][i] != c){
						is_all_matching = 0;
						break;
					}
				}
				if(is_all_matching == 0){
					break;
				}
				i++;
			}
			strncat(input,completions[0], i);
			strcat(input, "\0");
		}else{
			printf("%s\n", original_input);
			for(int i=0;i<no_of_completions;i++){
				printf("%s  ", completions[i]);
			}
			printf("\n");
			printf("$ ");
			strcpy(input, original_input);
			*tab_pressed = 0;
		}
	}

	free(completions);
	return input;
}

char* complete_args(char *input, int *tab_pressed,char *cmd, char *args[100], int no_of_args){
	char **completions;
	char *original_input = strdup(input);
	char path[PATH_MAX];
	char *val = getcwd(path, PATH_MAX);
	if(val == NULL){
		return input;
	}
	
	char *last_arg = strdup(args[no_of_args-1]);
	char *relative_path = (char*)malloc(sizeof(char) *(PATH_MAX));
	relative_path[0] = '\0';
	while(last_arg != NULL){
		char *folder = strsep(&last_arg, "/");
		if(folder[0] == '\0'){
			break;
		}
		if(last_arg == NULL){
			break;
		}
		strcat(relative_path,"/");
		strcat(relative_path,folder);
	}
	if(relative_path[0] != '\0'){
		strcat(path, relative_path);
	}
	free(last_arg);

	DIR* dir = opendir(path);
	if(dir == NULL){
		return input;
	}
	struct dirent* ent;
	
	struct trie *t = new_trie();
	int completions_count = 0;
	while((ent = readdir(dir)) != NULL){
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
			continue;
		}
		if(ent ->d_type == DT_DIR){
			char *folder = (char*)malloc(sizeof(char) * NAME_MAX);
			folder[0] = '\0';
			strcat(folder, ent->d_name);
			strcat(folder, "/");
			load_word(t, folder, 0);
			free(folder);
		}else if(ent->d_type == DT_REG){
			load_word(t, ent->d_name, 0);
		}
	}
	closedir(dir);

	completions = (char**)malloc(sizeof(char*) * 999);

	last_arg = strdup(args[no_of_args-1]);
	char *file = (char*)malloc(sizeof(char) * PATH_MAX);
	while(last_arg != NULL){
		file = strsep(&last_arg, "/");
	}
	free(last_arg);

	int no_of_completions = get_completion(t, file, &completions);

	if(no_of_completions == 0){
		printf("\a");
		free(completions);
		return input;
	}


	int i=0;
	int relative_path_len = strlen(relative_path);
	for(;i<relative_path_len;i++){
		relative_path[i] = relative_path[i+1];
	}
	relative_path[i] = '\0';

	input[0] = '\0';
	sprintf(input, "%s ", cmd);
	if(no_of_completions == 1){
		if(strlen(relative_path) > 0){
			strcat(input, relative_path);
			strcat(input, "/");
		}
		strcat(input, completions[0]);
		if(input[strlen(input)-1] != '/'){
			strcat(input, " ");
		}
		strcat(input, "\0");
	}else{
		if(*tab_pressed == 0){
			*tab_pressed += 1;
			int is_all_matching = 1;
			int i=0;
			while(1){
				if(i >= strlen(completions[0])){
					break;
				}
				char c = completions[0][i];
				for(int j=0;j<no_of_completions;j++){
					if(i >= strlen(completions[j]) || completions[j][i] != c){
						is_all_matching = 0;
						break;
					}
				}
				if(is_all_matching == 0){
					break;
				}
				i++;
			}
			if(strlen(relative_path) > 0){
				strcat(input, relative_path);
				strcat(input, "/");
			}
			strncat(input,completions[0], i);
			strcat(input, "\0");
		}else{
			printf("%s\n", original_input);
			for(int i=0;i<no_of_completions;i++){
				printf("%s  ", completions[i]);
			}
			printf("\n");
			printf("$ ");
			strcpy(input, original_input);
			*tab_pressed = 0;
		}
	}

	free(completions);
	return input;
}

// void get_files_completion(struct trie *t,char *path){
// 	DIR* dir = opendir(path);
// 	if(dir == NULL){
// 		printf("\nError fetching files\n");
// 	}
// 	struct dirent* ent;
//
// 	int completions_count = 0;
// 	while((ent = readdir(dir)) != NULL){
// 		if(ent ->d_type == DT_DIR){
// 				get_files_completion(t, );
// 		}
// 		if(ent->d_type == DT_REG){
// 			load_word(t, ent->d_name, 0);
// 		}
// 	}
// 	closedir(dir);
// }
