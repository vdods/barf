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

bool GetIsHexDigitChar (Uint8 c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

Uint8 GetHexDigit (Uint8 c)
{
    assert(GetIsHexDigitChar(c));
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

Uint8 GetEscapedChar (Uint8 const c)
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

string GetEscapedString (string const &text)
{
    string retval;
    for (string::const_iterator it = text.begin(),
                                it_end = text.end();
         it != it_end;
         ++it)
    {
        if (*it == '\\')
        {
            ++it;
            if (it == it_end)
                break;
            else if (*it == 'x')
            {
                ++it;
                if (it == it_end)
                    retval += 'x';
                else if (!GetIsHexDigitChar(*it))
                    retval += 'x', retval += *it;
                else
                {
                    Uint8 c = *it;
                    Uint8 hex_value = GetHexDigit(c);
                    ++it;
                    if (it == it_end)
                        retval += c;
                    else if (!GetIsHexDigitChar(*it))
                        retval += hex_value, retval += *it;
                    else
                        retval += (hex_value << 4) | GetHexDigit(*it);
                }
            }
            else
                retval += GetEscapedChar(*it);
        }
        else
            retval += *it;
    }
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
