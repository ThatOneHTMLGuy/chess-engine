#include "board.h"
#include <sstream>
#include <cstdio>
#include <random>
#include <iostream>

namespace Zobrist {
    uint64_t pieceKeys[15][64];
    uint64_t castleKeys[16];
    uint64_t epFileKeys[8];
    uint64_t sideKey;

    void init() {
        std::mt19937_64 rng(0x9E3779B97F4A7C15ULL);
        for (int p = 0; p < 15; p++)
            for (int s = 0; s < 64; s++)
                pieceKeys[p][s] = rng();
        for (int i = 0; i < 16; i++) castleKeys[i] = rng();
        for (int i = 0; i < 8; i++) epFileKeys[i] = rng();
        sideKey = rng();
    }
}

std::string squareToString(int sq) {
    if (sq == SQ_NONE) return "-";
    std::string s;
    s += char('a' + fileOf(sq));
    s += char('1' + rankOf(sq));
    return s;
}

int stringToSquare(const std::string& s) {
    if (s.size() < 2 || s == "-") return SQ_NONE;
    int file = s[0] - 'a';
    int rank = s[1] - '1';
    if (!onBoard(file, rank)) return SQ_NONE;
    return makeSquare(file, rank);
}

std::string Move::toUCI() const {
    std::string s = squareToString(from) + squareToString(to);
    if (isPromotion()) {
        char c = ' ';
        switch (promo) {
            case QUEEN: c = 'q'; break;
            case ROOK: c = 'r'; break;
            case BISHOP: c = 'b'; break;
            case KNIGHT: c = 'n'; break;
            default: break;
        }
        s += c;
    }
    return s;
}

void Board::computeHash() {
    hash = 0;
    for (int s = 0; s < 64; s++) {
        if (squares[s] != NO_PIECE) hash ^= Zobrist::pieceKeys[squares[s]][s];
    }
    hash ^= Zobrist::castleKeys[castleRights];
    if (epSquare != SQ_NONE) hash ^= Zobrist::epFileKeys[fileOf(epSquare)];
    if (sideToMove == BLACK) hash ^= Zobrist::sideKey;
}

void Board::setStartPos() {
    setFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

static Piece charToPiece(char c) {
    Color col = isupper((unsigned char)c) ? WHITE : BLACK;
    char lc = tolower(c);
    PieceType pt = NO_PIECE_TYPE;
    switch (lc) {
        case 'p': pt = PAWN; break;
        case 'n': pt = KNIGHT; break;
        case 'b': pt = BISHOP; break;
        case 'r': pt = ROOK; break;
        case 'q': pt = QUEEN; break;
        case 'k': pt = KING; break;
    }
    return makePiece(col, pt);
}

static char pieceToChar(Piece p) {
    if (p == NO_PIECE) return '.';
    char c;
    switch (typeOf(p)) {
        case PAWN: c = 'p'; break;
        case KNIGHT: c = 'n'; break;
        case BISHOP: c = 'b'; break;
        case ROOK: c = 'r'; break;
        case QUEEN: c = 'q'; break;
        case KING: c = 'k'; break;
        default: c = '.'; break;
    }
    if (colorOf(p) == WHITE) c = toupper(c);
    return c;
}

bool Board::setFromFEN(const std::string& fen) {
    squares.fill(NO_PIECE);
    castleRights = 0;
    epSquare = SQ_NONE;
    halfmoveClock = 0;
    fullmoveNumber = 1;
    kingSq[0] = kingSq[1] = SQ_NONE;

    std::istringstream iss(fen);
    std::string boardPart, sidePart, castlePart, epPart;
    std::string halfmovePart = "0", fullmovePart = "1";
    iss >> boardPart >> sidePart >> castlePart >> epPart >> halfmovePart >> fullmovePart;
    if (boardPart.empty()) return false;

    int rank = 7, file = 0;
    for (char c : boardPart) {
        if (c == '/') { rank--; file = 0; }
        else if (isdigit((unsigned char)c)) { file += c - '0'; }
        else {
            Piece p = charToPiece(c);
            int sq = makeSquare(file, rank);
            squares[sq] = p;
            if (typeOf(p) == KING) kingSq[colorOf(p)] = sq;
            file++;
        }
    }

    sideToMove = (sidePart == "b") ? BLACK : WHITE;

    for (char c : castlePart) {
        switch (c) {
            case 'K': castleRights |= WK_CASTLE; break;
            case 'Q': castleRights |= WQ_CASTLE; break;
            case 'k': castleRights |= BK_CASTLE; break;
            case 'q': castleRights |= BQ_CASTLE; break;
        }
    }

    epSquare = stringToSquare(epPart);

    try { halfmoveClock = std::stoi(halfmovePart); } catch (...) { halfmoveClock = 0; }
    try { fullmoveNumber = std::stoi(fullmovePart); } catch (...) { fullmoveNumber = 1; }

    computeHash();
    return true;
}

std::string Board::toFEN() const {
    std::ostringstream oss;
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            Piece p = squares[makeSquare(file, rank)];
            if (p == NO_PIECE) empty++;
            else {
                if (empty > 0) { oss << empty; empty = 0; }
                oss << pieceToChar(p);
            }
        }
        if (empty > 0) oss << empty;
        if (rank > 0) oss << '/';
    }
    oss << ' ' << (sideToMove == WHITE ? 'w' : 'b') << ' ';
    std::string cr;
    if (castleRights & WK_CASTLE) cr += 'K';
    if (castleRights & WQ_CASTLE) cr += 'Q';
    if (castleRights & BK_CASTLE) cr += 'k';
    if (castleRights & BQ_CASTLE) cr += 'q';
    oss << (cr.empty() ? "-" : cr) << ' ';
    oss << squareToString(epSquare) << ' ' << halfmoveClock << ' ' << fullmoveNumber;
    return oss.str();
}

void Board::print() const {
    for (int rank = 7; rank >= 0; rank--) {
        printf("%d  ", rank + 1);
        for (int file = 0; file < 8; file++) {
            printf("%c ", pieceToChar(squares[makeSquare(file, rank)]));
        }
        printf("\n");
    }
    printf("   a b c d e f g h\n");
    printf("FEN: %s\n", toFEN().c_str());
}

// ------------------- Attack detection -------------------

static const int knightOffsets[8][2] = { {1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2} };
static const int kingOffsets[8][2] = { {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1} };
static const int bishopDirs[4][2] = { {1,1},{1,-1},{-1,1},{-1,-1} };
static const int rookDirs[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };

bool Board::squareAttacked(int sq, Color byColor) const {
    int sf = fileOf(sq), sr = rankOf(sq);

    // Pawn attacks: a pawn of byColor attacks sq if it sits diagonally "in front" (from its own perspective)
    int pawnRankDir = (byColor == WHITE) ? -1 : 1; // where an attacking pawn would be relative to sq
    for (int df : {-1, 1}) {
        int f = sf + df, r = sr + pawnRankDir;
        if (onBoard(f, r)) {
            Piece p = squares[makeSquare(f, r)];
            if (p != NO_PIECE && colorOf(p) == byColor && typeOf(p) == PAWN) return true;
        }
    }

    // Knight
    for (auto& o : knightOffsets) {
        int f = sf + o[0], r = sr + o[1];
        if (onBoard(f, r)) {
            Piece p = squares[makeSquare(f, r)];
            if (p != NO_PIECE && colorOf(p) == byColor && typeOf(p) == KNIGHT) return true;
        }
    }

    // King
    for (auto& o : kingOffsets) {
        int f = sf + o[0], r = sr + o[1];
        if (onBoard(f, r)) {
            Piece p = squares[makeSquare(f, r)];
            if (p != NO_PIECE && colorOf(p) == byColor && typeOf(p) == KING) return true;
        }
    }

    // Bishop / Queen diagonals
    for (auto& d : bishopDirs) {
        int f = sf + d[0], r = sr + d[1];
        while (onBoard(f, r)) {
            Piece p = squares[makeSquare(f, r)];
            if (p != NO_PIECE) {
                if (colorOf(p) == byColor && (typeOf(p) == BISHOP || typeOf(p) == QUEEN)) return true;
                break;
            }
            f += d[0]; r += d[1];
        }
    }

    // Rook / Queen straight
    for (auto& d : rookDirs) {
        int f = sf + d[0], r = sr + d[1];
        while (onBoard(f, r)) {
            Piece p = squares[makeSquare(f, r)];
            if (p != NO_PIECE) {
                if (colorOf(p) == byColor && (typeOf(p) == ROOK || typeOf(p) == QUEEN)) return true;
                break;
            }
            f += d[0]; r += d[1];
        }
    }

    return false;
}

// ------------------- Move generation -------------------

void Board::addMove(std::vector<Move>& out, int from, int to, int flags, int promo) const {
    out.emplace_back(from, to, promo, flags);
}

void Board::genPawnMoves(std::vector<Move>& out, bool capturesOnly) const {
    Color us = sideToMove;
    Color them = (Color)(us ^ 1);
    int dir = (us == WHITE) ? 1 : -1;
    int startRank = (us == WHITE) ? 1 : 6;
    int promoRank = (us == WHITE) ? 7 : 0;

    for (int sq = 0; sq < 64; sq++) {
        Piece p = squares[sq];
        if (p == NO_PIECE || colorOf(p) != us || typeOf(p) != PAWN) continue;
        int f = fileOf(sq), r = rankOf(sq);

        // Single push
        int r1 = r + dir;
        if (!capturesOnly && onBoard(f, r1) && squares[makeSquare(f, r1)] == NO_PIECE) {
            int to = makeSquare(f, r1);
            if (r1 == promoRank) {
                for (int pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                    addMove(out, sq, to, MF_PROMOTION, pt);
            } else {
                addMove(out, sq, to, MF_NONE);
                // Double push
                int r2 = r + 2 * dir;
                if (r == startRank && squares[makeSquare(f, r2)] == NO_PIECE) {
                    addMove(out, sq, makeSquare(f, r2), MF_DOUBLE_PUSH);
                }
            }
        }

        // Captures
        for (int df : {-1, 1}) {
            int cf = f + df, cr = r + dir;
            if (!onBoard(cf, cr)) continue;
            int to = makeSquare(cf, cr);
            Piece target = squares[to];
            if (target != NO_PIECE && colorOf(target) == them) {
                if (cr == promoRank) {
                    for (int pt : {QUEEN, ROOK, BISHOP, KNIGHT})
                        addMove(out, sq, to, MF_CAPTURE | MF_PROMOTION, pt);
                } else {
                    addMove(out, sq, to, MF_CAPTURE);
                }
            } else if (to == epSquare && epSquare != SQ_NONE) {
                addMove(out, sq, to, MF_CAPTURE | MF_EN_PASSANT);
            }
        }
    }
}

void Board::genKnightMoves(std::vector<Move>& out, bool capturesOnly) const {
    Color us = sideToMove;
    for (int sq = 0; sq < 64; sq++) {
        Piece p = squares[sq];
        if (p == NO_PIECE || colorOf(p) != us || typeOf(p) != KNIGHT) continue;
        int f = fileOf(sq), r = rankOf(sq);
        for (auto& o : knightOffsets) {
            int nf = f + o[0], nr = r + o[1];
            if (!onBoard(nf, nr)) continue;
            int to = makeSquare(nf, nr);
            Piece target = squares[to];
            if (target == NO_PIECE) {
                if (!capturesOnly) addMove(out, sq, to);
            } else if (colorOf(target) != us) {
                addMove(out, sq, to, MF_CAPTURE);
            }
        }
    }
}

static void genSliderMoves(const Board& b, std::vector<Move>& out, PieceType pt, const int dirs[4][2], bool capturesOnly, Color us) {
    for (int sq = 0; sq < 64; sq++) {
        Piece p = b.squares[sq];
        if (p == NO_PIECE || colorOf(p) != us || typeOf(p) != pt) continue;
        int f = fileOf(sq), r = rankOf(sq);
        for (int di = 0; di < 4; di++) {
            int df = dirs[di][0], dr = dirs[di][1];
            int nf = f + df, nr = r + dr;
            while (onBoard(nf, nr)) {
                int to = makeSquare(nf, nr);
                Piece target = b.squares[to];
                if (target == NO_PIECE) {
                    if (!capturesOnly) out.emplace_back(sq, to);
                } else {
                    if (colorOf(target) != us) out.emplace_back(sq, to, NO_PIECE_TYPE, MF_CAPTURE);
                    break;
                }
                nf += df; nr += dr;
            }
        }
    }
}

void Board::genBishopMoves(std::vector<Move>& out, bool capturesOnly) const {
    genSliderMoves(*this, out, BISHOP, bishopDirs, capturesOnly, sideToMove);
}
void Board::genRookMoves(std::vector<Move>& out, bool capturesOnly) const {
    genSliderMoves(*this, out, ROOK, rookDirs, capturesOnly, sideToMove);
}
void Board::genQueenMoves(std::vector<Move>& out, bool capturesOnly) const {
    genSliderMoves(*this, out, QUEEN, bishopDirs, capturesOnly, sideToMove);
    genSliderMoves(*this, out, QUEEN, rookDirs, capturesOnly, sideToMove);
}

void Board::genKingMoves(std::vector<Move>& out, bool capturesOnly) const {
    Color us = sideToMove;
    int sq = kingSq[us];
    if (sq == SQ_NONE) return;
    int f = fileOf(sq), r = rankOf(sq);
    for (auto& o : kingOffsets) {
        int nf = f + o[0], nr = r + o[1];
        if (!onBoard(nf, nr)) continue;
        int to = makeSquare(nf, nr);
        Piece target = squares[to];
        if (target == NO_PIECE) {
            if (!capturesOnly) addMove(out, sq, to);
        } else if (colorOf(target) != us) {
            addMove(out, sq, to, MF_CAPTURE);
        }
    }

    if (capturesOnly) return;

    Color them = (Color)(us ^ 1);
    // Castling
    if (us == WHITE && sq == E1) {
        if ((castleRights & WK_CASTLE) && squares[F1] == NO_PIECE && squares[G1] == NO_PIECE &&
            squares[H1] == W_ROOK &&
            !squareAttacked(E1, them) && !squareAttacked(F1, them) && !squareAttacked(G1, them)) {
            addMove(out, E1, G1, MF_CASTLE_K);
        }
        if ((castleRights & WQ_CASTLE) && squares[D1] == NO_PIECE && squares[C1] == NO_PIECE && squares[B1] == NO_PIECE &&
            squares[A1] == W_ROOK &&
            !squareAttacked(E1, them) && !squareAttacked(D1, them) && !squareAttacked(C1, them)) {
            addMove(out, E1, C1, MF_CASTLE_Q);
        }
    } else if (us == BLACK && sq == E8) {
        if ((castleRights & BK_CASTLE) && squares[F8] == NO_PIECE && squares[G8] == NO_PIECE &&
            squares[H8] == B_ROOK &&
            !squareAttacked(E8, them) && !squareAttacked(F8, them) && !squareAttacked(G8, them)) {
            addMove(out, E8, G8, MF_CASTLE_K);
        }
        if ((castleRights & BQ_CASTLE) && squares[D8] == NO_PIECE && squares[C8] == NO_PIECE && squares[B8] == NO_PIECE &&
            squares[A8] == B_ROOK &&
            !squareAttacked(E8, them) && !squareAttacked(D8, them) && !squareAttacked(C8, them)) {
            addMove(out, E8, C8, MF_CASTLE_Q);
        }
    }
}

void Board::generatePseudoMoves(std::vector<Move>& out) const {
    out.reserve(48);
    genPawnMoves(out, false);
    genKnightMoves(out, false);
    genBishopMoves(out, false);
    genRookMoves(out, false);
    genQueenMoves(out, false);
    genKingMoves(out, false);
}

void Board::generateCaptures(std::vector<Move>& out) const {
    out.reserve(24);
    genPawnMoves(out, true);
    genKnightMoves(out, true);
    genBishopMoves(out, true);
    genRookMoves(out, true);
    genQueenMoves(out, true);
    genKingMoves(out, true);
}

void Board::generateLegalMoves(std::vector<Move>& out) const {
    std::vector<Move> pseudo;
    generatePseudoMoves(pseudo);
    out.reserve(pseudo.size());
    Board copy;
    for (const Move& m : pseudo) {
        copy = *this;
        UndoInfo u;
        copy.makeMove(m, u);
        if (!copy.squareAttacked(copy.kingSq[sideToMove], (Color)(sideToMove ^ 1))) {
            out.push_back(m);
        }
    }
}

// ------------------- Make / Unmake -------------------

void Board::makeMove(const Move& m, UndoInfo& undo) {
    undo.epSquare = epSquare;
    undo.castleRights = castleRights;
    undo.halfmoveClock = halfmoveClock;
    undo.hash = hash;
    undo.captured = NO_PIECE;
    undo.capturedSquare = SQ_NONE;

    Piece moving = squares[m.from];
    Color us = sideToMove;
    Color them = (Color)(us ^ 1);

    // Remove ep hash before modifying
    if (epSquare != SQ_NONE) hash ^= Zobrist::epFileKeys[fileOf(epSquare)];
    epSquare = SQ_NONE;

    hash ^= Zobrist::castleKeys[castleRights];

    if (m.flags & MF_EN_PASSANT) {
        int capSq = makeSquare(fileOf(m.to), rankOf(m.from));
        undo.captured = squares[capSq];
        undo.capturedSquare = capSq;
        hash ^= Zobrist::pieceKeys[squares[capSq]][capSq];
        squares[capSq] = NO_PIECE;
    } else if (squares[m.to] != NO_PIECE) {
        undo.captured = squares[m.to];
        undo.capturedSquare = m.to;
        hash ^= Zobrist::pieceKeys[squares[m.to]][m.to];
    }

    // Move piece
    hash ^= Zobrist::pieceKeys[moving][m.from];
    squares[m.from] = NO_PIECE;

    Piece placed = moving;
    if (m.flags & MF_PROMOTION) {
        placed = makePiece(us, (PieceType)m.promo);
    }
    squares[m.to] = placed;
    hash ^= Zobrist::pieceKeys[placed][m.to];

    if (typeOf(moving) == KING) {
        kingSq[us] = m.to;
        if (us == WHITE) castleRights &= ~(WK_CASTLE | WQ_CASTLE);
        else castleRights &= ~(BK_CASTLE | BQ_CASTLE);
    }

    // Castling rook move
    if (m.flags & MF_CASTLE_K) {
        int rf = (us == WHITE) ? H1 : H8;
        int rt = (us == WHITE) ? F1 : F8;
        hash ^= Zobrist::pieceKeys[squares[rf]][rf];
        squares[rt] = squares[rf];
        squares[rf] = NO_PIECE;
        hash ^= Zobrist::pieceKeys[squares[rt]][rt];
    } else if (m.flags & MF_CASTLE_Q) {
        int rf = (us == WHITE) ? A1 : A8;
        int rt = (us == WHITE) ? D1 : D8;
        hash ^= Zobrist::pieceKeys[squares[rf]][rf];
        squares[rt] = squares[rf];
        squares[rf] = NO_PIECE;
        hash ^= Zobrist::pieceKeys[squares[rt]][rt];
    }

    // Update castling rights if rook moved/captured
    auto clearRight = [&](int sq) {
        if (sq == A1) castleRights &= ~WQ_CASTLE;
        else if (sq == H1) castleRights &= ~WK_CASTLE;
        else if (sq == A8) castleRights &= ~BQ_CASTLE;
        else if (sq == H8) castleRights &= ~BK_CASTLE;
    };
    clearRight(m.from);
    clearRight(m.to);

    hash ^= Zobrist::castleKeys[castleRights];

    // Set new en passant square
    if (m.flags & MF_DOUBLE_PUSH) {
        epSquare = makeSquare(fileOf(m.from), (rankOf(m.from) + rankOf(m.to)) / 2);
        hash ^= Zobrist::epFileKeys[fileOf(epSquare)];
    }

    // Halfmove clock
    if (typeOf(moving) == PAWN || m.isCapture()) halfmoveClock = 0;
    else halfmoveClock++;

    if (us == BLACK) fullmoveNumber++;

    hash ^= Zobrist::sideKey;
    sideToMove = them;
}

void Board::unmakeMove(const Move& m, const UndoInfo& undo) {
    Color them = sideToMove; // side that just moved is opposite of current sideToMove
    Color us = (Color)(them ^ 1);
    sideToMove = us;

    Piece placed = squares[m.to];
    Piece original = placed;
    if (m.flags & MF_PROMOTION) {
        original = makePiece(us, PAWN);
    }

    squares[m.from] = original;
    squares[m.to] = NO_PIECE;

    if (typeOf(original) == KING) kingSq[us] = m.from;

    if (m.flags & MF_EN_PASSANT) {
        squares[undo.capturedSquare] = undo.captured;
    } else if (undo.captured != NO_PIECE) {
        squares[m.to] = undo.captured;
    }

    if (m.flags & MF_CASTLE_K) {
        int rf = (us == WHITE) ? H1 : H8;
        int rt = (us == WHITE) ? F1 : F8;
        squares[rf] = squares[rt];
        squares[rt] = NO_PIECE;
    } else if (m.flags & MF_CASTLE_Q) {
        int rf = (us == WHITE) ? A1 : A8;
        int rt = (us == WHITE) ? D1 : D8;
        squares[rf] = squares[rt];
        squares[rt] = NO_PIECE;
    }

    epSquare = undo.epSquare;
    castleRights = undo.castleRights;
    halfmoveClock = undo.halfmoveClock;
    hash = undo.hash;

    if (us == BLACK) fullmoveNumber--;
}

void Board::makeNullMove(UndoInfo& undo) {
    undo.epSquare = epSquare;
    undo.castleRights = castleRights;
    undo.halfmoveClock = halfmoveClock;
    undo.hash = hash;
    undo.captured = NO_PIECE;
    undo.capturedSquare = SQ_NONE;

    if (epSquare != SQ_NONE) hash ^= Zobrist::epFileKeys[fileOf(epSquare)];
    epSquare = SQ_NONE;
    hash ^= Zobrist::sideKey;
    sideToMove = (Color)(sideToMove ^ 1);
}

void Board::unmakeNullMove(const UndoInfo& undo) {
    sideToMove = (Color)(sideToMove ^ 1);
    epSquare = undo.epSquare;
    hash = undo.hash;
}

int Board::phase() const {
    // Standard phase calc: knight/bishop=1, rook=2, queen=4, max 24
    static const int val[7] = {0,0,1,1,2,4,0};
    int ph = 0;
    for (int s = 0; s < 64; s++) {
        Piece p = squares[s];
        if (p == NO_PIECE) continue;
        ph += val[typeOf(p)];
    }
    if (ph > 24) ph = 24;
    return ph;
}
