#pragma once

// SHA-256
// Implemented from the spec at https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf

#include <array>

namespace SHA256
{

using Hash = std::array<uint32_t, 8>; // 256 bits arranged in 8 DWORDs, from lowest order to highest order

static constexpr Hash s_initialHash = { { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 } };

static constexpr std::array<uint32_t, 64> s_K = { {   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                                                    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                                                    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                                                    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                                                    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                                                    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                                                    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                                                    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2  } };

using Schedule = std::array<uint32_t, 64>;

using Block = std::array<uint32_t, 16>; // 512-bit message block

template <uint8_t Count> inline uint32_t ROTR(uint32_t x)
{
    return (x >> Count) | (x << (32 - Count));
}

template <uint8_t Count> inline uint32_t SHR(uint32_t x)
{
    return x >> Count;
}

inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (~x & z);
}

inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32_t Sigma_0(uint32_t x)
{
    return ROTR<2>(x) ^ ROTR<13>(x) ^ ROTR<22>(x);
}

inline uint32_t Sigma_1(uint32_t x)
{
    return ROTR<6>(x) ^ ROTR<11>(x) ^ ROTR<25>(x);
}

inline uint32_t sigma_0(uint32_t x)
{
    return ROTR<7>(x) ^ ROTR<18>(x) ^ SHR<3>(x);
}

inline uint32_t sigma_1(uint32_t x)
{
    return ROTR<17>(x) ^ ROTR<19>(x) ^ SHR<10>(x);
}

inline void ProcessBlock(const Block& M, Schedule& W, Hash& H)
{
    // Prepare the message schedule {W_t}
    for (uint8_t t = 0; t < 16; ++t)
        W[t] = M[t];
    for (uint8_t t = 16; t < 64; ++t)
        W[t] = sigma_1(W[t - 2]) + W[t - 7] + sigma_0(W[t - 15]) + W[t - 16];

    // Initialize the working variables a-h with the previous hash value
    auto a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];

    for (uint8_t t = 0; t < 64; ++t)
    {
        const uint32_t T1 = h + Sigma_1(e) + Ch(e, f, g) + s_K[t] + W[t];
        const uint32_t T2 = Sigma_0(a) + Maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    // Update the hash value
    H[0] += a; H[1] += b; H[2] += c; H[3] += d;
    H[4] += e; H[5] += f; H[6] += g; H[7] += h;
}

inline Hash ProcessMessage(const Block* messageBlocks, size_t blockCount)
{
    Schedule W;
    Hash H = s_initialHash;
    for (size_t i = 0; i < blockCount; ++i)
        ProcessBlock(messageBlocks[i], W, H);
    return H;
}

inline ComputeHash(const uint8_t* byteStream, size_t sizeInBytes)
{

}

}