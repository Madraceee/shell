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
#include "history.h"
#include "trie.h"

struct termios org_trm;
struct history* history;

enum STATE {
	NORMAL,
	SINGLE,
	REDIRECT,
};

char* trim(char *str);

void termios_cleanup(){
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &org_trm);
}

void history_cleanup(){
	history_save(history, history->history_file_path, 'a');
}

void termios_startup(){
	tcgetattr(STDIN_FILENO, &org_trm);
	atexit(termios_cleanup);

	struct termios raw;
	tcgetattr(STDIN_FILENO, &raw);
	raw.c_lflag &= ~( ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


char *get_inbuilt_cmd_path(char *input) {
	char *path_value = getenv("PATH");
	if (path_value == NULL) {
		return NULL;
	}
	int no_of_path = 0;
	for (int i = 0; path_value[i] != '\0'; i++) {
		if (path_value[i] == ':') {
			no_of_path += 1;
		}
	}
	char *paths[no_of_path];

	int count = 0;
	char *path_value_copy = malloc((strlen(path_value) + 1) * sizeof(char *));
	strcpy(path_value_copy, path_value);
	while (path_value_copy != NULL) {
		char *path = strsep(&path_value_copy, ":");
		paths[count] = (char *)malloc(sizeof(char) * strlen(path));
		strcpy(paths[count], path);
		count += 1;
	}
	free(path_value_copy);

	for (int i = 0; i < no_of_path; i++) {
		DIR *dir = opendir(paths[i]);
		struct dirent *ent;
		while ((ent = readdir(dir))) {
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
				continue;
			}

			char *full_path = (char *)malloc(PATH_MAX * sizeof(char));
			snprintf(full_path, PATH_MAX, "%s/%s", paths[i], ent->d_name);
			if (strcmp(input, ent->d_name) == 0 && access(full_path, X_OK) == 0) {
				closedir(dir);
				return full_path;
			}
		}
		closedir(dir);
	}
	return NULL;
}

void echo(char **raw_arg, enum STATE *state) {
	char *input = *raw_arg;
	*state = NORMAL;
	int len = strlen(input);
	char *buf = (char *)malloc(sizeof(char) * 100);
	buf[0] = '\0';
	for (int i = 0; i < len; i++) {
		if (strncmp(&input[i], ">", 1) == 0 || strncmp(&input[i], "1>", 2) == 0){
			*state = REDIRECT;
			*raw_arg = &(*raw_arg)[i];
			break;
		}
		if (strncmp(&input[i], "'", 1) == 0) {
			if (state == NORMAL) {
				*state = SINGLE;
			} else {
				if (strlen(buf) > 0) {
					printf("%s", buf);
					buf = (char*)malloc(sizeof(char) * 100);
					buf[0] = '\0';
				}
				*state = NORMAL;
			}
			continue;
		}
		if (strncmp(&input[i], " ", 1) == 0 && state == NORMAL) {
			if (strlen(buf) > 0) {
				printf("%s", buf);
				buf = (char*)malloc(sizeof(char) * 100);
				buf[0] = '\0';
			}
			if(strncmp(&input[i-1]," ", 1) != 0){
				printf(" ");
			}
			continue;
		}
		buf = strncat(buf, &input[i], 1);
	}
	printf("%s", buf);
}

int get_args(char **raw_arg, char *args[], enum STATE *state) {
	char *input = *raw_arg;
	if (input == NULL || strlen(input) == 0) {
		return 0;
	}

	*state = NORMAL;
	int len = strlen(input);

	// MAX 100 args with each max length of 100
	int no_of_args = 0;
	char *buf = (char *)malloc(sizeof(char) * 100);
	buf[0] = '\0';
	for (int i = 0; i < len; i++) {
		if (strncmp(&input[i], ">", 1) == 0 || strncmp(&input[i], "1>", 2) == 0){
			*state = REDIRECT;
			*raw_arg = &(*raw_arg)[i];
			break;
		}
		if (strncmp(&input[i], "'", 1) == 0) {
			if (*state == NORMAL) {
				*state = SINGLE;
			} else {
				if (strlen(buf) > 0) {
					args[no_of_args++] = strdup(buf);
					buf = (char*)malloc(sizeof(char) * 100);
					buf[0] = '\0';
				}
				*state = NORMAL;
			}
			continue;
		}
		if (strncmp(&input[i], " ", 1) == 0 && *state == NORMAL) {
			if (strlen(buf) > 0) {
				args[no_of_args++] = strdup(buf);
				buf = (char*)malloc(sizeof(char) * 100);
				buf[0] = '\0';
			}
			continue;
		}
		buf = strncat(buf, &input[i], 1);
	}

	if (strlen(buf) > 0) {
		args[no_of_args++] = strdup(buf);
	}
	return no_of_args;
}

int main(int argc, char *argv[]) {
	termios_startup();
	
	// Command Completion
	struct trie* cmd_completion = new_trie();
	load(cmd_completion);

	const int no_of_cmds = 6;
	char cmds[6][15] = {"echo", "exit", "type", "pwd", "cd","history"};
	for(int i = 0;i<no_of_cmds;i++){
		load_word(cmd_completion, cmds[i], 0);
	}
	cmd_completion->total_inputs += no_of_cmds;

	setbuf(stdout, NULL);

	// History
	history = new_history(100);
	atexit(history_cleanup);

	while (1) {
		enum STATE *state = (enum STATE*)malloc(sizeof(enum STATE)*1);
		char *input = (char *)malloc(sizeof(char) * 500);
		char *output = (char*)malloc(sizeof(char) * (PATH_MAX+50));
		int is_tab_pressed = 0;
		output[0] = '\0';

		printf("$ ");
		int input_count = 0;
		while(1){
			char chr;
			read(STDIN_FILENO, &chr, 1);
			if(chr == '\n'){
				putc('\n', stdout);
				break;
			}
			if(chr == 127){
				if (input_count > 0) {
					printf("\b \b");
					fflush(stdout);
					input_count--;
				}
				continue;
			}
			if(chr == 9){
				if(input_count > 0){
					char **completions;
					input[input_count] = '\0';
					int no_of_completions =  get_completion(cmd_completion, input, &completions);

					if(no_of_completions == 1){
						input = completions[0];
						strcat(input, " ");
						input_count = strlen(input);
						printf("\r\033[2K$ %s", input);
					}else if(no_of_completions != 0){
						printf("\a");
						if(is_tab_pressed == 0){
							is_tab_pressed = 1;

							int is_all_matching = 1;
							int i=0;
							while(1){
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
							if(i>input_count){
								strncpy(input, completions[0], i);
								input_count = strlen(input);
								printf("\n$ %s",input);
								fflush(stdout);
								is_tab_pressed = 0;
							}
						}else{
							printf("\n");
							for(int i=0;i<no_of_completions;i++){
								printf("%s  ", completions[i]);
							}
							printf("\n$ %s",input);
							fflush(stdout);
							is_tab_pressed = 0;
						}
					}else{
						printf("\a");
					}

					free(completions);
				}
				continue;
			}
			if(chr == '['){
				read(STDIN_FILENO, &chr, 1);
				if(chr == 'A'){
					history_up(history, input);
					input_count = strlen(input);
					continue;
				}else if(chr == 'B'){
					history_down(history, input);
					input_count = strlen(input);
					continue;
				}
			}
			input[input_count++] = chr;
			is_tab_pressed = 0;
			putc(chr, stdout);
		}
		input[input_count] = '\0';
		char *input_ptr = input;
		char *input_copy = strdup(input);

		char *cmd;
		char *args[100];
		cmd = strsep(&input, " ");

		*state = NORMAL;
		int no_of_args = get_args(&input, args, state);

		insert_record(history, strdup(input_copy));

		if (strcmp(cmd, "exit") == 0) {
			return 0;
		// }else if (strcmp(cmd, "echo") == 0) {
		// 	// TODO: Change input+5 to args
		// 	strsep(&input_copy, " ");
		// 	echo(&strdup(input_copy),state);
		// 	// for (int i = 0; i < no_of_args; i++) {
		// 	// 	printf("%s", args[i]);
		// 	// }
		// 	printf("\n");
		} else if (strcmp(cmd, "pwd") == 0) {
			char path[PATH_MAX];
			char *val = getcwd(path, PATH_MAX);
			if (val == NULL) {
				output = "Could not get present working directory\n";
			} else {
				sprintf(output,"%s\n", path);
			}

		} else if (strcmp(cmd, "cd") == 0) {
			if (no_of_args == 0) {
				output = "cd: provide path\n" ;
			} else {
				char *path = args[0];
				if (strcmp(args[0], "~") == 0) {
					path = getenv("HOME");
				}
				int result = chdir(path);
				if (result != 0) {
					sprintf(output, "cd: %s: No such file or directory\n", path);
				}
			}
		} else if (strcmp(cmd, "type") == 0) {
			int isValid = 0;
			for (int i = 0; i < no_of_cmds; i++) {
				if (strncmp(args[0], cmds[i], strlen(cmds[i])) == 0) {
					sprintf(output,"%s is a shell builtin\n", cmds[i]);
					isValid = 1;
				}
			}
			if (isValid == 0) {
				char *path = get_inbuilt_cmd_path(args[0]);
				if (path != NULL) {
					sprintf(output,"%s is %s\n", args[0], path);
					isValid = 1;
				}
			}

			if (isValid == 0) {
				sprintf(output,"%s: not found\n", args[0]);
			}
		} else if (strcmp(cmd, "history") == 0){
			free(output);
			if(no_of_args > 0){
				if(strcmp(args[0],"-r") == 0){
					if(no_of_args == 1){
						output = strdup("Enter path\n");
					}else{
						history_load(history, args[1]);
						continue;
					}
				}else if(strcmp(args[0],"-w") == 0 || strcmp(args[0],"-a") == 0){
					if(no_of_args == 1){
						output = strdup("Enter path\n");
					}else{
						char mode = args[0][1];
						history_save(history, args[1], mode);
						continue;
					}
				}else{
					int limit = atoi(args[0]);
					output = get_history_limit(history, limit);
				}
			}else{
				output = get_history_all(history);
			}
		}else {
			int count = 0;
			char *path = get_inbuilt_cmd_path(cmd);

			char *new_args[no_of_args + 2];
			new_args[0] = strdup(cmd);
			int i = 0;
			for (i = 0; i < no_of_args; i++) {
				new_args[i + 1] = strdup(args[i]);
			}
			new_args[no_of_args + 1] = NULL;

			if (path == NULL) {
				sprintf(output,"%s: command not found\n", cmd);
			} else {
				pid_t pid = fork();
				if (pid == -1) {
					perror("Unable to execute process");
					exit(EXIT_FAILURE);
				} else if (pid == 0) {
					execv(path, new_args);
					exit(EXIT_SUCCESS);
				} else {
					waitpid(pid, NULL, 0);
				}
			}
		}
		if(*state == REDIRECT){
			if(strlen(output) != 0){
				strsep(&input, ">");
				input = trim(input);
				FILE *file = fopen(input, "w+");
				printf("%s\n",output);
				fprintf(file, "%s", output);
				fclose(file);
			}
		}else{
			printf("%s", output);
		}

		for(int i=0;i<no_of_args;i++){
			free(args[i]);
		}
		free(output);
		free(input_ptr);
		free(state);
	}

	return 0;
}


char* trim(char *str){
	int i = 0;
	int len = strlen(str);

	while(i < len && (isspace(str[i]) || strncmp(&str[i], "\"",1)==0 )){
		i++;
	}

	if(i == len){
		*str = '\0';
		return str;
	}

	memmove(str, str+i, len-i+1);
	return str;
}
