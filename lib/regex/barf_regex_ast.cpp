// ///////////////////////////////////////////////////////////////////////////
// barf_regex_ast.cpp by Victor Dods, created 2006/01/27
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_regex_ast.hpp"

#include <sstream>

namespace Barf {
namespace Regex {

string const &GetAstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AST_COUNT-Ast::AST_START_CUSTOM_TYPES_HERE_] =
    {
        "AST_BOUND",
        "AST_BRACKET_CHAR_SET",
        "AST_BRANCH",
        "AST_CHAR",
        "AST_PIECE",
        "AST_REGULAR_EXPRESSION",
        "AST_REGULAR_EXPRESSION_MAP"
    };

    assert(ast_type < AST_COUNT);
    if (ast_type < Ast::AST_START_CUSTOM_TYPES_HERE_)
        return Ast::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-Ast::AST_START_CUSTOM_TYPES_HERE_];
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void Bound::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    if (m_lower_bound == m_upper_bound)
        stream << Tabs(indent_level) << Stringify(GetAstType()) << " {" << m_lower_bound << '}' << endl;
    else if (m_upper_bound == -1)
        stream << Tabs(indent_level) << Stringify(GetAstType()) << " {" << m_lower_bound << ",}" << endl;
    else
        stream << Tabs(indent_level) << Stringify(GetAstType()) << " {" << m_lower_bound << ", " << m_upper_bound << '}' << endl;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void Piece::SetBound (Bound *bound)
{
    assert(bound != NULL);
    delete m_bound;
    m_bound = bound;
}

void Piece::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(m_atom != NULL);
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    m_atom->Print(stream, Stringify, indent_level+1);
    if (m_bound != NULL)
        m_bound->Print(stream, Stringify, indent_level+1);
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void Branch::AddAtom (Atom *atom)
{
    assert(atom != NULL);
    Append(new Piece(atom, new Bound(1, 1)));
    m_last_modification_was_atom_addition = true;
}

void Branch::AddBound (Bound *bound)
{
    assert(size() > 0);
    assert(bound != NULL);
    Piece *last_piece = GetElement(size()-1);
    last_piece->SetBound(bound);
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void Char::Escape ()
{
    // TODO: emit warning if trying to escape a hex char?

    if (GetIsControlChar())
        return;

    switch (m_char)
    {
        // conditional escape chars
        case 'y': m_conditional_type = CT_BEGINNING_OF_INPUT;     break;
        case 'Y': m_conditional_type = CT_NOT_BEGINNING_OF_INPUT; break;
        case 'z': m_conditional_type = CT_END_OF_INPUT;           break;
        case 'Z': m_conditional_type = CT_NOT_END_OF_INPUT;       break;
        case 'l': m_conditional_type = CT_BEGINNING_OF_LINE;      break;
        case 'L': m_conditional_type = CT_NOT_BEGINNING_OF_LINE;  break;
        case 'e': m_conditional_type = CT_END_OF_LINE;            break;
        case 'E': m_conditional_type = CT_NOT_END_OF_LINE;        break;
        case 'b': m_conditional_type = CT_WORD_BOUNDARY;          break;
        case 'B': m_conditional_type = CT_NOT_WORD_BOUNDARY;      break;
        // escaping '0' should not provide a literal '\0'.
        case '0': break;
        // otherwise, do normal char escaping (e.g. '\a', '\n', etc).
        default : m_char = GetEscapedChar(m_char);             break;
    }
}

void Char::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    if (GetIsControlChar())
        stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << m_conditional_type << ' ' << endl;
    else
        stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << GetCharLiteral(m_char) << endl;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void BracketCharSet::AddChar (Uint8 ch)
{
    m_char_set.set(ch);
    m_char_set.reset(0); // '\0' can never be in the set
}

void BracketCharSet::AddCharRange (Uint8 low_char, Uint8 high_char)
{
    assert(low_char <= high_char);
    if (high_char == 0xFF)
        m_char_set.set(high_char--);
    while (low_char <= high_char)
        m_char_set.set(low_char++);
    m_char_set.reset(0); // '\0' can never be in the set
}

void BracketCharSet::AddCharClass (string const &char_class)
{
    // see http://opengroup.org/onlinepubs/007908799/xbd/locale.html for more info on specific character classes.

    if (char_class == "alnum")
    {
        AddCharRange('a', 'z');
        AddCharRange('A', 'Z');
        AddCharRange('0', '9');
    }
    else if (char_class == "digit")
    {
        AddCharRange('0', '9');
    }
    else if (char_class == "punct")
    {
        // control chars below space are not included in this class
        // space is not included in this class
        AddCharRange('!', '/');
        // the decimal digits are not included in this class
        AddCharRange(':', '@');
        // the uppercase alphabetic chars are not included in this class
        AddCharRange('[', '`');
        // the lowercase alphabetic chars are not included in this class
        AddCharRange('{', '~');
        // the delete char and above are not included in this class
    }
    else if (char_class == "alpha")
    {
        AddCharRange('a', 'z');
        AddCharRange('A', 'Z');
    }
    else if (char_class == "graph")
    {
        // space is not included in this class
        AddCharRange('!', '~');
    }
    else if (char_class == "space")
    {
        AddChar(' ');
        AddChar('\n');
        AddChar('\t');
        AddChar('\r');
        AddChar('\f');
        AddChar('\v');
    }
    else if (char_class == "blank")
    {
        AddChar(' ');
        AddChar('\t');
    }
    else if (char_class == "lower")
    {
        AddCharRange('a', 'z');
    }
    else if (char_class == "upper")
    {
        AddCharRange('A', 'Z');
    }
    else if (char_class == "cntrl")
    {
        AddCharRange('\x00', '\x1F');
        AddCharRange('\x7F', '\xFF');
    }
    else if (char_class == "print")
    {
        AddCharRange(' ', '~');
    }
    else if (char_class == "xdigit")
    {
        AddCharRange('a', 'f');
        AddCharRange('A', 'F');
        AddCharRange('0', '9');
    }
    else
    {
        ostringstream out;
        out << "invalid char class [:" << char_class << ":]";
        throw out.str();
    }

    assert(!m_char_set.test(0) && "'\\0' should never be set");
}

void BracketCharSet::Negate ()
{
    m_char_set.flip();
    m_char_set.reset(0); // '\0' can never be in the set
}

void BracketCharSet::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(!m_char_set.test(0) && "'\\0' should never be set");

    stream << Tabs(indent_level) << Stringify(GetAstType()) << " [";

    // TODO: correct escaping of chars in the context of a bracket expression

    Uint16 c0 = 0;
    Uint16 c1;
    while (c0 < 256)
    {
        if (m_char_set.test(c0))
        {
            c1 = c0;
            while (c1 < 255 && m_char_set.test(c1+1))
                ++c1;

            assert(c0 < 256 && c1 < 256);
            if (c0 == c1)
                stream << GetCharLiteral(c0, false);
            else if (c0+1 == c1)
                stream << GetCharLiteral(c0, false) << GetCharLiteral(c1, false);
            else
                stream << GetCharLiteral(c0, false) << '-' << GetCharLiteral(c1, false);

            c0 = c1;
        }
        ++c0;
    }

    stream << ']' << endl;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void RegularExpression::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, GetAstTypeString, indent_level);
}

void RegularExpression::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    for_each(begin(), end(), PrintAst<Base>(stream, Stringify, indent_level+1));
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void RegularExpressionMap::Print (ostream &stream, Uint32 indent_level) const
{
    Ast::AstMap<RegularExpression>::Print(stream, GetAstTypeString, indent_level);
}

} // end of namespace Regex
} // end of namespace Barf
