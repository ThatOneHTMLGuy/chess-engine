#include "board.h"
#include <iostream>
#include <chrono>

static uint64_t perft(Board& b, int depth) {
    if (depth == 0) return 1;
    std::vector<Move> moves;
    b.generateLegalMoves(moves);
    if (depth == 1) return moves.size();
    uint64_t nodes = 0;
    for (auto& m : moves) {
        Board copy = b;
        UndoInfo u;
        copy.makeMove(m, u);
        nodes += perft(copy, depth - 1);
    }
    return nodes;
}

static void divide(Board& b, int depth) {
    std::vector<Move> moves;
    b.generateLegalMoves(moves);
    uint64_t total = 0;
    for (auto& m : moves) {
        Board copy = b;
        UndoInfo u;
        copy.makeMove(m, u);
        uint64_t n = perft(copy, depth - 1);
        total += n;
        std::cout << m.toUCI() << ": " << n << std::endl;
    }
    std::cout << "Total: " << total << std::endl;
}

int main(int argc, char** argv) {
    Zobrist::init();
    Board b;

    std::string fen = "startpos";
    int depth = 5;
    bool doDivide = false;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-fen" && i + 1 < argc) { fen = argv[++i]; }
        else if (a == "-depth" && i + 1 < argc) { depth = std::stoi(argv[++i]); }
        else if (a == "-divide") { doDivide = true; }
    }

    if (fen == "startpos") b.setStartPos();
    else b.setFromFEN(fen);

    if (doDivide) {
        divide(b, depth);
        return 0;
    }

    for (int d = 1; d <= depth; d++) {
        auto t0 = std::chrono::steady_clock::now();
        uint64_t nodes = perft(b, d);
        auto t1 = std::chrono::steady_clock::now();
        double sec = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "perft(" << d << ") = " << nodes << "  (" << sec << "s, "
                  << (sec > 0 ? (uint64_t)(nodes / sec) : nodes) << " nps)" << std::endl;
    }
    return 0;
}
