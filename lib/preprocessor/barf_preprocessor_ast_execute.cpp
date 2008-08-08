// ///////////////////////////////////////////////////////////////////////////
// barf_preprocessor_ast_execute.cpp by Victor Dods, created 2006/10/18
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_preprocessor_ast.hpp"

#include <sstream>

#include "barf_optionsbase.hpp"
#include "barf_preprocessor_parser.hpp"
#include "barf_preprocessor_textifier.hpp"

namespace Barf {
namespace Preprocessor {

void Body::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    for (const_iterator it = begin(), it_end = end(); it != it_end; ++it)
        (*it)->Execute(textifier, symbol_table);
}

void Conditional::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    assert(m_if_body != NULL);

    if (m_if_expression->GetIntegerValue(symbol_table) != 0)
        m_if_body->Execute(textifier, symbol_table);
    else if (m_else_body != NULL)
        m_else_body->Execute(textifier, symbol_table);
}

void DumpSymbolTable::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    ostringstream out;
    symbol_table.Print(out);
    textifier << out.str();
}

void DeclareArray::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    ArraySymbol *symbol = symbol_table.DefineArraySymbol(m_id->GetText(), m_id->GetFiLoc());
    assert(symbol->GetArrayElementCount() == 0);
}

void DeclareMap::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    MapSymbol *symbol = symbol_table.DefineMapSymbol(m_id->GetText(), m_id->GetFiLoc());
    assert(symbol->GetMapElementCount() == 0);
}

void Define::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    assert(m_body != NULL);

    ostringstream out;
    Textifier sandboxed_textifier(out);
    m_body->Execute(sandboxed_textifier, symbol_table);
    Body *evaluated_body = new Body();
    evaluated_body->Append(new Text(out.str(), FiLoc::ms_invalid));

    ScalarSymbol *symbol = symbol_table.DefineScalarSymbol(m_id->GetText(), m_id->GetFiLoc());
    symbol->SetScalarBody(evaluated_body);
}

void DefineArrayElement::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    assert(m_body != NULL);

    Symbol *symbol = symbol_table.GetSymbol(m_id->GetText());
    if (symbol != NULL && !symbol->GetIsArraySymbol())
    {
        EmitError("macro \"" + m_id->GetText() + "\" is not an array", m_id->GetFiLoc());
        return;
    }

    ostringstream out;
    Textifier sandboxed_textifier(out);
    m_body->Execute(sandboxed_textifier, symbol_table);
    Body *evaluated_body = new Body();
    evaluated_body->Append(new Text(out.str(), FiLoc::ms_invalid));

    ArraySymbol *array_symbol = Dsc<ArraySymbol *>(symbol);
    if (array_symbol == NULL)
        array_symbol = symbol_table.DefineArraySymbol(m_id->GetText(), m_id->GetFiLoc());
    array_symbol->AppendArrayElement(evaluated_body);
}

void DefineMapElement::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    assert(m_body != NULL);

    Symbol *symbol = symbol_table.GetSymbol(m_id->GetText());
    if (symbol != NULL && !symbol->GetIsMapSymbol())
    {
        EmitError("macro \"" + m_id->GetText() + "\" is not a map", m_id->GetFiLoc());
        return;
    }

    ostringstream out;
    Textifier sandboxed_textifier(out);
    m_body->Execute(sandboxed_textifier, symbol_table);
    Body *evaluated_body = new Body();
    evaluated_body->Append(new Text(out.str(), FiLoc::ms_invalid));

    MapSymbol *map_symbol = Dsc<MapSymbol *>(symbol);
    if (map_symbol == NULL)
        map_symbol = symbol_table.DefineMapSymbol(m_id->GetText(), m_id->GetFiLoc());
    map_symbol->SetMapElement(m_key->GetText(), evaluated_body);
}

void Undefine::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    symbol_table.UndefineSymbol(m_id->GetText(), m_id->GetFiLoc());
}

void Loop::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    assert(m_body != NULL);

    if (m_iterator_integer_body == NULL)
    {
        assert(m_iterator_integer == NULL);

        m_iterator_integer_body = new Body();
        m_iterator_integer = new Integer(0, FiLoc::ms_invalid);
        m_iterator_integer_body->Append(m_iterator_integer);
    }

    assert(m_iterator_integer_body != NULL);
    assert(m_iterator_integer != NULL);

    {
        ScalarSymbol *symbol =
            symbol_table.DefineScalarSymbol(
                m_iterator_id->GetText(),
                m_iterator_id->GetFiLoc());
        symbol->SetScalarBody(m_iterator_integer_body);
    }

    Sint32 iteration_count = m_iteration_count_expression->GetIntegerValue(symbol_table);
    if (iteration_count < 0)
    {
        ostringstream out;
        out << "negative value (" << iteration_count << ") invalid for loop iteration count";
        EmitError(out.str(), GetFiLoc());
        return;
    }
    for (m_iterator_integer->SetValue(0);
         m_iterator_integer->GetIntegerValue(symbol_table) < iteration_count;
         m_iterator_integer->SetValue(m_iterator_integer->GetIntegerValue(symbol_table)+1))
    {
        m_body->Execute(textifier, symbol_table);
    }
    symbol_table.UndefineSymbol(
        m_iterator_id->GetText(),
        m_iterator_id->GetFiLoc());
}

void ForEach::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    assert(m_body != NULL);

    MapSymbol *map_symbol = NULL;
    {
        Symbol *symbol = symbol_table.GetSymbol(m_map_id->GetText());
        if (symbol == NULL)
        {
            EmitError("undefined macro \"" + m_map_id->GetText() + "\"", m_map_id->GetFiLoc());
            return;
        }
        if (!symbol->GetIsMapSymbol())
        {
            EmitError("macro \"" + m_map_id->GetText() + "\" is not a map", m_map_id->GetFiLoc());
            return;
        }
        map_symbol = Dsc<MapSymbol *>(symbol);
    }

    if (m_key_text_body == NULL)
    {
        assert(m_key_text == NULL);

        m_key_text_body = new Body();
        m_key_text = new Text("", FiLoc::ms_invalid);
        m_key_text_body->Append(m_key_text);
    }

    assert(m_key_text_body != NULL);
    assert(m_key_text != NULL);

    {
        ScalarSymbol *symbol =
            symbol_table.DefineScalarSymbol(
                m_key_id->GetText(),
                m_key_id->GetFiLoc());
        symbol->SetScalarBody(m_key_text_body);
    }

    for (MapSymbol::BodyMap::const_iterator it = map_symbol->GetBegin(),
                                            it_end = map_symbol->GetEnd();
         it != it_end;
         ++it)
    {
        m_key_text->SetText(it->first);
        m_body->Execute(textifier, symbol_table);
    }
    symbol_table.UndefineSymbol(
        m_key_id->GetText(),
        m_key_id->GetFiLoc());
}

void Include::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    if (m_include_body_root == NULL)
    {
        string filename_expression(m_include_filename_expression->GetTextValue(symbol_table));
        EmitExecutionMessage("preprocessor encountered include(\"" + filename_expression + "\") directive");
        Parser parser;
        // figure out the pathname from the targets search path
        string filename(
            GetOptions().GetSearchPath().GetFilePath(
                filename_expression));
        if (filename.empty() || !parser.OpenFile(filename))
        {
            EmitError("file \"" + filename_expression + "\" not found in search path", GetFiLoc());
            return;
        }
        Ast::Base *parsed_tree_root = NULL;
        if (parser.Parse(&parsed_tree_root) != Parser::PRC_SUCCESS)
        {
            EmitError("parse error in include file \"" + filename + "\"", GetFiLoc());
            return;
        }
        m_include_body_root = Dsc<Body *>(parsed_tree_root);
    }

    assert(m_include_body_root != NULL);

    if (m_is_sandboxed)
    {
        SymbolTable local_symbol_table;
        m_include_body_root->Execute(textifier, local_symbol_table);
    }
    else
        m_include_body_root->Execute(textifier, symbol_table);
}

void Message::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    string message_text(m_message_expression->GetTextValue(symbol_table));
    switch (m_criticality)
    {
        case WARNING:
            EmitWarning(message_text, GetFiLoc());
            break;

        case ERROR:
            EmitError(message_text, GetFiLoc());
            break;

        case FATAL_ERROR:
            EmitFatalError(message_text, GetFiLoc());
            break;

        default:
            assert(false && "invalid Message::Criticality");
            break;
    }
}

void Text::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    textifier << m_text;
}

void Integer::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    textifier << GetIntegerValue(symbol_table);
}

void Sizeof::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    textifier << GetIntegerValue(symbol_table);
}

void Dereference::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    Body const *dereferenced_body = GetDereferencedBody(symbol_table);
    if (dereferenced_body == NULL)
        return;

    textifier.TextifyBody(*dereferenced_body, symbol_table);
}

void IsDefined::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    textifier << GetIntegerValue(symbol_table);
}

void Operation::Execute (Textifier &textifier, SymbolTable &symbol_table) const
{
    if (GetIsTextOperation())
        textifier << GetTextValue(symbol_table);
    else
        textifier << GetIntegerValue(symbol_table);
}

} // end of namespace Preprocessor
} // end of namespace Barf
