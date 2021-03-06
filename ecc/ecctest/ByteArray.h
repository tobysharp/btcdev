#pragma once

#include <array>
#include <vector>
#include <algorithm>

template <size_t N = 0>
class ByteArray
{
public:
    constexpr ByteArray() {}
    constexpr ByteArray(size_t size) 
    {
        if (N < size)
            throw std::runtime_error("Static array unexpectedly shorter than desired buffer");
    }
    constexpr ByteArray(const std::array<uint8_t, N>& arr) : bytes(arr) {}
    template <typename... Args> constexpr ByteArray(Args... args) : bytes{ args... } {}
    template <typename Iter> constexpr ByteArray(Iter begin, Iter end)
    {
        std::copy(begin, end, bytes.begin());
    }

    constexpr size_t size() const { return bytes.size(); }
    constexpr auto begin() const { return bytes.begin(); }
    constexpr auto end() const { return bytes.end(); }
    constexpr auto begin() { return bytes.begin(); }
    constexpr auto end() { return bytes.end(); }

    template <size_t Begin, size_t End>
    constexpr ByteArray<End - Begin> SubRange() const
    {
        static_assert(Begin < End && End <= N);
        std::array<uint8_t, End - Begin> rv;
        std::copy(bytes.begin() + Begin, bytes.begin() + End, rv.begin());
        return rv;
    }

    template <size_t R>
    friend ByteArray<N + R> operator |(const ByteArray& lhs, const ByteArray<R>& rhs)
    {
        ByteArray<N + R> rv;
        std::copy(lhs.begin(), lhs.end(), rv.begin());
        std::copy(rhs.begin(), rhs.end(), rv.begin() + lhs.size());
        return rv;
    }

    friend ByteArray<N + 1> operator |(const ByteArray& lhs, uint8_t rhs)
    {
        return lhs | ByteArray<1>(rhs);
    }

    friend ByteArray<N + 1> operator |(uint8_t lhs, const ByteArray& rhs)
    {
        return ByteArray<1>(lhs) | rhs;
    }

    constexpr uint8_t operator[](size_t i) const { return bytes[i]; }
    constexpr uint8_t& operator[](size_t i) { return bytes[i]; }

    template <typename Word = uint32_t>
    std::array<Word, (N + sizeof(Word) - 1) / sizeof(Word)> ToLittleEndianWords() const
    {
        std::array<Word, (N + sizeof(Word) - 1) / sizeof(Word)> rv = {};
        std::reverse_copy(bytes.begin(), bytes.end(), reinterpret_cast<uint8_t*>(&rv[0]));
        return rv;
    }

private:
    std::array<uint8_t, N> bytes;
};

template <>
class ByteArray<0>
{
public:
    ByteArray() {}
    ByteArray(size_t size) : bytes(size) {}
    template <typename Iter> ByteArray(Iter begin, Iter end) : bytes(begin, end) {}
    ByteArray(std::vector<uint8_t>&& rhs) : bytes(std::move(rhs)) {}
    ByteArray(size_t size, uint8_t value) : bytes(size, value) {}

    size_t size() const { return bytes.size(); }
    auto begin() const { return bytes.begin(); }
    auto end() const { return bytes.end(); }
    auto begin() { return bytes.begin(); }
    auto end() { return bytes.end(); }

    void push_back(uint8_t byte) { bytes.push_back(byte); }

    uint8_t operator [](size_t index) const { return bytes[index]; }
    uint8_t& operator [](size_t index) { return bytes[index]; }

    template <size_t  N>
    friend ByteArray operator |(const ByteArray& lhs, const ByteArray<N>& rhs)
    {
        std::vector<uint8_t> rv(lhs.size() + rhs.size());
        std::copy(lhs.begin(), lhs.end(), rv.begin());
        std::copy(rhs.begin(), rhs.end(), rv.begin() + lhs.size());
        return rv;
    }

    friend std::ostream& operator <<(std::ostream& os, const ByteArray& bytes)
    {
        for (auto x : bytes.bytes)
            os << std::hex << std::setw(2) << std::setfill('0') << +x;
        return os;
    }
private:
    std::vector<uint8_t> bytes;
};

// Convert an array of little-endian words to a byte array equivalent to big-endian words
// e.g. { 0x04030201, 0x08070605 } --> { 0x04, 0x03, 0x02, 0x01, 0x08, 0x078, 0x06, 0x05 }
template <size_t N> ByteArray<N * 4> ToBytesAsBigEndian(const std::array<uint32_t, N>& wordArray)
{
    std::array<uint8_t, N * 4> rv;
    auto pd = rv.begin();
    auto ps = wordArray.begin();
    for (size_t i = 0; i < N; ++i, ++ps)
    {
        *pd++ = *ps >> 24;
        *pd++ = *ps >> 16;
        *pd++ = *ps >> 8;
        *pd++ = *ps;
    }
    return rv;
}

// Convert an array of little-endian words to a byte array equivalent to little-endian words
// e.g. { 0x04030201, 0x08070605 } --> { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }
template <size_t N> ByteArray<N * 4> ToBytesAsLittleEndian(const std::array<uint32_t, N>& wordArray)
{
    std::array<uint8_t, N * 4> rv;
    auto pd = rv.begin();
    auto ps = wordArray.begin();
    for (size_t i = 0; i < N; ++i, ++ps)
    {
        *pd++ = *ps;
        *pd++ = *ps >> 8;
        *pd++ = *ps >> 16;
        *pd++ = *ps >> 24;
    }
    return rv;
}
