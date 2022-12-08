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
template <typename T>
void encode(void*& data, T v)
{
    T* pv = reinterpret_cast<T*>(data);
    *pv++ = v;
    data = pv;
}

template <typename T>
T decode(void*& data)
{
    T* pv = reinterpret_cast<T*>(data);
    T v = *pv++;
    data = pv;
    return v;
}
template <typename T>
T decode_safe(void*& data, int& datasize, T value)
{
    if (datasize >= sizeof(T))
    {
        T* pv = reinterpret_cast<T*>(data);
        T v = *pv++;
        data = pv;
        datasize -= sizeof(T);
        return v;
    }else return value;
}

template <typename T>
void rotLeft(int first, int count, int n_rot, T* buffer, T* tmp)
{
    buffer += first;
    for (int i = 0; i < n_rot; ++i) {
        tmp[i] = buffer[i];
    }
    for (int i = n_rot; i < count; ++i) {
        buffer[i - n_rot] = buffer[i];
    }
    for (int i = 0; i < n_rot; ++i) {
        buffer[i + count - n_rot] = tmp[i];
    }
}
template <typename T>
void rotRight(int first, int count, int n_rot, T* buffer, T* tmp)
{
    buffer += first;
    for (int i = 0; i < n_rot; ++i) {
        tmp[i] = buffer[count - n_rot + i];
    }
    for (int i = count - n_rot - 1; i >= 0; --i) {
        buffer[i + n_rot] = buffer[i];
    }
    for (int i = 0; i < n_rot; ++i) {
        buffer[i] = tmp[i];
    }
}
inline char* int_to_string(char*p,int value,int base=10)
{
    itoa(value, p, base);
    while(*p) p++;
    return p;
}