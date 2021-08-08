#pragma once

/*
* ASN.1 is a notation for structured data, and DER is a set of rules for transforming a 
* data structure (described in ASN.1) into a sequence of bytes, and back.
* 
* An ECDSA signature has the following ASN.1 description:

ECDSASignature ::= SEQUENCE {
    r   INTEGER,
    s   INTEGER
}
*/

#include "Wide.h"

#include <span>
#include <vector>

namespace DER
{
    template <size_t Bits> using Signature = std::pair<UIntW<Bits>, UIntW<Bits>>;
    using ByteStream = std::vector<uint8_t>;

    template <size_t Bits>
    unsigned int GetEncodedByteCount(const Signature<Bits>& rs)
    {
        const auto& r = rs.first;
        const auto& s = rs.second;
        const auto bytesPerScalar = Bits << 3;
        const auto bytesToEncodeR = (r.ActualBitCount() >> 3) + 1;
        const auto bytesToEncodeS = (s.ActualBitCount() >> 3) + 1;
        const auto totalEncodingBytes = bytesToEncodeR + bytesToEncodeS + 6;
        return static_cast<unsigned int>(totalEncodingBytes);
    }

    template <size_t Bits>
    std::span<uint8_t> EncodeInteger(const UIntW<Bits>& x, std::span<uint8_t> buffer)
    {
        uint8_t bytesToEncode = static_cast<uint8_t>(x.ActualBitCount() >> 3) + 1; // Minimal length encoding as signed integer
        const uint8_t totalEncodingBytes = bytesToEncode + 2;
        if (totalEncodingBytes > buffer.size())
            throw std::runtime_error("Insufficient buffer size");

        auto pb = buffer.begin();
        *pb++ = 0x02; // Type INTEGER
        *pb++ = bytesToEncode; // Length (Short form)
        if (x.GetBit(Bits - 1)) // Need this byte because we are supposed to write out a signed integer
        {
            *pb++ = 0x00;
            --bytesToEncode;
        }
        for (auto i = 0; i < bytesToEncode; ++i) // Contents
            *pb++ = x.GetByte(bytesToEncode - 1 - i); // High bytes first for big-endian
        return { pb, buffer.end() };
    }

    template <size_t Bits>
    unsigned int EncodeSignature(const Signature<Bits>& rs, std::span<uint8_t> buffer)
    {
        // To use the short form encoding we must have totalEncodingBytes - 2 <= 127.
        // So bytesToEncodeR + bytesToEncodeS <= 123. So max(bytesToEncodeR) <= 61. So Bits < 495.
        static_assert(Bits < 495); // Otherwise this function needs re-writing without short-form length-encoding

        const uint8_t totalEncodingBytes = GetEncodedByteCount(rs);
        if (totalEncodingBytes > buffer.size())
            throw std::runtime_error("Insufficient buffer size");

        auto pb = buffer.begin();
        *pb++ = 0x30;   // SEQUENCE of elements
        *pb++ = totalEncodingBytes - 2; // Length of sequence (Short form)
        
        buffer = EncodeInteger(rs.first, { pb, buffer.end() });
        buffer = EncodeInteger(rs.second, buffer);
        return totalEncodingBytes;
    }

    template <size_t Bits>
    ByteStream EncodeSignature(const Signature<Bits>& rs)
    {
        ByteStream vec(GetEncodedByteCount(rs));
        EncodeSignature(rs, vec);
        return vec;
    }

    template <size_t Bits>
    Signature<Bits> DecodeSignature(std::span<const uint8_t> buffer)
    {
        auto validate = [](bool ok)
        {
            if (!ok)
                throw std::runtime_error("Invalid DER buffer");
        };
        Signature<Bits> rs = { {}, {} };

        validate(buffer.size() >= 6);
        auto pb = buffer.begin();
        validate(*pb++ == 0x30);
        uint8_t totalEncodingBytes = *pb++ + 2;
        validate(totalEncodingBytes <= buffer.size());

        validate(*pb++ == 0x02);
        uint8_t bytesToEncodeR = *pb++;
        validate(bytesToEncodeR << 3 <= Bits + 8);
        for (auto i = 0; i < bytesToEncodeR; ++i, pb++)
            if (*pb != 0) // Avoid writing the extra byte needed purely for the signed format
                rs.first.SetByte(bytesToEncodeR - 1 - i, *pb);

        validate(*pb++ == 0x02);
        uint8_t bytesToEncodeS = *pb++;
        validate(bytesToEncodeS << 3 <= Bits + 8);
        for (auto i = 0; i < bytesToEncodeS; ++i, pb++)
            if (*pb != 0) // Avoid writing the extra byte needed purely for the signed format
                rs.second.SetByte(bytesToEncodeS - 1 - i, *pb);

        return rs;
    }
}