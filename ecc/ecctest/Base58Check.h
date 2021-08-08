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
    std::vector<char> Encode(const ByteArray<N>& bytes, uint8_t version = 0x00)
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

        std::vector<char> base58Digits(leadingZeroBytes, Base58Table[0]);

        // First perform the encoding allowing for leading zeros:
        UIntW<inputWithChecksum.size() * 8> bigInt = inputWithChecksum.ToLittleEndianWords();
        if (bigInt == 0)
            return base58Digits;

        using PowerOf58Type = decltype(bigInt);
        std::stack<PowerOf58Type> powerStack;
        PowerOf58Type powerOf58 = 1;
        while (true)
        {
            powerStack.push(powerOf58);
            auto nextPower = powerOf58 * UIntW<6>(58);
            if (nextPower > bigInt)
                break;
            powerOf58 = nextPower;
        }

        // Now bigInt <= 58^powerIndex
        bool isLeading = true;
        for (; bigInt > UIntW<1>(0); powerStack.pop())
        {
            if (bigInt >= powerStack.top())
            {
                auto qr = bigInt.DivideUnsignedQR(powerStack.top());
                if (qr.first * powerStack.top() + qr.second != bigInt)
                {
                    std::cout << "Dividend: " << bigInt << std::endl;
                    std::cout << "Divisor: " << powerStack.top() << std::endl;
                    std::cout << "Putative quotient: " << qr.first << std::endl;
                    std::cout << "Putative remainder: " << qr.second << std::endl;
                    std::cout << "Reformed as: " << (qr.first * powerStack.top() + qr.second) << std::endl;
                    throw std::runtime_error("Division bug");
                }
                base58Digits.push_back(Base58Table[qr.first.GetByte(0)]); // Write quotient as digit qr.first
                bigInt = qr.second;
                isLeading = false;
            }
            else if (!isLeading)
                base58Digits.push_back(Base58Table[0]); // Write zero Base58 digit
        }
        return base58Digits;
    }
}