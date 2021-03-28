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

    template <typename T> struct DoubleSize {};
    template <> struct DoubleSize<uint8_t> { using type = uint16_t; };
    template <> struct DoubleSize<uint16_t> { using type = uint32_t; };
    template <> struct DoubleSize<uint32_t> { using type = uint64_t; };
}

template <size_t Bits, typename Base = uint32_t>
class Wide
{
public:
    static constexpr size_t BitsPerElement = 8 * sizeof(Base);
    static constexpr size_t ElementCount = (Bits + BitsPerElement - 1) / BitsPerElement;
    static constexpr size_t ValidBitsInHighElement = Bits - BitsPerElement * (ElementCount - 1);
    static constexpr Base HighElementMask = Detail::GetHighElementMask<Base>(BitsPerElement, ValidBitsInHighElement);

    using Array = std::array<Base, ElementCount>;
    using DoubleBase = typename Detail::DoubleSize<Base>::type;

    Wide() : m_a{} {}
    Wide(const Array& a)
    {
        std::copy(a.begin(), a.end(), m_a.begin());
        EnforceBitLimit();
    }
    Wide(const Wide& rhs) : m_a(rhs.m_a) {}
    Wide(Wide&& rhs) : m_a(std::move(rhs.m_a)) {}
    Wide(Base a) : m_a{ a }
    {
        EnforceBitLimit();
    }

    const Array& Elements() const { return m_a; }
    Base& operator[](size_t i) { return m_a[i]; }
    const Base& operator[](size_t i) const { return m_a[i]; }

    template <size_t RBits>
    std::pair<Wide<std::max(Bits, RBits), Base>, bool> AddWithCarry(const Wide<RBits, Base>& rhs, bool carry = false) const
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
        Wide<std::max(Bits, RBits), Base> rv;
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
        *this = *this -(rhs);
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
    friend bool operator !=(const Wide& lhs, const Wide& rhs)
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
        return !operator==(lhs, rhs);
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
