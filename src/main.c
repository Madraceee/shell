#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>

char* get_inbuilt_cmd_path(char *input){
	char* path_value = getenv("PATH");
	if(path_value == NULL){
		return NULL;
	}
	int no_of_path = 0;
	for(int i=0; path_value[i] != '\0';i++){
		if(path_value[i] == ':'){
			no_of_path += 1;
		}
	}
	char *paths[no_of_path];

	int count = 0;
	char *path_value_copy = malloc((strlen(path_value)+1) * sizeof(char*));
	strcpy(path_value_copy, path_value);
	while(path_value_copy != NULL){
		char* path = strsep(&path_value_copy, ":");
		paths[count] = (char*)malloc(sizeof(char)* strlen(path));
		strcpy(paths[count], path);
		count += 1;
	}
	free(path_value_copy);

	for(int i=0;i<no_of_path;i++){
		DIR *dir = opendir(paths[i]);
		struct dirent *ent;
		while((ent = readdir(dir))){
			if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
				continue;
			}

			char *full_path = (char*)malloc(PATH_MAX* sizeof(char));
			snprintf(full_path, PATH_MAX, "%s/%s", paths[i], ent->d_name);
			if(strcmp(input, ent->d_name) == 0 && access(full_path, X_OK) == 0){
				closedir(dir);
				return full_path;
			}
		}
		closedir(dir);
	}
	return NULL;
}

int main(int argc, char *argv[]) {
  // Flush after every printf
	

	char cmds[3][5] = {
		"echo",
		"exit",
		"type"
	};
	
  setbuf(stdout, NULL);
	while(1){
		char* input = (char*)malloc(sizeof(char) * 500);
		// TODO: Uncomment the code below to pass the first stage
		printf("$ ");
		fgets(input, 500, stdin);
		input[strlen(input)-1] = '\0';
		if(strcmp(input,"exit") == 0){
			break;
		}else if(strncmp(input,"echo ",5) == 0){
			printf("%s\n", input + 5);
		}else if (strncmp(input,"type ",5) == 0) {
			int isValid = 0;
			for(int i=0;i<3;i++){
				if(strncmp(input+5, cmds[i], strlen(cmds[i]) ) == 0) {
					printf("%s is a shell builtin\n", cmds[i]);
					isValid = 1;
				}
			}
			if(isValid == 0){
				char *path = get_inbuilt_cmd_path(input+5);
				if(path != NULL){
					printf("%s is %s\n", input+5, path);
					isValid = 1;
				}
			}

			if(isValid == 0){
				printf("%s: not found\n",input+5);
			}
		}else {
			int count = 0;
			char *cmd;
			char *args[10];
			char *input_copy = strdup(input);

			while(input_copy != NULL){
				char *value = strsep(&input_copy, " ");
				if(value != NULL){
					if(count == 0){
						cmd = strdup(value);
					}
					args[count] = strdup(value);
				}
				count += 1;
			}
			free(input_copy);
			args[count] = NULL;

			char *path = get_inbuilt_cmd_path(cmd);
			if(path == NULL){
				printf("%s: command not found\n", input);
			}else {
				pid_t pid = fork();
				if(pid == -1){
					perror("Unable to execute process");
					exit(EXIT_FAILURE);
				}else if(pid == 0){
					execv(path, args);
					exit(EXIT_SUCCESS);
				}else {
					waitpid(pid, NULL ,0 );
				}
			}
		}
		free(input);
	}

  return 0;
}
