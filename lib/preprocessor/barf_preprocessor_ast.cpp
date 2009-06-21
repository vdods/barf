// ///////////////////////////////////////////////////////////////////////////
// barf_preprocessor_ast.cpp by Victor Dods, created 2006/10/15
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
#include "barf_preprocessor_textifier.hpp"

namespace Barf {
namespace Preprocessor {

string const &GetDereferenceTypeString (DereferenceType dereference_type)
{
    static string const s_dereference_type_string[2] =
    {
        "DEREFERENCE_ALWAYS",
        "DEREFERENCE_IFF_DEFINED"
    };

    assert(dereference_type >= 0 && dereference_type < 2);
    return s_dereference_type_string[dereference_type];
}

string const &GetAstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AST_COUNT-Ast::AST_START_CUSTOM_TYPES_HERE_] =
    {
        "AST_BODY",
        "AST_BODY_LIST",
        "AST_CONDITIONAL",
        "AST_DECLARE_ARRAY",
        "AST_DECLARE_MAP",
        "AST_DEFINE",
        "AST_DEFINE_ARRAY_ELEMENT",
        "AST_DEFINE_MAP_ELEMENT",
        "AST_DEREFERENCE",
        "AST_DUMP_SYMBOL_TABLE",
        "AST_FOR_EACH",
        "AST_INCLUDE",
        "AST_INTEGER",
        "AST_INTEGER_CAST",
        "AST_IS_DEFINED",
        "AST_LOOP",
        "AST_MESSAGE",
        "AST_OPERATION",
        "AST_SIZEOF",
        "AST_STRING_CAST",
        "AST_TEXT",
        "AST_UNDEFINE"
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

Body::Body (string const &body_text, FiLoc const &source_filoc)
    :
    ExecutableAstList(AST_BODY),
    Executable()
{
    Append(new Text(body_text, source_filoc));
}

Body::Body (Sint32 body_integer, FiLoc const &source_filoc)
    :
    ExecutableAstList(AST_BODY),
    Executable()
{
    Append(new Integer(body_integer, source_filoc));
}

bool Body::GetIsNativeIntegerValue (SymbolTable &symbol_table) const
{
    return size() == 1 &&
           dynamic_cast<Expression const *>(GetElement(0)) != NULL &&
           static_cast<Expression const *>(GetElement(0))->GetIsNativeIntegerValue(symbol_table);
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Conditional::~Conditional ()
{
    delete m_if_expression;
    delete m_if_body;
    delete m_else_body;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

DeclareArray::~DeclareArray ()
{
    delete m_id;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

DeclareMap::~DeclareMap ()
{
    delete m_id;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Define::~Define ()
{
    delete m_id;
    delete m_body;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

DefineMapElement::~DefineMapElement ()
{
    delete m_key;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Undefine::~Undefine ()
{
    delete m_id;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Loop::~Loop ()
{
    delete m_iterator_id;
    delete m_iteration_count_expression;
    delete m_body;
    delete m_iterator_integer_body;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

ForEach::~ForEach ()
{
    delete m_key_id;
    delete m_map_id;
    delete m_body;
    delete m_key_text_body;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Include::~Include ()
{
    delete m_include_filename_expression;
    delete m_include_body_root;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////


ostream &operator << (ostream &stream, Message::Criticality criticality)
{
    static string const s_criticality_string[Message::CRITICALITY_COUNT] =
    {
        "WARNING",
        "ERROR",
        "FATAL_ERROR"
    };
    assert(criticality >= 0 && criticality < Message::CRITICALITY_COUNT);
    return stream << s_criticality_string[criticality];
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Sint32 Text::GetIntegerValue (SymbolTable &symbol_table) const
{
    istringstream in(m_text);
    Sint32 retval = 0;
    in >> retval;
    return retval;
}

string Text::GetTextValue (SymbolTable &symbol_table) const
{
    return m_text;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Sint32 Integer::GetIntegerValue (SymbolTable &symbol_table) const
{
    return m_value;
}

string Integer::GetTextValue (SymbolTable &symbol_table) const
{
    ostringstream out;
    out << GetIntegerValue(symbol_table);
    return out.str();
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Sizeof::~Sizeof ()
{
    delete m_id;
}

Sint32 Sizeof::GetIntegerValue (SymbolTable &symbol_table) const
{
    Symbol *symbol = symbol_table.GetSymbol(m_id->GetText());
    if (symbol != NULL)
        return symbol->Sizeof();
    else
    {
        EmitError("can't return sizeof undefined macro \"" + m_id->GetText() + "\"", GetFiLoc());
        return 0;
    }
}

string Sizeof::GetTextValue (SymbolTable &symbol_table) const
{
    ostringstream out;
    out << GetIntegerValue(symbol_table);
    return out.str();
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Dereference::~Dereference ()
{
    delete m_id;
    delete m_element_index_expression;
}

bool Dereference::GetIsNativeIntegerValue (SymbolTable &symbol_table) const
{
    Body const *dereferenced_body = GetDereferencedBody(symbol_table);
    return (dereferenced_body != NULL) ?
           dereferenced_body->GetIsNativeIntegerValue(symbol_table) :
           false;
}

Sint32 Dereference::GetIntegerValue (SymbolTable &symbol_table) const
{
    Body const *dereferenced_body = GetDereferencedBody(symbol_table);
    if (dereferenced_body == NULL)
        return 0;

    ostringstream out;
    Textifier textifier(out);
    textifier.SetGeneratesLineDirectives(GetOptions().GetWithLineDirectives());
    dereferenced_body->Execute(textifier, symbol_table);
    istringstream in(out.str());
    Sint32 retval = 0;
    in >> retval;
    return retval;
}

string Dereference::GetTextValue (SymbolTable &symbol_table) const
{
    Body const *dereferenced_body = GetDereferencedBody(symbol_table);
    if (dereferenced_body == NULL)
        return g_empty_string;

    ostringstream out;
    Textifier textifier(out);
    textifier.SetGeneratesLineDirectives(GetOptions().GetWithLineDirectives());
    dereferenced_body->Execute(textifier, symbol_table);
    return out.str();
}

Body const *Dereference::GetDereferencedBody (SymbolTable &symbol_table) const
{
    Symbol *symbol = symbol_table.GetSymbol(m_id->GetText());
    if (symbol == NULL)
    {
        if (m_dereference_type == DEREFERENCE_ALWAYS)
            EmitError("undefined macro \"" + m_id->GetText() + "\"", m_id->GetFiLoc());
        return NULL;
    }

    Body const *dereferenced_body = NULL;
    if (symbol->GetIsScalarSymbol())
    {
        if (m_element_index_expression != NULL)
        {
            EmitError("trying to dereference a scalar macro as an array or map", GetFiLoc());
            return NULL;
        }
        dereferenced_body = Dsc<ScalarSymbol *>(symbol)->GetScalarBody();
        assert(dereferenced_body != NULL);
    }
    else if (symbol->GetIsArraySymbol())
    {
        if (m_element_index_expression == NULL)
        {
            EmitError("trying to dereference an array macro without an array index", GetFiLoc());
            return NULL;
        }
        Sint32 element_index = m_element_index_expression->GetIntegerValue(symbol_table);
        if (element_index < 0)
        {
            ostringstream out;
            out << "negative value (" << element_index << ") invalid for array index";
            EmitError(out.str(), GetFiLoc());
            return 0;
        }
        dereferenced_body = Dsc<ArraySymbol *>(symbol)->GetArrayElement(Uint32(element_index));
        if (dereferenced_body == NULL)
        {
            ostringstream out;
            out << "macro \"" << m_id->GetText() << "\" has no element " << element_index;
            EmitError(out.str(), GetFiLoc());
            return NULL;
        }
    }
    else
    {
        assert(symbol->GetIsMapSymbol());
        if (m_element_index_expression == NULL)
        {
            EmitError("trying to dereference a map macro without a map key", GetFiLoc());
            return NULL;
        }
        string element_key = m_element_index_expression->GetTextValue(symbol_table);
        dereferenced_body = Dsc<MapSymbol *>(symbol)->GetMapElement(element_key);
        if (dereferenced_body == NULL)
        {
            EmitError(
                "macro \"" + m_id->GetText() + "\" has no such element " + GetStringLiteral(element_key),
                GetFiLoc());
            return NULL;
        }
    }

    return dereferenced_body;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Sint32 IsDefined::GetIntegerValue (SymbolTable &symbol_table) const
{
    if (m_element_index_expression == NULL)
        return (symbol_table.GetSymbol(m_id->GetText()) != NULL) ? 1 : 0;
    else
        return (GetDereferencedBody(symbol_table) != NULL) ? 1 : 0;
}

string IsDefined::GetTextValue (SymbolTable &symbol_table) const
{
    ostringstream out;
    out << GetIntegerValue(symbol_table);
    return out.str();
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Operation::~Operation ()
{
    delete m_left;
    delete m_right;
}

bool Operation::GetIsNativeIntegerValue (SymbolTable &symbol_table) const
{
    switch (m_op)
    {
        case CONCATENATE:
        case STRING_CAST:
        case TO_CHARACTER_LITERAL:
        case TO_STRING_LITERAL:
            return false;

        case DIVIDE:
        case EQUAL:
        case GREATER_THAN:
        case GREATER_THAN_OR_EQUAL:
        case INT_CAST:
        case LESS_THAN:
        case LESS_THAN_OR_EQUAL:
        case LOGICAL_AND:
        case LOGICAL_NOT:
        case LOGICAL_OR:
        case MINUS:
        case MULTIPLY:
        case NEGATIVE:
        case NOT_EQUAL:
        case PLUS:
        case REMAINDER:
        case STRING_LENGTH:
            return true;

        default:
            assert(false && "invalid operator");
            return true;
    }
}

Sint32 Operation::GetIntegerValue (SymbolTable &symbol_table) const
{
    if (GetIsTextOperation())
    {
        EmitWarning("retrieving integer value from non-integer expression", GetFiLoc());
        istringstream in(GetTextValue(symbol_table));
        Sint32 retval = 0;
        in >> retval;
        return retval;
    }

    switch (m_op)
    {
        case PLUS:
            return m_left->GetIntegerValue(symbol_table) + m_right->GetIntegerValue(symbol_table);

        case MINUS:
            return m_left->GetIntegerValue(symbol_table) - m_right->GetIntegerValue(symbol_table);

        case NEGATIVE:
            return -m_right->GetIntegerValue(symbol_table);

        case MULTIPLY:
            return m_left->GetIntegerValue(symbol_table) * m_right->GetIntegerValue(symbol_table);

        case DIVIDE:
        {
            Sint32 right_operand = m_right->GetIntegerValue(symbol_table);
            if (right_operand == 0)
            {
                EmitWarning("divide by zero", GetFiLoc());
                return 0;
            }
            else
                return m_left->GetIntegerValue(symbol_table) / right_operand;
        }

        case REMAINDER:
        {
            Sint32 right_operand = m_right->GetIntegerValue(symbol_table);
            if (right_operand == 0)
            {
                EmitWarning("divide by zero", GetFiLoc());
                return 0;
            }
            else
                return m_left->GetIntegerValue(symbol_table) % right_operand;
        }

        case LOGICAL_NOT:
            return (m_right->GetIntegerValue(symbol_table) == 0) ? 1 : 0;

        case LOGICAL_AND:
            return (m_left->GetIntegerValue(symbol_table) != 0 && m_right->GetIntegerValue(symbol_table) != 0) ? 1 : 0;

        case LOGICAL_OR:
            return (m_left->GetIntegerValue(symbol_table) != 0 || m_right->GetIntegerValue(symbol_table) != 0) ? 1 : 0;

        case EQUAL:
            if (m_left->GetIsNativeIntegerValue(symbol_table) != m_right->GetIsNativeIntegerValue(symbol_table))
            {
                EmitError("int/string type mismatch for == operator", m_left->GetFiLoc());
                return 0;
            }
            if (m_left->GetIsNativeIntegerValue(symbol_table))
                return (m_left->GetIntegerValue(symbol_table) == m_right->GetIntegerValue(symbol_table)) ? 1 : 0;
            else
                return (m_left->GetTextValue(symbol_table) == m_right->GetTextValue(symbol_table)) ? 1 : 0;

        case NOT_EQUAL:
            if (m_left->GetIsNativeIntegerValue(symbol_table) != m_right->GetIsNativeIntegerValue(symbol_table))
            {
                EmitError("int/string type mismatch for != operator", m_left->GetFiLoc());
                return 0;
            }
            if (m_left->GetIsNativeIntegerValue(symbol_table))
                return (m_left->GetIntegerValue(symbol_table) != m_right->GetIntegerValue(symbol_table)) ? 1 : 0;
            else
                return (m_left->GetTextValue(symbol_table) != m_right->GetTextValue(symbol_table)) ? 1 : 0;

        case LESS_THAN:
            return (m_left->GetIntegerValue(symbol_table) < m_right->GetIntegerValue(symbol_table)) ? 1 : 0;

        case LESS_THAN_OR_EQUAL:
            return (m_left->GetIntegerValue(symbol_table) <= m_right->GetIntegerValue(symbol_table)) ? 1 : 0;

        case GREATER_THAN:
            return (m_left->GetIntegerValue(symbol_table) > m_right->GetIntegerValue(symbol_table)) ? 1 : 0;

        case GREATER_THAN_OR_EQUAL:
            return (m_left->GetIntegerValue(symbol_table) >= m_right->GetIntegerValue(symbol_table)) ? 1 : 0;

        case INT_CAST:
            return m_right->GetIntegerValue(symbol_table);

        case STRING_LENGTH:
            if (m_right->GetIsNativeIntegerValue(symbol_table))
            {
                EmitError("type mismatch for string_length operator (expecting a string)", m_left->GetFiLoc());
                return 0;
            }
            else
                return m_right->GetTextValue(symbol_table).length();

        default:
            assert(false && "invalid operator");
            return 0;
    }
}

string Operation::GetTextValue (SymbolTable &symbol_table) const
{
    if (!GetIsTextOperation())
    {
        EmitWarning("retrieving text value from non-text expression", GetFiLoc());
        ostringstream out;
        out << GetIntegerValue(symbol_table);
        return out.str();
    }

    switch (m_op)
    {
        case CONCATENATE:
            return m_left->GetTextValue(symbol_table) + m_right->GetTextValue(symbol_table);

        case STRING_CAST:
            return m_right->GetTextValue(symbol_table);

        case TO_CHARACTER_LITERAL:
        {
            Sint32 character_index = m_right->GetIntegerValue(symbol_table);
            if (character_index < 0 || character_index > 255)
                EmitWarning(FORMAT("truncating character literal index (" << character_index << ") to within 0-255"), GetFiLoc());
            return GetCharLiteral(Uint8(character_index));
        }

        case TO_STRING_LITERAL:
            return GetStringLiteral(m_right->GetTextValue(symbol_table));

        default:
            assert(false && "invalid operator");
            return 0;
    }
}

void Operation::ValidateOperands ()
{
    switch (m_op)
    {
        // binary ops
        case CONCATENATE:
        case DIVIDE:
        case EQUAL:
        case GREATER_THAN:
        case GREATER_THAN_OR_EQUAL:
        case LESS_THAN:
        case LESS_THAN_OR_EQUAL:
        case LOGICAL_AND:
        case LOGICAL_OR:
        case MINUS:
        case MULTIPLY:
        case NOT_EQUAL:
        case PLUS:
        case REMAINDER:
            assert(m_left != NULL);
            assert(m_right != NULL);
            break;

        // unary ops
        case LOGICAL_NOT:
        case NEGATIVE:
        case INT_CAST:
        case STRING_CAST:
        case STRING_LENGTH:
        case TO_CHARACTER_LITERAL:
        case TO_STRING_LITERAL:
            assert(m_left == NULL);
            assert(m_right != NULL);
            break;

        default:
            assert(false && "invalid operator");
            break;
    }
}

ostream &operator << (ostream &stream, Operation::Operator op)
{
    static string const s_operator_string[Operation::OPERATOR_COUNT] =
    {
        "CONCATENATE",
        "DIVIDE",
        "EQUAL",
        "GREATER_THAN",
        "GREATER_THAN_OR_EQUAL",
        "INT_CAST",
        "LESS_THAN",
        "LESS_THAN_OR_EQUAL",
        "LOGICAL_AND",
        "LOGICAL_NOT",
        "LOGICAL_OR",
        "MINUS",
        "MULTIPLY",
        "NEGATIVE",
        "NOT_EQUAL",
        "PLUS",
        "REMAINDER",
        "STRING_CAST",
        "STRING_LENGTH",
        "TO_CHARACTER_LITERAL",
        "TO_STRING_LITERAL"
    };
    assert(op >= 0 && op < Operation::OPERATOR_COUNT);
    return stream << s_operator_string[op];
}

} // end of namespace Preprocessor
} // end of namespace Barf
