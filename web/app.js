// SPDX-License-Identifier: MIT
"use strict";

const BOARD = 4;
const CELLS = BOARD * BOARD;
const FALLBACK_ALPHABET = "aąbcćdeęfghijklłmnńoóprsśtuwyzźż";

// Rough Polish letter frequency, used only for the "Losuj" button.
const WEIGHTS = {
  a: 9, ą: 1, b: 1, c: 1, ć: 1, d: 3, e: 8, ę: 1, f: 1, g: 1, h: 1,
  i: 8, j: 2, k: 3, l: 3, ł: 2, m: 2, n: 5, ń: 1, o: 8, ó: 1, p: 3,
  r: 5, s: 5, ś: 1, t: 4, u: 3, w: 2, y: 4, z: 5, ź: 1, ż: 1,
};

const DR = [0, 0, 1, -1, 1, 1, -1, -1];
const DC = [1, -1, 0, 0, -1, 1, -1, 1];
const SVG_NS = "http://www.w3.org/2000/svg";

let wasm = null;
let alphabet = FALLBACK_ALPHABET;
let ready = false;
let cells = [];
let words = [];
let animationToken = 0;
let activePath = [];
let trace = null;

const byId = (id) => document.getElementById(id);

document.addEventListener("DOMContentLoaded", boot);

async function boot() {
  buildGrid();
  wireControls();

  if (typeof createSlowa !== "function") {
    setStatus("Nie udało się wczytać slowa.js — uruchom skrypt budujący.");
    return;
  }

  try {
    setStatus("Ładowanie WebAssembly…");
    wasm = await createSlowa();
    alphabet = wasm.ccall("slowa_alphabet", "string", [], []) || FALLBACK_ALPHABET;
    await loadDictionary();
    ready = true;
    const nodes = wasm.ccall("slowa_node_count", "number", [], []);
    setStatus(`Gotowe — słownik: ${nodes.toLocaleString("pl-PL")} węzłów.`);
    await restoreFromUrl();
  } catch (error) {
    console.error(error);
    setStatus("Błąd: " + error.message);
  }
}

async function loadDictionary() {
  setStatus("Pobieranie słownika…");
  const response = await fetch("slownik.txt.gz");
  if (!response.ok) {
    throw new Error(`nie można pobrać słownika (HTTP ${response.status})`);
  }

  setStatus("Dekompresja słownika…");
  let bytes;
  if (typeof DecompressionStream !== "undefined") {
    const stream = response.body.pipeThrough(new DecompressionStream("gzip"));
    bytes = new Uint8Array(await new Response(stream).arrayBuffer());
  } else {
    bytes = new Uint8Array(await response.arrayBuffer());
  }

  setStatus("Budowanie słownika…");
  await nextFrame();
  const pointer = wasm._malloc(bytes.length);
  wasm.HEAPU8.set(bytes, pointer);
  wasm.ccall("slowa_load_dictionary", "number", ["number", "number"], [pointer, bytes.length]);
  wasm._free(pointer);
}

function buildGrid() {
  const grid = byId("grid");
  grid.innerHTML = "";
  cells = [];

  for (let index = 0; index < CELLS; index++) {
    const input = document.createElement("input");
    input.className = "cell";
    input.type = "text";
    input.maxLength = 1;
    input.autocomplete = "off";
    input.autocapitalize = "off";
    input.spellcheck = false;
    input.inputMode = "text";
    input.dataset.index = String(index);
    input.setAttribute("aria-label", `Pole ${index + 1}`);
    input.addEventListener("input", onCellInput);
    input.addEventListener("keydown", onCellKey);
    input.addEventListener("focus", clearHighlight);
    grid.appendChild(input);
    cells.push(input);
  }

  grid.addEventListener("paste", onPaste);
}

function wireControls() {
  byId("solve").addEventListener("click", solve);
  byId("random").addEventListener("click", randomBoard);
  byId("clear").addEventListener("click", clearAll);
  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape") {
      clearHighlight();
    }
  });
  window.addEventListener("resize", () => {
    if (activePath.length) {
      renderTrace(activePath, null);
    }
  });
}

function normalize(character) {
  return character.toLocaleLowerCase("pl");
}

function onCellInput(event) {
  const input = event.target;
  const characters = Array.from(input.value);
  let character = characters.length ? normalize(characters[characters.length - 1]) : "";

  if (character && !alphabet.includes(character)) {
    character = "";
  }
  input.value = character;

  if (character) {
    focusCell(Number(input.dataset.index) + 1);
  }
  clearHighlight();
}

function onCellKey(event) {
  const index = Number(event.target.dataset.index);
  if (event.key === "Backspace" && !event.target.value) {
    focusCell(index - 1);
    event.preventDefault();
  } else if (event.key === "ArrowLeft") {
    focusCell(index - 1);
    event.preventDefault();
  } else if (event.key === "ArrowRight") {
    focusCell(index + 1);
    event.preventDefault();
  } else if (event.key === "ArrowUp") {
    focusCell(index - BOARD);
    event.preventDefault();
  } else if (event.key === "ArrowDown") {
    focusCell(index + BOARD);
    event.preventDefault();
  } else if (event.key === "Enter") {
    solve();
    event.preventDefault();
  }
}

function onPaste(event) {
  const text = (event.clipboardData || window.clipboardData).getData("text");
  if (!text) {
    return;
  }

  const letters = Array.from(text)
    .map(normalize)
    .filter((character) => alphabet.includes(character));
  if (!letters.length) {
    return;
  }

  event.preventDefault();
  const start = Number(document.activeElement?.dataset?.index ?? 0);
  for (let i = 0; i < letters.length && start + i < CELLS; i++) {
    cells[start + i].value = letters[i];
  }
  clearHighlight();
}

function focusCell(index) {
  if (index >= 0 && index < CELLS) {
    cells[index].focus();
    cells[index].select();
  }
}

function readBoard() {
  // "." marks an empty cell: it is not in the alphabet, so the solver skips it.
  return cells.map((cell) => cell.value.trim() || ".").join("");
}

async function solve() {
  if (!ready) {
    setStatus("Słownik jeszcze się ładuje…");
    return;
  }
  if (!/[a-ząćęłńóśźż]/i.test(readBoard())) {
    setStatus("Wpisz litery na planszy.");
    return;
  }

  setStatus("Rozwiązywanie…");
  await nextFrame();

  const started = performance.now();
  const pointer = wasm.ccall("slowa_solve", "number", ["string"], [readBoard()]);
  const text = wasm.UTF8ToString(pointer);
  const elapsed = performance.now() - started;

  words = text ? text.split("\n").filter(Boolean) : [];
  renderResults(elapsed);
  setStatus(`Znaleziono ${words.length} słów w ${elapsed.toFixed(1)} ms.`);
  history.replaceState(null, "", "?board=" + encodeURIComponent(readBoard()));
}

async function restoreFromUrl() {
  const boardParam = new URLSearchParams(window.location.search).get("board");
  if (!boardParam) {
    return;
  }
  const characters = Array.from(boardParam).map(normalize);
  for (let i = 0; i < CELLS && i < characters.length; i++) {
    if (alphabet.includes(characters[i])) {
      cells[i].value = characters[i];
    }
  }
  if (readBoard().replace(/\./g, "")) {
    await solve();
  }
}

function renderResults(elapsed) {
  const container = byId("results");
  container.innerHTML = "";
  clearHighlight();

  if (!words.length) {
    container.innerHTML = '<p class="muted">Brak słów.</p>';
    byId("summary").textContent = "";
    return;
  }

  const groups = new Map();
  for (const word of words) {
    const length = Array.from(word).length;
    if (!groups.has(length)) {
      groups.set(length, []);
    }
    groups.get(length).push(word);
  }

  for (const length of [...groups.keys()].sort((a, b) => b - a)) {
    const group = document.createElement("section");
    group.className = "group";

    const heading = document.createElement("h3");
    const list = groups.get(length).sort((a, b) => a.localeCompare(b, "pl"));
    heading.textContent = `${length} liter (${list.length})`;
    group.appendChild(heading);

    const listElement = document.createElement("div");
    listElement.className = "word-list";
    for (const word of list) {
      const chip = document.createElement("span");
      chip.className = "word-chip";

      const button = document.createElement("button");
      button.type = "button";
      button.className = "word";
      button.textContent = word;
      button.addEventListener("click", () => showPath(word, button));

      const link = document.createElement("a");
      link.className = "word-link";
      link.href = "https://sjp.pl/" + encodeURIComponent(word);
      link.target = "_blank";
      link.rel = "noopener noreferrer";
      link.title = `Zobacz „${word}” w SJP.PL`;
      link.setAttribute("aria-label", `Zobacz „${word}” w SJP.PL`);
      link.textContent = "↗";

      chip.append(button, link);
      listElement.appendChild(chip);
    }
    group.appendChild(listElement);
    container.appendChild(group);
  }

  byId("summary").textContent = `${words.length} słów · ${elapsed.toFixed(1)} ms`;
}

function showPath(word, button) {
  const chip = button.closest(".word-chip");
  clearHighlight();

  const path = findPath(word);
  if (!path) {
    return;
  }

  chip.classList.add("active");
  activePath = path;
  const stepMs = stepDuration(path.length);
  renderTrace(path, stepMs);
  playPath(path, stepMs);

  // On narrow layouts the board is above the results, so bring it back into view.
  if (window.matchMedia("(max-width: 760px)").matches) {
    byId("grid").scrollIntoView({ behavior: "smooth", block: "start" });
  }
}

function stepDuration(length) {
  return Math.min(180, Math.max(90, Math.round(2200 / length)));
}

// Steps through the path one cell at a time so the eye can follow long words.
function playPath(path, stepMs) {
  const token = ++animationToken;
  const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;

  if (reduceMotion) {
    path.forEach((cellIndex, order) => {
      cells[cellIndex].classList.add("path");
      cells[cellIndex].dataset.order = String(order + 1);
    });
    return;
  }

  path.forEach((cellIndex, order) => {
    window.setTimeout(() => {
      if (token !== animationToken) {
        return;
      }
      markStep(cellIndex, order);
    }, order * stepMs);
  });
}

function markStep(cellIndex, order) {
  const cell = cells[cellIndex];
  cell.classList.add("path");
  cell.dataset.order = String(order + 1);

  // Draw the trace up to the next letter, so the line arrives as it lights up.
  if (trace && order + 1 < trace.cumulative.length) {
    trace.base.style.strokeDashoffset = String(trace.total - trace.cumulative[order + 1]);
  }
}

// Draws a permanent trace through the path, revealed in step with the
// highlights. Once it is complete the wind and dust start flowing.
function renderTrace(path, stepMs) {
  const svg = byId("trace");
  svg.innerHTML = "";
  trace = null;
  if (!path.length) {
    return;
  }

  const gridRect = byId("grid").getBoundingClientRect();
  const coordinates = path.map((index) => {
    const rect = cells[index].getBoundingClientRect();
    return {
      x: Math.round(rect.left - gridRect.left + rect.width / 2),
      y: Math.round(rect.top - gridRect.top + rect.height / 2),
    };
  });
  const d = "M " + coordinates.map((point) => `${point.x} ${point.y}`).join(" L ");

  const cumulative = [0];
  let total = 0;
  for (let i = 1; i < coordinates.length; i++) {
    total += Math.hypot(coordinates[i].x - coordinates[i - 1].x, coordinates[i].y - coordinates[i - 1].y);
    cumulative.push(total);
  }

  svg.setAttribute("viewBox", `0 0 ${Math.round(gridRect.width)} ${Math.round(gridRect.height)}`);

  const base = document.createElementNS(SVG_NS, "path");
  base.setAttribute("d", d);
  base.setAttribute("class", "trace-base");
  svg.appendChild(base);

  trace = { base, cumulative, total };

  const reduceMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  if (reduceMotion) {
    base.style.strokeDashoffset = "0";
    return;
  }

  // stepMs === null means "re-render an already drawn path" (e.g. on resize).
  if (stepMs === null) {
    base.style.strokeDashoffset = "0";
    return;
  }

  base.style.strokeDasharray = String(total);
  base.style.strokeDashoffset = String(total);
  base.style.transition = `stroke-dashoffset ${stepMs}ms linear`;
  // Commit the hidden state so the very first segment animates too.
  void base.getBoundingClientRect();
}

function findPath(word) {
  const letters = Array.from(word);
  const board = cells.map((cell) => normalize(cell.value.trim()));
  const used = new Array(CELLS).fill(false);
  const path = [];

  function dfs(index, depth) {
    if (board[index] !== letters[depth]) {
      return false;
    }
    path.push(index);
    used[index] = true;

    if (depth === letters.length - 1) {
      return true;
    }

    const row = Math.floor(index / BOARD);
    const col = index % BOARD;
    for (let direction = 0; direction < 8; direction++) {
      const nextRow = row + DR[direction];
      const nextCol = col + DC[direction];
      if (nextRow < 0 || nextRow >= BOARD || nextCol < 0 || nextCol >= BOARD) {
        continue;
      }
      const next = nextRow * BOARD + nextCol;
      if (!used[next] && dfs(next, depth + 1)) {
        return true;
      }
    }

    path.pop();
    used[index] = false;
    return false;
  }

  for (let index = 0; index < CELLS; index++) {
    if (dfs(index, 0)) {
      return path.slice();
    }
  }
  return null;
}

function clearHighlight() {
  animationToken++;
  activePath = [];
  trace = null;
  byId("trace").innerHTML = "";
  for (const cell of cells) {
    cell.classList.remove("path");
    delete cell.dataset.order;
  }
  document.querySelectorAll(".word-chip.active").forEach((element) => element.classList.remove("active"));
}

function randomBoard() {
  const pool = [];
  for (const [character, weight] of Object.entries(WEIGHTS)) {
    for (let i = 0; i < weight; i++) {
      pool.push(character);
    }
  }
  for (const cell of cells) {
    cell.value = pool[Math.floor(Math.random() * pool.length)];
  }
  clearHighlight();
  setStatus("Wylosowano planszę.");
}

function clearAll() {
  for (const cell of cells) {
    cell.value = "";
  }
  words = [];
  byId("results").innerHTML = "";
  byId("summary").textContent = "";
  clearHighlight();
  setStatus("Gotowe.");
}

function setStatus(message) {
  byId("status").textContent = message;
}

function nextFrame() {
  return new Promise((resolve) => requestAnimationFrame(() => resolve()));
}
