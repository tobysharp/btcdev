## btcdev

A sandbox for exploration of Bitcoin-related code including ECDSA, SHA256, etc.

For those curious, this was a vacation project for me during two weeks in Jamaica's rainy season! 
I implemented ECDSA secp256k1, SHA256, RIPEMD160, and DER from the specs without reference to existing implementations, purely for my own learning and development.
Now sharing by popular request. Note: I'm sure other, more mature implementations are better in many ways, and there are many ways in which this could be improved. 
This code has not been thoroughly tested and therefore should NOT be used for any Bitcoin transactions involving nonzero value.

This repo is modern C++. Code should be mostly self-explanatory. 

## References
- ECDSA: https://www.secg.org/sec1-v2.pdf
- secp256k1: https://www.secg.org/sec2-v2.pdf
- SHA-256: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
- RIPEMD160: https://homes.esat.kuleuven.be/~bosselae/ripemd160/pdf/AB-9601/AB-9601.pdf
- ASN.1 DER: https://www.itu.int/ITU-T/studygroups/com17/languages/X.690-0207.pdf
- Base58Check: https://en.bitcoin.it/wiki/Base58Check_encoding

## Tour

### main.cpp

Here is a short test program to exercise the functionality.

### Wide.h: `class UIntW<Bits>`

This class implements arbitrary-length unsigned integers and their arithmetic operations, commonly known as _bigint_. 
I thought it might be an advantage to template on the bit-length since this is often known at compile-time. 

### Fp.h: `class Fp<Bits, p>`

This class represents a member of the finite field F_p where p is an unsigned integer of length Bits. p may be any odd value but in practice will be a prime >= 3.
Here the modulo arithmetic is performed.

### EC.h: `class EllipticCurve<Bits, p, a, b, Gx, Gy, n>`
  
This class represents an elliptic curve specified by the prime p, the constants a, b the generator point (Gx, Gy) and the size n. 
Private key generation, public key transformation, message signing and verifying, are all static member operations of the curve object.

### secp256k1.h
  
Here are the constants for the specific elliptic curve named secp256k1.
  
### SHA256.h
  
Implementation of SHA-256 hashing, from the spec listed above. Pretty standard I presume.
  
### RIPEMD160.h
  
Likewise the implementation of RIPEMD160, the other (shorter) hash algorithm used by Bitcoin.
  
### DER.h

Distinguished Encoding Rules (DER) is a subset of Abstract Syntax Notation One (ASN.1) used by Bitcoin for encoding and decoding signatures as byte strings
  
### Base58Check.h
 
Base58Check is used to map byte strings to/from alphanumeric strings e.g. for Bitcoin addresses. It's a modified form of base-58 encoding.
  
### Bitcoin.h
  
Gathering all the pieces above into a collection of routines that allow, for example, generating private and public keys, generating Bitcoin addresses, 
signing and verifying messages, etc.
