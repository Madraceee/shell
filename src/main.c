#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
	while(1){
		char *input = (char*)malloc(sizeof(char) * 500);
		// TODO: Uncomment the code below to pass the first stage
		printf("$ ");
		scanf("%s",input);
		if(strcmp(input,"exit") == 0){
			break;
		}
		printf("%s: command not found\n", input);
	}

  return 0;
}
