#pragma once
#include "board.h"
#include "types.h"
#include <chrono>
#include <atomic>

enum TTFlag { TT_EXACT, TT_ALPHA, TT_BETA };

struct TTEntry {
    uint64_t key = 0;
    int depth = -1;
    Score score = 0;
    TTFlag flag = TT_EXACT;
    Move best;
    uint8_t age = 0;
};

class TranspositionTable {
public:
    void resize(size_t mb);
    void clear();
    bool probe(uint64_t key, int depth, int alpha, int beta, int ply, TTEntry& out);
    void store(uint64_t key, int depth, Score score, TTFlag flag, const Move& best, int ply);
    std::vector<TTEntry> table;
    size_t mask = 0;
    uint8_t currentAge = 0;
};

struct SearchLimits {
    int64_t movetimeMs = -1;
    int64_t wtimeMs = -1, btimeMs = -1, wincMs = 0, bincMs = 0;
    int movesToGo = 0;
    int depth = -1;
    int64_t nodes = -1;
    bool infinite = false;
};

class Search {
public:
    Search();
    void newGame();
    void go(Board& b, const SearchLimits& limits);
    void stop() { stopFlag = true; }
    TranspositionTable tt;

private:
    std::chrono::steady_clock::time_point startTime;
    int64_t allocatedMs = 0;
    std::atomic<bool> stopFlag{false};
    uint64_t nodeCount = 0;
    int rootDepth = 0;

    Move killers[MAX_PLY][2];
    int history[2][64][64];
    Move pvTable[MAX_PLY][MAX_PLY];
    int pvLength[MAX_PLY];

    Score negamax(Board& b, int depth, Score alpha, Score beta, int ply, bool doNull);
    Score quiescence(Board& b, Score alpha, Score beta, int ply);
    void orderMoves(Board& b, std::vector<Move>& moves, const Move& ttMove, int ply);
    bool checkTime();
    bool isRepetitionOrFifty(const Board& b, const std::vector<uint64_t>& history);

public:
    std::vector<uint64_t> gameHistory; // hash history for repetition detection
};

extern std::atomic<bool> g_uciStop;
