#include "search.h"
#include "eval.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <cmath>

std::atomic<bool> g_uciStop{false};

// ------------------- Transposition Table -------------------

void TranspositionTable::resize(size_t mb) {
    size_t bytes = mb * 1024ULL * 1024ULL;
    size_t count = bytes / sizeof(TTEntry);
    size_t pow2 = 1;
    while (pow2 * 2 <= count) pow2 *= 2;
    if (pow2 == 0) pow2 = 1;
    table.assign(pow2, TTEntry());
    mask = pow2 - 1;
}

void TranspositionTable::clear() {
    std::fill(table.begin(), table.end(), TTEntry());
    currentAge = 0;
}

bool TranspositionTable::probe(uint64_t key, int depth, int alpha, int beta, int ply, TTEntry& out) {
    if (table.empty()) return false;
    TTEntry& e = table[key & mask];
    if (e.key != key) return false;
    out = e;
    return true;
}

void TranspositionTable::store(uint64_t key, int depth, Score score, TTFlag flag, const Move& best, int ply) {
    if (table.empty()) return;
    TTEntry& e = table[key & mask];
    // Always replace unless existing entry is deeper and same key from same age
    if (e.key != key || depth >= e.depth || e.age != currentAge) {
        // Adjust mate scores to be ply-independent when storing
        Score storeScore = score;
        if (isMateScore(storeScore)) {
            if (storeScore > 0) storeScore += ply; else storeScore -= ply;
        }
        e.key = key;
        e.depth = depth;
        e.score = storeScore;
        e.flag = flag;
        e.best = best;
        e.age = currentAge;
    }
}

// ------------------- Search -------------------

Search::Search() {
    tt.resize(64);
    std::memset(history, 0, sizeof(history));
}

void Search::newGame() {
    tt.clear();
    std::memset(history, 0, sizeof(history));
    for (int i = 0; i < MAX_PLY; i++) killers[i][0] = killers[i][1] = NULL_MOVE;
    gameHistory.clear();
}

bool Search::checkTime() {
    if (stopFlag) return true;
    if (allocatedMs <= 0) return false;
    if ((nodeCount & 2047) != 0) return false;
    auto now = std::chrono::steady_clock::now();
    int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    if (elapsed >= allocatedMs) {
        stopFlag = true;
        return true;
    }
    return false;
}

static int mvvLva(const Board& b, const Move& m) {
    static const int val[7] = {0,100,320,330,500,900,20000};
    Piece attacker = b.squares[m.from];
    int victimType = PAWN;
    if (m.flags & MF_EN_PASSANT) victimType = PAWN;
    else if (b.squares[m.to] != NO_PIECE) victimType = typeOf(b.squares[m.to]);
    int attackerType = typeOf(attacker);
    return val[victimType] * 10 - val[attackerType];
}

void Search::orderMoves(Board& b, std::vector<Move>& moves, const Move& ttMove, int ply) {
    std::vector<std::pair<int, Move>> scored;
    scored.reserve(moves.size());
    for (const Move& m : moves) {
        int score = 0;
        if (!ttMove.isNull() && m == ttMove) {
            score = 1000000;
        } else if (m.isCapture()) {
            score = 100000 + mvvLva(b, m);
        } else if (m.isPromotion()) {
            score = 90000 + m.promo;
        } else if (killers[ply][0] == m) {
            score = 80000;
        } else if (killers[ply][1] == m) {
            score = 79000;
        } else {
            score = history[b.sideToMove][m.from][m.to];
        }
        scored.emplace_back(score, m);
    }
    std::sort(scored.begin(), scored.end(), [](const std::pair<int,Move>& a, const std::pair<int,Move>& b2) {
        return a.first > b2.first;
    });
    for (size_t i = 0; i < moves.size(); i++) moves[i] = scored[i].second;
}

Score Search::quiescence(Board& b, Score alpha, Score beta, int ply) {
    nodeCount++;
    if ((nodeCount & 2047) == 0 && checkTime()) return alpha;

    Score standPat = Eval::evaluate(b);
    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;
    if (ply >= MAX_PLY - 1) return standPat;

    std::vector<Move> caps;
    b.generateCaptures(caps);

    std::vector<std::pair<int, Move>> scored;
    scored.reserve(caps.size());
    for (auto& m : caps) scored.emplace_back(mvvLva(b, m), m);
    std::sort(scored.begin(), scored.end(), [](auto& a, auto& c) { return a.first > c.first; });

    for (auto& [sc, m] : scored) {
        // Skip clearly bad captures (losing material) using simple SEE-ish filter: victim < attacker and not defended check skipped for speed
        Board copy = b;
        UndoInfo u;
        copy.makeMove(m, u);
        if (copy.squareAttacked(copy.kingSq[b.sideToMove], (Color)(b.sideToMove ^ 1))) continue; // illegal

        Score score = -quiescence(copy, -beta, -alpha, ply + 1);
        if (stopFlag) return alpha;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

bool Search::isRepetitionOrFifty(const Board& b, const std::vector<uint64_t>& hist) {
    if (b.halfmoveClock >= 100) return true;
    int count = 0;
    for (int i = (int)hist.size() - 2; i >= 0 && i >= (int)hist.size() - b.halfmoveClock - 1; i -= 2) {
        if (hist[i] == b.hash) {
            count++;
            if (count >= 1) return true; // twofold is enough to bail (root-safe, treat as draw to avoid repeats)
        }
    }
    return false;
}

Score Search::negamax(Board& b, int depth, Score alpha, Score beta, int ply, bool doNull) {
    pvLength[ply] = ply;

    if (ply > 0 && isRepetitionOrFifty(b, gameHistory)) return 0;

    bool isPV = (beta - alpha) > 1;

    if (depth <= 0) return quiescence(b, alpha, beta, ply);

    nodeCount++;
    if (checkTime()) return alpha;
    if (ply >= MAX_PLY - 1) return Eval::evaluate(b);

    Score origAlpha = alpha;
    Move ttMove = NULL_MOVE;
    TTEntry tte;
    if (tt.probe(b.hash, depth, (int)alpha, (int)beta, ply, tte)) {
        ttMove = tte.best;
        if (tte.depth >= depth && ply > 0) {
            Score s = tte.score;
            if (isMateScore(s)) { if (s > 0) s -= ply; else s += ply; }
            if (tte.flag == TT_EXACT) return s;
            if (tte.flag == TT_ALPHA && s <= alpha) return alpha;
            if (tte.flag == TT_BETA && s >= beta) return beta;
        }
    }

    bool inCheck = b.inCheck(b.sideToMove);

    // Null move pruning
    if (doNull && !isPV && !inCheck && depth >= 3 && ply > 0) {
        // Avoid null move in pure king+pawn endgames (zugzwang risk) - crude check via phase
        if (b.phase() > 2) {
            UndoInfo u;
            b.makeNullMove(u);
            int R = 2 + depth / 4;
            Score score = -negamax(b, depth - 1 - R, -beta, -beta + 1, ply + 1, false);
            b.unmakeNullMove(u);
            if (stopFlag) return alpha;
            if (score >= beta) {
                return beta;
            }
        }
    }

    std::vector<Move> moves;
    b.generatePseudoMoves(moves);
    orderMoves(b, moves, ttMove, ply);

    int legalCount = 0;
    Score bestScore = -SCORE_INFINITE;
    Move bestMove = NULL_MOVE;
    TTFlag flag = TT_ALPHA;

    for (size_t i = 0; i < moves.size(); i++) {
        const Move& m = moves[i];
        Board copy = b;
        UndoInfo u;
        copy.makeMove(m, u);
        if (copy.squareAttacked(copy.kingSq[b.sideToMove], (Color)(b.sideToMove ^ 1))) continue; // illegal
        legalCount++;

        gameHistory.push_back(copy.hash);

        Score score;
        int newDepth = depth - 1;
        bool givesCheck = copy.inCheck(copy.sideToMove);

        // Late move reductions
        int reduction = 0;
        if (depth >= 3 && legalCount > 3 && !m.isCapture() && !m.isPromotion() && !inCheck && !givesCheck) {
            reduction = 1 + (legalCount > 8 ? 1 : 0);
            if (reduction > newDepth) reduction = newDepth > 0 ? newDepth - 1 : 0;
        }

        if (legalCount == 1) {
            score = -negamax(copy, newDepth, -beta, -alpha, ply + 1, true);
        } else {
            score = -negamax(copy, newDepth - reduction, -alpha - 1, -alpha, ply + 1, true);
            if (score > alpha && (reduction > 0 || score < beta)) {
                score = -negamax(copy, newDepth, -beta, -alpha, ply + 1, true);
            }
        }

        gameHistory.pop_back();

        if (stopFlag) return alpha;

        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
            if (score > alpha) {
                alpha = score;
                flag = TT_EXACT;

                pvTable[ply][ply] = m;
                for (int n = ply + 1; n < pvLength[ply + 1]; n++) pvTable[ply][n] = pvTable[ply + 1][n];
                pvLength[ply] = pvLength[ply + 1];

                if (alpha >= beta) {
                    flag = TT_BETA;
                    if (!m.isCapture()) {
                        killers[ply][1] = killers[ply][0];
                        killers[ply][0] = m;
                        history[b.sideToMove][m.from][m.to] += depth * depth;
                    }
                    break;
                }
            }
        }
    }

    if (legalCount == 0) {
        if (inCheck) return matedIn(ply);
        return 0; // stalemate
    }

    tt.store(b.hash, depth, bestScore, flag, bestMove, ply);
    return bestScore;
}

void Search::go(Board& b, const SearchLimits& limits) {
    startTime = std::chrono::steady_clock::now();
    stopFlag = false;
    nodeCount = 0;
    tt.currentAge++;

    // Time management
    allocatedMs = -1;
    if (limits.movetimeMs > 0) {
        allocatedMs = limits.movetimeMs;
    } else if (!limits.infinite && limits.depth < 0) {
        int64_t myTime = (b.sideToMove == WHITE) ? limits.wtimeMs : limits.btimeMs;
        int64_t myInc = (b.sideToMove == WHITE) ? limits.wincMs : limits.bincMs;
        if (myTime > 0) {
            int movesToGo = limits.movesToGo > 0 ? limits.movesToGo : 30;
            allocatedMs = myTime / movesToGo + myInc * 3 / 4;
            if (allocatedMs > myTime - 50) allocatedMs = myTime - 50;
            if (allocatedMs < 10) allocatedMs = 10;
        }
    }

    int maxDepth = (limits.depth > 0) ? limits.depth : MAX_PLY - 1;

    std::vector<Move> rootMoves;
    b.generateLegalMoves(rootMoves);
    if (rootMoves.empty()) {
        std::cout << "bestmove 0000" << std::endl;
        return;
    }

    Move bestMoveOverall = rootMoves[0];
    Score bestScoreOverall = 0;

    gameHistory.push_back(b.hash);

    for (int depth = 1; depth <= maxDepth; depth++) {
        rootDepth = depth;
        pvLength[0] = 0;

        Score score = negamax(b, depth, -SCORE_INFINITE, SCORE_INFINITE, 0, true);

        if (stopFlag && depth > 1) break;

        if (pvLength[0] > 0) {
            bestMoveOverall = pvTable[0][0];
            bestScoreOverall = score;
        }

        auto now = std::chrono::steady_clock::now();
        int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

        std::cout << "info depth " << depth << " score ";
        if (isMateScore(score)) {
            int mateN = (score > 0) ? (SCORE_MATE - score + 1) / 2 : -(SCORE_MATE + score + 1) / 2;
            std::cout << "mate " << mateN;
        } else {
            std::cout << "cp " << score;
        }
        std::cout << " nodes " << nodeCount << " time " << elapsed
                   << " nps " << (elapsed > 0 ? (nodeCount * 1000 / elapsed) : nodeCount)
                   << " pv";
        for (int n = 0; n < pvLength[0]; n++) std::cout << " " << pvTable[0][n].toUCI();
        std::cout << std::endl;

        if (g_uciStop) { stopFlag = true; break; }

        if (allocatedMs > 0) {
            int64_t elapsedNow = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            if (elapsedNow >= allocatedMs * 0.6 && depth >= 4) {
                // Not enough time for a full extra iteration in many cases; still allow one more try,
                // real stopping enforced inside search via checkTime()
            }
        }
        if (stopFlag) break;
    }

    gameHistory.pop_back();

    std::cout << "bestmove " << bestMoveOverall.toUCI() << std::endl;
}
