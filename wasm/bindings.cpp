// SPDX-License-Identifier: MIT
// Emscripten C API for the solver.
//
// Exposed to JavaScript:
//   slowa_load_dictionary(ptr, size) -> int   load UTF-8 dictionary from memory
//   slowa_solve(boardUtf8)           -> char* newline-separated UTF-8 words
//   slowa_alphabet()                 -> char* the 32-letter alphabet (UTF-8)
//   slowa_node_count()               -> int   packed trie node count

#include "solver.h"
#include "utf8.h"

#include <cstring>
#include <string>
#include <vector>

#include <emscripten/emscripten.h>

namespace
{
    Solver& SolverInstance()
    {
        static Solver solver;
        return solver;
    }

    std::string g_result;
    std::string g_alphabet;
}

extern "C"
{
    EMSCRIPTEN_KEEPALIVE
    int slowa_load_dictionary(const char* data, int size)
    {
        if (data == nullptr || size < 0)
        {
            return 0;
        }
        SolverInstance().LoadDictionaryFromMemory(data, (std::size_t)size);
        return 1;
    }

    EMSCRIPTEN_KEEPALIVE
    const char* slowa_solve(const char* boardUtf8)
    {
        const std::u32string letters = Utf8ToUtf32(boardUtf8, std::strlen(boardUtf8));

        std::vector<std::vector<char32_t>> board(
            (std::size_t)PARAMS::BOARD_SIZE,
            std::vector<char32_t>((std::size_t)PARAMS::BOARD_SIZE, 0));

        const int cellCount = PARAMS::BOARD_SIZE * PARAMS::BOARD_SIZE;
        for (int i = 0; i < cellCount && i < (int)letters.size(); i++)
        {
            board[(std::size_t)(i / PARAMS::BOARD_SIZE)][(std::size_t)(i % PARAMS::BOARD_SIZE)] = letters[(std::size_t)i];
        }

        SolverInstance().SetBoard(board);
        SolverInstance().Solve();

        g_result.clear();
        for (const std::u32string& word : SolverInstance().GetSolution())
        {
            for (char32_t ch : word)
            {
                AppendUtf8(g_result, ch);
            }
            g_result.push_back('\n');
        }
        return g_result.c_str();
    }

    EMSCRIPTEN_KEEPALIVE
    const char* slowa_alphabet()
    {
        g_alphabet = Utf8FromUtf32(PARAMS::Alphabet);
        return g_alphabet.c_str();
    }

    EMSCRIPTEN_KEEPALIVE
    int slowa_node_count()
    {
        return (int)SolverInstance().DictionaryNodeCount();
    }
}
