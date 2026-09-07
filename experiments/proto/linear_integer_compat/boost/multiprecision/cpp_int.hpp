#pragma once

// Local audit-only compatibility shim.  The supplied expert programs use only
// a small subset of boost::multiprecision::cpp_int; this maps that subset to
// the GMP C library already installed with the Windows MinGW toolchain.

#include <cstdint>
#include <cstring>
#include <limits>
#include <ostream>
#include <utility>

#include <gmp.h>

namespace boost::multiprecision {

class cpp_int {
public:
    cpp_int() { mpz_init(value_); }
    cpp_int(int value) {
        mpz_init(value_);
        if (value >= 0) set_u64(static_cast<std::uint64_t>(value));
        else mpz_set_si(value_, value);
    }
    cpp_int(std::uint64_t value) {
        mpz_init(value_);
        set_u64(value);
    }
    cpp_int(const cpp_int& other) { mpz_init_set(value_, other.value_); }
    cpp_int(cpp_int&& other) noexcept {
        mpz_init(value_);
        mpz_swap(value_, other.value_);
    }
    ~cpp_int() { mpz_clear(value_); }

    cpp_int& operator=(const cpp_int& other) {
        if (this != &other) mpz_set(value_, other.value_);
        return *this;
    }
    cpp_int& operator=(cpp_int&& other) noexcept {
        if (this != &other) mpz_swap(value_, other.value_);
        return *this;
    }

    cpp_int& operator+=(const cpp_int& other) {
        mpz_add(value_, value_, other.value_);
        return *this;
    }

    friend cpp_int operator*(const cpp_int& left, const cpp_int& right) {
        cpp_int result;
        mpz_mul(result.value_, left.value_, right.value_);
        return result;
    }

    friend cpp_int operator*(const cpp_int& left, std::uint64_t right) {
        cpp_int multiplier(right);
        return left * multiplier;
    }

    friend cpp_int operator*(std::uint64_t left, const cpp_int& right) {
        return right * left;
    }

    friend cpp_int operator/(const cpp_int& numerator, std::uint64_t denominator) {
        cpp_int divisor(denominator), result;
        mpz_tdiv_q(result.value_, numerator.value_, divisor.value_);
        return result;
    }

    friend std::uint64_t operator%(const cpp_int& numerator, std::uint64_t denominator) {
        cpp_int divisor(denominator), remainder;
        mpz_tdiv_r(remainder.value_, numerator.value_, divisor.value_);
        std::uint64_t result = 0;
        std::size_t count = 0;
        mpz_export(&result, &count, -1, sizeof(result), 0, 0, remainder.value_);
        return result;
    }

    friend cpp_int operator<<(const cpp_int& value, unsigned int bits) {
        cpp_int result;
        mpz_mul_2exp(result.value_, value.value_, bits);
        return result;
    }

    friend bool operator==(const cpp_int& left, std::uint64_t right) {
        cpp_int rhs(right);
        return mpz_cmp(left.value_, rhs.value_) == 0;
    }

    friend std::ostream& operator<<(std::ostream& out, const cpp_int& value) {
        char* text = mpz_get_str(nullptr, 10, value.value_);
        out << text;
        void (*free_function)(void*, std::size_t) = nullptr;
        mp_get_memory_functions(nullptr, nullptr, &free_function);
        free_function(text, std::strlen(text) + 1);
        return out;
    }

private:
    void set_u64(std::uint64_t value) {
        mpz_import(value_, 1, -1, sizeof(value), 0, 0, &value);
    }

    mpz_t value_;
};

} // namespace boost::multiprecision
