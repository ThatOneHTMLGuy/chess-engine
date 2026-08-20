#include "eval.h"
#include <cstring>

namespace Eval {

// Material values (midgame); index by PieceType
const int pieceValue[7] = { 0, 100, 320, 330, 500, 900, 20000 };
static const int pieceValueEG[7] = { 0, 120, 300, 320, 530, 950, 20000 };

// Piece-square tables, from White's perspective, a1=index0 rank1..rank8
// (PeSTO-style tables — widely used public-domain-style values for tapered eval)
static const int pawnMG[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
     98, 134,  61,  95,  68, 126,  34, -11,
     -6,   7,  26,  31,  65,  56,  25, -20,
    -14,  13,   6,  21,  23,  12,  17, -23,
    -27,  -2,  -5,  12,  17,   6,  10, -25,
    -26,  -4,  -4, -10,   3,   3,  33, -12,
    -35,  -1, -20, -23, -15,  24,  38, -22,
      0,   0,   0,   0,   0,   0,   0,   0,
};
static const int pawnEG[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0,
};
static const int knightMG[64] = {
   -167, -89, -34, -49,  61, -97, -15,-107,
    -73, -41,  72,  36,  23,  62,   7, -17,
    -47,  60,  37,  65,  84, 129,  73,  44,
     -9,  17,  19,  53,  37,  69,  18,  22,
    -13,   4,  16,  13,  28,  19,  21,  -8,
    -23,  -9,  12,  10,  19,  17,  25, -16,
    -29, -53, -12,  -3,  -1,  18, -14, -19,
   -105, -21, -58, -33, -17, -28, -19, -23,
};
static const int knightEG[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64,
};
static const int bishopMG[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};
static const int bishopEG[64] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3, -13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17,
};
static const int rookMG[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};
static const int rookEG[64] = {
    13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
     7,  7,  7,  5,  4,  -3,  -5,  -3,
     4,  3, 13,  1,  2,   1,  -1,   2,
     3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20,
};
static const int queenMG[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};
static const int queenEG[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41,
};
static const int kingMG[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};
static const int kingEG[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43,
};

static const int* mgTables[7] = { nullptr, pawnMG, knightMG, bishopMG, rookMG, queenMG, kingMG };
static const int* egTables[7] = { nullptr, pawnEG, knightEG, bishopEG, rookEG, queenEG, kingEG };

static inline int mirror(int sq) { return sq ^ 56; } // flip rank for black

static int fileOfSq(int s) { return s & 7; }
static int rankOfSq(int s) { return s >> 3; }

Score evaluate(const Board& b) {
    int mg[2] = {0, 0};
    int eg[2] = {0, 0};
    int phase = 0;
    static const int phaseInc[7] = {0,0,1,1,2,4,0};

    int pawnFileCount[2][8] = {{0}};
    int bishopCount[2] = {0,0};

    for (int s = 0; s < 64; s++) {
        Piece p = b.squares[s];
        if (p == NO_PIECE) continue;
        Color c = colorOf(p);
        PieceType pt = typeOf(p);
        int idx = (c == WHITE) ? s : mirror(s);

        mg[c] += pieceValue[pt] + mgTables[pt][idx];
        eg[c] += pieceValueEG[pt] + egTables[pt][idx];
        phase += phaseInc[pt];

        if (pt == PAWN) pawnFileCount[c][fileOfSq(s)]++;
        if (pt == BISHOP) bishopCount[c]++;
    }

    // Bishop pair bonus
    if (bishopCount[WHITE] >= 2) { mg[WHITE] += 30; eg[WHITE] += 45; }
    if (bishopCount[BLACK] >= 2) { mg[BLACK] += 30; eg[BLACK] += 45; }

    // Doubled / isolated pawns (simple heuristic)
    for (int c = 0; c < 2; c++) {
        for (int f = 0; f < 8; f++) {
            if (pawnFileCount[c][f] >= 2) {
                int penalty = (pawnFileCount[c][f] - 1) * 12;
                mg[c] -= penalty; eg[c] -= penalty * 2;
            }
            if (pawnFileCount[c][f] > 0) {
                bool leftHas = (f > 0) && pawnFileCount[c][f-1] > 0;
                bool rightHas = (f < 7) && pawnFileCount[c][f+1] > 0;
                if (!leftHas && !rightHas) {
                    mg[c] -= 10; eg[c] -= 15;
                }
            }
        }
    }

    // Mobility: count pseudo-legal-ish attacked squares per side cheaply via simple approach
    // (kept light-weight for speed — count squares reachable by sliders/knights)
    {
        static const int knightOff[8][2] = { {1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2} };
        static const int bishopD[4][2] = { {1,1},{1,-1},{-1,1},{-1,-1} };
        static const int rookD[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
        for (int s = 0; s < 64; s++) {
            Piece p = b.squares[s];
            if (p == NO_PIECE) continue;
            Color c = colorOf(p);
            PieceType pt = typeOf(p);
            int f = fileOfSq(s), r = rankOfSq(s);
            int mob = 0;
            if (pt == KNIGHT) {
                for (auto& o : knightOff) {
                    int nf = f + o[0], nr = r + o[1];
                    if (onBoard(nf, nr)) {
                        Piece t = b.squares[makeSquare(nf, nr)];
                        if (t == NO_PIECE || colorOf(t) != c) mob++;
                    }
                }
                mg[c] += mob * 3; eg[c] += mob * 3;
            } else if (pt == BISHOP || pt == ROOK || pt == QUEEN) {
                const int (*dirs)[2] = (pt == BISHOP) ? bishopD : rookD;
                int ndirs = 4;
                for (int d = 0; d < ndirs; d++) {
                    int nf = f + dirs[d][0], nr = r + dirs[d][1];
                    while (onBoard(nf, nr)) {
                        Piece t = b.squares[makeSquare(nf, nr)];
                        mob++;
                        if (t != NO_PIECE) break;
                        nf += dirs[d][0]; nr += dirs[d][1];
                    }
                }
                if (pt == QUEEN) {
                    for (int d = 0; d < 4; d++) {
                        int nf = f + bishopD[d][0], nr = r + bishopD[d][1];
                        while (onBoard(nf, nr)) {
                            Piece t = b.squares[makeSquare(nf, nr)];
                            mob++;
                            if (t != NO_PIECE) break;
                            nf += bishopD[d][0]; nr += bishopD[d][1];
                        }
                    }
                    mg[c] += mob * 1; eg[c] += mob * 2;
                } else if (pt == BISHOP) {
                    mg[c] += mob * 3; eg[c] += mob * 3;
                } else {
                    mg[c] += mob * 2; eg[c] += mob * 4;
                }
            }
        }
    }

    // Passed pawns, rook on open/semi-open files, king safety (pawn shield)
    {
        // Build per-file, per-color pawn presence + rank bitmask info
        bool pawnOnFile[2][8] = {{false}};
        int mostAdvanced[2][8]; // for white: highest rank; for black: lowest rank
        for (int c = 0; c < 2; c++) for (int f = 0; f < 8; f++) mostAdvanced[c][f] = (c == WHITE) ? -1 : 8;

        for (int s = 0; s < 64; s++) {
            Piece p = b.squares[s];
            if (p == NO_PIECE || typeOf(p) != PAWN) continue;
            Color c = colorOf(p);
            int f = fileOfSq(s), r = rankOfSq(s);
            pawnOnFile[c][f] = true;
            if (c == WHITE) { if (r > mostAdvanced[c][f]) mostAdvanced[c][f] = r; }
            else { if (r < mostAdvanced[c][f]) mostAdvanced[c][f] = r; }
        }

        static const int passedBonusMG[8] = {0, 5, 10, 20, 35, 55, 80, 0};
        static const int passedBonusEG[8] = {0, 10, 20, 35, 60, 100, 150, 0};

        for (int s = 0; s < 64; s++) {
            Piece p = b.squares[s];
            if (p == NO_PIECE || typeOf(p) != PAWN) continue;
            Color c = colorOf(p);
            Color opp = (Color)(c ^ 1);
            int f = fileOfSq(s), r = rankOfSq(s);
            (void)mostAdvanced;
            // Passed pawn check: no enemy pawn on same or adjacent file that is further advanced
            bool passed = true;
            for (int df = -1; df <= 1; df++) {
                int nf = f + df;
                if (nf < 0 || nf > 7 || !pawnOnFile[opp][nf]) continue;
                for (int s2 = 0; s2 < 64; s2++) {
                    Piece p2 = b.squares[s2];
                    if (p2 == NO_PIECE || typeOf(p2) != PAWN || colorOf(p2) != opp || fileOfSq(s2) != nf) continue;
                    int r2 = rankOfSq(s2);
                    if ((c == WHITE && r2 > r) || (c == BLACK && r2 < r)) { passed = false; break; }
                }
                if (!passed) break;
            }
            if (passed) {
                int rankIdx = (c == WHITE) ? r : 7 - r;
                mg[c] += passedBonusMG[rankIdx];
                eg[c] += passedBonusEG[rankIdx];
            }
        }

        // Rook on open / semi-open files
        for (int s = 0; s < 64; s++) {
            Piece p = b.squares[s];
            if (p == NO_PIECE || typeOf(p) != ROOK) continue;
            Color c = colorOf(p);
            int f = fileOfSq(s);
            bool ownPawn = pawnOnFile[c][f];
            bool oppPawn = pawnOnFile[c ^ 1][f];
            if (!ownPawn && !oppPawn) { mg[c] += 20; eg[c] += 10; }
            else if (!ownPawn) { mg[c] += 10; eg[c] += 5; }
        }

        // King safety: pawn shield in front of castled king (midgame only, scaled by phase later)
        for (int c = 0; c < 2; c++) {
            int ks = b.kingSq[c];
            if (ks == SQ_NONE) continue;
            int kf = fileOfSq(ks), kr = rankOfSq(ks);
            if ((c == WHITE && kf <= 2) || (c == BLACK && kf <= 2) ||
                (c == WHITE && kf >= 5) || (c == BLACK && kf >= 5)) {
                int shieldRank = (c == WHITE) ? kr + 1 : kr - 1;
                int shield = 0;
                for (int df = -1; df <= 1; df++) {
                    int f = kf + df;
                    if (f < 0 || f > 7 || shieldRank < 0 || shieldRank > 7) continue;
                    Piece sp = b.squares[makeSquare(f, shieldRank)];
                    if (sp != NO_PIECE && typeOf(sp) == PAWN && colorOf(sp) == (Color)c) shield++;
                }
                mg[c] += shield * 8;
            }
        }
    }

    if (phase > 24) phase = 24;
    int mgScore = mg[WHITE] - mg[BLACK];
    int egScore = eg[WHITE] - eg[BLACK];
    int taperedScore = (mgScore * phase + egScore * (24 - phase)) / 24;

    Score score = taperedScore;
    return (b.sideToMove == WHITE) ? score : -score;
}

} // namespace Eval
