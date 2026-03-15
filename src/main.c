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
#include "built-in.h"

struct termios org_trm;
struct history* history;

char* trim(char *str);
int get_args(char **raw_arg, char *args[], enum STATE *state);

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

int main(int argc, char *argv[]) {
	// Terminal Startup
	termios_startup();
	setbuf(stdout, NULL);
	
	// Command Completion
	struct trie* cmd_completion = new_trie();
	load(cmd_completion);

	const int no_of_cmds = 6;
	char **cmds = (char**)malloc(sizeof(char*)*no_of_cmds);
	cmds[0] = "echo";
	cmds[1] = "exit";
	cmds[2] = "type";
	cmds[3] = "pwd";
	cmds[4] = "cd";
	cmds[5] = "history";
	// char *cmds[] = {"echo", "exit", "type", "pwd", "cd","history"};
	for(int i = 0;i<no_of_cmds;i++){
		load_word(cmd_completion, cmds[i], 0);
	}
	cmd_completion->total_inputs += no_of_cmds;


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
			if(chr == '\t'){
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
							if(i>input_count){
								strncpy(input, completions[0], i);
								input[i] = '\0';
								input_count = i;
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
		}else if (strcmp(cmd, "echo") == 0) {
			echo(no_of_args, args, &output);
		} else if (strcmp(cmd, "pwd") == 0) {
			pwd(no_of_args, args, &output);
		} else if (strcmp(cmd, "cd") == 0) {
			cd(no_of_args, args, &output);
		} else if (strcmp(cmd, "type") == 0) {
			type(no_of_args, args, cmds, no_of_cmds,&output);
		} else if (strcmp(cmd, "history") == 0){
			if(history_cmd(no_of_args, args,&output, history) == 1){
				continue;
			}
		}else {
			exec_cmd(no_of_args, cmd, args, &output);
		}
		if(*state == REDIRECT){
			if(strlen(output) != 0){
				strsep(&input, ">");
				input = trim(input);
				FILE *file = fopen(input, "w+");
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
		free(input_copy);
		free(input_ptr);
		free(state);
	}

	return 0;
}


char* trim(char *str){
	int i = 0;
	int len = strlen(str);

	while(i < len && (isblank(str[i])!=0 )){
		i++;
	}

	if(i == len){
		*str = '\0';
		return str;
	}

	memmove(str, str+i, len-i+1);
	return str;
}

int get_args(char **raw_arg, char *args[], enum STATE *state) {
	char *input = *raw_arg;
	if (input == NULL || strlen(input) == 0) {
		return 0;
	}

	*state = NORMAL;
	int len = strlen(input);
	input = trim(input);

	char *output = (char*)malloc(sizeof(char) * (PATH_MAX+50));
	char output_count = 0;
	for (int i = 0; i < len; i++) {
		if (strncmp(&input[i], ">", 1) == 0 || strncmp(&input[i], "1>", 2) == 0){
			*state = REDIRECT;
			*raw_arg = &(*raw_arg)[i];
			break;
		}
		if(input[i] == '\''){
			if(input[i-1] == '\\'){
				output_count -= 1;
			}else{
				if(*state == NORMAL){
					*state = SINGLE;
					continue;
				}else if(*state == SINGLE) {
					*state = NORMAL;
					continue;
				}
			}
		}else if(input[i] == '\"'){
			if(input[i-1] == '\\' && *state != SINGLE){
				output_count -= 1;
			}else{
				if(*state == NORMAL){
					*state = DOUBLE;
					continue;
				}else if(*state == DOUBLE){
					*state = NORMAL;
					continue;
				}
			}
		}else if(input[i] == ' ' ){
			if(input[i-1] == '\\' && *state != SINGLE){
				input[i] = '\a';
				output_count -= 1;
				output[output_count++] = '\a';
				continue;
			}
			if(*state == NORMAL){
				if(input[i-1] == ' '){
					continue;
				}
			}else {
				output[output_count++] = '\a';
				continue;
			}
		}else if(input[i-1] == '\\'){
			if(input[i] != '\\' && *state != SINGLE){
				output_count -= 1;
			}
		}

		output[output_count++] = input[i];
	
	}
	output[output_count] = '\0';

	int no_of_args = 0;
	while(output != NULL){
		char *word = strsep(&output, " ");
		args[no_of_args++] = strdup(word);
	}

	for(int i=0;i<no_of_args;i++){
		int j=0;
		for(;args[i][j] != '\0';j++){
			if(args[i][j] == '\a'){
				args[i][j] = ' ';
			}
		}
	}

	return no_of_args;
}
