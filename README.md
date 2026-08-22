# A UCI chess engine in C++

A from-scratch chess engine that speaks the **UCI (Universal Chess Interface)**
protocol — the same protocol Stockfish uses. This is how real engines are
"plugged into" a GUI: you don't link a library into the GUI's source code,
you point the GUI at a compiled executable, and the GUI talks to it over
stdin/stdout.

## What's inside

- **Board**: 8x8 array representation, full legal move generation (castling,
  en passant, promotions, pins/checks handled correctly), Zobrist hashing.
  Verified against the standard `perft` test suite — see below.
- **Evaluation**: tapered midgame/endgame evaluation (PeSTO-style piece-square
  tables), material, mobility, bishop pair, doubled/isolated pawns, passed
  pawns, rook on open/semi-open files, king pawn-shield safety.
- **Search**: iterative deepening, negamax with alpha-beta, a transposition
  table (Zobrist-keyed), principal-variation search, null-move pruning, late
  move reductions, killer moves + history heuristic move ordering,
  quiescence search to avoid the horizon effect, basic time management.
- **UCI**: `uci`, `isready`, `ucinewgame`, `position [startpos|fen ...] [moves ...]`,
  `go [movetime|wtime/btime/winc/binc|depth|infinite]`, `stop`, `setoption
  name Hash value N`, `quit`, plus `d`/`eval` debug commands.

This is a solid hobby-strength engine (rough estimate: 1800–2200 Elo range
depending on time control) — not literally Stockfish-strength (Stockfish is
the product of over a decade of work by hundreds of contributors, NNUE
neural-network evaluation, and industrial-scale testing). But it's built the
same way real engines are: correct legal move generation, alpha-beta with
modern pruning, and a real evaluation function.

## Building

Requires a C++17 compiler.

```bash
make
```

This produces the `chess-engine` executable. There's also a `perft` test binary
you can build for validating move generation on any position:

```bash
g++ -std=c++17 -O3 -march=native -o perft src/perft.cpp src/board.cpp
./perft -depth 5                      # from the starting position
./perft -fen "<FEN>" -depth 4 -divide # per-move breakdown for a custom position
```

If `-march=native` isn't supported on your target machine (e.g. you're
compiling on one machine and running on another), edit the `Makefile` and
drop that flag.

## Using it in a GUI

Any UCI-compatible GUI will work — this is the standard way to "import" a
C++ chess engine:

- **Arena** (free, Windows/Linux) — Engines menu → Install New Engine → point
  it at `chess-engine`.
- **CuteChess** (free, cross-platform, good for engine-vs-engine testing) —
  Settings → Engines → Add → point at `chess-engine`.
- **En Croissant** (free, modern cross-platform GUI).
- **ChessBase / Fritz** (Windows) — Engine → Create UCI Engine.
- **BanksiaGUI**, **XBoard/WinBoard** (via polyglot adapter), etc.

You can also drive it by hand from a terminal to sanity-check it — paste
these lines in:

```
uci
isready
position startpos
go movetime 3000
```

It will print `info depth ... score ... pv ...` lines as it searches and
finish with `bestmove <move>`.

## Options

- `setoption name Hash value <MB>` — transposition table size (default 64MB).

## Files

```
src/types.h     Move/Square/Piece/Score types
src/board.h/.cpp   Board representation, legal move generation, make/unmake
src/eval.h/.cpp    Static evaluation function
src/search.h/.cpp  Alpha-beta search, transposition table, time management
src/main.cpp       UCI protocol loop
src/perft.cpp      Standalone move-generation correctness/speed tester
```

## Verifying correctness yourself

Move generation is the part of a chess engine where subtle bugs are most
costly (an illegal move or a missed check means the engine loses instantly
against anything competent). The included `perft.cpp` counts leaf nodes at a
given depth from a position — a mismatch against known-correct values
means there's a move generation bug. This engine's output has been checked
against the standard reference positions:

| Position  | Depth | Expected   | This engine |
|-----------|-------|------------|--------------|
| Startpos  | 6     | 119,060,324 | matches |
| Kiwipete  | 4     | 4,085,603   | matches |
| Position 3| 5     | 674,624     | matches |
| Position 4| 4     | 422,333     | matches |
| Position 5| 4     | 2,103,487   | matches |

("Kiwipete" and "Position 3–5" are the standard perft-testing FENs from the
chess programming community, used precisely because they stress castling,
en passant, promotions, and discovered checks.)

## Ideas for pushing it further

- Bitboards + magic bitboards for sliding-piece attacks (this version uses
  ray-casting, which is simpler/more obviously-correct but slower).
- A real static-exchange-evaluation (SEE) for capture ordering/pruning.
- Multi-threading (Lazy SMP).
- An opening book and/or endgame tablebases (Syzygy).
- NNUE-style learned evaluation instead of hand-tuned PSTs — this is the
  single biggest thing separating hobby engines from Stockfish-class ones.
