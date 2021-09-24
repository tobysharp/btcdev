#include "Bitcoin.h"

#include <iostream>

int main()
{
    std::random_device random;

    const auto privateKey = Bitcoin::GeneratePrivateKey(random);
    std::cout << "Private key: " << privateKey << std::endl;
    std::cout << "Wallet Import Format: " << Bitcoin::PrivateKeyToWalletImportFormat(privateKey) << std::endl;

    const auto publicKey = Bitcoin::EC::PrivateKeyToPublicKey(privateKey);
    std::cout << "Public key: " << publicKey << std::endl;

    const auto address = Bitcoin::PublicKeyToAddress(publicKey);
    std::cout << "Address: " << address << std::endl;

    const std::string abc = "abc";
    std::cout << "Message: " << abc << std::endl;

    const auto signature = Bitcoin::Sign(privateKey, abc.begin(), abc.end(), random);
    std::cout << "Signature: " << signature << std::endl;

    const bool isVerified = Bitcoin::Verify(publicKey, abc.begin(), abc.end(), signature);
    std::cout << "Verified: " << (isVerified ? "yes" : "no") << std::endl;

    return 0;
}