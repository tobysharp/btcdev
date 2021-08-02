#pragma once

#include "Wide.h"

template <size_t Bits>
constexpr UIntW<Bits> AddModuloM(const UIntW<Bits>& a, const UIntW<Bits>& b, const UIntW<Bits>& M)
{
    auto ab = a + b;
    if (ab >= M)
        ab -= M;
    return ab.Truncate<Bits>();
    //const auto [a_b, overflow] = a.AddWithCarry(b);
    //return (overflow || a_b >= M) ? a_b - M : a_b;
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

//template <size_t Bits, typename Base>
//constexpr UIntW<Base, Bits> DivideModuloPrime(const UIntW<Base, Bits>& a, const UIntW<Base, Bits>& b, const UIntW<Base, Bits>& p)
//{
//    // To compute x = a/b (mod p), first compute the extended gcd(b, p). 
//    // Note that if p is prime and 1 <= b < p then gcd(b, p) = 1.
//    // The extended GCD algorithm computes s, t, u such that sb + tp = u where u = gdc(b, p).
//    // So in the case that u = 1, we get s, t such that sb + tp = 1, i.e. sb = 1 (mod p).
//    // Then sab = a (mod p). So a/b = sa (mod p).
//    const auto gcd = ExtendedBinaryGCD(b, p); // TODO
//    assert(gcd.v == UIntW<Base, Bits>(1));
//    const auto s = gcd.a;
//    return MultiplyModuloM(s, a, p);
//}

template <const char* str>
constexpr size_t GetBitCount()
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

template <const char* str, typename Base = uint32_t>
constexpr auto GetUIntArray()
{
    constexpr size_t Bits = GetBitCount<str>();
    constexpr size_t BitsPerElement = sizeof(Base) * 8;
    constexpr size_t Elements = (Bits + BitsPerElement - 1) / BitsPerElement;
    std::array<Base, Elements> rv = {};
    int nibble = 0, index = (Bits + 3) / 4 - 1;
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
        auto lshift = (index & 7) << 2;
        rv[index / 8] |= nibble << lshift;
        --index;
    }
    return rv;
}

template <const char* p0x>
class Fp
{
public:
    static constexpr size_t Bits = GetBitCount<p0x>();
    using Type = UIntW<Bits>;
    using Base = Type::Base;
    using Array = typename Type::Array;
    static constexpr Type p = GetUIntArray<p0x, Base>();

    constexpr Fp() {}
    constexpr Fp(Base rhs) : x(rhs) {}
    constexpr Fp(const Array& rhs) : x(rhs) {}
    constexpr Fp(const Type& rhs) : x(rhs) {}

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
        return AddModuloM(lhs.x, rhs.x, p);
    }

    friend constexpr Fp operator -(const Fp& lhs, const Fp& rhs)
    {
        return lhs + (-rhs);
    }

    friend constexpr Fp operator *(const Fp& lhs, const Fp& rhs)
    {
        return MultiplyModuloM(lhs.x, rhs.x, p);
    }

    constexpr Fp Squared() const
    {
        return SquareModuloM(x, p);
    }

    //friend constexpr Fp operator /(const Fp& lhs, const Fp& rhs)
    //{
    //    return DivideModuloPrime(lhs.x, rhs.x, p);
    //}

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
