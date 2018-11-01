#include <stdbool.h>

// Alphabet size (# of symbols)
#define ALPHABET_SIZE (26)

// trie node
typedef struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];

    // isEndOfWord is true if the node represents
    // end of a word
    bool isEndOfWord;
}TrieNode;

struct TrieNode *getNode(void);
bool search(struct TrieNode *root, char *key);
void insert(struct TrieNode *root, char *key);
