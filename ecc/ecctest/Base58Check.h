#pragma once

#include "SHA256.h"
#include "Endian.h"

#include <vector>
#include <stack>

#include <iostream>

namespace Base58Check
{

    static constexpr std::array<char, 58> Base58Table =
    {
        '1', '2', '3', '4', '5', '6', '7', '8', '9', 
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M',
        'N', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
    };

    template <size_t N>
    std::string Encode(const ByteArray<N>& bytes, uint8_t version = 0x00)
    {
        const auto hash1 = ToBytesAsBigEndian(SHA256::Compute(bytes.begin(), bytes.end()));
        const auto hash2 = ToBytesAsBigEndian(SHA256::Compute(hash1.begin(), hash1.end()));
        auto inputWithChecksum = bytes | hash2.SubRange<0, 4>();
        
        // The number of Base58 characters in the result will be:
        // the number of leading zero bytes in the bytes array, plus
        // the number of Base58 characters to represent bytes = (N+5)*256/58, minus
        // the number of leading zeros in the Base58 representation.
        // i.e. it depends on the payload bytes
        size_t leadingZeroBytes = 0;
        while (leadingZeroBytes < inputWithChecksum.size() && bytes[leadingZeroBytes] == 0)
            ++leadingZeroBytes;

        std::string base58Digits(leadingZeroBytes, Base58Table[0]);

        // First perform the encoding allowing for leading zeros:
        using Wide = UIntW<inputWithChecksum.size() * 8>;
        Wide bigInt = inputWithChecksum.ToLittleEndianWords();
        if (bigInt == 0)
            return base58Digits;

        std::stack<Wide> powerStack;
        Wide powerOf58 = 1;
        while (true)
        {
            powerStack.push(powerOf58);
            auto nextPower = powerOf58 * 58;
            if (nextPower > bigInt)
                break;
            powerOf58 = nextPower;
        }

        // Now bigInt <= 58^powerIndex
        for (bool isLeading = true; bigInt > 0; powerStack.pop())
        {
            if (bigInt >= powerStack.top())
            {
                auto qr = bigInt.DivideUnsignedQR(powerStack.top());
                base58Digits.push_back(Base58Table[static_cast<uint32_t>(qr.first)]); // Write quotient as digit qr.first
                bigInt = qr.second;
                isLeading = false;
            }
            else if (!isLeading) // Write zero Base58 digit if it's not in a leading position
                base58Digits.push_back(Base58Table[0]); 
        }
        return base58Digits;
    }

    constexpr std::array<uint8_t, 256> GetReverseLUT()
    {
        std::array<uint8_t, 256> table;
        for (uint8_t i = 0; i < static_cast<uint8_t>(Base58Table.size()); ++i)
            table[Base58Table[i]] = i;
        return table;
    }

    bool IsEncodingValid(const std::string& encodedString)
    {
        const auto reverseLUT = GetReverseLUT();
        UIntW<200> sum = 0;

        for (auto i = encodedString.begin(); i != encodedString.end(); ++i)
        {
            sum *= 58;
            if (*i != '1')
                sum += reverseLUT[*i];
        }
        ByteArray<> inputWithChecksum(sum.beginBigEndianBytes(), sum.endBigEndianBytes());
        if (inputWithChecksum.size() < 4)
            return false;
        const auto hash1 = ToBytesAsBigEndian(SHA256::Compute(inputWithChecksum.begin(), inputWithChecksum.end() - 4));
        const auto hash2 = ToBytesAsBigEndian(SHA256::Compute(hash1.begin(), hash1.end()));
        const auto checksum = hash2.SubRange<0, 4>();
        const auto equal = std::equal(checksum.begin(), checksum.end(), inputWithChecksum.end() - 4);
        return equal;
    }
}