#pragma once
#include "types.h"
#include <array>
#include <string>
#include <vector>
#include <cstdint>

// Castling rights bits
enum CastleRight { WK_CASTLE = 1, WQ_CASTLE = 2, BK_CASTLE = 4, BQ_CASTLE = 8 };

struct UndoInfo {
    Piece captured = NO_PIECE;
    int epSquare = SQ_NONE;
    int castleRights = 0;
    int halfmoveClock = 0;
    uint64_t hash = 0;
    int capturedSquare = SQ_NONE; // for en passant, differs from move.to
};

class Board {
public:
    std::array<Piece, 64> squares{};
    Color sideToMove = WHITE;
    int castleRights = 0; // combination of CastleRight bits
    int epSquare = SQ_NONE;
    int halfmoveClock = 0;
    int fullmoveNumber = 1;
    uint64_t hash = 0;

    // King locations cached for speed
    int kingSq[2] = { SQ_NONE, SQ_NONE };

    void setStartPos();
    bool setFromFEN(const std::string& fen);
    std::string toFEN() const;

    void computeHash();

    bool squareAttacked(int sq, Color byColor) const;
    bool inCheck(Color c) const { return squareAttacked(kingSq[c], (Color)(c ^ 1)); }

    // Generates pseudo-legal moves, then this function filters to legal ones.
    void generateLegalMoves(std::vector<Move>& out) const;
    void generatePseudoMoves(std::vector<Move>& out) const;
    void generateCaptures(std::vector<Move>& out) const; // for quiescence (captures + queen promos)

    void makeMove(const Move& m, UndoInfo& undo);
    void unmakeMove(const Move& m, const UndoInfo& undo);
    void makeNullMove(UndoInfo& undo);
    void unmakeNullMove(const UndoInfo& undo);

    bool isLegalAfter(const Move& m) const; // helper: after pseudo move applied hypothetically elsewhere

    int phase() const; // 0 = endgame .. 24 = full material, used for eval tapering

    void print() const;

private:
    void addMove(std::vector<Move>& out, int from, int to, int flags = MF_NONE, int promo = NO_PIECE_TYPE) const;
    void genPawnMoves(std::vector<Move>& out, bool capturesOnly) const;
    void genKnightMoves(std::vector<Move>& out, bool capturesOnly) const;
    void genBishopMoves(std::vector<Move>& out, bool capturesOnly) const;
    void genRookMoves(std::vector<Move>& out, bool capturesOnly) const;
    void genQueenMoves(std::vector<Move>& out, bool capturesOnly) const;
    void genKingMoves(std::vector<Move>& out, bool capturesOnly) const;
};

// Zobrist keys
namespace Zobrist {
    extern uint64_t pieceKeys[15][64];
    extern uint64_t castleKeys[16];
    extern uint64_t epFileKeys[8];
    extern uint64_t sideKey;
    void init();
}
