#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

struct trie{
	struct trie* chars[255];
	int isValid;
	int total_inputs;
	char *word;
};

struct trie* new_trie();
void load(struct trie *t);
void load_word(struct trie *t, char *input, int i);
int get_completion(struct trie *t, char *input, char ***output);
