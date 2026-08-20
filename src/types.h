#pragma once
#include <cstdint>
#include <string>
#include <vector>

// ============================================================
//  Basic type definitions for the engine
// ============================================================

enum Color { WHITE = 0, BLACK = 1, COLOR_NB = 2 };

enum PieceType { NO_PIECE_TYPE = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6, PIECE_TYPE_NB = 7 };

// Piece encoding: color*8 + type, so white pawn = 1, black pawn = 9, etc.
// 0 = empty square
enum Piece {
    NO_PIECE = 0,
    W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
    B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14,
    PIECE_NB = 15
};

inline Piece makePiece(Color c, PieceType pt) { return (Piece)(c * 8 + pt); }
inline PieceType typeOf(Piece p) { return (PieceType)(p & 7); }
inline Color colorOf(Piece p) { return (Color)(p >> 3); }
inline bool isWhite(Piece p) { return p != NO_PIECE && colorOf(p) == WHITE; }
inline bool isBlack(Piece p) { return p != NO_PIECE && colorOf(p) == BLACK; }

// Squares: 0=a1 .. 7=h1, 8=a2 .. 63=h8  (little-endian rank-file mapping)
enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    SQ_NONE = 64, SQ_NB = 64
};

inline int fileOf(int sq) { return sq & 7; }
inline int rankOf(int sq) { return sq >> 3; }
inline int makeSquare(int file, int rank) { return rank * 8 + file; }
inline bool onBoard(int file, int rank) { return file >= 0 && file < 8 && rank >= 0 && rank < 8; }

std::string squareToString(int sq);
int stringToSquare(const std::string& s);

// Move encoding (16-bit-ish packed into int32):
// bits 0-5: from, 6-11: to, 12-14: promotion piece type, 15-17: flags
enum MoveFlag {
    MF_NONE = 0,
    MF_CAPTURE = 1,
    MF_DOUBLE_PUSH = 2,
    MF_EN_PASSANT = 4,
    MF_CASTLE_K = 8,
    MF_CASTLE_Q = 16,
    MF_PROMOTION = 32
};

struct Move {
    int16_t from = 0;
    int16_t to = 0;
    int8_t promo = NO_PIECE_TYPE; // promotion piece type
    int32_t flags = 0;

    Move() {}
    Move(int f, int t, int pr = NO_PIECE_TYPE, int fl = MF_NONE) : from(f), to(t), promo(pr), flags(fl) {}

    bool isCapture() const { return flags & (MF_CAPTURE | MF_EN_PASSANT); }
    bool isCastle() const { return flags & (MF_CASTLE_K | MF_CASTLE_Q); }
    bool isPromotion() const { return flags & MF_PROMOTION; }
    bool isNull() const { return from == 0 && to == 0 && flags == 0 && promo == NO_PIECE_TYPE; }

    bool operator==(const Move& o) const {
        return from == o.from && to == o.to && promo == o.promo && flags == o.flags;
    }
    bool operator!=(const Move& o) const { return !(*this == o); }

    std::string toUCI() const;
};

const Move NULL_MOVE = Move();

typedef int64_t Score;
const Score SCORE_INFINITE = 32000;
const Score SCORE_MATE = 31000;
const Score SCORE_NONE = 32001;
const int MAX_PLY = 128;

inline Score mateIn(int ply) { return SCORE_MATE - ply; }
inline Score matedIn(int ply) { return -SCORE_MATE + ply; }
inline bool isMateScore(Score s) { return s >= SCORE_MATE - MAX_PLY || s <= -SCORE_MATE + MAX_PLY; }
