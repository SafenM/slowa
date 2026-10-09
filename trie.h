// SPDX-License-Identifier: MIT
#ifndef TRIE_H
#define TRIE_H

#include <cstdint>
#include <string>
#include <vector>
#include "params.h"

enum class ETrieSearchResult
{
    NotFound,
    PrefixFound,
    WordFound
};

// Memory-compact trie.
//
// Instead of one 32-pointer array per node (264 B/node), every node stores
//   * a 32-bit bitmask of which alphabet letters have a child, and
//   * a 32-bit index of the first child inside a shared `edges` pool,
// and the terminal flag is packed into the top bit of that index.
// The children of a node are laid out contiguously in `edges`, ordered by
// alphabet index, so a child lookup is popcount(mask & (bit - 1)).
// This makes a node 8 bytes and a child edge 4 bytes.
//
// Building is done in one pass from the sorted list of words, which is what
// makes the packed layout possible without shifting.
class Trie
{
public:
    static constexpr int RootNode = 0;

    Trie() = default;
    Trie(const Trie& other) = delete;
    Trie& operator=(const Trie& other) = delete;

    void Add(std::u32string word);
    ETrieSearchResult SearchForWord(const std::u32string& word);
    void Finalize();

    // Incremental traversal used by the solver's depth-first search.
    // Returns the child node index or -1 when there is no such child.
    int Advance(int node, char32_t ch);
    bool IsWord(int node);
    std::size_t NodeCount();

private:
    struct Node
    {
        std::uint32_t mask;
        std::uint32_t firstAndTerminal;
    };

    static constexpr std::uint32_t TerminalFlag = 0x80000000u;

    void EnsureBuilt();
    void Build();
    std::uint32_t BuildRange(std::size_t lo, std::size_t hi, std::size_t depth);

    std::vector<std::string> pendingKeys; // alphabet-index keys, only while !built
    std::vector<Node> nodes;
    std::vector<std::uint32_t> edges;
    bool built = false;
};

int CalculateAlphabeticalIndex(char32_t ch);

#endif // TRIE_H
