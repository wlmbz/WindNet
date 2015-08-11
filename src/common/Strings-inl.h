/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <iterator>
#include <type_traits>

namespace detail {

/*
 * The following functions are type-overloaded helpers for
 * internalSplit().
 */
inline size_t delimSize(char)          { return 1; }
inline size_t delimSize(StringPiece s) { return s.size(); }
inline bool atDelim(const char* s, char c) {
 return *s == c;
}
inline bool atDelim(const char* s, StringPiece sp) {
  return !memcmp(s, sp.start(), sp.size());
}

// These are used to short-circuit internalSplit() in the case of
// 1-character strings.
inline char delimFront(char c) {
  // This one exists only for compile-time; it should never be called.
  abort();
  return c;
}
inline char delimFront(StringPiece s) {
  assert(!s.empty() && s.start() != nullptr);
  return *s.start();
}


/*
 * These output conversion templates allow us to support multiple
 * output string types, even when we are using an arbitrary
 * OutputIterator.
 */
template<class OutStringT> struct OutputConverter {};

template<> struct OutputConverter<std::string> {
  std::string operator()(StringPiece sp) const {
    return sp.toString();
  }
};

template<> struct OutputConverter<StringPiece> {
  StringPiece operator()(StringPiece sp) const { return sp; }
};


/*
 * Shared implementation for all the split() overloads.
 *
 * This uses some external helpers that are overloaded to let this
 * algorithm be more performant if the deliminator is a single
 * character instead of a whole string.
 *
 * @param ignoreEmpty iff true, don't copy empty segments to output
 */
template<class OutStringT, class DelimT, class OutputIterator>
void internalSplit(DelimT delim, StringPiece sp, OutputIterator out,
    bool ignoreEmpty) {
  assert(sp.empty() || sp.start() != nullptr);

  const char* s = sp.start();
  const size_t strSize = sp.size();
  const size_t dSize = delimSize(delim);

  OutputConverter<OutStringT> conv;

  if (dSize > strSize || dSize == 0) {
    if (!ignoreEmpty || strSize > 0) {
      *out++ = conv(sp);
    }
    return;
  }
  if (std::is_same<DelimT,StringPiece>::value && dSize == 1) {
    // Call the char version because it is significantly faster.
    return internalSplit<OutStringT>(delimFront(delim), sp, out,
      ignoreEmpty);
  }

  size_t tokenStartPos = 0;
  size_t tokenSize = 0;
  for (size_t i = 0; i <= strSize - dSize; ++i) {
    if (atDelim(&s[i], delim)) {
      if (!ignoreEmpty || tokenSize > 0) {
        *out++ = conv(StringPiece(&s[tokenStartPos], tokenSize));
      }

      tokenStartPos = i + dSize;
      tokenSize = 0;
      i += dSize - 1;
    } else {
      ++tokenSize;
    }
  }
  tokenSize = strSize - tokenStartPos;
  if (!ignoreEmpty || tokenSize > 0) {
    *out++ = conv(StringPiece(&s[tokenStartPos], tokenSize));
  }
}


template<class String> StringPiece prepareDelim(const String& s) {
  return StringPiece(s);
}
inline char prepareDelim(char c) { return c; }

template <class Dst>
struct convertTo {
  template <class Src>
  static Dst from(const Src& src) { return to<Dst>(src); }
  static Dst from(const Dst& src) { return src; }
};


template<bool exact,
         class Delim,
         class OutputType>
typename std::enable_if<
    std::is_arithmetic<OutputType>::value ||
    detail::IsSizableString<OutputType>::value,
    bool>::type
splitFixed(const Delim& delimiter,
           StringPiece input,
           OutputType& out) {
  if (exact && std::string::npos != input.find(delimiter)) {
    return false;
  }
  out = convertTo<OutputType>::from(input);
  return true;
}

template<bool exact,
         class Delim,
         class OutputType,
         class... OutputTypes>
typename std::enable_if<
    std::is_arithmetic<OutputType>::value ||
    detail::IsSizableString<OutputType>::value,
    bool>::type
splitFixed(const Delim& delimiter,
           StringPiece input,
           OutputType& outHead,
           OutputTypes&... outTail) {
  size_t cut = input.find(delimiter);
  if (cut == std::string::npos) {
    return false;
  }
  StringPiece head(input.begin(), input.begin() + cut);
  StringPiece tail(input.begin() + cut + detail::delimSize(delimiter),
                   input.end());
  if (splitFixed<exact>(delimiter, tail, outTail...)) {
    outHead = convertTo<OutputType>::from(head);
    return true;
  }
  return false;
}

} // namespace detail


//////////////////////////////////////////////////////////////////////

template<class Delim, class String, class OutputType>
void split(const Delim& delimiter,
           const String& input,
           std::vector<OutputType>& out,
           bool ignoreEmpty) {
  detail::internalSplit<OutputType>(
    detail::prepareDelim(delimiter),
    StringPiece(input),
    std::back_inserter(out),
    ignoreEmpty);
}


template<class OutputValueType, class Delim, class String,
         class OutputIterator>
void splitTo(const Delim& delimiter,
             const String& input,
             OutputIterator out,
             bool ignoreEmpty) {
  detail::internalSplit<OutputValueType>(
    detail::prepareDelim(delimiter),
    StringPiece(input),
    out,
    ignoreEmpty);
}


template<bool exact,
         class Delim,
         class OutputType,
         class... OutputTypes>
typename std::enable_if<
    std::is_arithmetic<OutputType>::value ||
    detail::IsSizableString<OutputType>::value,
    bool>::type
split(const Delim& delimiter,
      StringPiece input,
      OutputType& outHead,
      OutputTypes&... outTail) {
  return detail::splitFixed<exact>(
    detail::prepareDelim(delimiter),
    input,
    outHead,
    outTail...);
}

namespace detail {

/*
 * If a type can have its string size determined cheaply, we can more
 * efficiently append it in a loop (see internalJoinAppend). Note that the
 * struct need not conform to the std::string api completely (ex. does not need
 * to implement append()).
 */

template <class Iterator>
struct IsSizableStringContainerIterator :
    IsSizableString < typename std::iterator_traits<Iterator>::value_type > {
};


template <class Delim, class Iterator, class String>
void internalJoinAppend(Delim delimiter,
                        Iterator begin,
                        Iterator end,
                        String& output) {
  assert(begin != end);
  if (std::is_same<Delim, StringPiece>::value &&
      delimSize(delimiter) == 1) {
    internalJoinAppend(delimFront(delimiter), begin, end, output);
    return;
  }
  toAppend(&output, *begin);
  while (++begin != end) {
    toAppend(&output, delimiter, *begin);
  }
}


template <class Delim, class Iterator, class String>
typename std::enable_if<IsSizableStringContainerIterator<Iterator>::value>::type
internalJoin(Delim delimiter,
             Iterator begin,
             Iterator end,
             String& output) {
  output.clear();
  if (begin == end) {
    return;
  }
  const size_t dsize = delimSize(delimiter);
  Iterator it = begin;
  size_t size = it->size();
  while (++it != end) {
    size += dsize + it->size();
  }
  output.reserve(size);
  internalJoinAppend(delimiter, begin, end, output);
}

template <class Delim, class Iterator, class String>
typename
std::enable_if<!IsSizableStringContainerIterator<Iterator>::value>::type
internalJoin(Delim delimiter,
             Iterator begin,
             Iterator end,
             String& output) {
  output.clear();
  if (begin == end) {
    return;
  }
  internalJoinAppend(delimiter, begin, end, output);
}

} //namespace detail


template <class Delim, class Iterator, class String>
void join(const Delim& delimiter,
          Iterator begin,
          Iterator end,
          String& output) {
  detail::internalJoin(
    detail::prepareDelim(delimiter),
    begin,
    end,
    output);
}
