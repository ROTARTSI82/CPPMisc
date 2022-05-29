#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

typedef uint_fast16_t T3Bitboard;
typedef uint_fast8_t T3Square;

typedef int_fast16_t Score;
typedef int BasicInt;

constexpr Score SCORE_MAX = std::numeric_limits<Score>::max() - 2;
constexpr Score SCORE_MIN = std::numeric_limits<Score>::min() + 2;

constexpr T3Bitboard fullBoard = 0xFFFF;
constexpr T3Bitboard nil = fullBoard;
constexpr T3Bitboard winPatterns[9][4] = {
        {0x7, 0x111, 0x49, nil},// 0
        {0x92, 0x7, nil, nil},  // 1
        {0x7, 0x124, 0x54, nil},// 2

        {0x49, 0x38, nil, nil},   // 3
        {0x111, 0x54, 0x38, 0x92},// 4
        {0x124, 0x38, nil, nil},  // 5

        {0x49, 0x1c0, 0x54, nil}, // 6
        {0x92, 0x1c0, nil, nil},  // 7
        {0x111, 0x1c0, 0x124, nil}// 8
};

constexpr inline T3Bitboard to_bb(T3Square sq) {
    return 1 << sq;
}

enum class T3State : uint_fast8_t {
    ONGOING,
    DRAW,
    X_WIN,
    O_WIN
};

class TicTacToe {
public:
    T3Bitboard x = 0, o = 0;
    BasicInt num_moves = 0;
    BasicInt depth = 0;

    bool is_x_turn = false;
    T3State state = T3State::ONGOING;

    std::vector<T3Square> best;
    double expected_activation[9];
    Score scores[9];

    constexpr inline T3Bitboard &current_bb() {
        return is_x_turn ? x : o;
    }

    constexpr inline BasicInt perspective() {
        return is_x_turn ? 1 : -1;
    }

    constexpr inline bool is_legal(T3Square move) {
        return !((x & to_bb(move)) != 0 || (o & to_bb(move)) != 0);
    }

    void print() {
        for (int r = 2; r >= 0; r--) {
            for (int c = 0; c < 3; c++) {
                if (x & to_bb(r * 3 + c)) {
                    std::cout << "X ";
                } else if (o & to_bb(r * 3 + c)) {
                    std::cout << "O ";
                } else {
                    //          std::cout << std::to_string(r * 3 + c) << ' ';
                    std::cout << ". ";
                }
            }
            std::cout << '\n';
        }

        std::cout << (is_x_turn ? 'X' : 'O');
        std::cout << " is to move\n";
    }

    inline void make_move(T3Square move) {
        if (++num_moves >= 9) state = T3State::DRAW;

        current_bb() |= to_bb(move);

        for (int i = 0; i < 4; i++) {
            if ((current_bb() & winPatterns[move][i]) == winPatterns[move][i]) {
                state = is_x_turn ? T3State::X_WIN : T3State::O_WIN;
            }
        }

        is_x_turn ^= true;
    }

    inline void unmake_move(T3Square move) {
        num_moves--;
        state = T3State::ONGOING;

        is_x_turn ^= true;

        current_bb() ^= to_bb(move);
    }

    inline Score eval(Score alpha, Score beta) {
        switch (state) {
            case (T3State::ONGOING): {

                Score value = SCORE_MIN;
                for (T3Square mov = 0; mov < 9; mov++) {
                    if (!is_legal(mov)) continue;
                    make_move(mov);
                    depth++;
                    value = std::max((int16_t) value, (int16_t) -eval(-beta, -alpha));
                    depth--;
                    unmake_move(mov);

                    if (value > beta)
                        return value;

                    alpha = std::max(value, alpha);
                }

                return value;
            }
            case (T3State::DRAW):
                return 0;
            case (T3State::X_WIN):
                return (10 - depth) * perspective();
            case (T3State::O_WIN):
                return -(10 - depth) * perspective();
        }

        throw std::runtime_error{"What?"};
    }

    inline Score gen_best_moves() {
        Score alpha = SCORE_MIN;
        depth = 0;
        best.clear();

        for (T3Square mov = 0; mov < 9; mov++) {
            expected_activation[mov] = 0.0;
            if (!is_legal(mov)) {
                scores[mov] = -64;
                continue;
            }

            make_move(mov);
            depth++;
            Score val = -eval(SCORE_MIN, -alpha);
            scores[mov] = val;
            depth--;
            // std::cout << "info move " << (int) mov << " score " << (int) val << '\n';
            unmake_move(mov);

            if (val > alpha) {
                best.clear();
                best.push_back(mov);
                alpha = val;
            } else if (val == alpha) {
                best.push_back(mov);
            }
        }

        for (auto i: best) expected_activation[i] = 1.0;

        return alpha;
    }

    inline bool operator==(const TicTacToe &rhs) const {
        return x == rhs.x && o == rhs.o && is_x_turn == rhs.is_x_turn;
    }
};

std::size_t hash_tictactoe(const TicTacToe &rhs) {
    return static_cast<uint64_t>(rhs.x) << 1 | static_cast<uint64_t>(rhs.o) << 10 | (rhs.is_x_turn ? 1ULL : 0ULL);
}
