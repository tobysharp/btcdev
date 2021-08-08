#pragma once

#include "secp256k1.h"
#include "Endian.h"
#include "SHA256.h"
#include "RIPEMD160.h"
#include "Base58Check.h"

namespace Bitcoin
{
    using PrivateKey = secp256k1::EC::Wide;
    using PublicKey = secp256k1::EC::Point;
    using LongHash = ByteArray<32>;
    using ShortHash = ByteArray<20>;
    using Address = std::vector<char>;

    template <typename Iter> LongHash SHA256Hash(Iter begin, Iter end)
    {
        return ToBytesAsBigEndian(SHA256::Compute(begin, end));
    }

    template <typename Iter> ShortHash RIPEMD160Hash(Iter begin, Iter end)
    {
        return ToBytesAsLittleEndian(RIPEMD160::Compute(begin, end));
    }

    template <typename Iter> ShortHash DoubleHashShort(Iter begin, Iter end)
    {
        auto hash1 = SHA256Hash(begin, end);
        return RIPEMD160Hash(hash1.begin(), hash1.end());
    }

    template <typename Iter> LongHash DoubleHashLong(Iter begin, Iter end)
    {
        auto hash1 = SHA256Hash(begin, end);
        return SHA256Hash(hash1.begin(), hash1.end());
    }

    Address GenerateAddress(const PublicKey& publicKey, uint8_t version = 0x00)
    {
        const auto compressedPublicKey = publicKey.Compressed();
        const auto hashOfCompressedPublicKey = DoubleHashShort(compressedPublicKey.begin(), compressedPublicKey.end());

        ByteArray<21> bytesToEncode = { version };
        std::copy(hashOfCompressedPublicKey.begin(), hashOfCompressedPublicKey.end(), bytesToEncode.begin() + 1);
        return Base58Check::Encode(bytesToEncode);
    }

    std::ostream& operator <<(std::ostream& os, const Address& address)
    {
        for (auto x : address)
            os << x;
        return os;
    }
}