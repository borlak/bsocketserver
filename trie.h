#ifndef __TRIE_H
#define __TRIE_H

typedef struct trie TRIE;

TRIE *trie_create();
void trie_add(TRIE *head, char *str, void *object);
void *trie_find(TRIE *head, char *str);

struct trie {
	TRIE	*nodes[256];
	void	*object;	// object (goal) to point to
};

#endif
