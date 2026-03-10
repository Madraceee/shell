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
};

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

void echo(char input[]) {
	enum STATE state = NORMAL;
	int len = strlen(input);
	char *buf = (char *)malloc(sizeof(char) * 100);
	buf[0] = '\0';
	for (int i = 0; i < len; i++) {
		if (strncmp(&input[i], "'", 1) == 0) {
			if (state == NORMAL) {
				state = SINGLE;
			} else {
				if (strlen(buf) > 0) {
					printf("%s", buf);
					buf = (char*)malloc(sizeof(char) * 100);
					buf[0] = '\0';
				}
				state = NORMAL;
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

int get_args(char *input, char *args[]) {
	if (input == NULL || strlen(input) == 0) {
		return 0;
	}

	enum STATE state = NORMAL;
	int len = strlen(input);

	// MAX 100 args with each max length of 100
	int no_of_args = 0;
	char *buf = (char *)malloc(sizeof(char) * 100);
	buf[0] = '\0';
	for (int i = 0; i < len; i++) {
		if (strncmp(&input[i], "'", 1) == 0) {
			if (state == NORMAL) {
				state = SINGLE;
			} else {
				if (strlen(buf) > 0) {
					args[no_of_args++] = strdup(buf);
					buf = (char*)malloc(sizeof(char) * 100);
					buf[0] = '\0';
				}
				state = NORMAL;
			}
			continue;
		}
		if (strncmp(&input[i], " ", 1) == 0 && state == NORMAL) {
			if (strlen(buf) > 0) {
				args[no_of_args++] = strdup(buf);
				args[no_of_args++] = strdup(" ");
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

	const int no_of_cmds = 5;
	char cmds[5][5] = {"echo", "exit", "type", "pwd", "cd"};

	setbuf(stdout, NULL);
	while (1) {
		// TODO: Get the cmd and args from input
		char *input = (char *)malloc(sizeof(char) * 500);
		printf("$ ");
		fgets(input, 500, stdin);
		input[strlen(input) - 1] = '\0';
		char *input_ptr = input;
		char *input_copy = strdup(input);

		char *cmd;
		char *args[100];
		cmd = strsep(&input, " ");
		int no_of_args = get_args(input, args);

		if (strcmp(cmd, "exit") == 0) {
			break;
		} else if (strcmp(cmd, "echo") == 0) {
			// TODO: Change input+5 to args
			strsep(&input_copy, " ");
			echo(strdup(input_copy));
			// for (int i = 0; i < no_of_args; i++) {
			// 	printf("%s", args[i]);
			// }
			printf("\n");
		} else if (strcmp(cmd, "pwd") == 0) {
			char path[PATH_MAX];
			char *val = getcwd(path, PATH_MAX);
			if (val == NULL) {
				printf("Could not get present working directory\n");
			} else {
				printf("%s\n", path);
			}

		} else if (strcmp(cmd, "cd") == 0) {
			// TODO: Use args
			if (no_of_args == 0) {
				printf("cd: provide path\n");
			} else {
				char *path = args[0];
				if (strcmp(args[0], "~") == 0) {
					path = getenv("HOME");
				}
				int result = chdir(path);
				if (result != 0) {
					printf("cd: %s: No such file or directory\n", path);
				}
			}
		} else if (strcmp(cmd, "type") == 0) {
			int isValid = 0;
			for (int i = 0; i < no_of_cmds; i++) {
				if (strncmp(input + 5, cmds[i], strlen(cmds[i])) == 0) {
					printf("%s is a shell builtin\n", cmds[i]);
					isValid = 1;
				}
			}
			if (isValid == 0) {
				char *path = get_inbuilt_cmd_path(input + 5);
				if (path != NULL) {
					printf("%s is %s\n", input + 5, path);
					isValid = 1;
				}
			}

			if (isValid == 0) {
				printf("%s: not found\n", input + 5);
			}
		} else {
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
				printf("%s: command not found\n", input);
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
		for(int i=0;i<no_of_args;i++){
			free(args[i]);
		}
		free(input_ptr);
	}

	return 0;
}
