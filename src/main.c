#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
	char *input = (char*)malloc(sizeof(char) * 500);

  // TODO: Uncomment the code below to pass the first stage
  printf("$ ");
	scanf("%s",input);
	printf("%s: command not found\n", input);

  return 0;
}
