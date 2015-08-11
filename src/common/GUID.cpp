#include "guid.h"
#include <objbase.h>
#include "Strings.h"
#include "Logging.h"


inline char toUpper(char ch)
{
    if (ch >= 'a' && ch <= 'z')
        ch -= 'a' - 'A';
    return ch;
}

inline char toChar(size_t i)
{
    if (i <= 9) 
    {
        return static_cast<char>('0' + i);
    }
    else 
    {
        return static_cast<char>('a' + (i - 10));
    }
}

template <bool LSB = true>
inline bool fromHex(void* ptr, size_t size, StringPiece sp)
{
    if (size * 2 != sp.size())
    {
        return false;
    }
    auto pop_one = [&]()
    {
        char ch;
        if (LSB)
        {
            ch = sp.back();
            sp.pop_back();
        }
        else
        {
            ch = sp.front();
            sp.pop_front();
        }
        return ch;
    };
    const char xdigits[] = "0123456789ABCDEF";
    const char* xdigits_end = xdigits + 16;
    uint8_t* out = (uint8_t*)ptr;
    while (sp.size() >= 2)
    {
        char ch = toUpper(pop_one());
        const char* pos = std::find(xdigits, xdigits_end, ch);
        if (pos == xdigits_end)
        {
            return false;
        }
        uint8_t low = static_cast<uint8_t>(pos - xdigits);
        ch = toUpper(pop_one());
        pos = std::find(xdigits, xdigits_end, ch);
        if (pos == xdigits_end)
        {
            return false;
        }
        uint8_t hi = static_cast<uint8_t>(pos - xdigits);
        if (!LSB)
        {
            std::swap(hi, low);
        }
        *out++ = (hi << 4 | low);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////

CGUID::CGUID()
{
    memset(this, 0, sizeof(*this));
}

CGUID::CGUID(StringPiece sp)
{
    CHECK(fromString(sp));
}

bool CGUID::create()
{
    HRESULT hr = CoCreateGuid(this);
    return (hr != S_OK);
}

bool CGUID::fromString(StringPiece sp)
{
    if (sp.size() != 36)
    {
        return false;
    }
    StringPiece sp1, sp2, sp3, sp4, sp5;
    if (split<false>('-', sp, sp1, sp2, sp3, sp4, sp5))
    {
        GUID tmp = {};
        if (fromHex(&tmp.Data1, 4, sp1) &&
            fromHex(&tmp.Data2, 2, sp2) &&
            fromHex(&tmp.Data3, 2, sp3) &&
            fromHex<false>(&tmp.Data4, 2, sp4) &&
            fromHex<false>(&tmp.Data4[2], 6, sp5)
            )
        {
            memcpy(this, &tmp, sizeof(tmp));
            return true;
        }
    }
    return false;
}

std::string CGUID::toString() const
{
    char str[40];
    const char* fmt = "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X";
    int len = sprintf_s(str, _countof(str), fmt, Data1, Data2, Data3, Data4[0],
        Data4[1], Data4[2], Data4[3], Data4[4], Data4[5], Data4[6], Data4[7]);
    DCHECK(len > 0);
    return std::string(str, len);
}

bool CGUID::isNil() const
{
    for (auto iter = begin(); iter != end(); ++iter)
    {
        if (*iter != 0)
            return false;
    }
    return true;
}

