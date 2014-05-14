#include "guid.h"
#include <objbase.h>
#include <atlconv.h>
#include "logging.h"


CGUID::CGUID()
{
    memset(this, 0, sizeof(*this));
}

CGUID::~CGUID()
{
}

CGUID::CGUID(const char* str)
{
    CHECK(from_string(str));
}

CGUID::CGUID(const wchar_t* str)
{
    CHECK(from_string(str));
}

CGUID::CGUID(const CGUID& other)
{
    memcpy(data(), other.data(), sizeof(*this));
}


CGUID& CGUID::operator = (const CGUID& other)
{
    if (this != &other)
    {
        memcpy(data(), other.data(), sizeof(*this));
    }
    return *this;
}

bool CGUID::create()
{
    HRESULT hr = ::CoCreateGuid(this);
    return (hr != S_OK);
}

bool CGUID::from_string(const wchar_t* str)
{
    DCHECK_NOTNULL(str);
    LPOLESTR olestr = const_cast<LPOLESTR>(str);
    return (::IIDFromString(olestr, this) == S_OK);
}

bool CGUID::from_string(const char* str)
{
    DCHECK_NOTNULL(str);
    USES_CONVERSION;
    return from_string(A2W(str));
}

std::string CGUID::to_string() const
{
    char str[40];
    const char* fmt = "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X}";
    int len = sprintf_s(str, _countof(str), fmt, Data1, Data2, Data3, Data4[0],
        Data4[1], Data4[2], Data4[3], Data4[4], Data4[5], Data4[6], Data4[7]);
    DCHECK(len > 0);
    return std::string(str, len);
}

bool CGUID::is_nil() const
{
    for (const_iterator iter = begin(); iter != end(); ++iter)
    {
        if (*iter != 0)
            return false;
    }
    return true;
}

