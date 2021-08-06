#pragma once

#include "Fp.h"

#include <random>

template <size_t Bits, UIntW<Bits> p, UIntW<Bits> a, UIntW<Bits> b,
          UIntW<Bits> Gx, UIntW<Bits> Gy, UIntW<Bits> n>
class EllipticCurve
{
public:
    using Mod_p = Fp<Bits, p>;
    using Mod_n = Fp<Bits, n>;
    using Wide = Mod_p::Type;
    using Signature = std::pair<Wide, Wide>;

    static_assert(p > n);
    static_assert(4 * a.Squared() * a + 27 * b.Squared() != 0);

    class Point
    {
    public:
        constexpr Point() {}
        constexpr Point(const Point& rhs) : x(rhs.x), y(rhs.y) {}
        constexpr Point(Point&& rhs) : x(std::move(rhs.x)), y(std::move(rhs.y)) {}
        constexpr Point(const Mod_p& x, const Mod_p& y) : x(x), y(y) {}

        constexpr bool IsInfinity() const
        {
            return x == 0 && y == 0;
        }

        friend Point operator -(const Point& lhs)
        {
            return { lhs.x, -lhs.y };
        }

        // Add two points on the curve
        friend Point operator +(const Point& lhs, const Point& rhs)
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

        Point& operator =(const Point& rhs)
        {
            x = rhs.x;
            y = rhs.y;
            return *this;
        }

        Point& operator =(Point&& rhs)
        {
            x = std::move(rhs.x);
            y = std::move(rhs.y);
            return *this;
        }

        Point& operator +=(const Point& rhs)
        {
            return *this = *this + rhs;
        }

        // Scalar multiplication
        template <size_t Bits>
        friend Point operator *(const UIntW<Bits>& scalar, const Point& pt)
        {
            // Scalar multiplication of elliptic curve points can be computed efficiently using the 
            // addition rule together with the double-and-add algorithm
            Point sum;
            Point power = pt;
            for (size_t bitIndex = 0; bitIndex < scalar.BitCount; ++bitIndex)
            {
                if (scalar.GetBit(bitIndex))
                    sum += power;
                power += power;
            }
            return sum;
        }

        template <size_t xBits, UIntW<xBits> px>
        friend Point operator *(const Fp<xBits, px>& scalar, const Point& pt)
        {
            return scalar.x * pt;
        }

        friend std::ostream& operator <<(std::ostream& os, const Point& pt)
        {
            return os << "04" << pt.x << pt.y;
        }

        Mod_p x, y;
    };

    static constexpr Point G = { Gx, Gy };

    template <typename Rnd>
    inline static Wide GenerateRandomPrivateKey(Rnd& rnd)
    {
        Wide d;
        std::uniform_int_distribution<Wide::Base> uniform;
        do
        {
            typename Wide::Array arr;
            for (size_t i = 0; i < arr.size(); ++i)
                arr[i] = uniform(rnd);
            d = arr;
        } while (d == 0 || d >= n);
        return d;
    }

    inline static constexpr bool IsOnCurve(const Point& point)
    {
        if (point.IsInfinity())
            return true;
        const auto lhs = point.y.Squared();
        const auto rhs = (point.x.Squared() + a) * point.x + b;
        return lhs == rhs;
    }

    static_assert(IsOnCurve(G));

    inline static bool IsValidPrivateKey(const Wide& privateKey)
    {
        return privateKey > Wide(0) && privateKey < n;
    }

    inline static bool IsPublicKeyValid(const Point& publicKey)
    {
        if (publicKey.IsInfinity())
            return false;
        if (publicKey.x.x >= p || publicKey.y.x >= p)
            return false;
        if (!IsOnCurve(publicKey))
            return false;
        if (!(n * publicKey).IsInfinity())
            return false;
        return true;
    }

    inline static Point PrivateKeyToPublicKey(const Wide& privateKey)
    {
        if (!IsValidPrivateKey(privateKey))
            throw std::invalid_argument("Invalid private key");

        return privateKey * G;
    }

    template <typename Rnd, typename HashFunc>
    inline static Signature SignMessage(const Wide& privateKey, const char* byteStream, size_t sizeInBytes, Rnd& rnd, HashFunc& hashFunc)
    {
        while (true)
        {
            const Mod_n k = GenerateRandomPrivateKey(rnd);
            const Point R = k * G;
            const Mod_n r = R.x;
            if (r == 0)
                continue;
            const auto H = hashFunc(byteStream, sizeInBytes);
            const Mod_n e = HashToInt(H), d_U = privateKey;
            const Mod_n s = (e + r * d_U) / k;
            if (s != 0)
                return { r.x, s.x };
        }
    }

    template <typename HashFunc>
    inline static bool VerifySignature(const Point& publicKey, const Signature& signature, const char* byteStream, size_t sizeInBytes, HashFunc& hashFunc)
    {
        if (!IsPublicKeyValid(publicKey))
            throw std::invalid_argument("Invalid public key");
        if (signature.first == 0 || signature.first >= n)
            return false;
        if (signature.second == 0 || signature.second >= n)
            return false;
        const auto H = hashFunc(byteStream, sizeInBytes);
        const Mod_n e = HashToInt(H), r = signature.first, s = signature.second;
        const auto sinv = s.Inverse();
        const auto u1 = e * sinv;
        const auto u2 = r * sinv;
        const Point R = u1 * G + u2 * publicKey;
        if (R.IsInfinity())
            return false;
        return R.x.x == r.x;
    }

private:
    template <size_t Size> 
    inline static Wide HashToInt(const std::array<typename Wide::Base, Size>& hash)
    {
        auto H = hash;
        std::reverse(H.begin(), H.end()); // Reverse words to have the correct endian-ness for conversion to wide integer
        return H;
    }
};