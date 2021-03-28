#include "Wide.h"
#include "Fp.h"

#include <iostream>

int main()
{
    Wide<9, uint8_t> a = { { 0xFF, 0x03 } };
    Wide<9, uint8_t> b = { { 0xFF, 0x03 } };
    auto [q, r] = a.DivideQR(b);

    std::cout << "a = " << a << std::endl;
    std::cout << "b = " << b << std::endl;
    auto c = a - b;
    auto d = Wide<8, uint8_t>{ 0x80 }.AddExtend(Wide<8, uint8_t>{0x7F});
    std::cout << "c = " << c << std::endl;
    std::cout << "d = " << d << std::endl;
    auto ab = a.MultiplyExtend(b);
    auto ab2 = a * b;

    Wide<256, uint32_t> b256 = 0;
    auto b256_2 = b256;
    Wide<1> b1;
    Wide<64> b64;
    Wide<65> b65;
    std::cout << b256 << std::endl;
    std::cout << b1 << std::endl;
    std::cout << b64 << std::endl;
    std::cout << b65 << std::endl;
}