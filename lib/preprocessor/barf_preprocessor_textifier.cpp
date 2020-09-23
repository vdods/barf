// 2006.11.18 - Copyright Victor Dods - Licensed under Apache 2.0

#include "barf_preprocessor_textifier.hpp"

#include "barf_optionsbase.hpp"
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
        string line_directive(body.GetFiLoc().LineDirectiveString(GetOptions().LineDirectivesRelativeToPath()));
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
        string line_directive(m_output_filoc.LineDirectiveString(GetOptions().LineDirectivesRelativeToPath()));
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
