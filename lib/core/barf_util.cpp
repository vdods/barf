// ///////////////////////////////////////////////////////////////////////////
// barf_util.hpp by Victor Dods, created 2006/02/11
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_util.hpp"

#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <sstream>
#include <stdlib.h>

namespace Barf {

// this anonymous namespace is for local-file-scoping of certain functions
namespace {

bool GetCharNeedsHexEscaping (Uint8 const c)
{
    // normal escaping will suffice for these (a b t n v f r are contiguous).
    if (c == '\0' || (c >= '\a' && c <= '\r'))
        return false;
    // otherwise, only use hex escaping for anything outside the printables
    else
        return c < ' ' || c > '~';
}

bool GetCharLiteralCharNeedsNormalEscaping (Uint8 const c)
{
    // normal escaping will suffice for these (a b t n v f r are contiguous).
    return c == '\0' || (c >= '\a' && c <= '\r') || c == '\\' || c == '\'';
}

bool GetStringLiteralCharNeedsNormalEscaping (Uint8 const c)
{
    // normal escaping will suffice for these (a b t n v f r are contiguous).
    return c == '\0' || (c >= '\a' && c <= '\r') || c == '\\' || c == '\"';
}

Uint8 GetEscapeCode (Uint8 const c)
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

bool IsOctalDigit (Uint8 c)
{
    return '0' <= c && c <= '7';
}

bool IsHexDigit (Uint8 c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

Uint8 GetHexDigit (Uint8 c)
{
    assert(IsHexDigit(c));
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 0xA;
    else
        return c - 'a' + 0xA;
}

Uint8 GetHexChar (Uint8 hex_digit)
{
    assert(hex_digit < 0x10);
    if (hex_digit < 0xA)
        return hex_digit + '0';
    hex_digit -= 0xA;
    return hex_digit + 'A';
}

string GetHexCharLiteral (Uint8 const c, bool with_quotes)
{
    string retval;
    if (with_quotes)
        retval += '\'';
    retval += '\\';
    retval += 'x';
    retval += (char)GetHexChar(c >> 4);
    retval += (char)GetHexChar(c & 0xF);
    if (with_quotes)
        retval += '\'';
    return retval;
}

} // end of anonymous namespace

ostream &operator << (ostream &stream, Tabs const &tabs)
{
    string tab_string(2*tabs.m_count, ' ');
    stream << tab_string;
    return stream;
}

Uint8 SwitchCase (Uint8 c)
{
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 'A';
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    return c;
}

void EscapeChar (Uint8 &c)
{
    switch (c)
    {
        case '0': c = '\0'; break;
        case 'a': c = '\a'; break;
        case 'b': c = '\b'; break;
        case 't': c = '\t'; break;
        case 'n': c = '\n'; break;
        case 'v': c = '\v'; break;
        case 'f': c = '\f'; break;
        case 'r': c = '\r'; break;
        default : /* do nothing */ break;
    }
}

Uint8 GetEscapedChar (Uint8 const c)
{
    Uint8 retval = c;
    EscapeChar(retval);
    return retval;
}

// this function relies on the fact that an escaped string (i.e. a string with
// escape codes such as \n or \xA7 converted into single chars) is not longer
// than the original string.
EscapeStringStatus EscapeString (string &text)
{
    EscapeStringReturnCode return_code = ESRC_SUCCESS;
    string::size_type read_cursor = 0;
    string::size_type write_cursor = 0;
    Uint32 line_number_offset = 0;
    while (read_cursor < text.length())
    {
        assert(write_cursor <= read_cursor);
        if (text[read_cursor] == '\\')
        {
            ++read_cursor;
            // if we're at the end of the string, this is an error.
            if (read_cursor == text.length())
            {
                return_code = ESRC_UNEXPECTED_EOI;
                break;
            }
            // if there's an x next, we'll expect hex digits after it.
            // if there's an octal digit next, read octal.
            else if (text[read_cursor] == 'x' || IsOctalDigit(text[read_cursor]))
            {
                int radix = 8;
                string::size_type end_of_digits;

                if (text[read_cursor] == 'x')
                {
                    radix = 16;
                
                    ++read_cursor;
                    // if we're at the end of the string, or if there are
                    // no hex digits after the x, this is an error.
                    if (read_cursor == text.length() || !IsHexDigit(text[read_cursor]))
                    {
                        return_code = ESRC_MALFORMED_HEX_CHAR;
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
                Uint32 value = strtol(text.c_str()+read_cursor, NULL, 16);
                // handle out-of-range error
                if (value >= 256)
                {
                    if (radix == 16)
                        return_code = ESRC_HEX_ESCAPE_SEQUENCE_OUT_OF_RANGE;
                    else
                        return_code = ESRC_OCTAL_ESCAPE_SEQUENCE_OUT_OF_RANGE;
                    break;
                }

                // otherwise everything was ok, so write the hex char
                text[write_cursor] = Uint8(value);
                read_cursor = end_of_digits;
            }
            // otherwise it's a normal escape char
            else
            {
                if (text[read_cursor] == '\n')
                    ++line_number_offset;
                text[write_cursor] = GetEscapedChar(text[read_cursor]);
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

string GetEscapedString (string const &text, EscapeStringStatus *escape_string_status)
{
    string retval(text);
    EscapeStringStatus status = EscapeString(retval);
    if (escape_string_status != NULL)
        *escape_string_status = status;
    return retval;
}

string GetCharLiteral (Uint8 const c, bool const with_quotes)
{
    string retval;
    if (with_quotes)
        retval += '\'';

    if (GetCharLiteralCharNeedsNormalEscaping(c))
        retval += '\\', retval += GetEscapeCode(c);
    else if (GetCharNeedsHexEscaping(c))
        retval += GetHexCharLiteral(c, false);
    else
        retval += c;

    if (with_quotes)
        retval += '\'';
    return retval;
}

string GetStringLiteral (string const &text, bool const with_quotes)
{
    string retval;
    if (with_quotes)
        retval += '"';

    for (string::const_iterator it = text.begin(),
                                     it_end = text.end();
         it != it_end;
         ++it)
    {
        Uint8 c = *it;
        if (GetStringLiteralCharNeedsNormalEscaping(c))
            retval += '\\', retval += GetEscapeCode(c);
        else if (GetCharNeedsHexEscaping(c))
            retval += GetHexCharLiteral(c, false);
        else
            retval += char(c);
    }

    if (with_quotes)
        retval += '"';
    return retval;
}

Uint32 GetNewlineCount (string const &text)
{
    Uint32 newline_count = 0;

    Uint32 pos = 0;
    while (pos < text.length())
        if (text[pos++] == '\n')
            ++newline_count;

    return newline_count;
}

void ReplaceAllInString (
    string *const string_to_replace_in,
    string const &replace_this,
    string const &replacement)
{
    assert(string_to_replace_in != NULL);

    string::size_type pos = 0;
    string::size_type match;
    while ((match = string_to_replace_in->find(replace_this, pos)) != string::npos)
    {
        string_to_replace_in->replace(match, replace_this.length(), replacement);
        pos += replacement.length();
    }
}

string GetDirectoryPortion (string const &path)
{
    string::size_type last_slash = path.find_last_of(DIRECTORY_SLASH_STRING);
    if (last_slash == string::npos)
        return g_empty_string;
    else
        return path.substr(0, last_slash+1);
}

string GetFilenamePortion (string const &path)
{
    string::size_type last_slash = path.find_last_of(DIRECTORY_SLASH_STRING);
    if (last_slash == string::npos)
        return path;
    else
        return path.substr(last_slash+1);
}

bool GetIsValidDirectory (string const &directory)
{
    DIR *dir = opendir(directory.c_str());
    if (dir == NULL)
        return false;
    else
    {
        closedir(dir);
        return true;
    }
}

bool GetIsValidFile (string const &filename)
{
    int file_descriptor = open(filename.c_str(), O_RDONLY);
    if (file_descriptor == -1)
        return false;
    else
    {
        close(file_descriptor);
        return true;
    }
}

string GetCurrentDateAndTimeString ()
{
    time_t timestamp;
    time(&timestamp);
    string timestamp_string(ctime(&timestamp));
    return timestamp_string.substr(0, timestamp_string.find_first_of("\n"));
}

} // end of namespace Barf
