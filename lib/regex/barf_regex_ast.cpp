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
    static string const s_ast_type_string[AT_COUNT-AstCommon::AT_START_CUSTOM_TYPES_HERE_] =
    {
        "AT_BOUND",
        "AT_PIECE",
        "AT_BRANCH",
        "AT_CHARACTER",
        "AT_BRACKET_CHARACTER_SET",
        "AT_REGULAR_EXPRESSION",
        "AT_REGULAR_EXPRESSION_MAP"
    };

    assert(ast_type < AT_COUNT);
    if (ast_type < AstCommon::AT_START_CUSTOM_TYPES_HERE_)
        return AstCommon::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-AstCommon::AT_START_CUSTOM_TYPES_HERE_];
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

void Character::Escape ()
{
    // TODO: emit warning if trying to escape a hex char?

    if (GetIsControlCharacter())
        return;

    switch (m_character)
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
        // otherwise, do normal character escaping (e.g. '\a', '\n', etc).
        default : m_character = GetEscapedChar(m_character);             break;
    }
}

void Character::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    if (GetIsControlCharacter())
        stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << m_conditional_type << ' ' << endl;
    else
        stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << GetCharacterLiteral(m_character) << endl;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void BracketCharacterSet::AddCharacter (Uint8 character)
{
    m_character_set.set(character);
    m_character_set.reset(0); // '\0' can never be in the set
}

void BracketCharacterSet::AddCharacterRange (Uint8 low_character, Uint8 high_character)
{
    assert(low_character <= high_character);
    if (high_character == 0xFF)
        m_character_set.set(high_character--);
    while (low_character <= high_character)
        m_character_set.set(low_character++);
    m_character_set.reset(0); // '\0' can never be in the set
}

void BracketCharacterSet::AddCharacterClass (string const &character_class)
{
    // see http://opengroup.org/onlinepubs/007908799/xbd/locale.html for more info on specific character classes.

    if (character_class == "alnum")
    {
        AddCharacterRange('a', 'z');
        AddCharacterRange('A', 'Z');
        AddCharacterRange('0', '9');
    }
    else if (character_class == "digit")
    {
        AddCharacterRange('0', '9');
    }
    else if (character_class == "punct")
    {
        // control characters below space are not included in this class
        // space is not included in this class
        AddCharacterRange('!', '/');
        // the decimal digits are not included in this class
        AddCharacterRange(':', '@');
        // the uppercase alphabetic characters are not included in this class
        AddCharacterRange('[', '`');
        // the lowercase alphabetic characters are not included in this class
        AddCharacterRange('{', '~');
        // the delete character and above are not included in this class
    }
    else if (character_class == "alpha")
    {
        AddCharacterRange('a', 'z');
        AddCharacterRange('A', 'Z');
    }
    else if (character_class == "graph")
    {
        // space is not included in this class
        AddCharacterRange('!', '~');
        if (g_can_print_extended_ascii)
            AddCharacterRange('\x80', '\xFF');
    }
    else if (character_class == "space")
    {
        AddCharacter(' ');
        AddCharacter('\n');
        AddCharacter('\t');
        AddCharacter('\r');
        AddCharacter('\f');
        AddCharacter('\v');
    }
    else if (character_class == "blank")
    {
        AddCharacter(' ');
        AddCharacter('\t');
    }
    else if (character_class == "lower")
    {
        AddCharacterRange('a', 'z');
    }
    else if (character_class == "upper")
    {
        AddCharacterRange('A', 'Z');
    }
    else if (character_class == "cntrl")
    {
        AddCharacterRange('\x00', '\x1F');
        // space through tilde are not included in this class
        if (g_can_print_extended_ascii)
            AddCharacter('\x7F');
        else
            AddCharacterRange('\x7F', '\xFF');
    }
    else if (character_class == "print")
    {
        AddCharacterRange(' ', '~');
        if (g_can_print_extended_ascii)
            AddCharacterRange('\x80', '\xFF');
    }
    else if (character_class == "xdigit")
    {
        AddCharacterRange('a', 'f');
        AddCharacterRange('A', 'F');
        AddCharacterRange('0', '9');
    }
    else
    {
        ostringstream out;
        out << "invalid character class [:" << character_class << ":]";
        throw out.str();
    }

    assert(!m_character_set.test(0) && "'\\0' should never be set");
}

void BracketCharacterSet::Negate ()
{
    m_character_set.flip();
    m_character_set.reset(0); // '\0' can never be in the set
}

void BracketCharacterSet::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(!m_character_set.test(0) && "'\\0' should never be set");

    stream << Tabs(indent_level) << Stringify(GetAstType()) << " [";

    // TODO: correct escaping of characters in the context of a bracket expression

    Uint16 c0 = 0;
    Uint16 c1;
    while (c0 < 256)
    {
        if (m_character_set.test(c0))
        {
            c1 = c0;
            while (c1 < 255 && m_character_set.test(c1+1))
                ++c1;

            assert(c0 < 256 && c1 < 256);
            if (c0 == c1)
                stream << GetCharacterLiteral(c0, false);
            else if (c0+1 == c1)
                stream << GetCharacterLiteral(c0, false) << GetCharacterLiteral(c1, false);
            else
                stream << GetCharacterLiteral(c0, false) << '-' << GetCharacterLiteral(c1, false);

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
    for_each(begin(), end(), PrintAst<Ast>(stream, Stringify, indent_level+1));
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void RegularExpressionMap::Print (ostream &stream, Uint32 indent_level) const
{
    AstCommon::AstMap<RegularExpression>::Print(stream, GetAstTypeString, indent_level);
}

} // end of namespace Regex
} // end of namespace Barf
