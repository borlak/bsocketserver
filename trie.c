#include <stdlib.h>
#include <string.h>
#include "trie.h"

unsigned long trie_memory = 0;

TRIE *trie_create() {
	TRIE *trie = malloc(sizeof(TRIE));
	memset(trie, 0, sizeof(TRIE));
	trie_memory += sizeof(TRIE);
	return trie;
}

void trie_add(TRIE *head, char *str, void *object) {
	TRIE *trie = head;
	char *p = str;
	
	if(*str == '\0' || str == 0) return;
	
	while(1) {
		if(trie->nodes[(int)*p] == 0) {
			trie->nodes[(int)*p] = trie_create();
		}
		trie = trie->nodes[(int)*p];
		if(*(p+1) == '\0')
			break;
		p++;
	}
	trie->object = object;
}

void *trie_find(TRIE *head, char *str) {
	if(*str == '\0' || str == 0) return 0;
	
	while(*str != '\0') {
		if(head->nodes[(int)*str] == 0)
			return 0;
		head = head->nodes[(int)*str];
		str++;
	}
	
	return head->object;
}

