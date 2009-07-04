// ///////////////////////////////////////////////////////////////////////////
// barf_ast.cpp by Victor Dods, created 2006/10/22
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_ast.hpp"

#include <sstream>

namespace Barf {
namespace Ast {

string const &AstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AST_START_CUSTOM_TYPES_HERE_] =
    {
        "AST_CHAR",
        "AST_DIRECTIVE",
        "AST_DIRECTIVE_LIST",
        "AST_DUMB_CODE_BLOCK",
        "AST_ID",
        "AST_ID_LIST",
        "AST_ID_MAP",
        "AST_SIGNED_INTEGER",
        "AST_STRICT_CODE_BLOCK",
        "AST_STRING",
        "AST_THROW_AWAY",
        "AST_UNSIGNED_INTEGER"
    };

    assert(ast_type < AST_START_CUSTOM_TYPES_HERE_);
    return s_ast_type_string[ast_type];
}

void Base::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
}

void Char::Escape ()
{
    m_char = EscapedChar(m_char);
}

void Char::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << CharLiteral() << endl;
}

void SignedInteger::ShiftAndAdd (Sint32 value)
{
    assert(0 <= value && value <= 9);
    if (m_value > SINT32_UPPER_BOUND/10 || value > SINT32_UPPER_BOUND%10)
        THROW_STRING("integer overflow");
    m_value = (10 * m_value) + value;
}

void SignedInteger::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << m_value << endl;
}

void UnsignedInteger::ShiftAndAdd (Uint32 value)
{
    assert(0 <= value && value <= 9);
    if (m_value > UINT32_UPPER_BOUND/10 || value > UINT32_UPPER_BOUND%10)
        THROW_STRING("integer overflow");
    m_value = (10 * m_value) + value;
}

void UnsignedInteger::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << m_value << endl;
}

TextBase::~TextBase ()
{
}

string TextBase::DirectiveTypeString (AstType ast_type)
{
    if (ast_type == AST_STRING) return "%string";
    if (ast_type == AST_ID) return "%identifier";
    if (ast_type == AST_DUMB_CODE_BLOCK) return "%dumb_code_block";
    if (ast_type == AST_STRICT_CODE_BLOCK) return "%strict_code_block";

    assert(false && "invalid TextBase AstType");
    return g_empty_string;
}

void TextBase::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << StringLiteral(GetText()) << endl;
}

void Id::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << GetText() << endl;
}

CodeBlock::~CodeBlock ()
{
}

void Directive::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << " (" << GetText() << ')' << endl;
}

} // end of namespace Ast
} // end of namespace Barf
