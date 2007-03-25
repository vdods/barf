// ///////////////////////////////////////////////////////////////////////////
// barf_preprocessor_ast_print.cpp by Victor Dods, created 2006/10/18
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_preprocessor_ast.hpp"

#include "barf_util.hpp"

namespace Barf {
namespace Preprocessor {

// this is the non-virtual, top-level Print method, not
// to be confused with AstCommon::Ast::Print.
void Body::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, GetAstTypeString, indent_level);
}

void Conditional::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(m_if_body != NULL);

    AstCommon::Ast::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "if expression:" << endl;
    m_if_expression->Print(stream, Stringify, indent_level+2);
    stream << Tabs(indent_level+1) << "if body:" << endl;
    m_if_body->Print(stream, Stringify, indent_level+2);
    if (m_else_body != NULL)
    {
        stream << Tabs(indent_level+1) << "else body:" << endl;
        m_else_body->Print(stream, Stringify, indent_level+2);
    }
}

void DeclareArray::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << m_identifier->GetText() << endl;
}

void DeclareMap::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << m_identifier->GetText() << endl;
}

void Define::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(m_body != NULL);

    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    stream << Tabs(indent_level+1) << "identifier: " << m_identifier->GetText() << endl;
    stream << Tabs(indent_level+1) << "body: " << endl;
    m_body->Print(stream, Stringify, indent_level+2);
}

void DefineMapElement::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(m_body != NULL);

    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    stream << Tabs(indent_level+1) << "identifier: " << m_identifier->GetText() << endl;
    stream << Tabs(indent_level+1) << "key: " << m_key->GetText() << endl;
    stream << Tabs(indent_level+1) << "body: " << endl;
    m_body->Print(stream, Stringify, indent_level+2);
}

void Undefine::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    stream << Tabs(indent_level+1) << "identifier: " << m_identifier->GetText() << endl;
}

void Loop::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(m_body != NULL);

    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    stream << Tabs(indent_level+1) << "iterator identifier: " << m_iterator_identifier->GetText() << endl;
    stream << Tabs(indent_level+1) << "iteration count expression:" << endl;
    m_iteration_count_expression->Print(stream, Stringify, indent_level+2);
    stream << Tabs(indent_level+1) << "loop body:" << endl;
    m_body->Print(stream, Stringify, indent_level+2);
}

void ForEach::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(m_body != NULL);

    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    stream << Tabs(indent_level+1) << "key identifier: " << m_key_identifier->GetText() << endl;
    stream << Tabs(indent_level+1) << "map identifier:" << m_map_identifier->GetText() << endl;
    stream << Tabs(indent_level+1) << "loop body:" << endl;
    m_body->Print(stream, Stringify, indent_level+2);
}

void Include::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << (m_is_sandboxed ? "(sandboxed)" : "") << endl;
    stream << Tabs(indent_level+1) << "include filename expression:" << endl;
    m_include_filename_expression->Print(stream, Stringify, indent_level+2);
}

void Message::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << m_criticality << endl;
    stream << Tabs(indent_level+1) << "message expression:" << endl;
    m_message_expression->Print(stream, Stringify, indent_level+2);
}

void Text::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << GetStringLiteral(m_text) << endl;
}

void Integer::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << " value: " << m_value << endl;
}

void Sizeof::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << " identifier: " << m_identifier->GetText() << endl;
}

void Dereference::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    stream << Tabs(indent_level+1) << "identifier: " << m_identifier->GetText() << endl;
    if (m_element_index_expression != NULL)
    {
        stream << Tabs(indent_level+1) << "element index expression:" << endl;
        m_element_index_expression->Print(stream, Stringify, indent_level+2);
    }
    stream << Tabs(indent_level+1) << "dereference type: " << GetDereferenceTypeString(m_dereference_type) << endl;
}

void Operation::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << " operator " << m_op << endl;
    if (m_left != NULL)
    {
        stream << Tabs(indent_level+1) << "left side expression:" << endl;
        m_left->Print(stream, Stringify, indent_level+2);
    }
    if (m_right != NULL)
    {
        stream << Tabs(indent_level+1) << "right side expression:" << endl;
        m_right->Print(stream, Stringify, indent_level+2);
    }
}

} // end of namespace Preprocessor
} // end of namespace Barf
