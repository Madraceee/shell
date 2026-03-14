#include "trie.h"

struct trie* new_trie(){
	struct trie *t = (struct trie*)malloc(sizeof(struct trie));
	if(t == NULL){
		exit(EXIT_FAILURE);
	}
	for(int i=0;i<255;i++){
		t->chars[i] = NULL;
	}
	t->isValid = 0;
	t->total_inputs = 0;
	return t;
}

void load_word(struct trie *t, char *input, int i){
	if(i == strlen(input) || input == NULL || input[i] == '\0'){
		return;
	}
	int pos = input[i];
	if(t->chars[pos] == NULL){
		t->chars[pos] = new_trie();
	}
	if(i+1 == strlen(input)){
		t->isValid = 1;
		t->word = strdup(input);
	}
	return load_word(t->chars[pos], input, i+1);
}

void load(struct trie *t){
	char *path_value = getenv("PATH");
	if (path_value == NULL) {
		return;
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
			int pos = ent->d_name[0];
			if(t->chars[pos] == NULL){
				t->chars[pos] = new_trie();
			}
			load_word(t->chars[pos], ent->d_name, 1);
			t->total_inputs++;
		}
		closedir(dir);
	}
}

void finish_completion(struct trie* t, char **output, int *output_count){
	if(t->isValid == 1){
		output[*output_count] = strdup(t->word);
		*output_count += 1;
	}
	
	for(int i=0;i<255;i++){
		if(t->chars[i] != NULL){
			finish_completion(t->chars[i], output, output_count);
		}
	}
}

int get_completion(struct trie *t, char *input, char ***output){
	int len = strlen(input);

	*output = (char**)malloc(sizeof(char*) * t->total_inputs);
	int output_count = 0;
	for(int i=0;i<len;i++){
		int pos = input[i];
		if(t->chars[pos] != NULL){
			t = t->chars[pos];
		}else{
			return 0;
		}
	}
	finish_completion(t, *output, &output_count);
	return output_count;
}
