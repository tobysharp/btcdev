#pragma once

#include "EC.h"
#include "sha256.h"
#include <random>

namespace secp256k1
{
    namespace str
    {
        // Values copied from p9 of https://www.secg.org/sec2-v2.pdf
        static constexpr char  p[] = "FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F";
        static constexpr char  a[] = "00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000";
        static constexpr char  b[] = "00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000007";
        static constexpr char Gx[] = "79BE667E F9DCBBAC 55A06295 CE870B07 029BFCDB 2DCE28D9 59F2815B 16F81798";
        static constexpr char Gy[] = "483ADA77 26A3C465 5DA4FBFC 0E1108A8 FD17B448 A6855419 9C47D08F FB10D4B8";
        static constexpr char  n[] = "FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141";
    }
    using Fp = ::Fp<str::p>;
    using Wide = Fp::Type;
    constexpr Fp::Type p = Fp::p;
    constexpr Fp a = Parse::GetUIntArray<str::a>();
    constexpr Fp b = Parse::GetUIntArray<str::b>();
    constexpr Wide n = Parse::GetUIntArray<str::n>();
    using Signature = std::pair<Wide, Wide>;
    using Pt = EFp<Fp, a, b>;
    constexpr Pt G = { Parse::GetUIntArray<str::Gx>(), Parse::GetUIntArray<str::Gy>() };

    static_assert(G.IsOnCurve(), "G not verified on secp256k1 curve.");

    template <typename Rnd>
    inline Fp GenerateRandomPrivateKey(Rnd& rnd)
    {
        Fp::Type d;
        std::uniform_int_distribution<Fp::Type::Base> uniform;
        do
        {
            Fp::Array arr;
            for (size_t i = 0; i < arr.size(); ++i)
                arr[i] = uniform(rnd);
            d = arr;
        } while (d.IsZero() || d >= n);
        return d;
    }

    inline Pt PrivateKeyToPublicKey(const Fp& privateKey)
    {
        return privateKey * G;
    }

    inline bool IsPublicKeyValid(const Pt& publicKey)
    {
        if (publicKey.IsInfinity())
            return false;
        if (publicKey.x.x >= p || publicKey.y.x >= p)
            return false;
        if (!publicKey.IsOnCurve())
            return false;
        if (!(n * publicKey).IsInfinity())
            return false;
        return true;
    }
    
    template <typename Rnd>
    inline Signature SignMessage(Rnd& rnd, const Fp& privateKey, const uint8_t* byteStream, size_t sizeInBytes)
    {
        while (true)
        {
            Wide r;
            Fp k;
            do
            {
                k = GenerateRandomPrivateKey(rnd);
                const Pt R = PrivateKeyToPublicKey(k);
                r = R.x.x >= n ? (R.x.x - n) : R.x.x;
            } while (r.IsZero());
            SHA256::Hash H = SHA256::ComputeHash(byteStream, sizeInBytes);
            std::reverse(H.begin(), H.end()); // Reverse words to have the correct endian-ness for conversion to integer
            const Wide e = H;
            const Wide rdU = ModuloArithmetic::MultiplyModuloM(r, privateKey.x, n);
            const Wide e_rdU = ModuloArithmetic::AddModuloM(e, rdU, n);
            const Wide kinv = ModuloArithmetic::InvertModuloOdd(k.x, n);
            const Wide s = ModuloArithmetic::MultiplyModuloM(kinv, e_rdU, n);
            if (!s.IsZero())
                return { r, s };
        }
    }

    inline bool VerifySignature(const Pt& publicKey, const Signature& signature, const uint8_t* byteStream, size_t sizeInBytes)
    {
        if (signature.first.IsZero() || signature.first >= n)
            return false;
        if (signature.second.IsZero() || signature.second >= n)
            return false;
        auto H = SHA256::ComputeHash(byteStream, sizeInBytes);
        std::reverse(H.begin(), H.end()); // Reverse words to have the correct endian-ness for conversion to integer
        const Wide e = H;
        const Wide sinv = ModuloArithmetic::InvertModuloOdd(signature.second, n);
        const Wide u1 = ModuloArithmetic::MultiplyModuloM(e, sinv, n);
        const Wide u2 = ModuloArithmetic::MultiplyModuloM(signature.first, sinv, n);
        const Pt R = u1 * G + u2 * publicKey;
        if (R.IsInfinity())
            return false;
        const Wide v = R.x.x >= n ? R.x.x - n : R.x.x;
        return signature.first == v;
    }
}
