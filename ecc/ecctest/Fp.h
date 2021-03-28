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

template <size_t Bits, typename Base>
class Fp
{
public:
    Fp(const Wide<Bits, Base>& x, const Wide<Bits, Base>& p) : m_x(x), m_p(p) {}
    Fp(const Fp& rhs) : m_x(rhs.m_x), m_p(rhs.m_p) {}
    Fp(Fp&& rhs) : m_x(std::move(rhs.m_x)), m_p(std::move(rhs.m_p)) {}

    friend Fp operator +(const Fp& lhs, const Fp& rhs)
    {
        return { AddModuluM(lhs.m_x, rhs.m_x, lhs.m_p), m_p };
    }

    friend Fp operator -(const Fp& lhs)
    {
        return { m_p - lhs.m_x, m_p };
    }

    friend Fp operator -(const Fp& lhs, const Fp& rhs)
    {
        return lhs + (-rhs);
    }

    friend Fp operator *(const Fp& lhs, const Fp& rhs)
    {

    }
private:
    Wide<Bits, Base> m_x;
    Wide<Bits, Base> m_p;
};

