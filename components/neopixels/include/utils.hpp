#pragma once
#include <limits>
#include <tuple>
#include <cstddef>
#include <stdio.h>

template <typename T>
constexpr T next_pow2(T x) 
{
    constexpr size_t digits = 32; // don't use std::numeric_limits<T>::digits, clz returns 0-32
    const size_t shift = digits - __builtin_clz(x-1);
	return x == 1 ? T(2) : T(1)<<shift;
}
template <typename T>
constexpr std::tuple<T,size_t> next_pow2_sh(T x) 
{
    constexpr size_t digits = 32; // don't use std::numeric_limits<T>::digits, clz returns 0-32
    const size_t shift = digits - __builtin_clz(x-1);
    if (x==1) return {T(2),1};
    else return {T(1)<<shift,shift};
}