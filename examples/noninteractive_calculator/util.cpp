// ///////////////////////////////////////////////////////////////////////////
// util.cpp by Victor Dods, created 2017/08/17
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include <cassert>
#include "util.hpp"

bool IsOctalDigit (std::uint8_t c)
{
    return '0' <= c && c <= '7';
}

bool IsHexDigit (std::uint8_t c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

std::uint8_t HexChar (std::uint8_t hex_digit)
{
    assert(hex_digit < 0x10);
    if (hex_digit < 0xA)
        return hex_digit + '0';
    hex_digit -= 0xA;
    return hex_digit + 'A';
}

std::string HexCharLiteral (std::uint8_t const c, bool with_quotes)
{
    std::string retval;
    if (with_quotes)
        retval += '\'';
    retval += '\\';
    retval += 'x';
    retval += char(HexChar(c >> 4));
    retval += char(HexChar(c & 0xF));
    if (with_quotes)
        retval += '\'';
    return retval;
}

bool CharNeedsHexEscaping (std::uint8_t const c)
{
    // normal escaping will suffice for these (a b t n v f r are contiguous).
    if (c == '\0' || (c >= '\a' && c <= '\r'))
        return false;
    // otherwise, only use hex escaping for anything outside the printables
    else
        return c < ' ' || c > '~';
}

bool CharLiteralCharNeedsNormalEscaping (std::uint8_t const c)
{
    // normal escaping will suffice for these (a b t n v f r are contiguous).
    return c == '\0' || (c >= '\a' && c <= '\r') || c == '\\' || c == '\'';
}

bool StringLiteralCharNeedsNormalEscaping (std::uint8_t const c)
{
    // normal escaping will suffice for these (a b t n v f r are contiguous).
    return c == '\0' || (c >= '\a' && c <= '\r') || c == '\\' || c == '\"';
}

std::uint8_t EscapeCode (std::uint8_t const c)
{
    switch (c)
    {
        case '\0': return '0';
        case '\a': return 'a';
        case '\b': return 'b';
        case '\t': return 't';
        case '\n': return 'n';
        case '\v': return 'v';
        case '\f': return 'f';
        case '\r': return 'r';
        default  : return c;
    }
}

std::string CharLiteral (std::uint8_t const c, bool const with_quotes)
{
    std::string retval;
    if (with_quotes)
        retval += '\'';

    if (CharLiteralCharNeedsNormalEscaping(c))
        retval += '\\', retval += EscapeCode(c);
    else if (CharNeedsHexEscaping(c))
        retval += HexCharLiteral(c, false);
    else
        retval += c;

    if (with_quotes)
        retval += '\'';
    return retval;
}

std::string StringLiteral (std::string const &text, bool const with_quotes)
{
    std::string retval;
    if (with_quotes)
        retval += '"';

    for (std::string::const_iterator it = text.begin(),
                                     it_end = text.end();
         it != it_end;
         ++it)
    {
        std::uint8_t c = *it;
        if (StringLiteralCharNeedsNormalEscaping(c))
            retval += '\\', retval += EscapeCode(c);
        else if (CharNeedsHexEscaping(c))
            retval += HexCharLiteral(c, false);
        else
            retval += char(c);
    }

    if (with_quotes)
        retval += '"';
    return retval;
}

std::uint8_t EscapedChar (std::uint8_t c)
{
    switch (c)
    {
        case '0': return '\0';
        case 'a': return '\a';
        case 'b': return '\b';
        case 't': return '\t';
        case 'n': return '\n';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'r': return '\r';
        default : return c;
    }
}

EscapeStringStatus EscapeString (std::string &text)
{
    EscapeStringReturnCode return_code = EscapeStringReturnCode::SUCCESS;
    std::string::size_type read_cursor = 0;
    std::string::size_type write_cursor = 0;
    std::uint32_t line_number_offset = 0;
    while (read_cursor < text.length())
    {
        assert(write_cursor <= read_cursor);
        if (text[read_cursor] == '\\')
        {
            ++read_cursor;
            // if we're at the end of the string, this is an error.
            if (read_cursor == text.length())
            {
                return_code = EscapeStringReturnCode::UNEXPECTED_EOI;
                break;
            }
            // if there's an x next, we'll expect hex digits after it.
            // if there's an octal digit next, read octal.
            else if (text[read_cursor] == 'x' || IsOctalDigit(text[read_cursor]))
            {
                int radix = 8;
                std::string::size_type end_of_digits;

                if (text[read_cursor] == 'x')
                {
                    radix = 16;

                    ++read_cursor;
                    // if we're at the end of the string, or if there are
                    // no hex digits after the x, this is an error.
                    if (read_cursor == text.length() || !IsHexDigit(text[read_cursor]))
                    {
                        return_code = EscapeStringReturnCode::MALFORMED_HEX_CHAR;
                        break;
                    }

                    // otherwise find the range of hex digits and read them.
                    end_of_digits = read_cursor;
                    while (end_of_digits < text.length() && IsHexDigit(text[end_of_digits]))
                        ++end_of_digits;
                }
                else
                {
                    // find the range of octal digits and read them.
                    end_of_digits = read_cursor;
                    while (end_of_digits < text.length() && IsOctalDigit(text[end_of_digits]))
                        ++end_of_digits;
                }

                assert(end_of_digits > read_cursor);

                // decode the digits into a value
                std::uint32_t value = strtol(text.c_str()+read_cursor, NULL, 16);
                // handle out-of-range error
                if (value >= 256)
                {
                    if (radix == 16)
                        return_code = EscapeStringReturnCode::HEX_ESCAPE_SEQUENCE_OUT_OF_RANGE;
                    else
                        return_code = EscapeStringReturnCode::OCTAL_ESCAPE_SEQUENCE_OUT_OF_RANGE;
                    break;
                }

                // otherwise everything was ok, so write the hex char
                text[write_cursor] = std::uint8_t(value);
                read_cursor = end_of_digits;
            }
            // otherwise it's a normal escape char
            else
            {
                if (text[read_cursor] == '\n')
                    ++line_number_offset;
                text[write_cursor] = EscapedChar(text[read_cursor]);
                ++read_cursor;
            }
        }
        else
        {
            if (text[read_cursor] == '\n')
                ++line_number_offset;
            text[write_cursor] = text[read_cursor];
            ++read_cursor;
        }
        ++write_cursor;
    }
    // chop off the unused portion of the string, and return the return code.
    text.resize(write_cursor);
    return EscapeStringStatus(return_code, line_number_offset);
}

