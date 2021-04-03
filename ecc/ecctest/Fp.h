#pragma once

#include "Wide.h"

template <size_t Bits, typename Base>
Wide<Bits, Base> AddModuloM(const Wide<Bits, Base>& a, const Wide<Bits, Base>& b, const Wide<Bits, Base>& M)
{
    auto ab = a + b;
    if (ab >= M)
        ab -= M;
    return ab.Truncate<Bits>();
    //const auto [a_b, overflow] = a.AddWithCarry(b);
    //return (overflow || a_b >= M) ? a_b - M : a_b;
}

template <size_t Bits, typename Base>
Wide<Bits, Base> MultiplyModuloM(const Wide<Bits, Base>& a, const Wide<Bits, Base>& b, const Wide<Bits, Base>& M)
{
    return (a * b).DivideQR(M).second;
}

template <typename Base, Base... p0x>
class Fp
{
public:
    static constexpr size_t ElementCount = sizeof...(p0x);
    static constexpr size_t Bits = 8 * sizeof(Base) * ElementCount;
    using Type = Wide<Bits, Base>;
    static constexpr Type p = std::array<Base, ElementCount>{ p0x... };

    Fp(const Type& rhs) : x(rhs) {}
    Fp(Base rhs) : x(rhs) {}

    bool operator !=(const Fp& rhs) const
    {
        return x != rhs.x;
    }

    bool operator ==(const Fp& rhs) const
    {
        return x == rhs.x;
    }

    friend Fp operator -(const Fp& lhs)
    {
        return p - lhs.x;
    }

    friend Fp operator +(const Fp& lhs, const Fp& rhs)
    {
        return AddModuluM(lhs.x, rhs.x, p);
    }

    friend Fp operator -(const Fp& lhs, const Fp& rhs)
    {
        return lhs + (-rhs);
    }

    friend Fp operator *(const Fp& lhs, const Fp& rhs)
    {
        return MultiplyModuloM(lhs.x, rhs.x, p);
    }

    friend std::ostream& operator <<(std::ostream& s, const Fp& rhs)
    {
        return s << rhs.x;
    }

private:
    Type x;
};
