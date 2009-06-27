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

#include "barf_filoc.hpp"

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

Uint8 SwitchCase (Uint8 c);
Uint8 GetEscapedChar (Uint8 c);
string GetEscapedString (string const &text);
string GetCharLiteral (Uint8 c, bool with_quotes = true);
string GetStringLiteral (string const &text, bool with_quotes = true);

Uint32 GetNewlineCount (string const &text);

void ReplaceAllInString (
    string *string_to_replace_in,
    string const &replace_this,
    string const &replacement);

string GetDirectoryPortion (string const &path);
string GetFilenamePortion (string const &path);

bool GetIsValidDirectory (string const &directory);
bool GetIsValidFile (string const &filename);

string GetCurrentDateAndTimeString ();

} // end of namespace Barf

#endif // !defined(BARF_UTIL_HPP_)
