#include "secp256k1.h"
#include "sha256.h"
#include "DER.h"

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

    std::string abc = "abc";

    auto hash = SHA256::Compute(&abc[0], abc.size());
    std::cout << "Hash: " << hash << std::endl;

    const auto signature = secp256k1::SignMessage(privateKey, &abc[0], abc.size(), random, SHA256::Compute);
    const auto sigstream = DER::EncodeSignature(signature);
    std::cout << "Signature: " << sigstream << std::endl;

    const auto decoded = DER::DecodeSignature<256>(sigstream);
    const bool isVerified = secp256k1::VerifySignature(publicKey, decoded, &abc[0], abc.size(), SHA256::Compute);
    std::cout << "Verified: " << (isVerified ? "yes" : "no") << std::endl;


    // to/from BIP39
    // Bitcoin address

    std::cout << "Ok" << std::endl;
}