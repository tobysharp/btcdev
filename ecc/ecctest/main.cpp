#include "Fp.h"

#include <iostream>

// A point on an elliptic curve
template <typename Fp, Fp a, Fp b>
class EFp
{
public:
    static_assert(4 * a.Squared() * a + 27 * b.Squared() != 0, "Invalid elliptic curve parameters a, b");
    using Multi = typename Fp::Type;
    constexpr static const Multi p = Fp::p;
    
    constexpr EFp() {}
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
    // TODO: Get rid of bit count in Wide
    constexpr Fp aa = 0;
    constexpr Fp bb = 7;

    using EC = EFp<Fp, aa, bb>;
    constexpr Fp Gx = { { 0x16F81798, 0x59F2815B, 0x2DCE28D9, 0x029BFCDB, 0xCE870B07, 0x55A06295, 0xF9DCBBAC, 0x79BE667E } };
    constexpr Fp Gy = { { 0xFB10D4B8, 0x9C47D08F, 0xA6855419, 0xFD17B448, 0x0E1108A8, 0x5DA4FBFC, 0x26A3C465, 0x483ADA77 } };
    constexpr EC G = { Gx, Gy };
    //static_assert(G.IsOnCurve(), "Oops");
}

int main()
{
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