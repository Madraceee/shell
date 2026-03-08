#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
	
	char* path_value = getenv("PATH");
	if(path_value == NULL){
		return 1;
	}
	int no_of_path = 0;
	for(int i=0; path_value[i] != '\0';i++){
		if(path_value[i] == ':'){
			no_of_path += 1;
		}
	}

	char *paths[no_of_path];
	int count = 0;
	while(path_value != NULL){
		char* path = strsep(&path_value, ":");
		paths[count] = (char*)malloc(sizeof(char)* strlen(path));
		strcpy(paths[count], path);
		count += 1;
	}

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
				for(int i=0;i<no_of_path;i++){
					DIR *dir = opendir(paths[i]);
					struct dirent *ent;
					while((ent = readdir(dir))){
						if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
							continue;
						}

						char full_path[PATH_MAX];
						snprintf(full_path, PATH_MAX, "%s/%s", paths[i], ent->d_name);
						if(strcmp(input+5, ent->d_name) == 0 && access(full_path, X_OK) == 0){
							printf("%s is %s\n", input+5, full_path);
							isValid = 1;
							break;
						}
					}

					if(isValid == 1){
						break;
					}
				}
			}

			if(isValid == 0){
				printf("%s: not found\n",input+5);
			}
		}else {
			printf("%s: command not found\n", input);
		}
		free(input);
	}

  return 0;
}
