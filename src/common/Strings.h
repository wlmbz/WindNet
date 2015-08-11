/*
 * Copyright 2015 Facebook, Inc.
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

#include <string>
#include <vector>
#include <type_traits>
#include "Portability.h"
#include "Range.h"
#include "Conv.h"

/**
 * stringPrintf is much like printf but deposits its result into a
 * string. Two signatures are supported: the first simply returns the
 * resulting string, and the second appends the produced characters to
 * the specified string and returns a reference to it.
 */
std::string stringPrintf(FOLLY_PRINTF_FORMAT const char* format, ...)
  FOLLY_PRINTF_FORMAT_ATTR(1, 2);

/* Similar to stringPrintf, with different signature. */
void stringPrintf(std::string* out, FOLLY_PRINTF_FORMAT const char* fmt, ...)
  FOLLY_PRINTF_FORMAT_ATTR(2, 3);

std::string& stringAppendf(std::string* output,
                          FOLLY_PRINTF_FORMAT const char* format, ...)
  FOLLY_PRINTF_FORMAT_ATTR(2, 3);

/**
 * Similar to stringPrintf, but accepts a va_list argument.
 *
 * As with vsnprintf() itself, the value of ap is undefined after the call.
 * These functions do not call va_end() on ap.
 */
std::string stringVPrintf(const char* format, va_list ap);
void stringVPrintf(std::string* out, const char* format, va_list ap);
std::string& stringVAppendf(std::string* out, const char* format, va_list ap);

/*
 * Split a string into a list of tokens by delimiter.
 *
 * The split interface here supports different output types, selected
 * at compile time: StringPiece, fbstring, or std::string.  If you are
 * using a vector to hold the output, it detects the type based on
 * what your vector contains.  If the output vector is not empty, split
 * will append to the end of the vector.
 *
 * You can also use splitTo() to write the output to an arbitrary
 * OutputIterator (e.g. std::inserter() on a std::set<>), in which
 * case you have to tell the function the type.  (Rationale:
 * OutputIterators don't have a value_type, so we can't detect the
 * type in splitTo without being told.)
 *
 * Examples:
 *
 *   std::vector<StringPiece> v;
 *   split(":", "asd:bsd", v);
 *
 *   std::set<StringPiece> s;
 *   splitTo<StringPiece>(":", "asd:bsd:asd:csd",
 *    std::inserter(s, s.begin()));
 *
 * Split also takes a flag (ignoreEmpty) that indicates whether adjacent
 * delimiters should be treated as one single separator (ignoring empty tokens)
 * or not (generating empty tokens).
 */

template<class Delim, class String, class OutputType>
void split(const Delim& delimiter,
    const String& input,
    std::vector<OutputType>& out,
    bool ignoreEmpty = false);


template<class OutputValueType, class Delim, class String,
class OutputIterator>
    void splitTo(const Delim& delimiter,
    const String& input,
    OutputIterator out,
    bool ignoreEmpty = false);


/*
 * Split a string into a fixed number of string pieces and/or numeric types
 * by delimiter. Any numeric type that to<> can convert to from a
 * string piece is supported as a target. Returns 'true' if the fields were
 * all successfully populated.  Returns 'false' if there were too few fields
 * in the input, or too many fields if exact=true.  Casting exceptions will
 * not be caught.
 *
 * Examples:
 *
 *  StringPiece name, key, value;
 *  if (split('\t', line, name, key, value))
 *    ...
 *
 *  StringPiece name;
 *  double value;
 *  int id;
 *  if (split('\t', line, name, value, id))
 *    ...
 *
 * The 'exact' template parameter specifies how the function behaves when too
 * many fields are present in the input string. When 'exact' is set to its
 * default value of 'true', a call to split will fail if the number of fields in
 * the input string does not exactly match the number of output parameters
 * passed. If 'exact' is overridden to 'false', all remaining fields will be
 * stored, unsplit, in the last field, as shown below:
 *
 *  StringPiece x, y.
 *  if (split<false>(':', "a:b:c", x, y))
 *    assert(x == "a" && y == "b:c");
 *
 * Note that this will likely not work if the last field's target is of numeric
 * type, in which case to<> will throw an exception.
 */


namespace detail {

template <class T> struct IsSizableString {
  enum { value = std::is_same<T, std::string>::value
         || std::is_same<T, StringPiece>::value };
};

} // namespace detail

template<bool exact = true,
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
      OutputTypes&... outTail);

/*
 * Join list of tokens.
 *
 * Stores a string representation of tokens in the same order with
 * deliminer between each element.
 */


template <class Delim, class Iterator, class String>
void join(const Delim& delimiter,
          Iterator begin,
          Iterator end,
          String& output);

template <class Delim, class Container, class String>
void join(const Delim& delimiter,
          const Container& container,
          String& output) {
  join(delimiter, container.begin(), container.end(), output);
}

template <class Delim, class Value, class String>
void join(const Delim& delimiter,
          const std::initializer_list<Value>& values,
          String& output) {
  join(delimiter, values.begin(), values.end(), output);
}

template <class Delim, class Container>
std::string join(const Delim& delimiter,
                 const Container& container) {
  std::string output;
  join(delimiter, container.begin(), container.end(), output);
  return output;
}

template <class Delim, class Value>
std::string join(const Delim& delimiter,
                 const std::initializer_list<Value>& values) {
  std::string output;
  join(delimiter, values.begin(), values.end(), output);
  return output;
}

template <class Delim,
          class Iterator,
          typename std::enable_if<std::is_same<
              typename std::iterator_traits<Iterator>::iterator_category,
              std::random_access_iterator_tag>::value>::type* = nullptr>
std::string join(const Delim& delimiter, Iterator begin, Iterator end) {
  std::string output;
  join(delimiter, begin, end, output);
  return output;
}

/**
 * C-Escape a string, making it suitable for representation as a C string
 * literal.  Appends the result to the output string.
 *
 * Backslashes all occurrences of backslash and double-quote:
 *   "  ->  \"
 *   \  ->  \\
 *
 * Replaces all non-printable ASCII characters with backslash-octal
 * representation:
 *   <ASCII 254> -> \376
 *
 * Note that we use backslash-octal instead of backslash-hex because the octal
 * representation is guaranteed to consume no more than 3 characters; "\3760"
 * represents two characters, one with value 254, and one with value 48 ('0'),
 * whereas "\xfe0" represents only one character (with value 4064, which leads
 * to implementation-defined behavior).
 */
void cEscape(StringPiece str, std::string& out);

/**
 * Similar to cEscape above, but returns the escaped string.
 */
inline std::string cEscape(StringPiece str) 
{
    std::string out;
    cEscape(str, out);
    return out;
}

/**
 * C-Unescape a string; the opposite of cEscape above.  Appends the result
 * to the output string.
 *
 * Recognizes the standard C escape sequences:
 *
 * \' \" \? \\ \a \b \f \n \r \t \v
 * \[0-7]+
 * \x[0-9a-fA-F]+
 *
 * In strict mode (default), throws std::invalid_argument if it encounters
 * an unrecognized escape sequence.  In non-strict mode, it leaves
 * the escape sequence unchanged.
 */
void cUnescape(StringPiece str, std::string& out, bool strict = true);

/**
 * Similar to cUnescape above, but returns the escaped string.
 */
inline std::string cUnescape(StringPiece str, bool strict = true) 
{
    std::string out;
    cUnescape(str, out, strict);
    return out;
}


/**
 * URI-escape a string.  Appends the result to the output string.
 *
 * Alphanumeric characters and other characters marked as "unreserved" in RFC
 * 3986 ( -_.~ ) are left unchanged.  In PATH mode, the forward slash (/) is
 * also left unchanged.  In QUERY mode, spaces are replaced by '+'.  All other
 * characters are percent-encoded.
 */
enum class UriEscapeMode : unsigned char 
{
    // The values are meaningful, see generate_escape_tables.py
    ALL = 0,
    QUERY = 1,
    PATH = 2
};

void uriEscape(StringPiece str, std::string& out, UriEscapeMode mode = UriEscapeMode::ALL);

/**
 * Similar to uriEscape above, but returns the escaped string.
 */
inline std::string uriEscape(StringPiece str, UriEscapeMode mode = UriEscapeMode::ALL) 
{
    std::string out;
    uriEscape(str, out, mode);
    return out;
}

/**
 * URI-unescape a string.  Appends the result to the output string.
 *
 * In QUERY mode, '+' are replaced by space.  %XX sequences are decoded if
 * XX is a valid hex sequence, otherwise we throw invalid_argument.
 */
void uriUnescape(StringPiece str,
                 std::string& out,
                 UriEscapeMode mode = UriEscapeMode::ALL);   

/**
 * Similar to uriUnescape above, but returns the unescaped string.
 */
inline std::string uriUnescape(StringPiece str, UriEscapeMode mode = UriEscapeMode::ALL) 
{
    std::string out;
    uriUnescape(str, out, mode);
    return out;
}

/**
 * Backslashify a string, that is, replace non-printable characters
 * with C-style (but NOT C compliant) "\xHH" encoding.  If hex_style
 * is false, then shorthand notations like "\0" will be used instead
 * of "\x00" for the most common backslash cases.
 *
 * There are two forms, one returning the input string, and one
 * creating output in the specified output string.
 *
 * This is mainly intended for printing to a terminal, so it is not
 * particularly optimized.
 *
 * Do *not* use this in situations where you expect to be able to feed
 * the string to a C or C++ compiler, as there are nuances with how C
 * parses such strings that lead to failures.  This is for display
 * purposed only.  If you want a string you can embed for use in C or
 * C++, use cEscape instead.  This function is for display purposes
 * only.
 */
void backslashify(const std::string& input, std::string& output, bool hex_style = false);

inline std::string backslashify(const std::string& input, bool hex_style = false) 
{
    std::string output;
    backslashify(input, output, hex_style);
    return output;
}

/**
 * Take a string and "humanify" it -- that is, make it look better.
 * Since "better" is subjective, caveat emptor.  The basic approach is
 * to count the number of unprintable characters.  If there are none,
 * then the output is the input.  If there are relatively few, or if
 * there is a long "enough" prefix of printable characters, use
 * backslashify.  If it is mostly binary, then simply hex encode.
 *
 * This is an attempt to make a computer smart, and so likely is wrong
 * most of the time.
 */
void humanify(const std::string& input, std::string& output);

inline std::string humanify(const std::string& input) 
{
    std::string output;
    humanify(input, output);
    return output;
}

/**
 * Same functionality as Python's binascii.hexlify.  Returns true
 * on successful conversion.
 *
 * If append_output is true, append data to the output rather than
 * replace it.
 */
bool hexlify(const std::string& input, std::string& output, bool append=false);

/**
 * Same functionality as Python's binascii.unhexlify.  Returns true
 * on successful conversion.
 */
bool unhexlify(const std::string& input, std::string& output);

/*
 * A pretty-printer for numbers that appends suffixes of units of the
 * given type.  It prints 4 sig-figs of value with the most
 * appropriate unit.
 *
 * If `addSpace' is true, we put a space between the units suffix and
 * the value.
 *
 * Current types are:
 *   PRETTY_TIME         - s, ms, us, ns, etc.
 *   PRETTY_BYTES_METRIC - kB, MB, GB, etc (goes up by 10^3 = 1000 each time)
 *   PRETTY_BYTES        - kB, MB, GB, etc (goes up by 2^10 = 1024 each time)
 *   PRETTY_BYTES_IEC    - KiB, MiB, GiB, etc
 *   PRETTY_UNITS_METRIC - k, M, G, etc (goes up by 10^3 = 1000 each time)
 *   PRETTY_UNITS_BINARY - k, M, G, etc (goes up by 2^10 = 1024 each time)
 *   PRETTY_UNITS_BINARY_IEC - Ki, Mi, Gi, etc
 *   PRETTY_SI           - full SI metric prefixes from yocto to Yotta
 *                         http://en.wikipedia.org/wiki/Metric_prefix
 * @author Mark Rabkin <mrabkin@fb.com>
 */

enum PrettyType {
  PRETTY_TIME,

  PRETTY_BYTES_METRIC,
  PRETTY_BYTES_BINARY,
  PRETTY_BYTES = PRETTY_BYTES_BINARY,
  PRETTY_BYTES_BINARY_IEC,
  PRETTY_BYTES_IEC = PRETTY_BYTES_BINARY_IEC,

  PRETTY_UNITS_METRIC,
  PRETTY_UNITS_BINARY,
  PRETTY_UNITS_BINARY_IEC,

  PRETTY_SI,
  PRETTY_NUM_TYPES,
};

std::string prettyPrint(double val, PrettyType, bool addSpace = true);

/**
 * This utility converts StringPiece in pretty format (look above) to double,
 * with progress information. Alters the  StringPiece parameter
 * to get rid of the already-parsed characters.
 * Expects string in form <floating point number> {space}* [<suffix>]
 * If string is not in correct format, utility finds longest valid prefix and
 * if there at least one, returns double value based on that prefix and
 * modifies string to what is left after parsing. Throws and std::range_error
 * exception if there is no correct parse.
 * Examples(for PRETTY_UNITS_METRIC):
 * '10M' => 10 000 000
 * '10 M' => 10 000 000
 * '10' => 10
 * '10 Mx' => 10 000 000, prettyString == "x"
 * 'abc' => throws std::range_error
 */
double prettyToDouble(StringPiece *const prettyString, const PrettyType type);

/*
 * Same as prettyToDouble(StringPiece*, PrettyType), but
 * expects whole string to be correctly parseable. Throws std::range_error
 * otherwise
 */
double prettyToDouble(StringPiece prettyString, const PrettyType type);

/**
 * Returns a subpiece with all whitespace removed from the front of @sp.
 * Whitespace means any of [' ', '\n', '\r', '\t'].
 */
StringPiece skipWhitespace(StringPiece sp);

/**
 * Fast, in-place lowercasing of ASCII alphabetic characters in strings.
 * Leaves all other characters unchanged, including those with the 0x80
 * bit set.
 * @param str String to convert
 * @param len Length of str, in bytes
 */
void toLowerAscii(char* str, size_t length);

inline void toLowerAscii(MutableStringPiece str) {
  toLowerAscii(str.begin(), str.size());
}

#include "Strings-inl.h"
