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

#include "Strings.h"
#include <stdarg.h>
#include <ctype.h>
#include <array>
#include <memory>
#include <stdexcept>
#include <iterator>
#include <algorithm>


namespace {

inline int stringAppendfImplHelper(char* buf,
                                   size_t bufsize,
                                   const char* format,
                                   va_list args) 
{
    va_list args_copy;
    va_copy(args_copy, args);
    int bytes_used = vsnprintf(buf, bufsize, format, args_copy);
    va_end(args_copy);
    return bytes_used;
}

void stringAppendfImpl(std::string& output, const char* format, va_list args) 
{
    // Very simple; first, try to avoid an allocation by using an inline
    // buffer.  If that fails to hold the output string, allocate one on
    // the heap, use it instead.
    //
    // It is hard to guess the proper size of this buffer; some
    // heuristics could be based on the number of format characters, or
    // static analysis of a codebase.  Or, we can just pick a number
    // that seems big enough for simple cases (say, one line of text on
    // a terminal) without being large enough to be concerning as a
    // stack variable.
    std::array<char, 256> inline_buffer;

    int bytes_used = stringAppendfImplHelper(
        inline_buffer.data(), inline_buffer.size(), format, args);
    if (bytes_used < static_cast<int>(inline_buffer.size())) {
        if (bytes_used >= 0) {
            output.append(inline_buffer.data(), bytes_used);
            return;
        }
        // Error or MSVC running out of space.  MSVC 8.0 and higher
        // can be asked about space needed with the special idiom below:
        bytes_used = stringAppendfImplHelper(nullptr, 0, format, args);
        if (bytes_used < 0) {
            throw std::runtime_error(to<std::string>(
                "Invalid format string; snprintf returned negative "
                "with format string: ",
                format));
        }
    }

    // Increase the buffer size to the size requested by vsnprintf,
    // plus one for the closing \0.
    std::unique_ptr<char[]> heap_buffer(new char[bytes_used + 1]);
    int final_bytes_used =
        stringAppendfImplHelper(heap_buffer.get(), bytes_used + 1, format, args);
    // The second call can take fewer bytes if, for example, we were printing a
    // string buffer with null-terminating char using a width specifier -
    // vsnprintf("%.*s", buf.size(), buf)
    CHECK(bytes_used >= final_bytes_used);

    // We don't keep the trailing '\0' in our output string
    output.append(heap_buffer.get(), final_bytes_used);
}

} // anonymouse namespace

std::string stringPrintf(const char* format, ...) 
{
    va_list ap;
    va_start(ap, format);
    auto s = stringVPrintf(format, ap);
    va_end(ap);
    return std::move(s);
}

std::string stringVPrintf(const char* format, va_list ap) 
{
    std::string ret;
    stringAppendfImpl(ret, format, ap);
    return ret;
}

// Basic declarations; allow for parameters of strings and string
// pieces to be specified.
std::string& stringAppendf(std::string* output, const char* format, ...) 
{
    va_list ap;
    va_start(ap, format);
    auto s = stringVAppendf(output, format, ap);
    va_end(ap);
    return std::move(s);
}

std::string& stringVAppendf(std::string* output,
                            const char* format,
                            va_list ap) 
{
    stringAppendfImpl(*output, format, ap);
    return *output;
}

void stringPrintf(std::string* output, const char* format, ...) 
{
    va_list ap;
    va_start(ap, format);
    stringVPrintf(output, format, ap);
    va_end(ap);
}

void stringVPrintf(std::string* output, const char* format, va_list ap) 
{
    output->clear();
    stringAppendfImpl(*output, format, ap);
};


namespace detail {

// Map from character code to value of one-character escape sequence
// ('\n' = 10 maps to 'n'), 'O' if the character should be printed as
// an octal escape sequence, or 'P' if the character is printable and
// should be printed as is.
const char cEscapeTable[] =
  "OOOOOOOOOtnOOrOOOOOOOOOOOOOOOOOOPP\"PPPPPPPPPPPPPPPPPPPPPPPPPPPP?"
  "PPPPPPPPPPPPPPPPPPPPPPPPPPPP\\PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPO"
  "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"
  "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO";

// Map from the character code of the character following a backslash to
// the unescaped character if a valid one-character escape sequence
// ('n' maps to 10 = '\n'), 'O' if this is the first character of an
// octal escape sequence, 'X' if this is the first character of a
// hexadecimal escape sequence, or 'I' if this escape sequence is invalid.
const char cUnescapeTable[] =
  "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII\"IIII'IIIIIIIIOOOOOOOOIIIIIII?"
  "IIIIIIIIIIIIIIIIIIIIIIIIIIII\\IIII\a\bIII\fIIIIIII\nIII\rI\tI\vIXIIIIIII"
  "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII"
  "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII";

// Map from the character code to the hex value, or 16 if invalid hex char.
const unsigned char hexTable[] = 
{
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 16, 16, 16, 16, 16, 16, 
  16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 
};

// Map from character code to escape mode:
// 0 = pass through
// 1 = unused
// 2 = pass through in PATH mode
// 3 = space, replace with '+' in QUERY mode
// 4 = percent-encode
const unsigned char uriEscapeTable[] = 
{
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
  3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 2, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 
  4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 0, 
  4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 0, 4, 
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 
};

}  // namespace detail

void cEscape(StringPiece str, std::string& out) 
{
    char esc[4];
    esc[0] = '\\';
    out.reserve(out.size() + str.size());
    auto p = str.begin();
    auto last = p;  // last regular character
    // We advance over runs of regular characters (printable, not double-quote or
    // backslash) and copy them in one go; this is faster than calling push_back
    // repeatedly.
    while (p != str.end()) {
        char c = *p;
        unsigned char v = static_cast<unsigned char>(c);
        char e = detail::cEscapeTable[v];
        if (e == 'P') {  // printable
            ++p;
        }
        else if (e == 'O') {  // octal
            out.append(&*last, p - last);
            esc[1] = '0' + ((v >> 6) & 7);
            esc[2] = '0' + ((v >> 3) & 7);
            esc[3] = '0' + (v & 7);
            out.append(esc, 4);
            ++p;
            last = p;
        }
        else {  // special 1-character escape
            out.append(&*last, p - last);
            esc[1] = e;
            out.append(esc, 2);
            ++p;
            last = p;
        }
    }
    out.append(&*last, p - last);
}

void cUnescape(StringPiece str, std::string& out, bool strict)
{
    out.reserve(out.size() + str.size());
    auto p = str.begin();
    auto last = p;  // last regular character (not part of an escape sequence)
    // We advance over runs of regular characters (not backslash) and copy them
    // in one go; this is faster than calling push_back repeatedly.
    while (p != str.end()) {
        char c = *p;
        if (c != '\\') {  // normal case
            ++p;
            continue;
        }
        out.append(&*last, p - last);
        if (p == str.end()) {  // backslash at end of string
            if (strict) {
                throw std::invalid_argument("incomplete escape sequence");
            }
            out.push_back('\\');
            last = p;
            continue;
        }
        ++p;
        char e = detail::cUnescapeTable[static_cast<unsigned char>(*p)];
        if (e == 'O') {  // octal
            unsigned char val = 0;
            for (int i = 0; i < 3 && p != str.end() && *p >= '0' && *p <= '7';
                ++i, ++p) {
                val = (val << 3) | (*p - '0');
            }
            out.push_back(val);
            last = p;
        }
        else if (e == 'X') {  // hex
            ++p;
            if (p == str.end()) {  // \x at end of string
                if (strict) {
                    throw std::invalid_argument("incomplete hex escape sequence");
                }
                out.append("\\x");
                last = p;
                continue;
            }
            unsigned char val = 0;
            unsigned char h;
            for (; (p != str.end() &&
                (h = detail::hexTable[static_cast<unsigned char>(*p)]) < 16);
                ++p) {
                val = (val << 4) | h;
            }
            out.push_back(val);
            last = p;
        }
        else if (e == 'I') {  // invalid
            if (strict) {
                throw std::invalid_argument("invalid escape sequence");
            }
            out.push_back('\\');
            out.push_back(*p);
            ++p;
            last = p;
        }
        else {  // standard escape sequence, \' etc
            out.push_back(e);
            ++p;
            last = p;
        }
    }
    out.append(&*last, p - last);
}

void uriEscape(StringPiece str, std::string& out, UriEscapeMode mode) 
{
    static const char hexValues[] = "0123456789abcdef";
    char esc[3];
    esc[0] = '%';
    // Preallocate assuming that 25% of the input string will be escaped
    out.reserve(out.size() + str.size() + 3 * (str.size() / 4));
    auto p = str.begin();
    auto last = p;  // last regular character
    // We advance over runs of passthrough characters and copy them in one go;
    // this is faster than calling push_back repeatedly.
    unsigned char minEncode = static_cast<unsigned char>(mode);
    while (p != str.end()) {
        char c = *p;
        unsigned char v = static_cast<unsigned char>(c);
        unsigned char discriminator = detail::uriEscapeTable[v];
        if (discriminator <= minEncode) {
            ++p;
        }
        else if (mode == UriEscapeMode::QUERY && discriminator == 3) {
            out.append(&*last, p - last);
            out.push_back('+');
            ++p;
            last = p;
        }
        else {
            out.append(&*last, p - last);
            esc[1] = hexValues[v >> 4];
            esc[2] = hexValues[v & 0x0f];
            out.append(esc, 3);
            ++p;
            last = p;
        }
    }
    out.append(&*last, p - last);
}

void uriUnescape(StringPiece str, std::string& out, UriEscapeMode mode) 
{
    out.reserve(out.size() + str.size());
    auto p = str.begin();
    auto last = p;
    // We advance over runs of passthrough characters and copy them in one go;
    // this is faster than calling push_back repeatedly.
    while (p != str.end()) {
        char c = *p;
        switch (c) {
        case '%':
            {
                if (std::distance(p, str.end()) < 3) {
                    throw std::invalid_argument("incomplete percent encode sequence");
                }
                auto h1 = detail::hexTable[static_cast<unsigned char>(p[1])];
                auto h2 = detail::hexTable[static_cast<unsigned char>(p[2])];
                if (h1 == 16 || h2 == 16) {
                    throw std::invalid_argument("invalid percent encode sequence");
                }
                out.append(&*last, p - last);
                out.push_back((h1 << 4) | h2);
                p += 3;
                last = p;
                break;
            }
        case '+':
            if (mode == UriEscapeMode::QUERY) {
                out.append(&*last, p - last);
                out.push_back(' ');
                ++p;
                last = p;
                break;
            }
            // else fallthrough
        default:
            ++p;
            break;
        }
    }
    out.append(&*last, p - last);
}

void backslashify(const std::string& input, std::string& output, bool hex_style)
{
    static const char hexValues[] = "0123456789abcdef";
    output.clear();
    output.reserve(3 * input.size());
    for (unsigned char c : input) {
        // less than space or greater than '~' are considered unprintable
        if (c < 0x20 || c > 0x7e || c == '\\') {
            bool hex_append = false;
            output.push_back('\\');
            if (hex_style) {
                hex_append = true;
            }
            else {
                if (c == '\r') output += 'r';
                else if (c == '\n') output += 'n';
                else if (c == '\t') output += 't';
                else if (c == '\a') output += 'a';
                else if (c == '\b') output += 'b';
                else if (c == '\0') output += '0';
                else if (c == '\\') output += '\\';
                else {
                    hex_append = true;
                }
            }
            if (hex_append) {
                output.push_back('x');
                output.push_back(hexValues[(c >> 4) & 0xf]);
                output.push_back(hexValues[c & 0xf]);
            }
        }
        else {
            output += c;
        }
    }
}

void humanify(const std::string& input, std::string& output)
{
    size_t numUnprintable = 0;
    size_t numPrintablePrefix = 0;
    for (unsigned char c : input) {
        if (c < 0x20 || c > 0x7e || c == '\\') {
            ++numUnprintable;
        }
        if (numUnprintable == 0) {
            ++numPrintablePrefix;
        }
    }

    // hexlify doubles a string's size; backslashify can potentially
    // explode it by 4x.  Now, the printable range of the ascii
    // "spectrum" is around 95 out of 256 values, so a "random" binary
    // string should be around 60% unprintable.  We use a 50% hueristic
    // here, so if a string is 60% unprintable, then we just use hex
    // output.  Otherwise we backslash.
    //
    // UTF8 is completely ignored; as a result, utf8 characters will
    // likely be \x escaped (since most common glyphs fit in two bytes).
    // This is a tradeoff of complexity/speed instead of a convenience
    // that likely would rarely matter.  Moreover, this function is more
    // about displaying underlying bytes, not about displaying glyphs
    // from languages.
    if (numUnprintable == 0) {
        output = input;
    }
    else if (5 * numUnprintable >= 3 * input.size()) {
        // However!  If we have a "meaningful" prefix of printable
        // characters, say 20% of the string, we backslashify under the
        // assumption viewing the prefix as ascii is worth blowing the
        // output size up a bit.
        if (5 * numPrintablePrefix >= input.size()) {
            backslashify(input, output);
        }
        else {
            output = "0x";
            hexlify(input, output, true /* append output */);
        }
    }
    else {
        backslashify(input, output);
    }
}

bool hexlify(const std::string& input, std::string& output, bool append)
{
    if (!append)
        output.clear();
    static char hexValues[] = "0123456789abcdef";
    auto j = output.size();
    output.resize(2 * input.size() + output.size());
    for (size_t i = 0; i < input.size(); ++i) {
        int ch = input[i];
        output[j++] = hexValues[(ch >> 4) & 0xf];
        output[j++] = hexValues[ch & 0xf];
    }
    return true;
}

bool unhexlify(const std::string& input, std::string& output)
{
    if (input.size() % 2 != 0) {
        return false;
    }
    output.resize(input.size() / 2);
    int j = 0;
    auto unhex = [](char c) -> int {
        return c >= '0' && c <= '9' ? c - '0' :
            c >= 'A' && c <= 'F' ? c - 'A' + 10 :
            c >= 'a' && c <= 'f' ? c - 'a' + 10 :
            -1;
    };

    for (size_t i = 0; i < input.size(); i += 2) {
        int highBits = unhex(input[i]);
        int lowBits = unhex(input[i + 1]);
        if (highBits < 0 || lowBits < 0) {
            return false;
        }
        output[j++] = (highBits << 4) + lowBits;
    }
    return true;
}


namespace {

struct PrettySuffix {
  const char* suffix;
  double val;
};

const PrettySuffix kPrettyTimeSuffixes[] = {
  { "s ", 1e0L },
  { "ms", 1e-3L },
  { "us", 1e-6L },
  { "ns", 1e-9L },
  { "ps", 1e-12L },
  { "s ", 0 },
  { 0, 0 },
};

const PrettySuffix kPrettyBytesMetricSuffixes[] = {
  { "TB", 1e12L },
  { "GB", 1e9L },
  { "MB", 1e6L },
  { "kB", 1e3L },
  { "B ", 0L },
  { 0, 0 },
};

const PrettySuffix kPrettyBytesBinarySuffixes[] = {
  { "TB", int64_t(1) << 40 },
  { "GB", int64_t(1) << 30 },
  { "MB", int64_t(1) << 20 },
  { "kB", int64_t(1) << 10 },
  { "B ", 0L },
  { 0, 0 },
};

const PrettySuffix kPrettyBytesBinaryIECSuffixes[] = {
  { "TiB", int64_t(1) << 40 },
  { "GiB", int64_t(1) << 30 },
  { "MiB", int64_t(1) << 20 },
  { "KiB", int64_t(1) << 10 },
  { "B  ", 0L },
  { 0, 0 },
};

const PrettySuffix kPrettyUnitsMetricSuffixes[] = {
  { "tril", 1e12L },
  { "bil",  1e9L },
  { "M",    1e6L },
  { "k",    1e3L },
  { " ",      0  },
  { 0, 0 },
};

const PrettySuffix kPrettyUnitsBinarySuffixes[] = {
  { "T", int64_t(1) << 40 },
  { "G", int64_t(1) << 30 },
  { "M", int64_t(1) << 20 },
  { "k", int64_t(1) << 10 },
  { " ", 0 },
  { 0, 0 },
};

const PrettySuffix kPrettyUnitsBinaryIECSuffixes[] = {
  { "Ti", int64_t(1) << 40 },
  { "Gi", int64_t(1) << 30 },
  { "Mi", int64_t(1) << 20 },
  { "Ki", int64_t(1) << 10 },
  { "  ", 0 },
  { 0, 0 },
};

const PrettySuffix kPrettySISuffixes[] = {
  { "Y", 1e24L },
  { "Z", 1e21L },
  { "E", 1e18L },
  { "P", 1e15L },
  { "T", 1e12L },
  { "G", 1e9L },
  { "M", 1e6L },
  { "k", 1e3L },
  { "h", 1e2L },
  { "da", 1e1L },
  { "d", 1e-1L },
  { "c", 1e-2L },
  { "m", 1e-3L },
  { "u", 1e-6L },
  { "n", 1e-9L },
  { "p", 1e-12L },
  { "f", 1e-15L },
  { "a", 1e-18L },
  { "z", 1e-21L },
  { "y", 1e-24L },
  { " ", 0 },
  { 0, 0}
};

const PrettySuffix* const kPrettySuffixes[PRETTY_NUM_TYPES] = {
  kPrettyTimeSuffixes,
  kPrettyBytesMetricSuffixes,
  kPrettyBytesBinarySuffixes,
  kPrettyBytesBinaryIECSuffixes,
  kPrettyUnitsMetricSuffixes,
  kPrettyUnitsBinarySuffixes,
  kPrettyUnitsBinaryIECSuffixes,
  kPrettySISuffixes,
};

}  // namespace


std::string prettyPrint(double val, PrettyType type, bool addSpace) 
{
    char buf[100];

    // pick the suffixes to use
    assert(type >= 0);
    assert(type < PRETTY_NUM_TYPES);
    const PrettySuffix* suffixes = kPrettySuffixes[type];

    // find the first suffix we're bigger than -- then use it
    double abs_val = fabs(val);
    for (int i = 0; suffixes[i].suffix; ++i) {
        if (abs_val >= suffixes[i].val) {
            snprintf(buf, sizeof buf, "%.4g%s%s",
                (suffixes[i].val ? (val / suffixes[i].val)
                : val),
                (addSpace ? " " : ""),
                suffixes[i].suffix);
            return std::string(buf);
        }
    }

    // no suffix, we've got a tiny value -- just print it in sci-notation
    snprintf(buf, sizeof buf, "%.4g", val);
    return std::string(buf);
}

//TODO:
//1) Benchmark & optimize
double prettyToDouble(StringPiece *const prettyString, const PrettyType type)
{
    double value = to<double>(prettyString);
    while (prettyString->size() > 0 && isspace(prettyString->front())) {
        prettyString->advance(1); //Skipping spaces between number and suffix
    }
    const PrettySuffix* suffixes = kPrettySuffixes[type];
    int longestPrefixLen = -1;
    int bestPrefixId = -1;
    for (int j = 0; suffixes[j].suffix; ++j) {
        if (suffixes[j].suffix[0] == ' '){//Checking for " " -> number rule.
            if (longestPrefixLen == -1) {
                longestPrefixLen = 0; //No characters to skip
                bestPrefixId = j;
            }
        }
        else if (prettyString->startsWith(suffixes[j].suffix)) {
            int suffixLen = static_cast<int>(strlen(suffixes[j].suffix));
            //We are looking for a longest suffix matching prefix of the string
            //after numeric value. We need this in case suffixes have common prefix.
            if (suffixLen > longestPrefixLen) {
                longestPrefixLen = suffixLen;
                bestPrefixId = j;
            }
        }
    }
    if (bestPrefixId == -1) { //No valid suffix rule found
        throw std::invalid_argument(to<std::string>(
            "Unable to parse suffix \"",
            prettyString->toString(), "\""));
    }
    prettyString->advance(longestPrefixLen);
    return suffixes[bestPrefixId].val ? value * suffixes[bestPrefixId].val :
        value;
}

double prettyToDouble(StringPiece prettyString, const PrettyType type)
{
    double result = prettyToDouble(&prettyString, type);
    detail::enforceWhitespace(prettyString.data(), prettyString.data() + prettyString.size());
    return result;
}


StringPiece skipWhitespace(StringPiece sp) 
{
    // Spaces other than ' ' characters are less common but should be
    // checked.  This configuration where we loop on the ' '
    // separately from oddspaces was empirically fastest.
    auto oddspace = [](char c) {
        return c == '\n' || c == '\t' || c == '\r';
    };

loop:
    for (; !sp.empty() && sp.front() == ' '; sp.pop_front()) {
    }
    if (!sp.empty() && oddspace(sp.front())) {
        sp.pop_front();
        goto loop;
    }

    return sp;
}

namespace {

inline void toLowerAscii8(char& c) 
{
    // Branchless tolower, based on the input-rotating trick described
    // at http://www.azillionmonkeys.com/qed/asmexample.html
    //
    // This algorithm depends on an observation: each uppercase
    // ASCII character can be converted to its lowercase equivalent
    // by adding 0x20.

    // Step 1: Clear the high order bit. We'll deal with it in Step 5.
    unsigned char rotated = c & 0x7f;
    // Currently, the value of rotated, as a function of the original c is:
    //   below 'A':   0- 64
    //   'A'-'Z':    65- 90
    //   above 'Z':  91-127

    // Step 2: Add 0x25 (37)
    rotated += 0x25;
    // Now the value of rotated, as a function of the original c is:
    //   below 'A':   37-101
    //   'A'-'Z':    102-127
    //   above 'Z':  128-164

    // Step 3: clear the high order bit
    rotated &= 0x7f;
    //   below 'A':   37-101
    //   'A'-'Z':    102-127
    //   above 'Z':    0- 36

    // Step 4: Add 0x1a (26)
    rotated += 0x1a;
    //   below 'A':   63-127
    //   'A'-'Z':    128-153
    //   above 'Z':   25- 62

    // At this point, note that only the uppercase letters have been
    // transformed into values with the high order bit set (128 and above).

    // Step 5: Shift the high order bit 2 spaces to the right: the spot
    // where the only 1 bit in 0x20 is.  But first, how we ignored the
    // high order bit of the original c in step 1?  If that bit was set,
    // we may have just gotten a false match on a value in the range
    // 128+'A' to 128+'Z'.  To correct this, need to clear the high order
    // bit of rotated if the high order bit of c is set.  Since we don't
    // care about the other bits in rotated, the easiest thing to do
    // is invert all the bits in c and bitwise-and them with rotated.
    rotated &= ~c;
    rotated >>= 2;

    // Step 6: Apply a mask to clear everything except the 0x20 bit
    // in rotated.
    rotated &= 0x20;

    // At this point, rotated is 0x20 if c is 'A'-'Z' and 0x00 otherwise

    // Step 7: Add rotated to c
    c += rotated;
}

inline void toLowerAscii32(uint32_t& c) 
{
    // Besides being branchless, the algorithm in toLowerAscii8() has another
    // interesting property: None of the addition operations will cause
    // an overflow in the 8-bit value.  So we can pack four 8-bit values
    // into a uint32_t and run each operation on all four values in parallel
    // without having to use any CPU-specific SIMD instructions.
    uint32_t rotated = c & uint32_t(0x7f7f7f7fL);
    rotated += uint32_t(0x25252525L);
    rotated &= uint32_t(0x7f7f7f7fL);
    rotated += uint32_t(0x1a1a1a1aL);

    // Step 5 involves a shift, so some bits will spill over from each
    // 8-bit value into the next.  But that's okay, because they're bits
    // that will be cleared by the mask in step 6 anyway.
    rotated &= ~c;
    rotated >>= 2;
    rotated &= uint32_t(0x20202020L);
    c += rotated;
}

inline void toLowerAscii64(uint64_t& c) 
{
    // 64-bit version of toLower32
    uint64_t rotated = c & uint64_t(0x7f7f7f7f7f7f7f7fL);
    rotated += uint64_t(0x2525252525252525L);
    rotated &= uint64_t(0x7f7f7f7f7f7f7f7fL);
    rotated += uint64_t(0x1a1a1a1a1a1a1a1aL);
    rotated &= ~c;
    rotated >>= 2;
    rotated &= uint64_t(0x2020202020202020L);
    c += rotated;
}

} // anonymouse namespace


void toLowerAscii(char* str, size_t length) 
{
    static const size_t kAlignMask64 = 7;
    static const size_t kAlignMask32 = 3;

    // Convert a character at a time until we reach an address that
    // is at least 32-bit aligned
    size_t n = (size_t)str;
    n &= kAlignMask32;
    n = std::min(n, length);
    size_t offset = 0;
    if (n != 0) {
        n = std::min(4 - n, length);
        do {
            toLowerAscii8(str[offset]);
            offset++;
        } while (offset < n);
    }

    n = (size_t)(str + offset);
    n &= kAlignMask64;
    if ((n != 0) && (offset + 4 <= length)) {
        // The next address is 32-bit aligned but not 64-bit aligned.
        // Convert the next 4 bytes in order to get to the 64-bit aligned
        // part of the input.
        toLowerAscii32(*(uint32_t*)(str + offset));
        offset += 4;
    }

    // Convert 8 characters at a time
    while (offset + 8 <= length) {
        toLowerAscii64(*(uint64_t*)(str + offset));
        offset += 8;
    }

    // Convert 4 characters at a time
    while (offset + 4 <= length) {
        toLowerAscii32(*(uint32_t*)(str + offset));
        offset += 4;
    }

    // Convert any characters remaining after the last 4-byte aligned group
    while (offset < length) {
        toLowerAscii8(str[offset]);
        offset++;
    }
}
