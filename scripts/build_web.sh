#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
#
# Build the WebAssembly module and stage all web assets into web/.
# Requires the Emscripten SDK (emcmake on PATH, or installed in ~/emsdk).

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v emcmake >/dev/null 2>&1; then
  if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
    # shellcheck disable=SC1091
    source "$HOME/emsdk/emsdk_env.sh" >/dev/null 2>&1
  fi
fi

if ! command -v emcmake >/dev/null 2>&1; then
  echo "emcmake not found. Install and activate the Emscripten SDK first." >&2
  exit 1
fi

emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm -j"$(nproc 2>/dev/null || echo 4)"

mkdir -p web
cp build-wasm/slowa.js build-wasm/slowa.wasm web/
gzip -9 -c slownik.txt > web/slownik.txt.gz

echo
echo "Web assets ready in web/:"
echo "  web/slowa.js, web/slowa.wasm, web/slownik.txt.gz"
echo
echo "Serve locally with:"
echo "  python3 -m http.server 8000 --directory web"
echo "then open http://localhost:8000"
