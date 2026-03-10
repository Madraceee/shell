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

enum STATE {
	NORMAL,
	SINGLE,
	REDIRECT,
};

struct history{
	char *stack[100];
	int i;
	int max;
};

char* trim(char *str);

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
	// printf("No of args:%d\n", no_of_args);
	// for(int i=0;i<no_of_args;i++){
	// 	printf("%s\n", args[i]);
	// }
	return no_of_args;
}

int main(int argc, char *argv[]) {
	// Flush after every printf

	const int no_of_cmds = 6;
	char cmds[6][8] = {"echo", "exit", "type", "pwd", "cd","history"};

	setbuf(stdout, NULL);

	// History
	struct history history;
	history.i = 0;
	history.max = 100;

	while (1) {
		// TODO: Get the cmd and args from input
		enum STATE *state = (enum STATE*)malloc(sizeof(enum STATE)*1);
		char *input = (char *)malloc(sizeof(char) * 500);
		char *output = (char*)malloc(sizeof(char) * (PATH_MAX+50));
		output[0] = '\0';
		printf("$ ");
		fgets(input, 500, stdin);
		input[strlen(input) - 1] = '\0';
		char *input_ptr = input;
		char *input_copy = strdup(input);

		char *cmd;
		char *args[100];
		cmd = strsep(&input, " ");

		*state = NORMAL;
		int no_of_args = get_args(&input, args, state);

		history.stack[history.i++] = strdup(input_copy);

		if (strcmp(cmd, "exit") == 0) {
			break;
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
			for(int i=0;i<history.i;i++){
				char *line = (char*)malloc(sizeof(char)*strlen(history.stack[i]));
				sprintf(line,"%d %s\n", i+1, history.stack[i]);
				strcat(output, line);
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
