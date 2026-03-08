#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
				printf("%s: not found",input+5);
			}
		}else {
			printf("%s: command not found\n", input);
		}
		free(input);
	}

  return 0;
}
