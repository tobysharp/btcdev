#include "secp256k1.h"
#include "sha256.h"
#include "DER.h"
#include "RIPEMD160.h"

#include <iostream>
#include <chrono>

int main()
{
    const std::string str(1000000, 'a');

    {
        auto h = RIPEMD160::Compute(str.data(), str.size());
        std::cout << "RIPEMD160 hash: " << h << std::endl;
    }
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::mt19937 random(static_cast<unsigned int>(now));

    static constexpr char priv[] = "a966eb6058f8ec9f47074a2faadd3dab42e2c60ed05bc34d39d6c0e1d32b8bdf";
    const secp256k1::Wide privateKey = Parse::GetUIntArray<uint32_t, 8>(priv);
    std::cout << "Private key: " << privateKey << std::endl;

    const auto publicKey = secp256k1::EC::PrivateKeyToPublicKey(privateKey);
    std::cout << "Public key: " << publicKey << std::endl;

    std::string abc = "abc";

    auto hash = SHA256::Compute(&abc[0], abc.size());
    std::cout << "Hash: " << hash << std::endl;

    const auto signature = secp256k1::EC::SignMessage(privateKey, &abc[0], abc.size(), random, SHA256::Compute);
    const auto sigstream = DER::EncodeSignature(signature);
    std::cout << "Signature: " << sigstream << std::endl;

    const auto decoded = DER::DecodeSignature<256>(sigstream);
    const bool isVerified = secp256k1::EC::VerifySignature(publicKey, decoded, &abc[0], abc.size(), SHA256::Compute);
    std::cout << "Verified: " << (isVerified ? "yes" : "no") << std::endl;


    // to/from BIP39
    // Bitcoin address

    std::cout << "Ok" << std::endl;
}