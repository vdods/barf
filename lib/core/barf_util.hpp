// ///////////////////////////////////////////////////////////////////////////
// barf_util.hpp by Victor Dods, created 2006/02/11
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_UTIL_HPP_)
#define BARF_UTIL_HPP_

#include "barf.hpp"

#include <ostream>

#include "barf_filelocation.hpp"

namespace Barf {

struct Tabs
{
    Uint32 const m_count;

    Tabs (Uint32 count)
        :
        m_count(count)
    { }
}; // end of struct Tabs

ostream &operator << (ostream &stream, Tabs const &tabs);

template <typename T>
struct PrintAst
{
    PrintAst (ostream &stream, StringifyAstType Stringify, Uint32 indent_level)
        :
        m_stream(stream),
        m_StringifyAstType(Stringify),
        m_indent_level(indent_level)
    { }

    void operator () (T const *ast)
    {
        assert(ast != NULL);
        ast->Print(m_stream, m_StringifyAstType, m_indent_level);
    }
    void operator () (pair<string, T const *> const &map_pair)
    {
        assert(map_pair.second != NULL);
        map_pair.second->Print(m_stream, m_StringifyAstType, m_indent_level);
    }

private:

    ostream &m_stream;
    StringifyAstType const m_StringifyAstType;
    Uint32 const m_indent_level;
};

enum EscapeStringReturnCode
{
    ESRC_SUCCESS = 0,
    // if there's a backslash immediately before the end of the string
    ESRC_UNEXPECTED_EOI,
    // if there's \x without a hex digit after it
    ESRC_MALFORMED_HEX_CHAR,
    // if the hex code's value exceeded 255
    ESRC_HEX_ESCAPE_SEQUENCE_OUT_OF_RANGE,
    // if the octal code's value exceeded 255
    ESRC_OCTAL_ESCAPE_SEQUENCE_OUT_OF_RANGE
}; // end of enum EscapeStringReturnCode

struct EscapeStringStatus
{
    EscapeStringReturnCode m_return_code;
    Uint32 m_line_number_offset;

    EscapeStringStatus (EscapeStringReturnCode return_code, Uint32 line_number_offset)
        :
        m_return_code(return_code),
        m_line_number_offset(line_number_offset)
    { }
}; // end of struct EscapeStringStatus

Uint8 SwitchCase (Uint8 c);
// in-place char-escaping
void EscapeChar (Uint8 &c);
Uint8 EscapedChar (Uint8 c);
// in-place string-escaping
EscapeStringStatus EscapeString (string &text);
string EscapedString (string const &text, EscapeStringReturnCode *escape_string_return_code = NULL);
string CharLiteral (Uint8 c, bool with_quotes = true);
string StringLiteral (string const &text, bool with_quotes = true);

Uint32 NewlineCount (string const &text);

void ReplaceAllInString (
    string *string_to_replace_in,
    string const &replace_this,
    string const &replacement);

string DirectoryPortion (string const &path);
string FilenamePortion (string const &path);

bool IsValidDirectory (string const &directory);
bool IsValidFile (string const &filename);

string CurrentDateAndTimeString ();

} // end of namespace Barf

#endif // !defined(BARF_UTIL_HPP_)
