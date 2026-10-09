// SPDX-License-Identifier: MIT
#include "solver.h"
#include "utf8.h"

#include <fstream>
#include <iostream>

namespace
{
    // Neighbour order must stay identical to the original recursive solver so
    // that the produced solution list is unchanged.
    constexpr int Dr[8] = {0, 0, 1, -1, 1, 1, -1, -1};
    constexpr int Dc[8] = {1, -1, 0, 0, -1, 1, -1, 1};
}

Solver::Solver()
{

}

void Solver::AddDictionaryLine(const char* data, std::size_t size)
{
    while (size > 0 && data[size - 1] == '\r')
    {
        --size;
    }
    dictionary.Add(Utf8ToUtf32(data, size));
}

void Solver::LoadDictionary(std::string path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "File cannot be found: " << path << std::endl;
    }
    else
    {
        std::cerr << path << " Opening..." << std::endl;
    }
    std::string line;
    while (std::getline(file, line))
    {
        AddDictionaryLine(line.data(), line.size());
    }
    dictionary.Finalize();
}

void Solver::LoadDictionaryFromMemory(const char* data, std::size_t size)
{
    std::size_t start = 0;
    for (std::size_t i = 0; i < size; i++)
    {
        if (data[i] == '\n')
        {
            AddDictionaryLine(data + start, i - start);
            start = i + 1;
        }
    }
    if (start < size)
    {
        AddDictionaryLine(data + start, size - start);
    }
    dictionary.Finalize();
}

void Solver::SetBoard(const std::vector<std::vector<char32_t>> &newBoard)
{
    this->board = newBoard;
}

void Solver::PrepareBoard()
{
    for (int row = 0; row < BoardSize; row++)
    {
        for (int col = 0; col < BoardSize; col++)
        {
            char32_t letter = 0;
            if (row < (int)board.size() && col < (int)board[(std::size_t)row].size())
            {
                letter = board[(std::size_t)row][(std::size_t)col];
            }
            cellLetters[(std::size_t)(row * BoardSize + col)] = letter;
        }
    }
}

void Solver::Solve()
{
    solution.clear();
    PrepareBoard();

    const std::size_t nodeCount = dictionary.NodeCount();
    reported.assign((nodeCount + 63) / 64, 0);

    std::u32string path;
    path.reserve(CellCount);

    for (int row = 0; row < BoardSize; row++)
    {
        for (int col = 0; col < BoardSize; col++)
        {
            const int cell = row * BoardSize + col;
            const char32_t letter = cellLetters[(std::size_t)cell];
            const int node = dictionary.Advance(Trie::RootNode, letter);
            if (node < 0)
            {
                continue;
            }
            path.push_back(letter);
            Traverse(row, col, (std::uint16_t)(1u << cell), node, path);
            path.pop_back();
        }
    }
}

void Solver::Traverse(int row, int col, std::uint16_t visited, int trieNode, std::u32string& path)
{
    if (dictionary.IsWord(trieNode))
    {
        const std::size_t node = (std::size_t)trieNode;
        std::uint64_t& wordBit = reported[node >> 6];
        const std::uint64_t bit = 1ull << (node & 63u);
        if ((wordBit & bit) == 0)
        {
            wordBit |= bit;
            solution.push_back(path);
        }
    }

    for (int direction = 0; direction < 8; direction++)
    {
        const int nextRow = row + Dr[direction];
        const int nextCol = col + Dc[direction];
        if (nextRow < 0 || nextRow >= BoardSize || nextCol < 0 || nextCol >= BoardSize)
        {
            continue;
        }

        const int cell = nextRow * BoardSize + nextCol;
        if ((visited & (1u << cell)) != 0)
        {
            continue;
        }

        const int nextNode = dictionary.Advance(trieNode, cellLetters[(std::size_t)cell]);
        if (nextNode < 0)
        {
            continue;
        }

        path.push_back(cellLetters[(std::size_t)cell]);
        Traverse(nextRow, nextCol, (std::uint16_t)(visited | (1u << cell)), nextNode, path);
        path.pop_back();
    }
}

std::vector<std::u32string> Solver::GetSolution()
{
    return solution;
}

std::size_t Solver::DictionaryNodeCount()
{
    return dictionary.NodeCount();
}

bool Solver::WordInDictionary(std::u32string word)
{
    if (dictionary.SearchForWord(word) == ETrieSearchResult::WordFound)
    {
        return true;
    }
    return false;
}
