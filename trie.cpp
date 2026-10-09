// SPDX-License-Identifier: MIT
#include "trie.h"

#include <algorithm>
#include <array>

#if defined(__GLIBC__)
#include <malloc.h>
#endif

int CalculateAlphabeticalIndex(char32_t ch)
{
    // O(1) lookup table for the small set of characters that can appear.
    // PARAMS::Alphabet currently ends at 'ż' (U+017C).
    static const std::array<std::int8_t, 0x180> indexOf = [] {
        std::array<std::int8_t, 0x180> table{};
        table.fill(-1);
        for (int i = 0; i < PARAMS::ALPHA_SIZE; i++)
        {
            table[(std::size_t)PARAMS::Alphabet[(std::size_t)i]] = (std::int8_t)i;
        }
        return table;
    }();

    if (ch < indexOf.size())
    {
        return indexOf[(std::size_t)ch];
    }
    return -1;
}

void Trie::Add(std::u32string word)
{
    std::string key;
    key.reserve(word.size());
    for (char32_t ch : word)
    {
        int index = CalculateAlphabeticalIndex(ch);
        if (index < 0)
        {
            return; // character outside PARAMS::Alphabet: skip the word
        }
        key.push_back((char)index);
    }
    pendingKeys.push_back(std::move(key));
    built = false;
}

void Trie::EnsureBuilt()
{
    if (!built)
    {
        Build();
    }
}

void Trie::Finalize()
{
    EnsureBuilt();
}

void Trie::Build()
{
    built = true;
    nodes.clear();
    edges.clear();

    if (pendingKeys.empty())
    {
        nodes.push_back(Node{0u, 0u}); // still need the root node
        return;
    }

    std::sort(pendingKeys.begin(), pendingKeys.end());
    BuildRange(0, pendingKeys.size(), 0); // creates the root at index 0

    // Release the temporary key storage and the geometric growth slack.
    std::vector<std::string>().swap(pendingKeys);
    nodes.shrink_to_fit();
    edges.shrink_to_fit();
#if defined(__GLIBC__)
    // glibc tends to keep the freed blocks in its arena; hand them back to the
    // OS so the resident footprint reflects the compact trie.
    malloc_trim(0);
#endif
}

std::uint32_t Trie::BuildRange(std::size_t lo, std::size_t hi, std::size_t depth)
{
    const std::uint32_t nodeIndex = (std::uint32_t)nodes.size();
    nodes.push_back(Node{0u, 0u});

    // First pass: find the distinct letters below this node and whether a word
    // ends exactly here.
    std::uint32_t mask = 0;
    std::uint32_t groupCount = 0;
    bool terminal = false;
    for (std::size_t i = lo; i < hi;)
    {
        const std::string& key = pendingKeys[i];
        if (key.size() == depth)
        {
            terminal = true;
            ++i;
            continue;
        }
        const std::uint8_t letter = (std::uint8_t)key[depth];
        mask |= (1u << letter);
        ++groupCount;
        ++i;
        while (i < hi && pendingKeys[i].size() > depth && (std::uint8_t)pendingKeys[i][depth] == letter)
        {
            ++i;
        }
    }

    // Reserve all child slots up front so they stay contiguous even though the
    // recursive calls append their own edges below us.
    const std::uint32_t firstEdge = (std::uint32_t)edges.size();
    edges.resize(edges.size() + groupCount);

    std::uint32_t childSlot = 0;
    for (std::size_t i = lo; i < hi;)
    {
        const std::string& key = pendingKeys[i];
        if (key.size() == depth)
        {
            ++i;
            continue;
        }
        const std::uint8_t letter = (std::uint8_t)key[depth];
        const std::size_t groupBegin = i;
        ++i;
        while (i < hi && pendingKeys[i].size() > depth && (std::uint8_t)pendingKeys[i][depth] == letter)
        {
            ++i;
        }
        edges[firstEdge + childSlot] = BuildRange(groupBegin, i, depth + 1);
        ++childSlot;
    }

    nodes[nodeIndex].mask = mask;
    nodes[nodeIndex].firstAndTerminal = firstEdge | (terminal ? TerminalFlag : 0u);
    return nodeIndex;
}

int Trie::Advance(int node, char32_t ch)
{
    EnsureBuilt();
    const int index = CalculateAlphabeticalIndex(ch);
    if (index < 0)
    {
        return -1;
    }

    const Node& current = nodes[(std::size_t)node];
    const std::uint32_t bit = 1u << index;
    if ((current.mask & bit) == 0)
    {
        return -1;
    }

    const std::uint32_t first = current.firstAndTerminal & ~TerminalFlag;
    const std::uint32_t rank = (std::uint32_t)__builtin_popcount(current.mask & (bit - 1));
    return (int)edges[(std::size_t)(first + rank)];
}

bool Trie::IsWord(int node)
{
    EnsureBuilt();
    return (nodes[(std::size_t)node].firstAndTerminal & TerminalFlag) != 0;
}

std::size_t Trie::NodeCount()
{
    EnsureBuilt();
    return nodes.size();
}

ETrieSearchResult Trie::SearchForWord(const std::u32string& word)
{
    EnsureBuilt();
    int node = RootNode;
    for (char32_t ch : word)
    {
        node = Advance(node, ch);
        if (node < 0)
        {
            return ETrieSearchResult::NotFound;
        }
    }
    return IsWord(node) ? ETrieSearchResult::WordFound : ETrieSearchResult::PrefixFound;
}
