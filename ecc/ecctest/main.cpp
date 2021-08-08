#include "Bitcoin.h"

#include <iostream>
#include <chrono>

int main()
{
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::mt19937 random(static_cast<unsigned int>(now));

    static constexpr char priv[] = "18e14a7b6a307f426a94f8114701e7c8e774e7f9a47e2c2035db29a206321725";
    const Bitcoin::PrivateKey privateKey = Parse::GetUIntArray<uint32_t, 8>(priv);
    std::cout << "Private key: " << privateKey << std::endl;

    const auto publicKey = secp256k1::EC::PrivateKeyToPublicKey(privateKey);

    const auto address = Bitcoin::PublicKeyToAddress(publicKey);
    std::cout << "Address: " << address << std::endl;
    std::cout << "Valid: " << (Base58Check::IsEncodingValid(address) ? "yes" : "no") << std::endl;
    std::cout << "Check: " << (address == "1PMycacnJaSqwwJqjawXBErnLsZ7RkXUAs" ? "yes" : "no") << std::endl;

    const std::string abc = "abc";
    const auto signature = Bitcoin::Sign(privateKey, abc.begin(), abc.end(), random);
    std::cout << "Signature: " << signature << std::endl;

    const bool isVerified = Bitcoin::Verify(publicKey, abc.begin(), abc.end(), signature);
    std::cout << "Verified: " << (isVerified ? "yes" : "no") << std::endl;

    // to/from BIP39
    // xpubs

    std::cout << "Ok" << std::endl;
}