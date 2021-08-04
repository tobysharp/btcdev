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

    std::string abc = "abc";
    
    auto hash = SHA256::Compute(&abc[0], abc.size());
    std::cout << "Hash: " << hash << std::endl;

    const auto signature = secp256k1::SignMessage(privateKey, &abc[0], abc.size(), random, SHA256::Compute);
    std::cout << "Signature: " << signature.first << signature.second << std::endl;

    const bool isVerified = secp256k1::VerifySignature(publicKey, signature, &abc[0], abc.size(), SHA256::Compute);
    std::cout << "Verified: " << (isVerified ? "yes" : "no") << std::endl;


    // to/from BIP39
    // Bitcoin address

    std::cout << "Ok" << std::endl;
}