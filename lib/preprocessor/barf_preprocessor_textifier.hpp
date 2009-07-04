// ///////////////////////////////////////////////////////////////////////////
// barf_preprocessor_textifier.hpp by Victor Dods, created 2006/11/18
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_PREPROCESSOR_TEXTIFIER_HPP_)
#define BARF_PREPROCESSOR_TEXTIFIER_HPP_

#include "barf_preprocessor.hpp"

#include <ostream>

#include "barf_filelocation.hpp"

namespace Barf {
namespace Preprocessor {

class Body;
class SymbolTable;

class Textifier
{
public:

    Textifier (ostream &output_stream, string const &output_filename)
        :
        m_output_stream(output_stream),
        m_output_filoc(output_filename, 1),
        m_generates_line_directives(true)
    { }
    Textifier (ostream &output_stream)
        :
        m_output_stream(output_stream),
        m_output_filoc(FileLocation::ms_invalid),
        m_generates_line_directives(false)
    { }

    FileLocation const &OutputFiLoc () const { return m_output_filoc; }
    bool GeneratesLineDirectives () const { return m_generates_line_directives; }

    void GeneratesLineDirectives (bool generates_line_directives)
    {
        m_generates_line_directives = generates_line_directives;
    }

    void TextifyBody (Body const &body, SymbolTable &symbol_table);

    Textifier &operator << (string const &text);
    Textifier &operator << (Sint32 value);

private:

    ostream &m_output_stream;
    FileLocation m_output_filoc;
    bool m_generates_line_directives;
}; // end of class Textifier

} // end of namespace Preprocessor
} // end of namespace Barf

#endif // !defined(BARF_PREPROCESSOR_TEXTIFIER_HPP_)
