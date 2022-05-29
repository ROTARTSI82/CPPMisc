#include <functional>
#include <string>

template<typename T, unsigned R, unsigned C>
class Matrix {
    static_assert(R > 0, "matrix must not be empty");
    static_assert(C > 0, "matrix must not be empty");

public:
    typedef T type;
    typedef Matrix<T, R, C> self;
    constexpr static unsigned rows = R;
    constexpr static unsigned cols = C;

    T dat[R][C];

    T *operator[](unsigned r) { return dat[r]; }
    T const *operator[](unsigned r) const { return dat[r]; }

    T &operator()(unsigned r, unsigned c) { return dat[r][c]; }

    T operator()(unsigned r, unsigned c) const { return dat[r][c]; }

    self &operator+=(const self &rhs) {
        for (unsigned r = 0; r < rows; r++)
            for (unsigned c = 0; c < cols; c++)
                dat[r][c] += rhs[r][c];
        return *this;
    }

    inline self operator+(const self &rhs) const {
        return self{*this} += rhs;
    }

    self &operator*=(T rhs) {
        for (unsigned r = 0; r < rows; r++)
            for (unsigned c = 0; c < cols; c++)
                dat[r][c] *= rhs[r][c];
        return *this;
    }

    inline self operator*(T rhs) const {
        return self{*this} *= rhs;
    }

    template<unsigned O>
    Matrix<T, R, O> operator*(const Matrix<T, C, O> &rhs) const {
        Matrix<T, R, O> x{};
        for (unsigned r = 0; r < R; r++)
            for (unsigned c = 0; c < O; c++) {
                x[r][c] = 0;
                for (unsigned mr = 0; mr < C; mr++)
                    x[r][c] += rhs[mr][c] * dat[r][mr];
            }
        return x;
    }

    T det() const;
    self &invert();

    self &ref();
    self &rref();

    inline self inverted() const {
        return self{*this}.invert();
    }

    inline self rrefed() const {
        return self{*this}.rref();
    }

    inline self refed() const {
        return self{*this}.ref();
    }

    Matrix<T, C, R> transposed() const {
        Matrix<T, C, R> x{};
        for (unsigned r = 0; r < rows; r++)
            for (unsigned c = 0; c < cols; c++)
                x[c][r] = dat[r][c];
        return x;
    }

    std::string to_string() const {
        std::string ret = "";
        for (unsigned r = 0; r < rows; r++) {
            for (unsigned c = 0; c < cols; c++)
                ret += std::to_string(dat[r][c]) + "\t";
            ret += "\n";
        }
        return ret;
    }

    void swap_rows(unsigned a, unsigned b) {
        for (unsigned i = 0; i < cols; i++) {
            T tmp = dat[a][i];
            dat[a][i] = dat[b][i];
            dat[b][i] = tmp;
        }
    }

    void row_add_mult(unsigned src, unsigned dst, T mult) {
        for (unsigned i = 0; i < cols; i++)
            dat[dst][i] += dat[src][i] * mult;
    }

    void row_map(unsigned row, std::function<T(T)> map) {
        for (unsigned i = 0; i < cols; i++)
            dat[row][i] = map(dat[row][i]);
    }
};

template<unsigned R, unsigned C>
using Mat = Matrix<double, R, C>;

template<typename T, unsigned S>
using Vector = Matrix<T, S, 1>;

template<unsigned S>
using Vec = Vector<double, S>;


template<typename T, unsigned R, unsigned C>
Matrix<T, R, C> &Matrix<T, R, C>::ref() {
    static_assert(C >= R, "Gaussian Elimination may only be run on matrices with more columns than rows");

    for (unsigned r = 0; r < rows; r++) {
        unsigned c = r;

        {
            unsigned use = r;
            for (unsigned i = r + 1; i < rows; i++)
                if (dat[i][c] > dat[use][c])
                    use = i;

            if (dat[use][c] == 0)
                continue;// skip row if 0

            if (use != r)
                swap_rows(use, r);
        }// this code is questionable; I probably shouldn't be doing this greedily!

        row_map(r, [&, div{dat[r][c]}](T in) { return in / div; });

        for (unsigned mr = r + 1; mr < rows; mr++)
            row_add_mult(r, mr, -dat[mr][c] / dat[r][c]);
    }

    return *this;
}

template<typename T, unsigned R, unsigned C>
Matrix<T, R, C> &Matrix<T, R, C>::rref() {
    ref();

    for (unsigned r = rows - 1; r >= 0; r--) {
        unsigned c = r;

        for (unsigned mr = 0; mr < r; mr++)
            row_add_mult(r, mr, -dat[mr][c] / dat[r][c]);
    }

    return *this;
}

template<typename T, unsigned R, unsigned C>
Matrix<T, R, C> &Matrix<T, R, C>::invert() {
    static_assert(R == C, "Can only invert square matrices");

    Matrix<T, R, C * 2> st;
    for (unsigned r = 0; r < R; r++)
        for (unsigned c = 0; c < C; c++) {
            st[r][c] = dat[r][c];
            st[r][C + c] = r == c ? 1 : 0;
        }

    st = st.rref();

    for (unsigned r = 0; r < R; r++)
        for (unsigned c = 0; c < C; c++) {
            dat[r][c] = st[r][C + c];
            if (st[r][c] != (r == c ? 1 : 0))// raw float equality?
                throw std::runtime_error{"Singular matrix"};
        }

    return *this;
}

template<unsigned RA, unsigned CA, unsigned RB, unsigned CB, typename T>
typename std::enable_if<RA == RB && (RA == 1 || CA == 1) && (RB == 1 || CB == 1), T>::type dot(const Matrix<T, RA, CA> &a, const Matrix<T, RB, CB> &b) {
    return (a.transposed() * b)[0][0];
}

template<unsigned RA, unsigned CA, unsigned RB, unsigned CB, typename T>
typename std::enable_if<RA != RB && (RA == 1 || CA == 1) && (RB == 1 || CB == 1), T>::type dot(const Matrix<T, RA, CA> &a, const Matrix<T, RB, CB> &b) {
    return (a * b)[0][0];
}

constexpr inline unsigned umax(unsigned a, unsigned b) {
    return a > b ? a : b;
}

template<typename T, unsigned R, unsigned C>
T Matrix<T, R, C>::det() const {
    static_assert(R == C, "can only det() square matrices");

    if constexpr (R == 1 && C == 1)
        return dat[0][0];
    else if constexpr (R == 2 && C == 1)
        return dat[0][0] * dat[1][1] - dat[1][0] * dat[0][1];


    T ret = 0;
    int flip = 1;

    // laplace expansion time!
    for (unsigned c = 0; c < cols; c++) {
        Matrix<T, umax(R - 1, 1), umax(C - 1, 1)> sub;

        for (unsigned cr = 1; cr < rows; cr++) {
            for (unsigned cc = 0; cc < c; cc++)
                sub[cr - 1][cc] = dat[cr][cc];
            for (unsigned cc = c + 1; cc < cols; cc++)
                sub[cr - 1][cc - 1] = dat[cr][cc];
        }

        ret += flip * dat[0][c] * sub.det();
        flip *= -1;
    }

    return ret;
}