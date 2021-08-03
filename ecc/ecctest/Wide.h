#pragma once

#include <array>
#include <iomanip>

namespace Detail
{
    template <typename Base>
    static constexpr Base GetHighElementMask(size_t BitsPerElement, size_t ValidBitsInHighElement)
    {
        return (ValidBitsInHighElement < BitsPerElement ? ((Base)1 << ValidBitsInHighElement) : 0) - 1;
    }

    static constexpr size_t Log2(size_t x)
    {
        // 1 << rv == x
        size_t rv = 0;
        while (((size_t)1 << rv) < x)
            ++rv;
        return rv;
    }

    template <typename T> struct DoubleSize {};
    template <> struct DoubleSize<uint8_t> { using type = uint16_t; };
    template <> struct DoubleSize<uint16_t> { using type = uint32_t; };
    template <> struct DoubleSize<uint32_t> { using type = uint64_t; };
}

template <size_t Bits>
class UIntW
{
public:
    using Base = uint32_t;
    static_assert(!std::is_signed_v<Base>);
    static constexpr size_t BitCount = Bits;
    static constexpr size_t BitsPerElement = 8 * sizeof(Base);
    static constexpr size_t ElementCount = (Bits + BitsPerElement - 1) / BitsPerElement;
    static constexpr size_t ValidBitsInHighElement = Bits - BitsPerElement * (ElementCount - 1);
    static constexpr Base HighElementMask = Detail::GetHighElementMask<Base>(BitsPerElement, ValidBitsInHighElement);
    static constexpr size_t Log2BitsPerElement = Detail::Log2(BitsPerElement);

    using Array = std::array<Base, ElementCount>;
    using DoubleBase = typename Detail::DoubleSize<Base>::type;

    constexpr UIntW() : m_a{} {}
    constexpr UIntW(const Array& a)
    {
        std::copy(a.begin(), a.end(), m_a.begin());
        EnforceBitLimit();
    }
    constexpr UIntW(const UIntW& rhs) : m_a(rhs.m_a) {}
    constexpr UIntW(UIntW&& rhs) : m_a(std::move(rhs.m_a)) {}
    constexpr UIntW(Base a) : m_a{ a }
    {
        EnforceBitLimit();
    }
    template <size_t RBits> constexpr UIntW(const UIntW<RBits>& rhs)
    {
#ifdef _DEBUG
        if (rhs.ActualBitCount() > Bits)
            throw std::runtime_error("Overflow! Use Truncate to explicitly remove high order bits.");
#endif
        size_t copySize = std::min(rhs.m_a.size(), m_a.size());
        std::copy(rhs.m_a.begin(), rhs.m_a.begin() + copySize, m_a.begin());
        std::fill(m_a.begin() + copySize, m_a.end(), (Base)0);
        EnforceBitLimit();
    }
    constexpr const Array& Elements() const { return m_a; }
    constexpr Base& operator[](size_t i) { return m_a[i]; }
    constexpr const Base& operator[](size_t i) const { return m_a[i]; }

    constexpr bool IsZero() const
    {
        for (auto y : m_a)
            if (y != 0)
                return false;
        return true;
    }

    size_t ActualBitCount() const
    {
        for (size_t elementIndex = ElementCount - 1; elementIndex != (size_t)-1; --elementIndex)
        {
            auto element = m_a[elementIndex];
            if (element != 0)
            {
                size_t usedBits = 0;
                for (; element != 0; ++usedBits, element >>= 1);
                return elementIndex * sizeof(Base) * 8 + usedBits;
            }
        }
        return 0;
    }

    template <size_t RBits>
    constexpr std::pair<UIntW<std::max(Bits, RBits)>, bool> AddWithCarry(const UIntW<RBits>& rhs, bool carry = false) const
    {
        /*
        * Adding with carry:
        * 
        * The true result of (a+b+c) where c is a carry bit will always be in the range
        *  2min(a,b)+c <= a+b+c <= 2max(a,b)+c
        * and we're interested in the case where that caused an overflow so we can set the new carry bit.
        * In that case, the overflowed result is less than the true value,
        *    r = (a+b+c)-2^n < a+b+c
        *  So r <= 2max(a,b) - 2^n + c <= max(a, b) + (2^n - 1) - 2^n + c <= max(a, b) -1 + c < max(a, b) + c
        *  But if there is no overflow, then 
        *    r = (a+b+c) >= max(a,b) + c
        *  So the test (r < max(a,b) + c) is sufficient to determine carry.
        *  Computationally, we can use (c ? (r <= max(a,b)) : (r < max(a,b))) to avoid overflow in the test.
        */
        UIntW<std::max(Bits, RBits)> rv;
        for (size_t i = 0; i < std::max(ElementCount, rhs.ElementCount); ++i)
        {
            Base l = i < ElementCount ? m_a[i] : 0;
            Base r = i < rhs.ElementCount ? rhs[i] : 0;
            rv.m_a[i] = l + r + carry;
            auto maxab = std::max(l, r);
            carry = carry ? (rv.m_a[i] <= maxab) : (rv.m_a[i] < maxab);
        }
        carry |= rv.m_a.back() > HighElementMask;
        rv.EnforceBitLimit();
        return std::make_pair(rv, carry);
    }

    template <size_t RBits>
    constexpr UIntW<Bits + RBits> MultiplyUnsignedExtend(const UIntW<RBits>& rhs) const
    {
        UIntW<Bits + RBits> rv = 0;
        for (size_t i = 0; i < rhs.ElementCount; ++i)
        {
            Base c = 0;
            for (size_t j = 0; j < ElementCount; ++j)
            {
                DoubleBase xy = m_a[j] * (DoubleBase)rhs[i];
                DoubleBase uv = rv[i + j] + xy + c;
                c = (Base)(uv >> BitsPerElement);
                rv[i + j] = (Base)uv;
            }
            if (i + ElementCount < rv.ElementCount)
                rv[i + ElementCount] = c;
        }
        return rv;
    }

    constexpr bool GetBit(size_t bitIndex) const
    {
        size_t elementIndex = bitIndex >> Log2BitsPerElement;
        size_t bitWithinElement = bitIndex - (elementIndex << Log2BitsPerElement);
        Base bitMask = (Base)1 << bitWithinElement;
        return (m_a[elementIndex] & bitMask) != 0;
    }

    // Returns the bit index of the highest bit that is not set to the clear bit.
    // The clear bit is zero for unsigned base types, and equal to the sign bit for signed base types.
    constexpr size_t Log2() const
    {
        bool clearBit = false;
        const Base clearElement = clearBit ? (Base)-1 : 0;
        for (size_t rElementIndex = 0; rElementIndex < ElementCount; ++rElementIndex)
        {
            size_t elementIndex = ElementCount - 1 - rElementIndex;
            if (m_a[elementIndex] != clearElement)
            {
                for (size_t rBitIndex = 0; rBitIndex < BitsPerElement; ++rBitIndex)
                {
                    size_t bitIndex = (elementIndex + 1) * BitsPerElement - 1 - rBitIndex;
                    if (GetBit(bitIndex))
                        return bitIndex;
                }
            }
        }
        return -1;
    }

    static constexpr UIntW Exp2(size_t bitIndex)
    {
        UIntW x;
        x.SetBit(bitIndex, true);
        return x;
    }

    constexpr void SetBit(size_t bitIndex, bool value)
    {
        size_t elementIndex = bitIndex >> Log2BitsPerElement;
        size_t bitWithinElement = bitIndex - (elementIndex << Log2BitsPerElement);
        Base bitMask = (Base)1 << bitWithinElement;
        m_a[elementIndex] = (m_a[elementIndex] & ~bitMask) | ((Base)value << bitWithinElement);
    }

    template <size_t RBits>
    constexpr std::pair<UIntW<Bits>, UIntW<RBits>> DivideUnsignedQR(const UIntW<RBits>& rhs) const
    {
        static_assert(RBits <= Bits, "Invalid size for DivideUnsignedQR");
        if (rhs == 0)
            throw std::invalid_argument("Division by zero");

        UIntW<Bits> remainder = 0;
        UIntW<Bits> quotient = 0;
        for (size_t bitIndex = Bits - 1; bitIndex != (size_t)-1; --bitIndex)
        {
            remainder <<= 1; // BUG: Have to be careful not to truncate before ">= rhs" test.
            remainder.SetBit(0, GetBit(bitIndex));
            if (remainder >= rhs)
            {
                remainder -= rhs;
                quotient.SetBit(bitIndex, true);
            }
        }
        return { quotient, remainder.Truncate<RBits>() };
    }

    constexpr UIntW<Bits> ShiftLeftTruncate(size_t shift) const
    {
        UIntW<Bits> rv;

        //const size_t ElementShift = shift >> BitsPerElement;
        const size_t BitShift = shift;// -(ElementShift << BitsPerElement);
        //if (ElementShift > 0)
        //    throw std::runtime_error("To Do");
        Base prev = 0;
        for (size_t i = 0; i < ElementCount; ++i)
        {
            rv[i] = (m_a[i] << BitShift) | prev;
            prev = m_a[i] >> (BitsPerElement - BitShift);
        }
        return rv;
    }

    constexpr UIntW<Bits> ShiftLogicalRight(size_t shift) const
    {
        UIntW<Bits> rv;
        Base prev = 0;
        for (size_t i = ElementCount - 1; i != (size_t)-1; --i)
        {
            rv[i] = (m_a[i] >> shift) | prev;
            prev = m_a[i] << (BitsPerElement - shift);
        }
        return rv;
    }

    constexpr UIntW<Bits> operator >>(size_t Shift) const
    {
        return ShiftLogicalRight(Shift);
    }

    constexpr UIntW& operator <<=(size_t Shift)
    {
        return operator =(ShiftLeftTruncate(Shift));
    }

    constexpr UIntW& operator >>=(size_t Shift)
    {
        return operator =(ShiftLogicalRight(Shift));
    }

    constexpr UIntW& operator =(UIntW&& rhs)
    {
        m_a = std::move(rhs.m_a);
        return *this;
    }

    template <size_t RBits>
    constexpr UIntW<std::max(Bits, RBits) + 1> AddExtend(const UIntW<RBits>& rhs) const
    {
        UIntW<std::max(Bits, RBits) + 1> rv;
        bool carry = false;
        Base maxab;
        size_t i = 0;
        for (; i < std::max(ElementCount, rhs.ElementCount); ++i)
        {
            Base l = i < ElementCount ? m_a[i] : 0;
            Base r = i < rhs.ElementCount ? rhs[i] : 0;
            rv[i] = l + r + carry;
            maxab = std::max(l, r);
            carry = carry ? (rv[i] <= maxab) : (rv[i] < maxab);
        }
        if (i < rv.ElementCount)
            rv[i] = carry;
        return rv;
    }

    template <size_t RBits>
    constexpr UIntW<std::max(Bits, RBits)> AddTruncate(const UIntW<RBits>& rhs) const
    {
        return AddWithCarry(rhs).first;
    }

    constexpr UIntW<Bits> TwosComplement() const
    {
        UIntW<Bits> rv;
        rv.m_a[0] = (Base)(-(std::make_signed_t<Base>)m_a[0]);
        for (size_t i = 1; i < ElementCount; ++i)
            rv.m_a[i] = (Base)(-(std::make_signed_t<Base>)(m_a[i] + 1));
        rv.EnforceBitLimit();
        return rv;
    }

    template <size_t NewBits>
    constexpr UIntW<NewBits> Truncate() const
    {
        static_assert(NewBits <= Bits, "Invalid bit count");
        if constexpr (NewBits == Bits)
            return *this;

        typename UIntW<NewBits>::Array a;
        std::copy(m_a.begin(), m_a.begin() + UIntW<NewBits>::ElementCount, a.begin());
        return a;
    }

    template <size_t NewBits>
    constexpr UIntW<NewBits> ZeroExtend() const
    {
        static_assert(NewBits >= Bits, "Invalid bit count");
        typename UIntW<NewBits>::Array a = {};
        std::copy(m_a.begin(), m_a.end(), a.begin());
        return a;
    }

    template <size_t NewBits>
    constexpr UIntW<NewBits> SignExtend() const
    {
        static_assert(NewBits >= Bits, "Invalid bit count");
        typename UIntW<NewBits>::Array a;
        std::fill(a.begin(), a.end(), GetBit(Bits - 1) ? (Base)-1 : 0);
        std::copy(m_a.begin(), m_a.end(), a.begin());
        return a;
    }

    template <size_t NewBits>
    constexpr UIntW<NewBits> TypeExtend() const
    {
        static_assert(NewBits >= Bits, "Invalid bit count");
        return ZeroExtend<NewBits>();
    }

    template <size_t NewBits>
    constexpr UIntW<NewBits> Resize() const
    {
        if constexpr (NewBits > Bits)
            return TypeExtend<NewBits>();
        else if constexpr (NewBits < Bits)
            return Truncate<NewBits>();
        else
            return *this;
    }

    template <size_t RBits>
    friend constexpr auto operator +(const UIntW<Bits>& lhs, const UIntW<RBits>& rhs)
    {
        //return lhs.AddTruncate(rhs);
        return lhs.AddExtend(rhs);
    }

    template <size_t RBits>
    constexpr UIntW& operator +=(const UIntW<RBits>& rhs)
    {
        return *this = (*this + rhs).Truncate<Bits>();
    }

    template <size_t RBits>
    friend constexpr auto operator *(const UIntW<Bits>& lhs, const UIntW<RBits>& rhs)
    {
        return lhs.MultiplyUnsignedExtend(rhs);
    }

    constexpr UIntW<Bits*2> Squared() const
    {
        // TODO: Optimization
        return *this * *this;
    }

    friend constexpr auto operator -(const UIntW<Bits>& lhs)
    {
        return lhs.TwosComplement();
    }

    template <size_t RBits>
    friend constexpr auto operator -(const UIntW<Bits>& lhs, const UIntW<RBits>& rhs)
    {
        return lhs.AddTruncate(-rhs.ZeroExtend<std::max(Bits, RBits)>());
    }

    template <size_t RBits>
    constexpr UIntW& operator -=(const UIntW<RBits>& rhs)
    {
        return *this = (*this - rhs).Truncate<Bits>();
    }

    UIntW& operator++()
    {
        return *this += UIntW(1);
    }

    UIntW& operator--()
    {
        return *this -= UIntW(1);
    }

    template <size_t RBits>
    friend constexpr bool operator <(const UIntW<Bits>& lhs, const UIntW<RBits>& rhs)
    {
        // Start with the high order elements
        for (size_t i = std::max(lhs.ElementCount, rhs.ElementCount) - 1; i != (size_t)-1; --i)
        {
            if (i >= lhs.ElementCount)
            {
                if (rhs[i] > 0)
                    return true;
            }
            else if (i >= rhs.ElementCount)
            {
                if (lhs[i] > 0)
                    return false;
            }
            else if (lhs.m_a[i] < rhs.m_a[i])
                return true;
            else if (rhs.m_a[i] < lhs.m_a[i])
                return false;
        }
        return false;
    }

    template <size_t RBits>
    friend constexpr bool operator <=(const UIntW<Bits>& lhs, const UIntW<RBits>& rhs)
    {
        // Start with the high order elements
        for (size_t i = std::max(lhs.ElementCount, rhs.ElementCount) - 1; i != (size_t)-1; --i)
        {
            if (i >= lhs.ElementCount)
            {
                if (rhs[i] > 0)
                    return true;
            }
            else if (i >= rhs.ElementCount)
            {
                if (lhs[i] > 0)
                    return false;
            }
            else if (lhs.m_a[i] < rhs.m_a[i])
                return true;
            else if (rhs.m_a[i] < lhs.m_a[i])
                return false;
        }
        return true;
    }

    template <size_t RBits>
    friend constexpr bool operator !=(const UIntW& lhs, const UIntW<RBits>& rhs)
    {
        for (size_t i = 0; i < std::max(lhs.ElementCount, rhs.ElementCount); ++i)
        {
            if (i >= lhs.ElementCount && rhs[i] != 0)
                return true;
            else if (i >= rhs.ElementCount && lhs[i] != 0)
                return true;
            else if (lhs.m_a[i] != rhs.m_a[i])
                return true;
        }
        return false;
    }

    template <size_t RBits>
    friend constexpr bool operator ==(const UIntW& lhs, const UIntW<RBits>& rhs)
    {
        return !operator!=(lhs, rhs);
    }

    template <size_t RBits>
    friend constexpr bool operator >(const UIntW& lhs, const UIntW<RBits>& rhs)
    {
        return rhs < lhs;
    }

    template <size_t RBits>
    friend constexpr bool operator >=(const UIntW& lhs, const UIntW<RBits>& rhs)
    {
        return rhs <= lhs;
    }

    Base operator &(const Base& rhs) const
    {
        return m_a[0] & rhs;
    }

    friend std::ostream& operator <<(std::ostream& s, const UIntW& rhs)
    {
        constexpr int charsPerElement = BitsPerElement / 4;
        auto i = rhs.m_a.rbegin();
        s << "0x ";
        while (true)
        {
            s << std::hex << std::setw(charsPerElement) << std::setfill('0') << static_cast<size_t>(*i++);
            if (i == rhs.m_a.rend())
                break;
            s << " ";
        }
        return s;
    }

    constexpr void EnforceBitLimit()
    {
        m_a.back() &= HighElementMask;
    }

    Array m_a;
};