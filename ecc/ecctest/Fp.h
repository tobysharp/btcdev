#pragma once

#include "Wide.h"

namespace ModuloArithmetic
{
    template <size_t Bits>
    constexpr UIntW<Bits> SubtractModuloM(const UIntW<Bits>& a, const UIntW<Bits>& b, const UIntW<Bits>& M)
    {
        if (a < b)
            return M - b + a;
        else
            return a - b;
    }

    template <size_t Bits>
    constexpr UIntW<Bits> AddModuloM(const UIntW<Bits>& a, const UIntW<Bits>& b, const UIntW<Bits>& M)
    {
        auto ab = a + b;
        if (ab >= M)
            ab -= M;
        return ab;
    }

    template <size_t Bits>
    constexpr UIntW<Bits> MultiplyModuloM(const UIntW<Bits>& a, const UIntW<Bits>& b, const UIntW<Bits>& M)
    {
        return (a * b).DivideUnsignedQR(M).second;
    }

    template <size_t Bits>
    constexpr UIntW<Bits> SquareModuloM(const UIntW<Bits>& a, const UIntW<Bits>& M)
    {
        return a.Squared().DivideUnsignedQR(M).second;
    }

    template <size_t Bits>
    inline constexpr bool IsEven(const UIntW<Bits>& x)
    {
        return (x & 1) == 0;
    }

    // Given p odd, and 0 <= x < p, return y s.t. y = x/2 (mod p)
    template <size_t Bits>
    inline constexpr UIntW<Bits> HalfModuloOdd(const UIntW<Bits>& x, const UIntW<Bits>& p)
    {
        /*
        * y = x/2 mod p
        * 2y = x mod p
        * If x is even, y = x/2 <= x < p is exact
        * If x is odd, y = (x+p)/2 < p is exact
        */
        if (IsEven(x))
            return x >> 1;
        else
            return (x + p) >> 1;
    }

    // For an odd prime p, and b < p, return x s.t. xb = 1 (mod p)
    // (Actually, this should work for all odd p, even composite.)
    // An optimized version can be seen here: https://eprint.iacr.org/2020/972.pdf
    template <size_t Bits>
    inline constexpr UIntW<Bits> InvertModuloOdd(const UIntW<Bits>& b, const UIntW<Bits>& p)
    {
        UIntW<Bits> aa = b, uu = 1, bb = p, vv = 0;
        while (aa != 0)
        {
            if (IsEven(aa))
            {
                aa >>= 1;
                uu = HalfModuloOdd(uu, p);
            }
            else
            {
                if (aa < bb)
                {
                    std::swap(aa, bb);
                    std::swap(uu, vv);
                }
                aa = (aa - bb) >> 1;
                const auto num = uu >= vv ? uu - vv : UIntW<Bits>(uu + p - vv);
                uu = HalfModuloOdd(num, p);
            }
        }
        if (bb != 1)
            throw std::runtime_error("Value not invertible mod p");
        return vv;
    }

    template <size_t Bits>
    constexpr UIntW<Bits> DivideModuloOdd(const UIntW<Bits>& a, const UIntW<Bits>& b, const UIntW<Bits>& p)
    {
        // To compute x = a/b (mod p), first compute the extended gcd(b, p). 
        // Note that if p is prime and 1 <= b < p then gcd(b, p) = 1.
        // The extended GCD algorithm computes s, t, u such that sb + tp = u where u = gdc(b, p).
        // So in the case that u = 1, we get s, t such that sb + tp = 1, i.e. sb = 1 (mod p).
        // Then sab = a (mod p). So a/b = sa (mod p).
        const auto s = InvertModuloOdd(b, p);
        return MultiplyModuloM(s, a, p);
    }
}

namespace Parse
{
    size_t GetBitCount(const char* str)
    {
        size_t bitCount = 0;
        for (auto ichar = 0; str[ichar] != '\0'; ++ichar)
        {
            if (str[ichar] == ' ')
                continue;
            if ((str[ichar] >= 'A' && str[ichar] <= 'F') ||
                (str[ichar] >= 'a' && str[ichar] <= 'f') ||
                (str[ichar] >= '0' && str[ichar] <= '9'))
                bitCount += 4;
            else
                throw;
        }
        return bitCount;
    }

    template <typename Word = uint32_t, size_t Elements>
    auto GetUIntArray(const char* str, const bool MSWFirst = true)
    {
        constexpr size_t Bits = Elements * sizeof(Word) * 8;
        std::array<Word, Elements> rv = {};
        int nibble = 0, index = MSWFirst ? (Bits + 3) / 4 - 1 : 1;
        constexpr int nibblesPerElement = sizeof(Word) * 2;
        for (auto ichar = 0; str[ichar] != '\0'; ++ichar)
        {
            if (str[ichar] >= 'A' && str[ichar] <= 'F')
                nibble = str[ichar] - 'A' + 0x0A;
            else if (str[ichar] >= 'a' && str[ichar] <= 'f')
                nibble = str[ichar] - 'a' + 0x0A;
            else if (str[ichar] >= '0' && str[ichar] <= '9')
                nibble = str[ichar] - '0';
            else if (str[ichar] == ' ')
                continue;
            else
                throw;
            if (index < 0)
                throw;
            auto lshift = (index & (nibblesPerElement - 1)) << 2;
            rv[index / nibblesPerElement] |= nibble << lshift;
            if (MSWFirst)
                --index;
            else
                index = (index & 1) ? index - 1 : index + 3;
        }
        return rv;
    }
}

template <size_t Bits, UIntW<Bits> p>
class Fp
{
public:
    //static constexpr size_t Bits = Parse::GetBitCount(p0x);
    using Type = UIntW<Bits>;
    using Base = Type::Base;
    using Array = typename Type::Array;
    //static constexpr Type p = Parse::GetUIntArray<Base>(p0x);
    static_assert(p.IsOdd());

    constexpr Fp() {}
    constexpr Fp(const Base& rhs) : Fp(Type{ rhs }) {}
    constexpr Fp(const Array& rhs) : Fp(Type{ rhs }) {}
    template <size_t RBits, UIntW<Bits> q> constexpr Fp(const Fp<RBits, q>& rhs) : Fp(rhs.x) {}
    constexpr Fp(const Fp& rhs) : x(rhs.x) {}
    constexpr Fp(Fp&& rhs) : x(std::move(rhs.x)) {}
    constexpr Fp(const Type& rhs) : x(rhs) 
    {
        if (x >= p)
            x = x.DivideUnsignedQR(p).second;
    }

    Fp& operator =(const Fp& rhs)
    {
        x = rhs.x;
        return *this;
    }

    Fp& operator =(Fp&& rhs)
    {
        x = std::move(rhs.x);
        return *this;
    }

    bool constexpr operator !=(const Fp& rhs) const
    {
        return x != rhs.x;
    }

    bool constexpr operator ==(const Fp& rhs) const
    {
        return x == rhs.x;
    }

    friend constexpr Fp operator -(const Fp& lhs)
    {
        return p - lhs.x;
    }

    friend constexpr Fp operator +(const Fp& lhs, const Fp& rhs)
    {
        return ModuloArithmetic::AddModuloM(lhs.x, rhs.x, p);
    }

    friend constexpr Fp operator -(const Fp& lhs, const Fp& rhs)
    {
        return ModuloArithmetic::SubtractModuloM(lhs.x, rhs.x, p);
    }

    friend constexpr Fp operator *(const Fp& lhs, const Fp& rhs)
    {
        return ModuloArithmetic::MultiplyModuloM(lhs.x, rhs.x, p);
    }

    constexpr Fp Squared() const
    {
        return ModuloArithmetic::SquareModuloM(x, p);
    }

    friend constexpr Fp operator /(const Fp& lhs, const Fp& rhs)
    {
        return ModuloArithmetic::DivideModuloOdd(lhs.x, rhs.x, p);
    }

    Fp Inverse() const
    {
        return ModuloArithmetic::InvertModuloOdd(x, p);
    }

    friend constexpr std::ostream& operator <<(std::ostream& s, const Fp& rhs)
    {
        return s << rhs.x;
    }

    constexpr const Type* operator ->() const
    {
        return &x;
    }

    Type x;
};
