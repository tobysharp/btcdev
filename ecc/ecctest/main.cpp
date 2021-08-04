#include "secp256k1.h"
#include "sha256.h"

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

#include <cassert>

int main()
{
    const char* abc = "abc";
    auto hash = SHA256::ComputeHash(reinterpret_cast<const uint8_t*>(abc), strlen(abc));
    std::cout << hash << std::endl;

    auto log2 = secp256k1::n.Log2();

    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::mt19937 random(static_cast<unsigned int>(now));
    
    // Generate a private key from random bits
    const auto privateKey = secp256k1::GenerateRandomPrivateKey(random);

    // A public key is a point on the EC that is uniquely determined by the private key
    const auto publicKey = secp256k1::PrivateKeyToPublicKey(privateKey);

    const auto signature = secp256k1::SignMessage(random, privateKey, reinterpret_cast<const uint8_t*>(abc), strlen(abc));
    const bool isVerified = secp256k1::VerifySignature(publicKey, signature, reinterpret_cast<const uint8_t*>(abc), strlen(abc));

    if (!secp256k1::IsPublicKeyValid(publicKey))
        throw std::runtime_error("Invalid public key");

    // The key pair is (private, public)
    const auto keyPair = std::make_pair(privateKey, publicKey);


    // sign, verify, 
    // to/from BIP39
    // Bitcoin address

    std::cout << "Ok" << std::endl;
}