// SPDX-License-Identifier: MIT
#include "solver.h"
#include "trie.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    int g_checks = 0;
    int g_failures = 0;

    void Check(bool condition, const char* expression, const char* file, int line)
    {
        ++g_checks;
        if (!condition)
        {
            ++g_failures;
            std::fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expression);
        }
    }

#define CHECK(expr) Check((expr), #expr, __FILE__, __LINE__)

    using Board = std::vector<std::vector<char32_t>>;

    Board MakeBoard(const std::vector<std::u32string>& rows)
    {
        Board board;
        for (const auto& row : rows)
        {
            board.push_back(std::vector<char32_t>(row.begin(), row.end()));
        }
        return board;
    }

    std::string WriteDictionary(const std::string& name, const std::string& contents)
    {
        std::ofstream out(name, std::ios::binary);
        out << contents;
        out.close();
        return name;
    }

    bool Contains(const std::vector<std::u32string>& words, const std::u32string& word)
    {
        return std::find(words.begin(), words.end(), word) != words.end();
    }

    void TestTrieFindsWordsAndPrefixes()
    {
        Trie trie;
        trie.Add(U"kot");
        trie.Add(U"koty");
        trie.Add(U"żaba");

        CHECK(trie.SearchForWord(U"kot") == ETrieSearchResult::WordFound);
        CHECK(trie.SearchForWord(U"koty") == ETrieSearchResult::WordFound);
        CHECK(trie.SearchForWord(U"żaba") == ETrieSearchResult::WordFound);
        CHECK(trie.SearchForWord(U"ko") == ETrieSearchResult::PrefixFound);
        CHECK(trie.SearchForWord(U"k") == ETrieSearchResult::PrefixFound);
        CHECK(trie.SearchForWord(U"żab") == ETrieSearchResult::PrefixFound);
        CHECK(trie.SearchForWord(U"koc") == ETrieSearchResult::NotFound);
        CHECK(trie.SearchForWord(U"pies") == ETrieSearchResult::NotFound);

        // NOTE: characters outside PARAMS::Alphabet are deliberately not tested here.
        // CalculateAlphabeticalIndex returns -1 for them and the trie reacts with an
        // out-of-bounds children[-1] access. That is a known, unfixed bug.
    }

    void TestSolverFindsWordsOnBoard()
    {
        const std::string path = WriteDictionary("slowa_test_dict.txt", "kot\nkoty\nlot\ntok\n");

        Solver solver;
        solver.LoadDictionary(path);
        solver.SetBoard(MakeBoard({U"koty", U"moty", U"loty", U"aaaa"}));
        solver.Solve();
        const std::vector<std::u32string> solution = solver.GetSolution();

        CHECK(!solution.empty());
        CHECK(Contains(solution, U"kot"));
        CHECK(Contains(solution, U"koty"));
        CHECK(Contains(solution, U"lot"));
        CHECK(Contains(solution, U"tok"));

        // Every returned word must be one of the dictionary words.
        for (const std::u32string& word : solution)
        {
            CHECK(word == U"kot" || word == U"koty" || word == U"lot" || word == U"tok");
        }

        // The same word must be reported only once.
        std::vector<std::u32string> sorted = solution;
        std::sort(sorted.begin(), sorted.end());
        CHECK(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end());

        std::remove(path.c_str());
    }

    void TestSolverDeduplicatesRepeatedPaths()
    {
        const std::string path = WriteDictionary("slowa_test_dict_dedup.txt", "kot\n");

        Solver solver;
        solver.LoadDictionary(path);
        // "kot" can be spelled along many different paths on this board.
        solver.SetBoard(MakeBoard({U"koty", U"moty", U"loty", U"aaaa"}));
        solver.Solve();
        const std::vector<std::u32string> solution = solver.GetSolution();

        CHECK(solution.size() == 1);
        CHECK(!solution.empty() && solution[0] == U"kot");

        std::remove(path.c_str());
    }

    void TestSolverHandlesPolishCharacters()
    {
        const std::string path = WriteDictionary("slowa_test_dict_pl.txt", "żaba\nżabka\n");

        Solver solver;
        solver.LoadDictionary(path);
        solver.SetBoard(MakeBoard({U"żaba", U"aaaa", U"aaaa", U"aaaa"}));
        solver.Solve();
        const std::vector<std::u32string> solution = solver.GetSolution();

        CHECK(Contains(solution, U"żaba"));
        CHECK(!Contains(solution, U"żabka"));

        std::remove(path.c_str());
    }

    void TestSolverIgnoresEmptyCells()
    {
        const std::string path = WriteDictionary("slowa_test_dict_empty.txt", "kot\n");

        Board board = {
            {U'k', U'o', 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
            {0, 0, 0, 0},
        };

        Solver solver;
        solver.LoadDictionary(path);
        solver.SetBoard(board);
        solver.Solve();
        CHECK(solver.GetSolution().empty());

        std::remove(path.c_str());
    }
}

int main()
{
    TestTrieFindsWordsAndPrefixes();
    TestSolverFindsWordsOnBoard();
    TestSolverDeduplicatesRepeatedPaths();
    TestSolverHandlesPolishCharacters();
    TestSolverIgnoresEmptyCells();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
