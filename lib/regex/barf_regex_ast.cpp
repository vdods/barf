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

#include "barf_util.hpp"

namespace Barf {
namespace Regex {

string const &GetAstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AST_COUNT-Ast::AST_START_CUSTOM_TYPES_HERE_] =
    {
        "AST_BAKED_CONTROL_CHAR",
        "AST_BOUND",
        "AST_BRACKET_CHAR_SET",
        "AST_BRANCH",
        "AST_CHAR",
        "AST_CONDITIONAL_CHAR",
        "AST_MODE_CONTROL_CHAR",
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

void Piece::ReplaceBound (Bound *bound)
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
    last_piece->ReplaceBound(bound);
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Atom *Char::Escaped ()
{
    Atom *escaped = this;

    switch (m_char)
    {
        // conditional escape chars
        case 'y': escaped = new ConditionalChar(CT_BEGINNING_OF_INPUT);     break;
        case 'Y': escaped = new ConditionalChar(CT_NOT_BEGINNING_OF_INPUT); break;
        case 'z': escaped = new ConditionalChar(CT_END_OF_INPUT);           break;
        case 'Z': escaped = new ConditionalChar(CT_NOT_END_OF_INPUT);       break;
        case 'l': escaped = new ConditionalChar(CT_BEGINNING_OF_LINE);      break;
        case 'L': escaped = new ConditionalChar(CT_NOT_BEGINNING_OF_LINE);  break;
        case 'e': escaped = new ConditionalChar(CT_END_OF_LINE);            break;
        case 'E': escaped = new ConditionalChar(CT_NOT_END_OF_LINE);        break;
        case 'b': escaped = new ConditionalChar(CT_WORD_BOUNDARY);          break;
        case 'B': escaped = new ConditionalChar(CT_NOT_WORD_BOUNDARY);      break;
        // baked control escape chars
        case 'c': escaped = new BakedControlChar(BCCT_CASE_SENSITIVITY_DISABLE); break;
        case 'C': escaped = new BakedControlChar(BCCT_CASE_SENSITIVITY_ENABLE);  break;
        // escaping '0' should not provide a literal '\0'.
        case '0': break;
        // otherwise, do normal char escaping (e.g. '\t', '\n', etc).
        default : m_char = GetEscapedChar(m_char); break;
    }

    return escaped;
}

void Char::EscapeInsideBracketExpression ()
{
    switch (m_char)
    {
        // escaping '0' should not provide a literal '\0'.
        case '0': break;
        // otherwise, do normal char escaping (e.g. '\t', '\n', etc).
        default : m_char = GetEscapedChar(m_char); break;
    }
}

void Char::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << GetCharLiteral(m_char) << endl;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void ConditionalChar::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << GetConditionalTypeString(m_conditional_type) << endl;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void BakedControlChar::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << GetBakedControlCharTypeString(m_baked_control_char_type) << endl;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

bool BracketCharSet::GetIsCharInSet (Uint8 ch, bool is_case_sensitive) const
{
    return m_char_set.test(ch)
           ||
           (!is_case_sensitive && m_char_set.test(SwitchCase(ch)));
}

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

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

bool NodesAreEqual (Ast::Base const *left, Ast::Base const *right)
{
    assert(left != NULL);
    assert(right != NULL);
    
    if (left->GetAstType() != right->GetAstType())
        return false;

    switch (left->GetAstType())
    {
        case AST_BAKED_CONTROL_CHAR:
        {
            BakedControlChar const *left_ch = Dsc<BakedControlChar const *>(left);
            BakedControlChar const *right_ch = Dsc<BakedControlChar const *>(right);
            return left_ch->m_baked_control_char_type == right_ch->m_baked_control_char_type;
        }
    
        case AST_BOUND:
        {
            Bound const *left_bound = Dsc<Bound const *>(left);
            Bound const *right_bound = Dsc<Bound const *>(right);
            return left_bound->m_lower_bound == right_bound->m_lower_bound &&
                   left_bound->m_upper_bound == right_bound->m_upper_bound;
        }
            
        case AST_BRACKET_CHAR_SET:
        {
            BracketCharSet const *left_set = Dsc<BracketCharSet const *>(left);
            BracketCharSet const *right_set = Dsc<BracketCharSet const *>(right);
            return left_set->m_char_set == right_set->m_char_set;
        }
            
        case AST_BRANCH:
        {
            Branch const *left_branch = Dsc<Branch const *>(left);
            Branch const *right_branch = Dsc<Branch const *>(right);
            if (left_branch->m_last_modification_was_atom_addition != right_branch->m_last_modification_was_atom_addition ||
                left_branch->size() != right_branch->size())
            {
                return false;
            }
            for (Branch::size_type i = 0; i < left_branch->size(); ++i)
                if (!NodesAreEqual(left_branch->GetElement(i), right_branch->GetElement(i)))
                    return false;
            return true;
        }
            
        case AST_CHAR:
        {
            Char const *left_ch = Dsc<Char const *>(left);
            Char const *right_ch = Dsc<Char const *>(right);
            return left_ch->m_char == right_ch->m_char;
        }

        case AST_CONDITIONAL_CHAR:
        {
            ConditionalChar const *left_ch = Dsc<ConditionalChar const *>(left);
            ConditionalChar const *right_ch = Dsc<ConditionalChar const *>(right);
            return left_ch->m_conditional_type == right_ch->m_conditional_type;
        }

        case AST_PIECE:
        {
            Piece const *left_piece = Dsc<Piece const *>(left);
            Piece const *right_piece = Dsc<Piece const *>(right);
            return NodesAreEqual(left_piece->m_atom, right_piece->m_atom) && NodesAreEqual(left_piece->m_bound, right_piece->m_bound);
        }
            
        case AST_REGULAR_EXPRESSION:
        {
            RegularExpression const *left_regex = Dsc<RegularExpression const *>(left);
            RegularExpression const *right_regex = Dsc<RegularExpression const *>(right);
            if (left_regex->size() != right_regex->size())
                return false;
            for (Branch::size_type i = 0; i < left_regex->size(); ++i)
                if (!NodesAreEqual(left_regex->GetElement(i), right_regex->GetElement(i)))
                    return false;
            return true;
        }
            
        case AST_REGULAR_EXPRESSION_MAP:
        default:
            assert(false && "invalid type to compare");
            return false;
    }

    assert(false && "forgot to return a value");
    return false;
}

} // end of namespace Regex
} // end of namespace Barf
