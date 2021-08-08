#include "Bitcoin.h"

#include <iostream>
#include <chrono>

int main()
{
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::mt19937 random(static_cast<unsigned int>(now));

    const auto privateKey = Bitcoin::GeneratePrivateKey(random);
    std::cout << "Private key: " << privateKey << std::endl;

    const auto publicKey = secp256k1::EC::PrivateKeyToPublicKey(privateKey);
    std::cout << "Public key: " << publicKey << std::endl;

    const auto address = Bitcoin::PublicKeyToAddress(publicKey);
    std::cout << "Address: " << address << std::endl;

    const std::string abc = "abc";

    const auto signature = Bitcoin::Sign(privateKey, abc.begin(), abc.end(), random);
    std::cout << "Signature: " << signature << std::endl;

    const bool isVerified = Bitcoin::Verify(publicKey, abc.begin(), abc.end(), signature);
    std::cout << "Verified: " << (isVerified ? "yes" : "no") << std::endl;

    // to/from BIP39
    // xpubs
    // transactions

    std::cout << "Ok" << std::endl;
}