#pragma once

template <bool Cond, typename True, typename False>
struct Ternary {};

template <typename True, typename False>
struct Ternary<true, True, False> {
    using value = True;
};

template <typename True, typename False>
struct Ternary<false, True, False> {
    using value = False;
};

