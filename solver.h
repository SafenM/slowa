// SPDX-License-Identifier: MIT
#ifndef SOLVER_H
#define SOLVER_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "trie.h"

class Solver
{
private:
    static constexpr int BoardSize = PARAMS::BOARD_SIZE;
    static constexpr int CellCount = BoardSize * BoardSize;

    std::vector<std::vector<char32_t>> board;
    Trie dictionary;
    std::vector<std::u32string> solution;

    // Flattened board for the depth-first search, plus the per-cell letters.
    std::array<char32_t, CellCount> cellLetters{};

    // Bitset over trie nodes. A trie node uniquely identifies a word, and every
    // board path that spells the same word ends at the same node, so this
    // deduplicates solutions with an O(1) bit test and no string hashing.
    std::vector<std::uint64_t> reported;

public:
    Solver();
    void SetBoard(const std::vector<std::vector<char32_t>> &newBoard);
    void LoadDictionary(std::string path);
    void LoadDictionaryFromMemory(const char* data, std::size_t size);
    void Solve();
    std::vector<std::u32string> GetSolution();
    std::size_t DictionaryNodeCount();
    bool WordInDictionary(std::u32string word);
private:
    void AddDictionaryLine(const char* data, std::size_t size);
    void PrepareBoard();
    void Traverse(int row, int col, std::uint16_t visited, int trieNode, std::u32string& path);
};

#endif // SOLVER_H
