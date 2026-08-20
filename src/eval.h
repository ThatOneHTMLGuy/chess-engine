#pragma once
#include "board.h"
#include "types.h"

namespace Eval {
    Score evaluate(const Board& b);
    extern const int pieceValue[7];
}
