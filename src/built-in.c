#include "built-in.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void cd(int argc, char **argv, char** output, char** error) {
	if (argc == 0) {
		*error = "cd: provide path\n" ;
	} else {
		char *path = argv[0];
		if (strcmp(argv[0], "~") == 0) {
			path = getenv("HOME");
		}
		int result = chdir(path);
		if (result != 0) {
			sprintf(*output, "cd: %s: No such file or directory\n", path);
		}
	}
}

void pwd(int argc, char **argv, char** output, char** error){
	char path[PATH_MAX];
	char *val = getcwd(path, PATH_MAX);
	if (val == NULL) {
		*error = "Could not get present working directory\n";
	} else {
		sprintf(*output,"%s\n", path);
	}
}

void type(int argc, char **argv,char cmds[6][10], int no_of_cmds, char** output, char** error){
	int isValid = 0;
	for (int i = 0; i < no_of_cmds; i++) {
		if (strcmp(argv[0], cmds[i]) == 0) {
			sprintf(*output,"%s is a shell builtin\n", cmds[i]);
			isValid = 1;
		}
	}
	if (isValid == 0) {
		char *path = get_inbuilt_cmd_path(argv[0]);
		if (path != NULL) {
			sprintf(*output,"%s is %s\n", argv[0], path);
			isValid = 1;
		}
	}

	if (isValid == 0) {
		sprintf(*error,"%s: not found\n", argv[0]);
	}
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


int history_cmd(int argc, char **argv, char** output,char** error, struct history* history){
	if(argc > 0){
		if(strcmp(argv[0],"-r") == 0){
			if(argc == 1){
				*error = strdup("Enter path\n");
			}else{
				history_load(history, argv[1]);
				return 1;
			}
		}else if(strcmp(argv[0],"-w") == 0 || strcmp(argv[0],"-a") == 0){
			if(argc == 1){
				*error = strdup("Enter path\n");
			}else{
				char mode = argv[0][1];
				history_save(history, argv[1], mode);
				return 1;
			}
		}else{
			int limit = atoi(argv[0]);
			*output = get_history_limit(history, limit);
		}
	}else{
		*output = get_history_all(history);
	}
	return 0;
}

void exec_cmd(int argc, char* cmd,char **argv,enum STATE state, char** output, char** error){
	int count = 0;
	char *path = get_inbuilt_cmd_path(cmd);

	char *new_args[argc + 2];
	new_args[0] = strdup(cmd);
	int i = 0;
	for (i = 0; i < argc; i++) {
		new_args[i + 1] = strdup(argv[i]);
	}
	new_args[argc + 1] = NULL;

	if (path == NULL) {
		sprintf(*error,"%s: command not found\n", cmd);
	} else {
		int stdout_ids[2];
		int stderr_ids[2];

		if(pipe(stdout_ids) == -1 || pipe(stderr_ids) == -1){
			perror("Unable to execute process");
			exit(EXIT_FAILURE);
		}
		int saved_stderr = dup(STDERR_FILENO);
		int saved_stdout = dup(STDOUT_FILENO);
		switch(state){
			case REDIRECT_SUCCESS:
				dup2(stdout_ids[1], STDOUT_FILENO);
				break;
			case REDIRECT_FAILURE:
				dup2(stderr_ids[1], STDERR_FILENO);
				break;
			default:
				break;
		}

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
		dup2(saved_stdout, STDOUT_FILENO);
		dup2(saved_stderr, STDERR_FILENO);
		close(stdout_ids[1]);
		close(stderr_ids[1]);

		if(state == REDIRECT_SUCCESS){
			char buf[100];
			while(1){
				int n = read(stdout_ids[0],buf,100);
				if(n == 0){
					break;
				}
				strncat(*output, buf,n);
			}
			strcat(*output, "\0");
		}

		if(state == REDIRECT_FAILURE){
			char buf[100];
			while(1){
				int n = read(stderr_ids[0],buf,100);
				if(n == 0){
					break;
				}
				strncat(*error, buf,n);
			}
			strcat(*error, "\0");
		}
	}
}

void echo(int argc, char **argv,char **output, char** error){
	for(int i=0;i<argc;i++){
		strcat(*output, argv[i]);
		if(i < argc-1){
			strcat(*output, " ");
		}
	}
	strcat(*output, "\n");
}

