#include "Bitcoin.h"

#include <iostream>
#include <chrono>

template <size_t N>
std::ostream& operator <<(std::ostream& os, const std::array<uint8_t, N>& bytes)
{
    for (auto x : bytes)
        os << std::hex << std::setw(2) << std::setfill('0') << +x;
    return os;
}


int main()
{
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::mt19937 random(static_cast<unsigned int>(now));

    static constexpr char priv[] = "18e14a7b6a307f426a94f8114701e7c8e774e7f9a47e2c2035db29a206321725";
    const Bitcoin::PrivateKey privateKey = Parse::GetUIntArray<uint32_t, 8>(priv);
    std::cout << "Private key: " << privateKey << std::endl;

    const auto publicKey = secp256k1::EC::PrivateKeyToPublicKey(privateKey);

    const auto address = Bitcoin::GenerateAddress(publicKey);

    //const std::string abc = "abc";
    //const auto signature = secp256k1::EC::SignMessage(privateKey, &abc[0], abc.size(), random, SHA256::Compute<const char*>);
    //const auto sigstream = DER::EncodeSignature(signature);
    //std::cout << "Signature: " << sigstream << std::endl;

    //const auto decoded = DER::DecodeSignature<256>(sigstream);
    //const bool isVerified = secp256k1::EC::VerifySignature(publicKey, decoded, &abc[0], abc.size(), SHA256::Compute<const char*>);
    //std::cout << "Verified: " << (isVerified ? "yes" : "no") << std::endl;


    // to/from BIP39
    // Bitcoin address

    std::cout << "Ok" << std::endl;
}