#include "Fp.h"

#include <iostream>
#include <random>
#include <chrono>

template <typename Fp>
struct Point
{
    Fp x, y;
};

template <typename Fp, Fp a, Fp b> 
struct EllipticCurve
{
    static constexpr bool IsInfinity(const Point<Fp>& pt)
    {
        return pt.x == 0 && pt.y == 0;
    }
};

// returns floor(sqrt(x))
template <typename Uint>
inline Uint SqrtFloor(const Uint& x)
{
/*
* Newton's method:
* f(u) = u^2 - x
* f'(u) = 2u
* u = x
* u -= f(u) / f'(u)
* u -= (u^2 - x) / 2u
*/
    if (x.IsZero())
        return x;
    Uint u = Uint::Exp2((x.Log2() >> 1) + 1); // In the range (sqrt(x), 2*sqrt(x)] to converge from above for +ve delta
    while (true)
    {
        auto delta = (u.Squared() - x).DivideQR(u + u);
        auto deltaTrunc = delta.first.Truncate<u.BitCount>(); // >= 0
        if (deltaTrunc.IsZero())
        {
            // Reached rounding error
            return delta.second.IsZero() ? u : --u;
        }
        u -= deltaTrunc;
    }
}


template <typename Uint>
inline bool IsPrime(const Uint& x)
{
    // All primes greater than 3 are of the form (6k +- 1) with k > 0.
    if (x <= Uint(3))
        return x > Uint(1);
    if (!(x[0] & 1))
        return false;
    if (x.DivideQR(Uint(3)).second.IsZero())
        return false;

    // Let u = 6k - 1, starting with k = 1.
    Uint u = 5;
    while (u.Squared() <= x)
    {
        if (x.DivideQR(u).second.IsZero() || x.DivideQR(u + Uint(2)).second.IsZero())
            return false;
        u += Uint(6);
    }
    return true;
}

template <typename Uint>
inline bool ValidateECDomainParams(
    const Uint& p,
    const Uint& a,
    const Uint& b,
    const Point<Uint>& G,
    const Uint& n,
    uint32_t h = 1,
    uint32_t t = 256)
{
    static constexpr std::array<uint32_t, 5> tvals = { 80, 112, 128, 192, 256 };
    if (std::find(tvals.begin(), tvals.end(), [&](auto tval) { return tval == t; }) == tvals.end())
        return false;

    // 1. Check that p is an odd prime such that ceil(log_2 p) = 2t if 80 < t < 256,
    // or such that ceil(log_2 p) = 521 if t = 256, or such that ceil(log_2 p) = 192 if t = 80.
    
    // Check that p is odd
    if (!(p[0] & 1))
        return false;
    
    // Check that p is prime
    if (!IsPrime(p))
        return false;
}

// A point on an elliptic curve
template <typename Fp, Fp a, Fp b>
class EFp
{
    static_assert(4 * a.Squared() * a + 27 * b.Squared() != 0, "Invalid elliptic curve parameters a, b");
public:
    using Multi = typename Fp::Type;
    constexpr static const Multi p = Fp::p;
    
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

namespace secp256k1
{
    using Base = uint32_t;
    using Fp = ::Fp<Base, 0xFFFFFC2F, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF>;
    constexpr Fp aa = 0;
    constexpr Fp bb = 7;

    using EC = EFp<Fp, aa, bb>;
    constexpr Fp Gx = { { 0x16F81798, 0x59F2815B, 0x2DCE28D9, 0x029BFCDB, 0xCE870B07, 0x55A06295, 0xF9DCBBAC, 0x79BE667E } };
    constexpr Fp Gy = { { 0xFB10D4B8, 0x9C47D08F, 0xA6855419, 0xFD17B448, 0x0E1108A8, 0x5DA4FBFC, 0x26A3C465, 0x483ADA77 } };
    constexpr EC G = { Gx, Gy };
    static_assert(G.IsOnCurve(), "G not verified on secp256k1 curve.");

    constexpr Fp n  = { { 0xD0364141, 0xBFD25E8C, 0xAF48A03B, 0xBAAEDCE6, 0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF } };
    constexpr Base h = 1;

    template <typename Rnd>
    inline Fp GenerateRandomPrivateKey(Rnd& rnd)
    {
        Fp::Type d;
        do
        {
            std::array<Base, Fp::ElementCount> arr;
            for (size_t i = 0; i < arr.size(); ++i)
                arr[i] = rnd();
            d = arr;
        } while (d.IsZero() || d >= n.x);
        return d;
    }

    inline EC PrivateKeyToPublicKey(const Fp& privateKey)
    {
        return privateKey * G;
    }
}

#include <cassert>

int main()
{
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::mt19937 random(static_cast<unsigned int>(now));
    auto privateKey = secp256k1::GenerateRandomPrivateKey(random);
    auto publicKey = secp256k1::PrivateKeyToPublicKey(privateKey);

    bool prime = IsPrime(secp256k1::Fp::p);
    std::cout << "prime? " << (prime ? "yes " : "no ") << std::endl;
    return 0;

    secp256k1::EC G = secp256k1::G;
    bool yes = G.IsOnCurve();
    std::cout << (yes ? "good" : "bad") << std::endl;
    std::cout << G.x << std::endl << G.y << std::endl;
    /* secp256k1 
    
       p = FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F
       a = 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
       b = 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000007
       G = 04 
           79BE667E F9DCBBAC 55A06295 CE870B07 029BFCDB 2DCE28D9 59F2815B 16F81798 
           483ADA77 26A3C465 5DA4FBFC 0E1108A8 FD17B448 A6855419 9C47D08F FB10D4B8
       n = FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141
       h = 01

       Want to write:

       using EC = EFp<secp256k1_p, secp256k1_a, secp256k1_b>;
       using secp256k1 = ECDomain<EC, secp256k1_G, secp256k1_n, secp256k1_h>;


    */
    using Fp5 = Fp < uint8_t, 0x05, 0x00, 0x00>;
    Fp5 a = 2, b = 3;
    const auto x = a.p;
    auto c = a * b; // = 1
    std::cout << c << std::endl;

    /*
    Wide<9, uint8_t> a = { { 0xFF, 0x03 } };
    Wide<9, uint8_t> b = { { 0xFF, 0x03 } };
    auto [q, r] = a.DivideQR(b);

    typedef Fp<4, uint8_t, 5> Fp5;
    Fp5 a = { 2 }, b = { 3 };
    auto c = a * b;

    std::cout << "a = " << a << std::endl;
    std::cout << "b = " << b << std::endl;
    auto c = a - b;
    auto d = Wide<8, uint8_t>{ 0x80 }.AddExtend(Wide<8, uint8_t>{0x7F});
    std::cout << "c = " << c << std::endl;
    std::cout << "d = " << d << std::endl;
    auto ab = a.MultiplyExtend(b);
    auto ab2 = a * b;

    Wide<256, uint32_t> b256 = 0;
    auto b256_2 = b256;
    Wide<1> b1;
    Wide<64> b64;
    Wide<65> b65;
    std::cout << b256 << std::endl;
    std::cout << b1 << std::endl;
    std::cout << b64 << std::endl;
    std::cout << b65 << std::endl;*/
}