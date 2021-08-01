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

template <size_t Bits, bool IsSigned = false>
class Wide
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

    constexpr Wide() : m_a{} {}
    constexpr Wide(const Array& a)
    {
        std::copy(a.begin(), a.end(), m_a.begin());
        EnforceBitLimit();
    }
    constexpr Wide(const Wide& rhs) : m_a(rhs.m_a) {}
    constexpr Wide(Wide&& rhs) : m_a(std::move(rhs.m_a)) {}
    constexpr Wide(Base a) : m_a{ a }
    {
        EnforceBitLimit();
    }

    constexpr const Array& Elements() const { return m_a; }
    constexpr Base& operator[](size_t i) { return m_a[i]; }
    constexpr const Base& operator[](size_t i) const { return m_a[i]; }

    constexpr bool IsZero() const
    {
        Base x = 0;
        for (auto y : m_a)
            x |= y;
        return x == 0;
    }

    template <size_t RBits>
    constexpr std::pair<Wide<std::max(Bits, RBits), IsSigned>, bool> AddWithCarry(const Wide<RBits, IsSigned>& rhs, bool carry = false) const
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
        Wide<std::max(Bits, RBits), IsSigned> rv;
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
    constexpr Wide<Bits + RBits, false> MultiplyUnsignedExtend(const Wide<RBits, false>& rhs) const
    {
        Wide<Bits + RBits, false> rv = 0;
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
        if constexpr (std::is_signed_v<Base>)
            clearBit = GetBit(BitCount - 1);
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

    static constexpr Wide Exp2(size_t bitIndex)
    {
        Wide x;
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
    constexpr std::pair<Wide<Bits, false>, Wide<RBits, false>> DivideUnsignedQR(const Wide<RBits, false>& rhs) const
    {
        static_assert(RBits <= Bits, "Invalid size for DivideUnsignedQR");
        if (rhs == 0)
            throw std::invalid_argument("Division by zero");

        Wide<Bits, false> remainder = 0;
        Wide<Bits, false> quotient = 0;
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

    constexpr Wide<Bits, IsSigned> ShiftLeftTruncate(size_t shift) const
    {
        Wide<Bits, IsSigned> rv;

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

    constexpr Wide& operator <<=(size_t Shift)
    {
        return operator =(ShiftLeftTruncate(Shift));
    }

    constexpr Wide& operator =(Wide&& rhs)
    {
        m_a = std::move(rhs.m_a);
        return *this;
    }

    template <size_t RBits>
    constexpr Wide<std::max(Bits, RBits) + 1, IsSigned> AddExtend(const Wide<RBits, IsSigned>& rhs) const
    {
        Wide<std::max(Bits, RBits) + 1, IsSigned> rv;
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
    constexpr Wide<std::max(Bits, RBits), IsSigned> AddTruncate(const Wide<RBits, IsSigned>& rhs) const
    {
        return AddWithCarry(rhs).first;
    }

    constexpr Wide<Bits, IsSigned> TwosComplement() const
    {
        Wide<Bits, IsSigned> rv;
        rv.m_a[0] = (Base)(-(std::make_signed_t<Base>)m_a[0]);
        for (size_t i = 1; i < ElementCount; ++i)
            rv.m_a[i] = (Base)(-(std::make_signed_t<Base>)(m_a[i] + 1));
        rv.EnforceBitLimit();
        return rv;
    }

    template <size_t NewBits>
    constexpr Wide<NewBits, IsSigned> Truncate() const
    {
        static_assert(NewBits <= Bits, "Invalid bit count");
        if constexpr (NewBits == Bits)
            return *this;

        typename Wide<NewBits, IsSigned>::Array a;
        std::copy(m_a.begin(), m_a.begin() + Wide<NewBits, IsSigned>::ElementCount, a.begin());
        return a;
    }

    template <size_t NewBits>
    constexpr Wide<NewBits, IsSigned> ZeroExtend() const
    {
        static_assert(NewBits >= Bits, "Invalid bit count");
        typename Wide<NewBits, IsSigned>::Array a = {};
        std::copy(m_a.begin(), m_a.end(), a.begin());
        return a;
    }

    template <size_t NewBits>
    constexpr Wide<NewBits, IsSigned> SignExtend() const
    {
        static_assert(NewBits >= Bits, "Invalid bit count");
        typename Wide<NewBits, IsSigned>::Array a;
        std::fill(a.begin(), a.end(), GetBit(Bits - 1) ? (Base)-1 : 0);
        std::copy(m_a.begin(), m_a.end(), a.begin());
        return a;
    }

    template <size_t NewBits>
    constexpr Wide<NewBits, IsSigned> TypeExtend() const
    {
        static_assert(NewBits >= Bits, "Invalid bit count");
        if constexpr (IsSigned)
            return SignExtend<NewBits>();
        else
            return ZeroExtend<NewBits>();
    }

    template <size_t NewBits>
    constexpr Wide<NewBits, IsSigned> Resize() const
    {
        if constexpr (NewBits > Bits)
            return TypeExtend<NewBits>();
        else if constexpr (NewBits < Bits)
            return Truncate<NewBits>();
        else
            return *this;
    }

    template <size_t RBits>
    friend constexpr auto operator +(const Wide<Bits, IsSigned>& lhs, const Wide<RBits, IsSigned>& rhs)
    {
        //return lhs.AddTruncate(rhs);
        return lhs.AddExtend(rhs);
    }

    template <size_t RBits>
    constexpr Wide& operator +=(const Wide<RBits, IsSigned>& rhs)
    {
        return *this = (*this + rhs).Truncate<Bits>();
    }

    template <size_t RBits>
    friend constexpr auto operator *(const Wide<Bits, IsSigned>& lhs, const Wide<RBits, IsSigned>& rhs)
    {
        static_assert(!IsSigned);
        return lhs.MultiplyUnsignedExtend(rhs);
    }

    constexpr Wide<Bits*2, IsSigned> Squared() const
    {
        // TODO: Optimization
        return *this * *this;
    }

    friend constexpr auto operator -(const Wide<Bits, IsSigned>& lhs)
    {
        return lhs.TwosComplement();
    }

    template <size_t RBits>
    friend constexpr auto operator -(const Wide<Bits, IsSigned>& lhs, const Wide<RBits, IsSigned>& rhs)
    {
        return lhs.TypeExtend<std::max(Bits, RBits)>() + (-rhs.TypeExtend<std::max(Bits, RBits)>());
    }

    template <size_t RBits>
    constexpr Wide& operator -=(const Wide<RBits, IsSigned>& rhs)
    {
        return *this = (*this - rhs).Truncate<Bits>();
    }

    Wide& operator++()
    {
        return *this += Wide(1);
    }

    Wide& operator--()
    {
        return *this -= Wide(1);
    }

    template <size_t RBits>
    friend constexpr bool operator <(const Wide<Bits, IsSigned>& lhs, const Wide<RBits, IsSigned>& rhs)
    {
        static_assert(!IsSigned);
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
    friend constexpr bool operator <=(const Wide<Bits, IsSigned>& lhs, const Wide<RBits, IsSigned>& rhs)
    {
        static_assert(!IsSigned);
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
    friend constexpr bool operator !=(const Wide& lhs, const Wide<RBits, IsSigned>& rhs)
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
    friend constexpr bool operator ==(const Wide& lhs, const Wide<RBits, IsSigned>& rhs)
    {
        return !operator!=(lhs, rhs);
    }

    template <size_t RBits>
    friend constexpr bool operator >(const Wide& lhs, const Wide<RBits, IsSigned>& rhs)
    {
        return rhs < lhs;
    }

    template <size_t RBits>
    friend constexpr bool operator >=(const Wide& lhs, const Wide<RBits, IsSigned>& rhs)
    {
        return rhs <= lhs;
    }

    friend std::ostream& operator <<(std::ostream& s, const Wide& rhs)
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

#if 0
template <typename Base = uint32_t>
class Big
{
public:
    static constexpr size_t BitsPerElement = 8 * sizeof(Base);
    //static constexpr size_t ElementCount = (Bits + BitsPerElement - 1) / BitsPerElement;
    size_t ElementCount() const { return m_a.size(); }

    //static constexpr size_t ValidBitsInHighElement = Bits - BitsPerElement * (ElementCount - 1);
    //static constexpr Base HighElementMask = Detail::GetHighElementMask<Base>(BitsPerElement, ValidBitsInHighElement);
    static constexpr size_t Log2BitsPerElement = Detail::Log2(BitsPerElement);

    using Array = std::vector<Base>;
    using DoubleBase = typename Detail::DoubleSize<Base>::type;

    Wide() : m_a{} {}
    template <typename Iter>
    Wide(Iter begin, Iter end) : m_a(end - begin)
    {
        std::copy(begin, end, m_a.begin());
    }
    Wide(const Wide& rhs) : m_a(rhs.m_a) {}
    Wide(Wide&& rhs) : m_a(std::move(rhs.m_a)) {}
    Wide(Base a) : m_a{ a } {}

    const Array& Elements() const { return m_a; }
    Base& operator[](size_t i) { return m_a[i]; }
    const Base& operator[](size_t i) const { return m_a[i]; }

    void ZeroExtend(size_t elementCount)
    {
        if (elementCount <= m_a.size())
            return;
        m_a.resize(elementCount, 0);
    }

    // Strip leading zeros / sign bits
    void Trim()
    {
        uint size = m_a.size();
        Base leading = 0;
        if (!m_a.empty() && m_a.back() == (Base)-1)
            leading = m_a.back();
        while (size > 0 && m_a[size - 1] != leading)
            --size;
        m_a.resize(size);
    }

    std::pair<Big<Base>, bool> AddWithCarry(const Big<Base>& rhs, bool carry = false) const
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
        Big<Base> rv;
        bool carry = false;
        rv.ZeroExtend(std::max(ElementCount(), rhs.ElementCount()));
        for (size_t i = 0; i < rv.ElementCount(); ++i)
        {
            Base l = i < ElementCount ? m_a[i] : 0;
            Base r = i < rhs.ElementCount ? rhs[i] : 0;
            rv.m_a[i] = l + r + carry;
            auto maxab = std::max(l, r);
            carry = carry ? (rv.m_a[i] <= maxab) : (rv.m_a[i] < maxab);
        }
        rv.Trim();
        return std::make_pair(rv, carry);
    }

    template <size_t RBits>
    Wide<Bits + RBits, Base> MultiplyExtend(const Wide<RBits, Base>& rhs) const
    {
        Wide<Bits + RBits, Base> rv = 0;
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

    bool GetBit(size_t bitIndex) const
    {
        size_t elementIndex = bitIndex >> Log2BitsPerElement;
        size_t bitWithinElement = bitIndex - (elementIndex << Log2BitsPerElement);
        Base bitMask = (Base)1 << bitWithinElement;
        return (m_a[elementIndex] & bitMask) != 0;
    }

    void SetBit(size_t bitIndex, bool value)
    {
        size_t elementIndex = bitIndex >> Log2BitsPerElement;
        size_t bitWithinElement = bitIndex - (elementIndex << Log2BitsPerElement);
        Base bitMask = (Base)1 << bitWithinElement;
        m_a[elementIndex] = (m_a[elementIndex] & ~bitMask) | ((Base)value << bitWithinElement);
    }

    template <size_t RBits>
    std::pair<Wide<Bits, Base>, Wide<RBits, Base>> DivideQR(const Wide<RBits, Base>& rhs) const
    {
        static_assert(RBits <= Bits, "Invalid size for DivideQR");
        if (rhs == 0)
            throw std::invalid_argument("Division by zero");

        Wide<Bits, Base> numerator = 0;
        Wide<RBits, Base> quotient = 0;
        for (size_t bitIndex = Bits - 1; bitIndex != (size_t)-1; --bitIndex)
        {
            numerator <<= 1;
            numerator.SetBit(0, GetBit(bitIndex));
            if (numerator >= rhs)
            {
                numerator -= rhs;
                quotient.SetBit(bitIndex, true);
            }
        }
        return { quotient, numerator };
    }

    Wide<Bits, Base> ShiftLeftTruncate(size_t shift) const
    {
        Wide<Bits, Base> rv;
        const size_t ElementShift = shift >> BitsPerElement;
        const size_t BitShift = shift - (ElementShift << BitsPerElement);
        if (ElementShift > 0)
            throw std::runtime_error("To Do");
        Base prev = 0;
        for (size_t i = 0; i < ElementCount; ++i)
        {
            rv[i] = (m_a[i] << BitShift) | prev;
            prev = m_a[i] >> (BitsPerElement - BitShift);
        }
        return rv;
    }

    constexpr Wide& operator <<=(size_t Shift)
    {
        *this = ShiftLeftTruncate(Shift);
        return *this;
    }

    Wide& operator =(Wide&& rhs)
    {
        m_a = std::move(rhs.m_a);
        return *this;
    }

    template <size_t RBits>
    Wide<std::max(Bits, RBits) + 1, Base> AddExtend(const Wide<RBits, Base>& rhs) const
    {
        Wide<std::max(Bits, RBits) + 1, Base> rv;
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
    Wide<std::max(Bits, RBits), Base> AddTruncate(const Wide<RBits, Base>& rhs) const
    {
        return AddWithCarry(rhs).first;
    }

    Wide TwosComplement() const
    {
        Wide rv;
        rv.m_a[0] = -m_a[0];
        for (size_t i = 1; i < ElementCount; ++i)
            rv.m_a[i] = -(Base)(m_a[i] + 1);
        rv.EnforceBitLimit();
        return rv;
    }

    template <size_t NewBits>
    Wide<NewBits, Base> Truncate() const
    {
        static_assert(NewBits <= Bits, "Invalid bit count");
        typename Wide<NewBits, Base>::Array a = {};
        std::copy(m_a.begin(), m_a.begin() + Wide<NewBits, Base>::ElementCount, a.begin());
        return a;
    }

    template <size_t NewBits>
    Wide<NewBits, Base> Extend() const
    {
        static_assert(NewBits >= Bits, "Invalid bit count");
        typename Wide<NewBits, Base>::Array a = {};
        std::copy(m_a.begin(), m_a.end(), a.begin());
        return a;
    }

    template <size_t NewBits>
    Wide<NewBits, Base> Resize() const
    {
        if constexpr (NewBits > Bits)
            return Extend<NewBits>();
        else if constexpr (NewBits < Bits)
            return Truncate<NewBits>();
        else
            return *this;
    }

    template <size_t RBits>
    friend auto operator +(const Wide& lhs, const Wide<RBits, Base>& rhs)
    {
        return lhs.AddTruncate(rhs);
    }

    template <size_t RBits>
    friend auto operator *(const Wide& lhs, const Wide<RBits, Base>& rhs)
    {
        return lhs.MultiplyExtend(rhs);
    }

    friend auto operator -(const Wide& lhs)
    {
        return lhs.TwosComplement();
    }

    template <size_t RBits>
    friend auto operator -(const Wide& lhs, const Wide<RBits, Base>& rhs)
    {
        return lhs + (-rhs);
    }

    template <size_t RBits>
    Wide<Bits, Base>& operator -=(const Wide<RBits, Base>& rhs)
    {
        *this = *this - (rhs);
        return *this;
    }

    template <size_t RBits>
    friend bool operator <(const Wide& lhs, const Wide<RBits, Base>& rhs)
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
    friend bool operator <=(const Wide& lhs, const Wide<RBits, Base>& rhs)
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
    friend bool operator !=(const Wide& lhs, const Wide<RBits, Base>& rhs)
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
    friend bool operator ==(const Wide& lhs, const Wide<RBits, Base>& rhs)
    {
        return !operator!=(lhs, rhs);
    }

    template <size_t RBits>
    friend bool operator >(const Wide& lhs, const Wide<RBits, Base>& rhs)
    {
        return rhs < lhs;
    }

    template <size_t RBits>
    friend bool operator >=(const Wide& lhs, const Wide<RBits, Base>& rhs)
    {
        return rhs <= lhs;
    }

    friend std::ostream& operator <<(std::ostream& s, const Wide& rhs)
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

    void EnforceBitLimit()
    {
        m_a.back() &= HighElementMask;
    }

private:
    Array m_a;
};
#endif
