#pragma once

#include <cstdint>
#include <type_traits>

template<typename T, typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_signed_v<T>, T>>
struct saturated
{
    T val;

    saturated(T in) : val(in) { }

    saturated operator-(T rhs) const noexcept
    {
        if (rhs > val) {
            return 0;
        }

        return saturated<T>(val - rhs);
    }

    saturated& operator-=(T rhs) noexcept
    {
        if (rhs > val) {
            val = 0;
        } else {
            val -= rhs;
        }

        return *this;
    }

    saturated operator+(T rhs) const noexcept
    {
        T sum = val + rhs;
        if (sum < val) {
            return std::numeric_limits<T>::max();
        }

        return sum;
    }

    saturated& operator+=(T rhs) noexcept
    {
        T sum = val + rhs;
        if (sum < val) {
            val = std::numeric_limits<T>::max();
        } else {
            val = sum;
        }

        return *this;
    }

    operator unsigned char() const { return val; }
    operator unsigned short() const { return val; }
    operator unsigned int() const { return val; }
    operator unsigned long() const { return val; }
    operator unsigned long long() const { return val; }
};

template<typename T>
static T saturated_sub(T lhs, T rhs)
{
    return saturated<T>(lhs) - rhs;
}

template<typename T>
static T saturated_add(T lhs, T rhs)
{
    return saturated<T>(lhs) + rhs;
}