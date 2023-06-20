// Libraries and std syntax
#include "position.h"
#include "tables.h"
#include "types.h"
#include <climits>
#include <iostream>
#include "pst.hpp"
#include <unordered_map>
#include <utility>
#include <ctime>
using namespace std;

constexpr int DEPTH = 6;
constexpr int R = 2;

constexpr Color ENGINE_COLOR = WHITE;
constexpr Color PLAYER_COLOR = ~ENGINE_COLOR;

constexpr int REAL_MIN = INT_MIN + 1;
constexpr int REAL_MAX = INT_MAX;

int nodes_searched = 0;
// SCORE FUNCTION (helper)


int score_piece(Bitboard p, const int pst[64], int piece_value) {
    int value = 0;
    while(p != 0) {
        const Square s = pop_lsb(&p);
        value += piece_value + pst[s];
    }
    return value;
}

constexpr int ZERO_PS[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

template<Color Us = ENGINE_COLOR>
int eval_pos(Position &p) {
    constexpr Color Them = ~Us;
    return score_piece(p.bitboard_of(Us, PAWN), PAWN_TABLE, 10) 
         + score_piece(p.bitboard_of(Us, BISHOP), BISHOP_TABLE, 30) 
         + score_piece(p.bitboard_of(Us, KNIGHT), KNIGHT_TABLE, 30)
         + score_piece(p.bitboard_of(Us, ROOK), ROOK_TABLE, 50)
         + score_piece(p.bitboard_of(Us, QUEEN), QUEEN_TABLE, 90)
         + score_piece(p.bitboard_of(Us, KING), KING_TABLE, 900)
         - score_piece(p.bitboard_of(Them, PAWN), PAWN_TABLE, 10) 
         - score_piece(p.bitboard_of(Them, BISHOP), BISHOP_TABLE, 30) 
         - score_piece(p.bitboard_of(Them, KNIGHT), KNIGHT_TABLE, 30)
         - score_piece(p.bitboard_of(Them, ROOK), ROOK_TABLE, 50)
         - score_piece(p.bitboard_of(Them, QUEEN), QUEEN_TABLE, 90) // eval based on piece count (# of pieces on each side * value in decipawns)
         - score_piece(p.bitboard_of(Them, KING), KING_TABLE, 900);
}

// int eval_pos(Position &p) {
//     return pop_count(p.bitboard_of(ENGINE_COLOR, PAWN)) * 10 
//             + pop_count(p.bitboard_of(ENGINE_COLOR, BISHOP)) * 30 
//             + pop_count(p.bitboard_of(ENGINE_COLOR, KNIGHT)) * 30
//             + pop_count(p.bitboard_of(ENGINE_COLOR, ROOK)) * 50
//             + pop_count(p.bitboard_of(ENGINE_COLOR, QUEEN)) * 90
//             - pop_count(p.bitboard_of(PLAYER_COLOR, PAWN)) * 10 
//             - pop_count(p.bitboard_of(PLAYER_COLOR, BISHOP)) * 30 
//             - pop_count(p.bitboard_of(PLAYER_COLOR, KNIGHT)) * 30
//             - pop_count(p.bitboard_of(PLAYER_COLOR, ROOK)) * 50
//             - pop_count(p.bitboard_of(PLAYER_COLOR, QUEEN)) * 90; // eval based on piece count (# of pieces on each side * value in decipawns)
// }

enum TtTableFlags {
    EXACT, LOWERBOUND, UPPERBOUND
};
struct TtTableEntry {
    int depth;
    int value;
    TtTableFlags flag;
};
typedef unordered_map<uint64_t, TtTableEntry> TtTable;

template<Color C>
int negamax(Position &board, int alpha, int beta, int depth) {
    if(depth <= 0) return eval_pos<C>(board);

    nodes_searched += 1;

    MoveList<C> mvs{board};

    for(Move mv : mvs) {
        board.play<C>(mv);
        int score = -negamax<~C>(board, -beta, -alpha, depth - 1);
        alpha = max(alpha, score);
        board.undo<C>(mv);
        if(alpha >= beta) return beta;
    }
    return alpha;
}

template<Color C>
int alphabeta(Position &board, int depth, int alpha, int beta, TtTable &tt_table, int mate) {
    if(depth == 0) return eval_pos<ENGINE_COLOR>(board);
    auto key = board.get_hash();
    auto retr = tt_table.find(key);
    if(retr != tt_table.end() && retr->second.depth >= depth) {
        auto entry = retr->second;
        if(entry.flag == EXACT) return entry.value;
        if(entry.flag == LOWERBOUND) alpha = max(alpha, entry.value);
        if(entry.flag == UPPERBOUND) beta = min(beta, entry.value);
        if(alpha <= beta) return entry.value;
    }

    
    MoveList<C> moves{board};

    if (moves.size() == 0) {
        if(!board.in_check<C>()) return 0;
        return -mate;
    }

    int value = 0;
    TtTableFlags flag = EXACT;

    nodes_searched += 1;

    // vector<pair<Move, int>> mvs{moves.size()};
    // for(auto mv : moves) {
    //     board.play<C>(mv);
    //     mvs.emplace_back(make_pair(mv, eval_pos(board)));
    //     board.undo<C>(mv);
    // }
    
    if(depth > 1) {
        Move best;
        int max = INT_MIN;
        int ind = -1;
        for(int i = 0; i < moves.size(); ++i) {
            auto mv = moves[i];
            board.play<C>(mv);
            int eval = eval_pos(board);
            if(eval > max) {
                max = eval;
                best = mv;
                ind = i;
            }
            board.undo<C>(mv);
        }
        auto temp = moves[0];
        moves[0] = best;
        moves[ind] = temp;
    
    }
    
    
    
    if(C == ENGINE_COLOR) {
        // if (depth > 1) {
        //     sort(mvs.begin(), mvs.end(), [](pair<Move, int> a, pair<Move, int> b) {
        //         return a.second > b.second;
        //     });
        // }

        for(auto mv : moves) {
            // auto mv = (x.first);
            board.play<C>(mv);
            
            value = alphabeta<~C>(board, depth - 1, alpha, beta, tt_table, mate - 1);
            
            board.undo<C>(mv);
            if (value >= beta) {
                flag = LOWERBOUND;
                beta = value;
                goto end;
            }
            if (value > alpha) {
                alpha = value;
            }
        }
        value = alpha;
    } else {
        // if (depth > 1) {
        //     sort(mvs.begin(), mvs.end(), [](pair<Move, int> a, pair<Move, int> b) {
        //         return a.second > b.second;
        //     });
        // }
        for(auto mv : moves) {
            // auto mv = x.first;
            board.play<C>(mv);
            value = alphabeta<~C>(board, depth - 1, alpha, beta, tt_table, mate - 1);
            board.undo<C>(mv);
            if (value <= alpha) {
                flag = UPPERBOUND;
                value = alpha;
                goto end;
            }
            if (value < beta) {
                beta = value;
            }
        }
        value = beta;
    }
    end:
    TtTableEntry entry;
    entry.flag = flag;
    entry.depth = depth;
    entry.value = value;
    tt_table[key] = entry;

    return value;

}

template<Color C>
int minimax(Position &board, int depth, TtTable tbl) {
    // return negamax<C>(board, REAL_MIN, REAL_MAX, DEPTH);
    return alphabeta<C>(board, depth, INT_MIN, INT_MAX, tbl, REAL_MIN);
}
// template<Color C>
// int minimax(Position &board, int depth) {
//     if(depth == 0) return eval_pos(board);
//     auto list = MoveList<C>(board);
//     //Engine is maximizing, player is minimizing
//     Move best;
//     int value = C == ENGINE_COLOR ? INT_MIN : INT_MAX;
//     for(Move mv: list) {
//         board.play<C>(mv);
//         int sc = minimax<~C>(board, depth - 1);
//         // cout << mv << " ";
//         board.undo<C>(mv);
//         if((C == ENGINE_COLOR && sc > value) || (C == PLAYER_COLOR && sc < value)) {
//             value = sc;
//             best = mv;
//         }
//     }
//     return value;
// }

int main(int argc, char ** argv) {
    initialise_all_databases();
    zobrist::initialise_zobrist_keys();
    // set up piecetables and zobrist kets
    Color currentColor = WHITE;// set starting color to white

    string fen;
        
    if(argc == 1) fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    else getline(cin, fen);
    
    
    Position p;
    Position::set(fen, p); // define positon and set as starting position -- in future allow user input?

    TtTable tbl;

    while(true) {
        if(currentColor == PLAYER_COLOR) /* user turn */ {
            string value;
            cin >> value;
            //Parse input
            auto m = Move(value); 
            MoveList<PLAYER_COLOR> list(p);
            for(Move mv : list) {
                if(mv.from() == m.from() && mv.to() == m.to()){
                    p.play<PLAYER_COLOR>(mv);
                    break;
                }
            }
        } else /* daniel turn */ {
            // clock_t start = clock();

            tbl.reserve(100000);
            Move best;
            int max = INT_MIN;
            MoveList<ENGINE_COLOR> list(p);

            // for(int i = DEPTH; i < DEPTH || ((clock() - start) / static_cast<double>(CLOCKS_PER_SEC)) < 10; ++i) {
            //     max = INT_MIN;
            //     cout << "Running at depth: " << i << "\n";
            //     for(Move m : list) {
            //         p.play<ENGINE_COLOR>(m);
            //         int sc = alphabeta<PLAYER_COLOR>(p, i, INT_MIN, INT_MAX, tbl, i);
            //         p.undo<ENGINE_COLOR>(m);
            //         if (sc > max) {
            //             best = m;
            //             max = sc;
            //         }
            //     }
            // }
            for(auto it = list.begin(); it < list.end(); ++it) {
                auto m = *it;
                auto new_board = p;
                cout << m ;
                p.play<ENGINE_COLOR>(m);
                // int sc = pvs<~C>(p, DEPTH, )
                int sc = minimax<PLAYER_COLOR>(p, DEPTH, tbl);
                cout << " - " << sc << endl;
                if(sc > max) {
                    best = m;
                    max = sc;
                }
                p.undo<ENGINE_COLOR>(m);
            }
            // break;
            p.play<ENGINE_COLOR>(best);
            cout << "Nodes Searched: " << nodes_searched << endl;
            nodes_searched = 0;
            cout << p << endl;
            cout << "Best: " << best << " – " << max << endl;
        }
        
        currentColor = ~currentColor;
    }
    
    return 0;
}