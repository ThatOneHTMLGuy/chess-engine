#include "board.h"
#include "search.h"
#include "eval.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <string>

static Board board;
static Search searcher;
static std::thread searchThread;

static void stopSearchThread() {
    if (searchThread.joinable()) {
        g_uciStop = true;
        searcher.stop();
        searchThread.join();
    }
    g_uciStop = false;
}

static void cmdPosition(std::istringstream& iss) {
    std::string token;
    iss >> token;
    std::string fen;
    if (token == "startpos") {
        board.setStartPos();
        iss >> token; // maybe "moves"
    } else if (token == "fen") {
        std::vector<std::string> parts;
        while (iss >> token && token != "moves") parts.push_back(token);
        std::string fenStr;
        for (size_t i = 0; i < parts.size(); i++) { fenStr += parts[i]; if (i + 1 < parts.size()) fenStr += " "; }
        board.setFromFEN(fenStr);
    }

    searcher.gameHistory.clear();
    searcher.gameHistory.push_back(board.hash);

    if (token == "moves") {
        while (iss >> token) {
            std::vector<Move> legal;
            board.generateLegalMoves(legal);
            bool found = false;
            for (auto& m : legal) {
                std::string uci = m.toUCI();
                if (uci == token) {
                    UndoInfo u;
                    board.makeMove(m, u);
                    searcher.gameHistory.push_back(board.hash);
                    found = true;
                    break;
                }
            }
            if (!found) break;
        }
    }
}

static void cmdGo(std::istringstream& iss) {
    SearchLimits limits;
    std::string token;
    while (iss >> token) {
        if (token == "movetime") { int64_t v; iss >> v; limits.movetimeMs = v; }
        else if (token == "wtime") { int64_t v; iss >> v; limits.wtimeMs = v; }
        else if (token == "btime") { int64_t v; iss >> v; limits.btimeMs = v; }
        else if (token == "winc") { int64_t v; iss >> v; limits.wincMs = v; }
        else if (token == "binc") { int64_t v; iss >> v; limits.bincMs = v; }
        else if (token == "movestogo") { int v; iss >> v; limits.movesToGo = v; }
        else if (token == "depth") { int v; iss >> v; limits.depth = v; }
        else if (token == "nodes") { int64_t v; iss >> v; limits.nodes = v; }
        else if (token == "infinite") { limits.infinite = true; }
    }

    stopSearchThread();
    Board boardCopy = board;
    searchThread = std::thread([boardCopy, limits]() mutable {
        searcher.go(boardCopy, limits);
    });
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    Zobrist::init();
    board.setStartPos();

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "uci") {
            std::cout << "id name MyEngine 1.0\n";
            std::cout << "id author Claude\n";
            std::cout << "option name Hash type spin default 64 min 1 max 4096\n";
            std::cout << "option name Threads type spin default 1 min 1 max 1\n";
            std::cout << "uciok" << std::endl;
        } else if (cmd == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (cmd == "ucinewgame") {
            stopSearchThread();
            searcher.newGame();
            board.setStartPos();
        } else if (cmd == "position") {
            stopSearchThread();
            cmdPosition(iss);
        } else if (cmd == "go") {
            cmdGo(iss);
        } else if (cmd == "stop") {
            g_uciStop = true;
            searcher.stop();
        } else if (cmd == "setoption") {
            std::string tok, name, value;
            iss >> tok; // "name"
            while (iss >> tok && tok != "value") { if (!name.empty()) name += " "; name += tok; }
            while (iss >> tok) { if (!value.empty()) value += " "; value += tok; }
            if (name == "Hash") {
                try { searcher.tt.resize(std::stoul(value)); } catch (...) {}
            }
        } else if (cmd == "quit") {
            stopSearchThread();
            break;
        } else if (cmd == "d" || cmd == "print") {
            board.print();
        } else if (cmd == "eval") {
            std::cout << "eval: " << Eval::evaluate(board) << std::endl;
        }
    }

    return 0;
}
