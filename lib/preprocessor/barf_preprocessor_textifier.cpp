// ///////////////////////////////////////////////////////////////////////////
// barf_preprocessor_textifier.cpp by Victor Dods, created 2006/11/18
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_preprocessor_textifier.hpp"

#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_symboltable.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace Preprocessor {

void Textifier::TextifyBody (Body const &body, SymbolTable &symbol_table)
{
    if (m_generates_line_directives && body.GetFiLoc().IsValid())
    {
        m_output_filoc.IncrementLineNumber(2);
        string line_directive(body.GetFiLoc().LineDirectiveString());
        m_output_stream << '\n' << line_directive << '\n';
    }

    // turn off line directives inside the call to Execute
    {
        bool saved_generates_line_directives = m_generates_line_directives;
        m_generates_line_directives = false;
        body.Execute(*this, symbol_table);
        m_generates_line_directives = saved_generates_line_directives;
    }

    if (m_generates_line_directives && body.GetFiLoc().IsValid())
    {
        m_output_filoc.IncrementLineNumber(2);
        string line_directive(m_output_filoc.LineDirectiveString());
        m_output_stream << '\n' << line_directive << '\n';
    }
}

Textifier &Textifier::operator << (string const &text)
{
    if (m_output_filoc.IsValid())
        m_output_filoc.IncrementLineNumber(NewlineCount(text));
    m_output_stream << text;
    return *this;
}

Textifier &Textifier::operator << (Sint32 value)
{
    m_output_stream << value;
    return *this;
}

} // end of namespace Preprocessor
} // end of namespace Barf
