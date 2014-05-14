/**
 *  @file:   GUID.h
 *  @author: ichenq@gmail.com
 *  @date:   Oct 11, 2011
 *  @brief:  globally unique identifiers
 *
 */

#pragma once

#include "DefType.h"
#include <guiddef.h>
#include <string>
#include <algorithm>
#include <functional>


class CGUID : public GUID
{
public:    
    enum {MIN_LEN = 38}; // '{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}' format

    typedef byte            value_type;
    typedef byte&           reference;
    typedef byte const&     const_reference;
    typedef byte*           iterator;
    typedef const byte*     const_iterator;
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  difference_type;

public:    
    CGUID();
    explicit CGUID(const char* str);
    explicit CGUID(const wchar_t* str);
    CGUID(const CGUID& other);
    ~CGUID();

    CGUID& operator = (const CGUID& other);

    size_type       size() const    {return sizeof(*this); }
    byte*           data()          { return reinterpret_cast<byte*>(this); }
    const byte*     data() const    { return reinterpret_cast<const byte*>(this); }

    // iterator
    iterator        begin()         {return data(); }
    const_iterator  begin() const   {return data(); }
    iterator        end()           {return data() + size(); }
    const_iterator  end() const     {return data() + size(); }
    
    // interface
    bool            create();
    bool            from_string(const wchar_t* str);
    bool            from_string(const char* str);
    std::string     to_string() const;
    bool            is_nil() const;
};


// comparison
inline bool operator == (const CGUID& lhs, const CGUID& rhs)
{
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

inline bool operator < (const CGUID& lhs, const CGUID& rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

inline bool operator > (const CGUID& lhs, const CGUID& rhs) 
{
    return rhs < lhs;
}

inline bool operator <= (const CGUID& lhs, const CGUID& rhs) 
{
    return !(rhs < lhs);
}

inline bool operator >= (const CGUID& lhs, const CGUID& rhs) 
{
    return !(lhs < rhs);
}

// hash container support
inline size_t hash_value(const CGUID& guid)
{
    std::size_t seed = 0;
    for(CGUID::const_iterator iter = guid.begin(); iter != guid.end(); ++iter)
    {
        seed ^= static_cast<std::size_t>(*iter) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

inline CGUID create_guid()
{
    CGUID guid;
    guid.create();
    return guid;
}

