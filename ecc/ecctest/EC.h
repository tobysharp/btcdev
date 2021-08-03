#pragma once

#include "Fp.h"

// A point on an elliptic curve
template <typename Fp, Fp a, Fp b>
class EFp
{
    static_assert(4 * a.Squared() * a + 27 * b.Squared() != 0, "Invalid elliptic curve parameters a, b");
public:
    using Multi = typename Fp::Type;
    static constexpr Multi p = Fp::p;

    constexpr EFp() {}
    constexpr EFp(const EFp& rhs) : x(rhs.x), y(rhs.y) {}
    constexpr EFp(EFp&& rhs) : x(std::move(rhs.x)), y(std::move(rhs.y)) {}
    constexpr EFp(const Fp& x, const Fp& y) : x(x), y(y) {}

    constexpr bool IsInfinity() const
    {
        return x == 0 && y == 0;
    }

    friend EFp operator -(const EFp& lhs)
    {
        return { lhs.x, -lhs.y };
    }

    // Add two points on the curve
    friend EFp operator +(const EFp& lhs, const EFp& rhs)
    {
        if (lhs.IsInfinity() || rhs.IsInfinity())
            return { lhs.x + rhs.x, lhs.y + rhs.y };
        else if (lhs.x != rhs.x)
        {
            const Fp lambda = (rhs.y - lhs.y) / (rhs.x - lhs.x);
            const Fp x3 = lambda.Squared() - lhs.x - rhs.x;
            const Fp y3 = lambda * (lhs.x - x3) - lhs.y;
            return { x3, y3 };
        }
        else if (lhs.y == -rhs.y)
            return {};
        else
        {
            // Add a (non-infinity) point to itself 
            const Fp lambda = (3 * lhs.x.Squared() + a) / (lhs.y + lhs.y);
            const Fp x3 = lambda.Squared() - (lhs.x + lhs.x);
            const Fp y3 = lambda * (lhs.x - x3) - lhs.y;
            return { x3, y3 };
        }
    }

    EFp& operator =(const EFp& rhs)
    {
        x = rhs.x;
        y = rhs.y;
        return *this;
    }

    EFp& operator =(EFp&& rhs)
    {
        x = std::move(rhs.x);
        y = std::move(rhs.y);
        return *this;
    }

    EFp& operator +=(const EFp& rhs)
    {
        return *this = *this + rhs;
    }

    // Scalar multiplication
    friend EFp operator *(const Fp& scalar, const EFp& pt)
    {
        // Scalar multiplication of elliptic curve points can be computed efficiently using the 
        // addition rule together with the double-and-add algorithm
        EFp sum;
        EFp power = pt;
        for (size_t bitIndex = 0; bitIndex < scalar.x.BitCount; ++bitIndex)
        {
            if (scalar.x.GetBit(bitIndex))
                sum += power;
            power += power;
        }
        return sum;
    }

    constexpr bool IsOnCurve() const
    {
        if (IsInfinity())
            return true;
        const auto lhs = y.Squared();
        const auto rhs = (x.Squared() + a) * x + b;
        return lhs == rhs;
    }

    Fp x, y;
};