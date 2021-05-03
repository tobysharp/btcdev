#pragma once

#include "Wide.h"

template <size_t Bits, typename Base>
constexpr Wide<Base, Bits> AddModuloM(const Wide<Base, Bits>& a, const Wide<Base, Bits>& b, const Wide<Base, Bits>& M)
{
    auto ab = a + b;
    if (ab >= M)
        ab -= M;
    return ab.Truncate<Bits>();
    //const auto [a_b, overflow] = a.AddWithCarry(b);
    //return (overflow || a_b >= M) ? a_b - M : a_b;
}

template <size_t Bits, typename Base>
constexpr Wide<Base, Bits> MultiplyModuloM(const Wide<Base, Bits>& a, const Wide<Base, Bits>& b, const Wide<Base, Bits>& M)
{
    return (a * b).DivideQR(M).second;
}

template <size_t Bits, typename Base>
constexpr Wide<Base, Bits> SquareModuloM(const Wide<Base, Bits>& a, const Wide<Base, Bits>& M)
{
    return a.Squared().DivideQR(M).second;
}

template <size_t Bits, typename Base>
constexpr Wide<Base, Bits> DivideModuloPrime(const Wide<Base, Bits>& a, const Wide<Base, Bits>& b, const Wide<Base, Bits>& p)
{
    // To compute x = a/b (mod p), first compute the extended gcd(b, p). 
    // Note that if p is prime and 1 <= b < p then gcd(b, p) = 1.
    // The extended GCD algorithm computes s, t, u such that sb + tp = u where u = gdc(b, p).
    // So in the case that u = 1, we get s, t such that sb + tp = 1, i.e. sb = 1 (mod p).
    // Then sab = a (mod p). So a/b = sa (mod p).
    const auto gcd = ExtendedBinaryGCD(b, p); // TODO
    assert(gcd.v == Wide<Base, Bits>(1));
    const auto s = gcd.a;
    return MultiplyModuloM(s, a, p);
}

template <typename Base, Base... p0x>
class Fp
{
public:
    static constexpr size_t ElementCount = sizeof...(p0x);
    static constexpr size_t Bits = 8 * sizeof(Base) * ElementCount;
    using Type = Wide<Base, Bits>;
    using Array = typename Type::Array;
    static constexpr Type p = Array{ p0x... };

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

    friend constexpr Fp operator /(const Fp& lhs, const Fp& rhs)
    {
        return DivideModuloPrime(lhs.x, rhs.x, p);
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
