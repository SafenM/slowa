// SPDX-License-Identifier: MIT
// Benchmark harness for the slowa solver.
//
// Measures:
//   * time and peak/steady RSS to load the whole dictionary into the trie
//   * time to solve a set of fixed 4x4 boards (same boards every run)
//
// Usage: slowa_benchmark [dictionary] [repeats]

#include "solver.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;
    using Board = std::vector<std::vector<char32_t>>;

    double MsSince(Clock::time_point start)
    {
        return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    }

    // Reads a numeric field such as "VmRSS:" from /proc/self/status (kB).
    // Returns -1 when unavailable (non-Linux).
    long ReadProcStatusKb(const char* key)
    {
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line))
        {
            if (line.compare(0, std::strlen(key), key) == 0)
            {
                const char* value = line.c_str() + std::strlen(key);
                while (*value == ' ' || *value == '\t')
                {
                    ++value;
                }
                return std::strtol(value, nullptr, 10);
            }
        }
        return -1;
    }

    Board MakeBoard(const std::vector<std::u32string>& rows)
    {
        Board board;
        for (const std::u32string& row : rows)
        {
            board.emplace_back(row.begin(), row.end());
        }
        return board;
    }

    std::vector<Board> FixedBoards()
    {
        return {
            MakeBoard({U"aokt", U"ersn", U"milw", U"cdyp"}),
            MakeBoard({U"prze", U"woda", U"kieł", U"styn"}),
            MakeBoard({U"mart", U"owek", U"syna", U"lice"}),
            MakeBoard({U"koty", U"male", U"sine", U"rado"}),
            MakeBoard({U"bals", U"roki", U"temu", U"nywa"}),
            MakeBoard({U"żaba", U"mlek", U"ryba", U"cudo"}),
        };
    }
}

int main(int argc, char** argv)
{
    const std::string dictionaryPath = argc > 1 ? argv[1] : "slownik.txt";
    const int repeats = argc > 2 ? std::atoi(argv[2]) : 1;

    const std::vector<Board> boards = FixedBoards();

    Solver solver;

    const long peakBefore = ReadProcStatusKb("VmHWM:");

    const Clock::time_point loadStart = Clock::now();
    solver.LoadDictionary(dictionaryPath);
    const double loadMs = MsSince(loadStart);

    const long rssAfterLoad = ReadProcStatusKb("VmRSS:");
    const long peakAfterLoad = ReadProcStatusKb("VmHWM:");
    (void)peakAfterLoad;

    double solveMs = 0.0;
    std::size_t totalSolutions = 0;

    for (int repetition = 0; repetition < repeats; ++repetition)
    {
        for (const Board& board : boards)
        {
            solver.SetBoard(board);
            const Clock::time_point solveStart = Clock::now();
            solver.Solve();
            solveMs += MsSince(solveStart);
            totalSolutions += solver.GetSolution().size();
        }
    }

    const long peakEnd = ReadProcStatusKb("VmHWM:");

    const double peakDeltaMb = (peakEnd - peakBefore) / 1024.0;

    std::printf("RESULT load_ms=%.1f solve_ms=%.1f boards=%zu repeats=%d solutions=%zu"
                " rss_after_load_mb=%.1f peak_growth_mb=%.1f peak_total_mb=%.1f\n",
                loadMs,
                solveMs,
                boards.size(),
                repeats,
                totalSolutions,
                rssAfterLoad / 1024.0,
                peakDeltaMb,
                peakEnd / 1024.0);

    return 0;
}
