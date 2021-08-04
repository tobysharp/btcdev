#pragma once

#include "EC.h"

namespace secp256k1_str
{
    // Values copied from p9 of https://www.secg.org/sec2-v2.pdf
    static constexpr char  p[] = "FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE FFFFFC2F";
    static constexpr char  a[] = "00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000";
    static constexpr char  b[] = "00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000007";
    static constexpr char Gx[] = "79BE667E F9DCBBAC 55A06295 CE870B07 029BFCDB 2DCE28D9 59F2815B 16F81798";
    static constexpr char Gy[] = "483ADA77 26A3C465 5DA4FBFC 0E1108A8 FD17B448 A6855419 9C47D08F FB10D4B8";
    static constexpr char  n[] = "FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141";
}

using secp256k1 = EllipticCurve<secp256k1_str::p, secp256k1_str::a, secp256k1_str::b, secp256k1_str::Gx, secp256k1_str::Gy, secp256k1_str::n>;

static_assert(secp256k1::Wide::BitCount == 256);
