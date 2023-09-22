#ifndef TRIE_H
#define TRIE_H
#include <QChar>
#include <QString>
#include "params.h"

enum class ETrieSearchResult
{
    NotFound,
    PrefixFound,
    WordFound
};

struct Node
{
    bool wordExists = false;
    Node* children[PARAMS::ALPHA_SIZE];
};

class Trie
{
public:
    Trie();
    Trie(const Trie& other) = delete;
    Trie& operator=(const Trie& other) = delete;
    ~Trie();
    void Add(QString word);
    ETrieSearchResult SearchForWord(QString word);
private:
    Node Root;
    Node* MakeNode();
    void DeleteNode(Node* node);

};

int CalculateAlphabeticalIndex(QChar ch);

#endif // TRIE_H
