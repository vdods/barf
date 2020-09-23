// 2006.10.15 - Copyright Victor Dods - Licensed under Apache 2.0

#include "barf_preprocessor_ast.hpp"

#include <sstream>

#include "barf_optionsbase.hpp"
#include "barf_preprocessor_textifier.hpp"

namespace Barf {
namespace Preprocessor {

string const &DereferenceTypeString (DereferenceType dereference_type)
{
    static string const s_dereference_type_string[2] =
    {
        "DEREFERENCE_ALWAYS",
        "DEREFERENCE_IFF_DEFINED"
    };

    assert(Uint32(dereference_type) < 2);
    return s_dereference_type_string[dereference_type];
}

string const &AstTypeString (AstType ast_type)
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
        return Ast::AstTypeString(ast_type);
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

bool Body::IsNativeIntegerValue (SymbolTable &symbol_table) const
{
    return size() == 1 &&
           dynamic_cast<Expression const *>(Element(0)) != NULL &&
           static_cast<Expression const *>(Element(0))->IsNativeIntegerValue(symbol_table);
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

Sint32 Text::IntegerValue (SymbolTable &symbol_table) const
{
    istringstream in(m_text);
    Sint32 retval = 0;
    in >> retval;
    return retval;
}

string Text::TextValue (SymbolTable &symbol_table) const
{
    return m_text;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Sint32 Integer::IntegerValue (SymbolTable &symbol_table) const
{
    return m_value;
}

string Integer::TextValue (SymbolTable &symbol_table) const
{
    ostringstream out;
    out << IntegerValue(symbol_table);
    return out.str();
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Sizeof::~Sizeof ()
{
    delete m_id;
}

Sint32 Sizeof::IntegerValue (SymbolTable &symbol_table) const
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

string Sizeof::TextValue (SymbolTable &symbol_table) const
{
    ostringstream out;
    out << IntegerValue(symbol_table);
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

bool Dereference::IsNativeIntegerValue (SymbolTable &symbol_table) const
{
    Body const *dereferenced_body = DereferencedBody(symbol_table);
    return (dereferenced_body != NULL) ?
           dereferenced_body->IsNativeIntegerValue(symbol_table) :
           false;
}

Sint32 Dereference::IntegerValue (SymbolTable &symbol_table) const
{
    Body const *dereferenced_body = DereferencedBody(symbol_table);
    if (dereferenced_body == NULL)
        return 0;

    ostringstream out;
    Textifier textifier(out);
    textifier.GeneratesLineDirectives(GetOptions().WithLineDirectives());
    dereferenced_body->Execute(textifier, symbol_table);
    istringstream in(out.str());
    Sint32 retval = 0;
    in >> retval;
    return retval;
}

string Dereference::TextValue (SymbolTable &symbol_table) const
{
    Body const *dereferenced_body = DereferencedBody(symbol_table);
    if (dereferenced_body == NULL)
        return g_empty_string;

    ostringstream out;
    Textifier textifier(out);
    textifier.GeneratesLineDirectives(GetOptions().WithLineDirectives());
    dereferenced_body->Execute(textifier, symbol_table);
    return out.str();
}

Body const *Dereference::DereferencedBody (SymbolTable &symbol_table) const
{
    Symbol *symbol = symbol_table.GetSymbol(m_id->GetText());
    if (symbol == NULL)
    {
        if (m_dereference_type == DEREFERENCE_ALWAYS)
            EmitError("undefined macro \"" + m_id->GetText() + "\"", m_id->GetFiLoc());
        return NULL;
    }

    Body const *dereferenced_body = NULL;
    if (symbol->IsScalarSymbol())
    {
        if (m_element_index_expression != NULL)
        {
            EmitError("trying to dereference a scalar macro \"" + symbol->Id() + "\" as an array or map", GetFiLoc());
            return NULL;
        }
        dereferenced_body = Dsc<ScalarSymbol *>(symbol)->ScalarBody();
        assert(dereferenced_body != NULL);
    }
    else if (symbol->IsArraySymbol())
    {
        if (m_element_index_expression == NULL)
        {
            EmitError("trying to dereference an array macro \"" + symbol->Id() + "\" without an array index", GetFiLoc());
            return NULL;
        }
        Sint32 element_index = m_element_index_expression->IntegerValue(symbol_table);
        if (element_index < 0)
        {
            ostringstream out;
            out << "negative value (" << element_index << ") invalid for array index for array macro \"" << symbol->Id() << '"';
            EmitError(out.str(), GetFiLoc());
            return 0;
        }
        dereferenced_body = Dsc<ArraySymbol *>(symbol)->ArrayElement(Uint32(element_index));
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
        assert(symbol->IsMapSymbol());
        if (m_element_index_expression == NULL)
        {
            EmitError("trying to dereference a map macro \"" + symbol->Id() + "\" without a map key", GetFiLoc());
            return NULL;
        }
        string element_key = m_element_index_expression->TextValue(symbol_table);
        dereferenced_body = Dsc<MapSymbol *>(symbol)->MapElement(element_key);
        if (dereferenced_body == NULL)
        {
            EmitError("macro \"" + m_id->GetText() + "\" has no such element " + StringLiteral(element_key), GetFiLoc());
            return NULL;
        }
    }

    return dereferenced_body;
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Sint32 IsDefined::IntegerValue (SymbolTable &symbol_table) const
{
    if (m_element_index_expression == NULL)
        return (symbol_table.GetSymbol(m_id->GetText()) != NULL) ? 1 : 0;
    else
        return (DereferencedBody(symbol_table) != NULL) ? 1 : 0;
}

string IsDefined::TextValue (SymbolTable &symbol_table) const
{
    ostringstream out;
    out << IntegerValue(symbol_table);
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

bool Operation::IsNativeIntegerValue (SymbolTable &symbol_table) const
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

Sint32 Operation::IntegerValue (SymbolTable &symbol_table) const
{
    if (IsTextOperation())
    {
        EmitWarning("retrieving integer value from non-integer expression", GetFiLoc());
        istringstream in(TextValue(symbol_table));
        Sint32 retval = 0;
        in >> retval;
        return retval;
    }

    switch (m_op)
    {
        case PLUS:
            return m_left->IntegerValue(symbol_table) + m_right->IntegerValue(symbol_table);

        case MINUS:
            return m_left->IntegerValue(symbol_table) - m_right->IntegerValue(symbol_table);

        case NEGATIVE:
            return -m_right->IntegerValue(symbol_table);

        case MULTIPLY:
            return m_left->IntegerValue(symbol_table) * m_right->IntegerValue(symbol_table);

        case DIVIDE:
        {
            Sint32 right_operand = m_right->IntegerValue(symbol_table);
            if (right_operand == 0)
            {
                EmitWarning("divide by zero", GetFiLoc());
                return 0;
            }
            else
                return m_left->IntegerValue(symbol_table) / right_operand;
        }

        case REMAINDER:
        {
            Sint32 right_operand = m_right->IntegerValue(symbol_table);
            if (right_operand == 0)
            {
                EmitWarning("divide by zero", GetFiLoc());
                return 0;
            }
            else
                return m_left->IntegerValue(symbol_table) % right_operand;
        }

        case LOGICAL_NOT:
            return (m_right->IntegerValue(symbol_table) == 0) ? 1 : 0;

        case LOGICAL_AND:
            return (m_left->IntegerValue(symbol_table) != 0 && m_right->IntegerValue(symbol_table) != 0) ? 1 : 0;

        case LOGICAL_OR:
            return (m_left->IntegerValue(symbol_table) != 0 || m_right->IntegerValue(symbol_table) != 0) ? 1 : 0;

        case EQUAL:
            if (m_left->IsNativeIntegerValue(symbol_table) != m_right->IsNativeIntegerValue(symbol_table))
            {
                EmitError("int/string type mismatch for == operator", m_left->GetFiLoc());
                return 0;
            }
            if (m_left->IsNativeIntegerValue(symbol_table))
                return (m_left->IntegerValue(symbol_table) == m_right->IntegerValue(symbol_table)) ? 1 : 0;
            else
                return (m_left->TextValue(symbol_table) == m_right->TextValue(symbol_table)) ? 1 : 0;

        case NOT_EQUAL:
            if (m_left->IsNativeIntegerValue(symbol_table) != m_right->IsNativeIntegerValue(symbol_table))
            {
                EmitError("int/string type mismatch for != operator", m_left->GetFiLoc());
                return 0;
            }
            if (m_left->IsNativeIntegerValue(symbol_table))
                return (m_left->IntegerValue(symbol_table) != m_right->IntegerValue(symbol_table)) ? 1 : 0;
            else
                return (m_left->TextValue(symbol_table) != m_right->TextValue(symbol_table)) ? 1 : 0;

        case LESS_THAN:
            return (m_left->IntegerValue(symbol_table) < m_right->IntegerValue(symbol_table)) ? 1 : 0;

        case LESS_THAN_OR_EQUAL:
            return (m_left->IntegerValue(symbol_table) <= m_right->IntegerValue(symbol_table)) ? 1 : 0;

        case GREATER_THAN:
            return (m_left->IntegerValue(symbol_table) > m_right->IntegerValue(symbol_table)) ? 1 : 0;

        case GREATER_THAN_OR_EQUAL:
            return (m_left->IntegerValue(symbol_table) >= m_right->IntegerValue(symbol_table)) ? 1 : 0;

        case INT_CAST:
            return m_right->IntegerValue(symbol_table);

        case STRING_LENGTH:
            if (m_right->IsNativeIntegerValue(symbol_table))
            {
                EmitError("type mismatch for string_length operator (expecting a string)", m_left->GetFiLoc());
                return 0;
            }
            else
                return m_right->TextValue(symbol_table).length();

        default:
            assert(false && "invalid operator");
            return 0;
    }
}

string Operation::TextValue (SymbolTable &symbol_table) const
{
    if (!IsTextOperation())
    {
        EmitWarning("retrieving text value from non-text expression", GetFiLoc());
        ostringstream out;
        out << IntegerValue(symbol_table);
        return out.str();
    }

    switch (m_op)
    {
        case CONCATENATE:
            return m_left->TextValue(symbol_table) + m_right->TextValue(symbol_table);

        case STRING_CAST:
            return m_right->TextValue(symbol_table);

        case TO_CHARACTER_LITERAL:
        {
            Sint32 character_index = m_right->IntegerValue(symbol_table);
            if (character_index < 0 || character_index > 255)
                EmitWarning(FORMAT("truncating character literal index (" << character_index << ") to within 0-255"), GetFiLoc());
            return CharLiteral(Uint8(character_index));
        }

        case TO_STRING_LITERAL:
            return StringLiteral(m_right->TextValue(symbol_table));

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
