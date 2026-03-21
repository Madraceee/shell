#include "util.h"

char* trim(char *str){
	int i = 0;
	int len = strlen(str);

	while(i < len && (isblank(str[i])!=0 )){
		i++;
	}

	if(i == len){
		*str = '\0';
		return str;
	}

	memmove(str, str+i, len-i+1);

	i = strlen(str);

	while(i > 0 && isblank(str[i]) != 0){
		i--;
	}

	str[i+1] = '\0';
	return str;
}

int get_cmd_and_args(char **raw_arg, char **cmd, char *args[], enum STATE *state) {
	char *input = *raw_arg;
	if (input == NULL || strlen(input) == 0) {
		return 0;
	}

	*state = NORMAL;
	int len = strlen(input);
	input = trim(input);

	char *output = (char*)malloc(sizeof(char) * (PATH_MAX+50));
	char output_count = 0;
	for (int i = 0; i < len; i++) {
		if (input[i-1] == ' ' && (strncmp(&input[i], ">", 1) == 0 || strncmp(&input[i], "1>", 2) == 0 || strncmp(&input[i], "2>",2) == 0)){
			output_count -= 1;
			*raw_arg = &(*raw_arg)[i];
			if(strncmp(&input[i], "2>",2) == 0){
				*state = REDIRECT_FAILURE;
			}else{
				*state = REDIRECT_SUCCESS;
			}
			break;
		}
		if(input[i] == '|'){
			*raw_arg = &(*raw_arg)[i];
			*state = PIPE;
			break;
		}
		if(input[i] == '\''){
			if(input[i-1] == '\\' && *state != SINGLE){
				output_count -= 1;
			}else{
				if(*state == NORMAL){
					*state = SINGLE;
					continue;
				}else if(*state == SINGLE) {
					*state = NORMAL;
					continue;
				}
			}
		}else if(input[i] == '\"'){
			if(input[i-1] == '\\' && *state != SINGLE){
				output_count -= 1;
			}else{
				if(*state == NORMAL){
					*state = DOUBLE;
					continue;
				}else if(*state == DOUBLE){
					*state = NORMAL;
					continue;
				}
			}
		}else if(input[i] == ' ' ){
			if(input[i-1] == '\\' && *state != SINGLE){
				input[i] = '\a';
				output_count -= 1;
				output[output_count++] = '\a';
				continue;
			}
			if(*state == NORMAL){
				if(input[i-1] == ' '){
					continue;
				}
			}else {
				output[output_count++] = '\a';
				continue;
			}
		}else if(input[i-1] == '\\'){
			if(*state != SINGLE ){
				output_count -= 1;
			}
		}

		output[output_count++] = input[i];
		if(i > 0  && input[i] == '\\' && input[i-1] == '\\'){
			input[i] = '\a';
		}
	}
	if(output[output_count-1] == ' '){
		output_count -= 1;
	}
	output[output_count] = '\0';

	int no_of_args = 0;
	char *raw_cmd = strsep(&output, " ");
	while(output != NULL){
		char *word = strsep(&output, " ");
		args[no_of_args++] = strdup(word);
	}

	for(int i=0;i<no_of_args;i++){
		int j=0;
		for(;args[i][j] != '\0';j++){
			if(args[i][j] == '\a'){
				args[i][j] = ' ';
			}
		}
	}

	for(int j=0;raw_cmd[j] != '\0';j++){
		if(raw_cmd[j] == '\a'){
			raw_cmd[j] = ' ';
		}
	}
	*cmd = raw_cmd;

	return no_of_args;
}

