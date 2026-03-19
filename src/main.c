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
#include "util.h"
#include "completion.h"

struct termios org_trm;
struct history* history;
struct trie* cmd_completion;

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

// TODO: REDUCE HEAP USAGE
int main(int argc, char *argv[]) {
	// Terminal Startup
	termios_startup();
	setbuf(stdout, NULL);
	
	// Command Completion
	cmd_completion = new_trie();
	load(cmd_completion);

	const int no_of_cmds = 6;
	char **cmds = (char**)malloc(sizeof(char*)*no_of_cmds);
	cmds[0] = "echo";
	cmds[1] = "exit";
	cmds[2] = "type";
	cmds[3] = "pwd";
	cmds[4] = "cd";
	cmds[5] = "history";
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
		char *error = (char*)malloc(sizeof(char) * (PATH_MAX+50));
		int is_tab_pressed = 0;
		output[0] = '\0';
		error[0] = '\0';

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
				input = handle_tab(input, &input_count, &output, &is_tab_pressed, cmd_completion);
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

		*state = NORMAL;
		int no_of_args = get_cmd_and_args(&input, &cmd, args, state);

		insert_record(history, strdup(input_copy));
		if (strcmp(cmd, "exit") == 0) {
			return 0;
		}else if (strcmp(cmd, "echo") == 0) {
			echo(no_of_args, args, &output, &error);
		} else if (strcmp(cmd, "pwd") == 0) {
			pwd(no_of_args, args, &output, &error);
		} else if (strcmp(cmd, "cd") == 0) {
			cd(no_of_args, args, &output, &error);
		} else if (strcmp(cmd, "type") == 0) {
			type(no_of_args, args, cmds, no_of_cmds,&output, &error);
		} else if (strcmp(cmd, "history") == 0){
			if(history_cmd(no_of_args, args,&output, &error, history) == 1){
				continue;
			}
		}else {
			exec_cmd(no_of_args, cmd, args, *state, &output, &error);
		}
		if(*state == REDIRECT_SUCCESS){
			strsep(&input, ">");
			char *mode = "w+";
			if(input[0] == '>'){
				strsep(&input, ">");
				mode = "a+";
			}
			input = trim(input);
			FILE *file = fopen(input, mode);
			fprintf(file, "%s", output);
			fclose(file);
		}else if(*state == REDIRECT_FAILURE){
			strsep(&input, ">");
			char *mode = "w+";
			if(input[0] == '>'){
				strsep(&input, ">");
				mode = "a+";
			}
			input = trim(input);
			FILE *file = fopen(input, mode);
			fprintf(file, "%s", error);
			fclose(file);
		}

		if(*state != REDIRECT_SUCCESS){
			printf("%s", output);
		}
		if(*state != REDIRECT_FAILURE){
			printf("%s", error);
		}
		

		for(int i=0;i<no_of_args;i++){
			free(args[i]);
		}
		free(output);
		free(error);
		free(input_copy);
		free(input_ptr);
		free(state);
	}
	
	free(cmds);

	return 0;
}


