#include "secp256k1.h"
#include "sha256.h"

#include <iostream>
#include <chrono>

int main()
{
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::mt19937 random(static_cast<unsigned int>(now));

    const auto privateKey = secp256k1::GenerateRandomPrivateKey(random);
    std::cout << "Private key: " << privateKey << std::endl;

    const auto publicKey = secp256k1::PrivateKeyToPublicKey(privateKey);
    std::cout << "Public key: " << publicKey << std::endl;

    if (!secp256k1::IsPublicKeyValid(publicKey))
        throw std::runtime_error("Invalid public key");

    const char* abc = "abc";
    
    auto hash = SHA256::ComputeHash(reinterpret_cast<const uint8_t*>(abc), strlen(abc));
    std::cout << "Hash: " << hash << std::endl;

    const auto signature = secp256k1::SignMessage(random, privateKey, reinterpret_cast<const uint8_t*>(abc), strlen(abc));
    std::cout << "Signature: " << signature.first << signature.second << std::endl;

    const bool isVerified = secp256k1::VerifySignature(publicKey, signature, reinterpret_cast<const uint8_t*>(abc), strlen(abc));
    std::cout << "Verified: " << (isVerified ? "yes" : "no") << std::endl;


    // to/from BIP39
    // Bitcoin address

    std::cout << "Ok" << std::endl;
}