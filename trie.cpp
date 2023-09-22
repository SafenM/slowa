#include "trie.h"
#include <QDebug>
#include "params.h"

int CalculateAlphabeticalIndex(QChar ch)
{
    QString Alphabet = PARAMS::Alphabet;
    for (int i = 0; i < Alphabet.length(); i++)
    {
        QChar ch2 = Alphabet[i];
        if (ch == ch2)
        {
            return i;
        }
    }
    return -1; //Might also throw an exception
}

Trie::Trie()
{
    for (int i = 0; i < PARAMS::ALPHA_SIZE; i++)
    {
        Root.children[i] = nullptr;
    }
}

Trie::~Trie()
{
    for (auto& child : Root.children)
    {
        if (child)
        {
            DeleteNode(child);
        }
    }
}

void Trie::DeleteNode(Node* node)
{
    for (auto& child : node->children)
    {
        if (child)
        {
            DeleteNode(child);
        }
    }
    delete node;
}

Node* Trie::MakeNode()
{
    Node* newNode = new Node;
    for (auto& child : newNode->children)
    {
        child = nullptr;
    }
    return newNode;
}

void Trie::Add(QString word)
{
    Node* currentNode = &Root;
    for (int i = 0; i < word.length(); i++)
    {
        int index = CalculateAlphabeticalIndex(word[i]);
        if (currentNode->children[index] == nullptr)
        {
            currentNode->children[index] = MakeNode();
        }
        currentNode = currentNode->children[index];
    }
    currentNode->wordExists = true;
    //qDebug() << word << " added to dictionary";
}

ETrieSearchResult Trie::SearchForWord(QString word)
{
    Node* currentNode = &Root;
    for (int i = 0; i < word.length(); i++)
    {
        int index = CalculateAlphabeticalIndex(word[i]);
        if (currentNode->children[index] == nullptr)
        {
            return ETrieSearchResult::NotFound;
        }
        currentNode = currentNode->children[index];
    }
    if (currentNode->wordExists)
    {
        return ETrieSearchResult::WordFound;
    }
    return ETrieSearchResult::PrefixFound;
}
