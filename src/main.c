#include "assert.h"
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

void process(char *cmds[], int index, int max_cmds);
int process_cmd(char *input);

const int no_of_inbuilt_cmds = 6;
char inbuilt_cmds[6][10] = {"echo", "exit", "type", "pwd", "cd", "history"};

int main(int argc, char *argv[]) {
	// Terminal Startup
	termios_startup();
	setbuf(stdout, NULL);
	
	// Command Completion
	struct trie* cmd_completion = new_trie();
	load(cmd_completion);

	for(int i = 0;i<no_of_inbuilt_cmds;i++){
		load_word(cmd_completion, inbuilt_cmds[i], 0);
	}
	cmd_completion->total_inputs += no_of_inbuilt_cmds;

	// History
	history = new_history(100);
	atexit(history_cleanup);

	while (1) {
		enum STATE state = NORMAL;
		char *input = (char *)malloc(sizeof(char) * (PATH_MAX + 500));
		int is_tab_pressed = 0;

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
				input = handle_tab(input, &input_count,&is_tab_pressed, cmd_completion);
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
		int saved_stdin = dup(STDIN_FILENO);
		int saved_stdout = dup(STDOUT_FILENO);
		int saved_stderr = dup(STDERR_FILENO);

		int no_of_cmds = 0;
		char *cmds[PATH_MAX * 2];
		while(input != NULL){
			cmds[no_of_cmds++] = strsep(&input, "|");
		}
		process(cmds, 0, no_of_cmds);
		assert(dup2(saved_stdin, STDIN_FILENO) != -1);
		assert(dup2(saved_stdout, STDOUT_FILENO) != -1);
		assert(dup2(saved_stderr, STDERR_FILENO) != -1);
		close(saved_stdin);
		close(saved_stdout);
		close(saved_stderr);
		free(input);
	}
	
	return 0;
}

void process(char *cmds[], int index, int max_cmds) {
	if(index+1< max_cmds){
		int fds[2];
		if(pipe(fds) == -1){
			perror("Error: pipe()");
			exit(EXIT_FAILURE);
		}

		pid_t pid = fork();
		if(pid == 0){
			close(fds[0]);
			dup2(fds[1], STDOUT_FILENO);
			process_cmd(cmds[index]);
			close(fds[1]);
			_exit(EXIT_SUCCESS);
		}else{
			close(fds[1]);
			dup2(fds[0],STDIN_FILENO);
			process(cmds, index+1, max_cmds);
			close(fds[0]);
			waitpid(pid, NULL, 0);
		}
	}else{
		process_cmd(cmds[index]);
	}
}


int process_cmd(char *input){
	char *output = (char*)malloc(sizeof(char) * (PATH_MAX*500));
	char *error = (char*)malloc(sizeof(char) * (PATH_MAX*500));
	output[0] = '\0';
	error[0] = '\0';

	char *input_ptr = input;
	char *input_copy = strdup(input);

	char *cmd;
	char *args[NAME_MAX];

	enum STATE state = NORMAL;
	int no_of_args = get_cmd_and_args(&input, &cmd, args, &state);
	if(no_of_args > 0 && strlen(args[no_of_args-1]) == 0){
		no_of_args = 0;
	}
	insert_record(history, strdup(input_copy));

	if (strcmp(cmd, "exit") == 0) {
		 exit(0);
	}else if (strcmp(cmd, "echo") == 0) {
		echo(no_of_args, args, &output, &error);
	} else if (strcmp(cmd, "pwd") == 0) {
		pwd(no_of_args, args, &output, &error);
	} else if (strcmp(cmd, "cd") == 0) {
		cd(no_of_args, args, &output, &error);
	} else if (strcmp(cmd, "type") == 0) {
		type(no_of_args, args, inbuilt_cmds, no_of_inbuilt_cmds,&output, &error);
	} else if (strcmp(cmd, "history") == 0){
		history_cmd(no_of_args, args,&output, &error, history);
	}else {
		exec_cmd(no_of_args, cmd, args, state, &output, &error);
	}

	if(state == REDIRECT_SUCCESS){
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
	}else if(state == REDIRECT_FAILURE){
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

	if(state != REDIRECT_SUCCESS){
		printf("%s", output);
	}
	if(state != REDIRECT_FAILURE){
		printf("%s", error);
	}
	for(int i=0;i<no_of_args;i++){
		free(args[i]);
	}

	free(input_copy);
	free(output);
	free(error);
	return 0;
}
