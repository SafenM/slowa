# Znajdź Słowa

A dependency-free C++17 library that solves a 4x4 word game (Boggle-style) for
the Polish language, plus a WebAssembly web app. It uses a memory-compact trie
built from a dictionary and a depth-first search over the board.

No external dependencies. The repository builds:
- a static library (`slowa`),
- a test executable (`slowa_tests`),
- a benchmark tool (`slowa_benchmark`),
- a WebAssembly module + static web frontend.

## Dictionary

The dictionary is a plain UTF-8 text file with one word per line. The provided
`slownik.txt` is a trimmed Polish word list (no `x`/`v`/`q`, no two-letter
words). Any UTF-8 word list can be used; characters outside `PARAMS::Alphabet`
are simply skipped.

## Alphabet

Words are indexed by the (currently 32) characters in `PARAMS::Alphabet`
(`params.h`). Dictionary and board characters outside that set are ignored.

## Build and test (native)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the benchmark (dictionary path, optional repeats):

```sh
./build/slowa_benchmark slownik.txt 20
```

## Web app (WebAssembly)

Requires the [Emscripten SDK](https://emscripten.org/). One command builds the
WASM module and stages the assets into `web/`:

```sh
./scripts/build_web.sh
python3 -m http.server 8000 --directory web
# open http://localhost:8000
```

The dictionary is served gzipped (`slownik.txt.gz`, ~8 MB) and decompressed in
the browser with `DecompressionStream`, then loaded into WASM memory. The trie
is built client-side (~5.2M nodes, ~63 MB) on first load.

Features: 4x4 board with keyboard/paste entry, keyboard navigation, random
board, results grouped by word length, and click-a-word to highlight its path
on the board. The current board is stored in the URL (`?board=...`), so a board
can be shared as a link.

### Web library usage

```cpp
Solver solver;
solver.LoadDictionary("slownik.txt");          // or LoadDictionaryFromMemory(...)
solver.SetBoard(board);                        // std::vector<std::vector<char32_t>>
solver.Solve();
std::vector<std::u32string> words = solver.GetSolution();
```

## Implementation notes

- **Packed trie.** Each node is 8 bytes: a 32-bit bitmask of which alphabet
  letters have a child, plus a 32-bit index of the node's first child in a
  shared edge pool (the terminal flag is packed into the top bit). Children of a
  node are stored contiguously, ordered by alphabet index, so a child lookup is
  `popcount(mask & (bit - 1))`. The trie is built in one pass from the sorted
  list of words.
- **O(1) alphabet lookup.** A small table maps a code point to its alphabet
  index.
- **Allocation-free search.** The solver flattens the board once and runs the
  DFS with a 16-bit visited mask and an incremental trie traversal, so no board
  or word is copied per step.
- **Deduplication by node identity.** A trie node uniquely identifies a word,
  and every board path spelling the same word ends at the same node. A per-node
  bitset (`reported`) therefore drops duplicate solutions with an O(1) bit test
  and no string hashing.

## Benchmark

Measured on the full 3.18M-word `slownik.txt`, Release build, 6 fixed boards,
with deduplication enabled:

| Metric                    | Before | After  | Improvement |
|---------------------------|-------:|-------:|------------:|
| Dictionary load           | 1165 ms | 655 ms | 1.8x faster |
| Solve (6 boards)          | 4.0 ms | 0.41 ms | ~10x faster |
| Resident memory (loaded)  | 1358 MB | 63 MB | 21.5x less |
| Peak memory (during load) | 1358 MB | 215 MB | 6.3x less |

The trie is ~5.22M nodes -> ~59.8 MB (8 B/node + 4 B/edge). The peak is the
transient key buffer used while building; it is freed before steady state.

## License

The source code is released under the [MIT License](LICENSE).

The Polish word list (`slownik.txt`) is a modified subset of the SJP.PL game
dictionary and is used under the **CC BY 4.0** license. See
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) for the required attribution
and the list of modifications.

## Deployment

`.github/workflows/pages.yml` builds the WASM module on every push. Deployment
to GitHub Pages is intentionally disabled while the repository is private
(GitHub Pages on the Free plan only publishes from public repositories). To
enable it: make the repository public, turn on Pages with "GitHub Actions" as
the source, and set the repository variable `PAGES_ENABLED=true`.

# PL

Biblioteka C++17 bez zależności zewnętrznych do rozwiązywania gry słownej na
planszy 4x4 w języku polskim, oraz aplikacja webowa w WebAssembly (kompaktowe
trie + przeszukiwanie w głąb). Projekt buduje bibliotekę statyczną `slowa`,
testy `slowa_tests`, benchmark `slowa_benchmark` i moduł WASM z frontendem.

```sh
# natywnie
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
ctest --test-dir build --output-on-failure

# web (wymaga Emscripten)
./scripts/build_web.sh
python3 -m http.server 8000 --directory web
```

Słownik to plik tekstowy UTF-8, jedno słowo w linii. Znaki spoza
`PARAMS::Alphabet` (`params.h`) są pomijane. W przeglądarce słownik jest
pobierany jako gzip (~8 MB), dekompresowany (`DecompressionStream`) i
wczytywany do pamięci WASM; trie budowane jest po stronie klienta.

Optymalizacje: kompaktowe trie (8 B/węzeł + 4 B/krawędź), tablica indeksów
alfabetu w O(1), DFS bez kopiowania planszy (maska odwiedzin) oraz deduplikacja
rozwiązań po tożsamości węzła trie (bit test, bez haszowania). Na pełnym
słowniku 3,18 mln słów: wczytywanie 1165 -> 655 ms, rozwiązywanie 4,0 -> 0,41
ms, pamięć rezydentna 1358 -> 63 MB.

## Licencja

Kod źródłowy na licencji [MIT](LICENSE). Słownik (`slownik.txt`) to
zmodyfikowany podzbiór słownika do gier SJP.PL, używany na licencji
**CC BY 4.0** — szczegóły, wymagane oznaczenie autorstwa i opis zmian w
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

Wdrożenie na GitHub Pages jest przygotowane w `.github/workflows/pages.yml`,
ale celowo wyłączone, dopóki repozytorium jest prywatne.
