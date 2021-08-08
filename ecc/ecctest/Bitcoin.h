#pragma once

#include "secp256k1.h"
#include "SHA256.h"
#include "RIPEMD160.h"
#include "Base58Check.h"
#include "DER.h"

namespace Bitcoin
{
    using EC = secp256k1::EC;
    using PrivateKey = EC::Wide;
    using PublicKey = EC::Point;
    using LongHash = ByteArray<32>;
    using ShortHash = ByteArray<20>;
    using Address = std::string;
    using Signature = ByteArray<>;

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

    template <typename Rnd>
    PrivateKey GeneratePrivateKey(Rnd& rnd)
    {
        return EC::GenerateRandomPrivateKey(rnd);
    }

    Address PublicKeyToAddress(const PublicKey& publicKey, uint8_t version = 0x00)
    {
        const auto compressedPublicKey = publicKey.Compressed();
        const auto hashOfCompressedPublicKey = DoubleHashShort(compressedPublicKey.begin(), compressedPublicKey.end());

        ByteArray<21> bytesToEncode = { version };
        std::copy(hashOfCompressedPublicKey.begin(), hashOfCompressedPublicKey.end(), bytesToEncode.begin() + 1);
        return Base58Check::Encode(bytesToEncode);
    }

    template <typename Iter, typename Rnd>
    Signature Sign(const PrivateKey& privateKey, Iter begin, Iter end, Rnd& rnd)
    {
        const char* beginChar = (begin < end) ? reinterpret_cast<const char*>(&begin[0]) : nullptr;
        const char* endChar = (begin < end) ? reinterpret_cast<const char*>(&begin[0] + (end - begin)) : nullptr;
        const size_t sizeChars = static_cast<size_t>(endChar - beginChar);
        const auto signature = EC::SignMessage(privateKey, beginChar, sizeChars, rnd, SHA256::Compute<const char*>);
        return DER::EncodeSignature(signature);
    }

    template <typename Iter>
    bool Verify(const PublicKey& publicKey, Iter begin, Iter end, const Signature& signature)
    {
        const char* beginChar = (begin < end) ? reinterpret_cast<const char*>(&begin[0]) : nullptr;
        const char* endChar = (begin < end) ? reinterpret_cast<const char*>(&begin[0] + (end - begin)) : nullptr;
        const size_t sizeChars = static_cast<size_t>(endChar - beginChar);
        const auto decoded = DER::DecodeSignature<256>(signature);
        return EC::VerifySignature(publicKey, decoded, beginChar, sizeChars, SHA256::Compute<const char*>);
    }
}