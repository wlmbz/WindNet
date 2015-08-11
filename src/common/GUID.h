// Copyright (C) 2014-2015 ichenq@outlook.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#pragma once

#include <guiddef.h>
#include <string>
#include <algorithm>
#include <functional>
#include "DefType.h"
#include "Range.h"


class CGUID : public GUID
{
public:    
    typedef byte            value_type;
    typedef byte&           reference;
    typedef byte const&     const_reference;
    typedef byte*           iterator;
    typedef const byte*     const_iterator;
    typedef std::size_t     size_type;
    typedef std::ptrdiff_t  difference_type;

public:    
    CGUID();
    explicit CGUID(StringPiece sp);

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
    bool            fromString(StringPiece sp);
    std::string     toString() const;
    bool            isNil() const;
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
    for(auto ch : guid)
    {
        seed ^= static_cast<std::size_t>(ch) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

namespace std {
template <>
struct hash <CGUID>
{
    size_t operator()(const CGUID& key)
    {
        return hash_value(key);
    }
};
}

inline CGUID createGuid()
{
    CGUID guid;
    guid.create();
    return guid;
}

