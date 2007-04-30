// ///////////////////////////////////////////////////////////////////////////
// barf_preprocessor_ast.hpp by Victor Dods, created 2006/10/15
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_PREPROCESSOR_AST_HPP_)
#define _BARF_PREPROCESSOR_AST_HPP_

#include "barf_preprocessor.hpp"

#include <vector>

#include "barf_astcommon.hpp"
#include "barf_preprocessor_symboltable.hpp"

namespace Barf {
namespace Preprocessor {

class Textifier;

/*

// ///////////////////////////////////////////////////////////////////////////
// class hierarchy
// ///////////////////////////////////////////////////////////////////////////

ExecutableAstList

Executable (abstract)

ExecutableAstList, Executable
    Body

AstCommon::Ast, Executable
    ExecutableAst (abstract)
        Directive (abstract)
            DeclareArray
            DeclareMap
            Define
                DefineArrayElement
                DefineMapElement
            Undefine
            Loop
            Include
        Expression (abstract)
            Text
            Integer
            Sizeof
            Dereference

// ///////////////////////////////////////////////////////////////////////////
// class composition
// ///////////////////////////////////////////////////////////////////////////

Body
    Executable[]

Define
    AstCommon::Id (macro id)
    Body (macro body)

Undefine
    AstCommon::Id (macro id)

Loop
    AstCommon::Id (iterator id)
    Expression (iteration count expression)
    Body (loop body)

Include
    Expression (include filename expression)

Sizeof
    AstCommon::Id (operand id)

Dereference
    AstCommon::Id (operand id)
    Expression (element index expression)

*/

class IsDefined;
class Dereference;
class Operation;
class Sizeof;
class IntegerCast;
class StringCast;
class ExecutableAstList;
class Body;
class BodyList;
class Conditional;
class Define;
class Directive;
class DumpSymbolTable;
class Executable;
class ExecutableAst;
class Expression;
class Integer;
class Loop;
class Text;
class Undefine;

enum DereferenceType
{
    DEREFERENCE_IFF_DEFINED,
    DEREFERENCE_ALWAYS
}; // end of enum DereferenceType

string const &GetDereferenceTypeString (DereferenceType dereference_type);

enum
{
    AT_BODY = AstCommon::AT_START_CUSTOM_TYPES_HERE_,
    AT_BODY_LIST,
    AT_CONDITIONAL,
    AT_DUMP_SYMBOL_TABLE,
    AT_DECLARE_ARRAY,
    AT_DECLARE_MAP,
    AT_DEFINE,
    AT_DEFINE_ARRAY_ELEMENT,
    AT_DEFINE_MAP_ELEMENT,
    AT_UNDEFINE,
    AT_LOOP,
    AT_FOR_EACH,
    AT_INCLUDE,
    AT_MESSAGE,
    AT_TEXT,
    AT_INTEGER,
    AT_SIZEOF,
    AT_INTEGER_CAST,
    AT_STRING_CAST,
    AT_IS_DEFINED,
    AT_DEREFERENCE,
    AT_OPERATION,

    AT_COUNT
};

string const &GetAstTypeString (AstType ast_type);

class Executable
{
public:

    virtual ~Executable () { }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const = 0;
}; // end of class Executable

class ExecutableAst : public AstCommon::Ast, public Executable
{
public:

    ExecutableAst (FiLoc const &filoc, AstType ast_type)
        :
        AstCommon::Ast(filoc, ast_type),
        Executable()
    { }
}; // end of class ExecutableAst

class ExecutableAstList : public AstCommon::AstList<ExecutableAst>
{
public:

    ExecutableAstList (AstType ast_type) : AstCommon::AstList<ExecutableAst>(ast_type) { }
}; // end of class ExecutableAstList

class Body : public ExecutableAstList, public Executable
{
public:

    Body () : ExecutableAstList(AT_BODY), Executable() { }
    Body (string const &body_text, FiLoc const &source_filoc);
    Body (Sint32 body_integer, FiLoc const &source_filoc);

    bool GetIsNativeIntegerValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;

    // this is the non-virtual, top-level Print method, not
    // to be confused with AstCommon::Ast::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    using ExecutableAstList::Print;
}; // end of class Body

class BodyList : public AstCommon::AstList<Body>
{
public:

    BodyList () : AstCommon::AstList<Body>(AT_BODY_LIST) { }
}; // end of class BodyList

class Directive : public ExecutableAst
{
public:

    Directive (FiLoc const &filoc, AstType ast_type)
        :
        ExecutableAst(filoc, ast_type)
    { }

    // is there any reason for this class to exist?
    // ... is there any reason for ANY class to exist?  i mean, REALLY?

}; // end of class Directive

class Expression : public ExecutableAst
{
public:

    Expression (FiLoc const &filoc, AstType ast_type)
        :
        ExecutableAst(filoc, ast_type)
    { }

    virtual bool GetIsNativeIntegerValue (SymbolTable &symbol_table) const = 0;

    virtual Sint32 GetIntegerValue (SymbolTable &symbol_table) const = 0;
    virtual string GetTextValue (SymbolTable &symbol_table) const = 0;
}; // end of class Expression

class Conditional : public ExecutableAst
{
public:

    Conditional (Expression const *if_expression)
        :
        ExecutableAst(if_expression->GetFiLoc(), AT_CONDITIONAL),
        m_if_expression(if_expression),
        m_if_body(NULL),
        m_else_body(NULL)
    {
        assert(m_if_expression != NULL);
    }
    virtual ~Conditional ();

    void SetIfBody (Body *if_body)
    {
        assert(m_if_body == NULL);
        assert(if_body != NULL);
        m_if_body = if_body;
    }
    void SetElseBody (Body *else_body)
    {
        assert(m_else_body == NULL);
        m_else_body = else_body;
    }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Expression const *const m_if_expression;
    Body *m_if_body;
    Body *m_else_body;
}; // end of class Conditional

class DumpSymbolTable : public ExecutableAst
{
public:

    DumpSymbolTable () : ExecutableAst(FiLoc::ms_invalid, AT_DUMP_SYMBOL_TABLE) { }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
}; // end of class DumpSymbolTable

class DeclareArray : public Directive
{
public:

    DeclareArray (AstCommon::Id const *id)
        :
        Directive(id->GetFiLoc(), AT_DECLARE_ARRAY),
        m_id(id)
    {
        assert(m_id != NULL);
    }
    virtual ~DeclareArray ();

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    AstCommon::Id const *const m_id;
}; // end of class DeclareArray

class DeclareMap : public Directive
{
public:

    DeclareMap (AstCommon::Id const *id)
        :
        Directive(id->GetFiLoc(), AT_DECLARE_ARRAY),
        m_id(id)
    {
        assert(m_id != NULL);
    }
    virtual ~DeclareMap ();

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    AstCommon::Id const *const m_id;
}; // end of class DeclareMap

class Define : public Directive
{
public:

    Define (AstCommon::Id *id)
        :
        Directive(id->GetFiLoc(), AT_DEFINE),
        m_id(id),
        m_body(NULL)
    {
        assert(m_id != NULL);
    }
    virtual ~Define ();

    void SetBody (Body *body)
    {
        assert(m_body == NULL);
        assert(body != NULL);
        m_body = body;
    }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    Define (AstCommon::Id *id, AstType ast_type)
        :
        Directive(id->GetFiLoc(), ast_type),
        m_id(id),
        m_body(NULL)
    {
        assert(m_id != NULL);
    }

    AstCommon::Id *const m_id;
    Body *m_body;
}; // end of class Define

class DefineArrayElement : public Define
{
public:

    DefineArrayElement (AstCommon::Id *id)
        :
        Define(id, AT_DEFINE_ARRAY_ELEMENT)
    { }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
}; // end of class DefineArrayElement

class DefineMapElement : public Define
{
public:

    DefineMapElement (AstCommon::Id *id, Text const *key)
        :
        Define(id, AT_DEFINE_ARRAY_ELEMENT),
        m_key(key)
    {
        assert(m_key != NULL);
    }
    virtual ~DefineMapElement ();

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Text const *const m_key;
}; // end of class DefineMapElement

class Undefine : public Directive
{
public:

    Undefine (AstCommon::Id *id)
        :
        Directive(id->GetFiLoc(), AT_UNDEFINE),
        m_id(id)
    {
        assert(m_id != NULL);
    }
    virtual ~Undefine ();

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    AstCommon::Id *const m_id;
}; // end of class Undefine

class Loop : public Directive
{
public:

    Loop (
        AstCommon::Id *iterator_id,
        Expression *iteration_count_expression)
        :
        Directive(iterator_id->GetFiLoc(), AT_LOOP),
        m_iterator_id(iterator_id),
        m_iteration_count_expression(iteration_count_expression),
        m_body(NULL),
        m_iterator_integer_body(NULL),
        m_iterator_integer(NULL)
    {
        assert(m_iterator_id != NULL);
        assert(m_iteration_count_expression != NULL);
    }
    virtual ~Loop ();

    void SetBody (Body *body)
    {
        assert(m_body == NULL);
        assert(body != NULL);
        m_body = body;
    }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    AstCommon::Id *const m_iterator_id;
    Expression *const m_iteration_count_expression;
    Body *m_body;

    // helper used in Execute() -- delete
    mutable Body *m_iterator_integer_body;
    // do not delete this -- it will be deleted by m_iterator_integer_body
    mutable Integer *m_iterator_integer;
}; // end of class Loop

class ForEach : public Directive
{
public:

    ForEach (
        AstCommon::Id *key_id,
        AstCommon::Id *map_id)
        :
        Directive(key_id->GetFiLoc(), AT_FOR_EACH),
        m_key_id(key_id),
        m_map_id(map_id),
        m_body(NULL),
        m_key_text_body(NULL),
        m_key_text(NULL)
    {
        assert(m_key_id != NULL);
        assert(m_map_id != NULL);
    }
    virtual ~ForEach ();

    void SetBody (Body *body)
    {
        assert(m_body == NULL);
        assert(body != NULL);
        m_body = body;
    }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    AstCommon::Id *const m_key_id;
    AstCommon::Id *const m_map_id;
    Body *m_body;

    // helper used in Execute() -- delete
    mutable Body *m_key_text_body;
    // do not delete this -- it will be deleted by m_key_text_body
    mutable Text *m_key_text;
}; // end of class ForEach

class Include : public Directive
{
public:

    Include (Expression *include_filename_expression, bool is_sandboxed)
        :
        Directive(include_filename_expression->GetFiLoc(), AT_INCLUDE),
        m_is_sandboxed(is_sandboxed),
        m_include_filename_expression(include_filename_expression),
        m_include_body_root(NULL)
    {
        assert(m_include_filename_expression != NULL);
    }
    virtual ~Include ();

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    bool const m_is_sandboxed;
    Expression *const m_include_filename_expression;
    mutable Body *m_include_body_root;
}; // end of class Include

class Message : public Directive
{
public:

    enum Criticality
    {
        WARNING = 0,
        ERROR,
        FATAL_ERROR,

        CRITICALITY_COUNT
    }; // end of enum Message::Criticality

    Message (Expression *message_expression, Criticality criticality)
        :
        Directive(message_expression->GetFiLoc(), AT_MESSAGE),
        m_message_expression(message_expression),
        m_criticality(criticality)
    {
        assert(m_message_expression != NULL);
        assert(m_criticality >= 0 && m_criticality < CRITICALITY_COUNT);
    }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Expression *const m_message_expression;
    Criticality const m_criticality;
}; // end of class Message

ostream &operator << (ostream &stream, Message::Criticality criticality);

class Text : public Expression
{
public:

    // the text passed to this constructor should not contain any macro code
    Text (string const &text, FiLoc const &filoc)
        :
        Expression(filoc, AT_TEXT),
        m_text(text)
    { }

    inline string const &GetText () const { return m_text; }
    inline void SetText (string const &text) { m_text = text; }

    virtual bool GetIsNativeIntegerValue (SymbolTable &symbol_table) const { return false; }

    virtual Sint32 GetIntegerValue (SymbolTable &symbol_table) const;
    virtual string GetTextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    string m_text;
}; // end of class Text

class Integer : public Expression
{
public:

    Integer (Sint32 value, FiLoc const &filoc)
        :
        Expression(filoc, AT_INTEGER),
        m_value(value)
    { }

    inline Sint32 GetValue () const { return m_value; }
    inline void SetValue (Sint32 value) { m_value = value; }

    virtual bool GetIsNativeIntegerValue (SymbolTable &symbol_table) const { return true; }

    virtual Sint32 GetIntegerValue (SymbolTable &symbol_table) const;
    virtual string GetTextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Sint32 m_value;
}; // end of class Integer

class Sizeof : public Expression
{
public:

    Sizeof (AstCommon::Id *id)
        :
        Expression(id->GetFiLoc(), AT_SIZEOF),
        m_id(id)
    {
        assert(m_id != NULL);
    }
    virtual ~Sizeof ();

    virtual bool GetIsNativeIntegerValue (SymbolTable &symbol_table) const { return true; }

    virtual Sint32 GetIntegerValue (SymbolTable &symbol_table) const;
    virtual string GetTextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    AstCommon::Id *const m_id;
}; // end of class Sizeof

class Dereference : public Expression
{
public:

    Dereference (AstCommon::Id *id, Expression *element_index_expression, DereferenceType dereference_type)
        :
        Expression(id->GetFiLoc(), AT_DEREFERENCE),
        m_id(id),
        m_element_index_expression(element_index_expression),
        m_dereference_type(dereference_type)
    {
        assert(m_id != NULL);
        // m_element_index_expression can be NULL
        assert(m_dereference_type == DEREFERENCE_IFF_DEFINED || m_dereference_type == DEREFERENCE_ALWAYS);
    }
    virtual ~Dereference ();

    virtual bool GetIsNativeIntegerValue (SymbolTable &symbol_table) const;

    virtual Sint32 GetIntegerValue (SymbolTable &symbol_table) const;
    virtual string GetTextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    Dereference (AstCommon::Id *id, Expression *element_index_expression, DereferenceType dereference_type, AstType ast_type)
        :
        Expression(id->GetFiLoc(), ast_type),
        m_id(id),
        m_element_index_expression(element_index_expression),
        m_dereference_type(dereference_type)
    {
        assert(m_id != NULL);
        // m_element_index_expression can be NULL
    }

    Body const *GetDereferencedBody (SymbolTable &symbol_table) const;

    AstCommon::Id *const m_id;
    Expression *const m_element_index_expression;
    DereferenceType const m_dereference_type;
}; // end of class Dereference

class IsDefined : public Dereference
{
public:

    IsDefined (AstCommon::Id *id, Expression *element_index_expression)
        :
        Dereference(id, element_index_expression, DEREFERENCE_ALWAYS, AT_IS_DEFINED)
    { }

    virtual bool GetIsNativeIntegerValue (SymbolTable &symbol_table) const { return true; }

    virtual Sint32 GetIntegerValue (SymbolTable &symbol_table) const;
    virtual string GetTextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
}; // end of class IsDefined

class Operation : public Expression
{
public:

    enum Operator
    {
        CONCATENATE = 0,
        LOGICAL_OR,
        LOGICAL_AND,
        EQUAL,
        NOT_EQUAL,
        LESS_THAN,
        LESS_THAN_OR_EQUAL,
        GREATER_THAN,
        GREATER_THAN_OR_EQUAL,
        PLUS,
        MINUS,
        MULTIPLY,
        DIVIDE,
        REMAINDER,
        LOGICAL_NOT,
        INT_CAST,
        STRING_CAST,
        STRING_LENGTH,

        OPERATOR_COUNT
    }; // end of enum Operation::Operator

    Operation (
        Expression const *left_expression,
        Operator op,
        Expression const *right_expression)
        :
        Expression(left_expression->GetFiLoc(), AT_OPERATION),
        m_op(op),
        m_left(left_expression),
        m_right(right_expression)
    {
        ValidateOperands();
    }
    Operation (
        Operator op,
        Expression const *right_expression)
        :
        Expression(right_expression->GetFiLoc(), AT_OPERATION),
        m_op(op),
        m_left(NULL),
        m_right(right_expression)
    {
        ValidateOperands();
    }
    virtual ~Operation ();

    virtual bool GetIsNativeIntegerValue (SymbolTable &symbol_table) const;

    virtual Sint32 GetIntegerValue (SymbolTable &symbol_table) const;
    virtual string GetTextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    inline bool GetIsTextOperation () const { return m_op == CONCATENATE || m_op == STRING_CAST; }

    void ValidateOperands ();

    Operator const m_op;
    Expression const *const m_left;
    Expression const *const m_right;
}; // end of class Operation

ostream &operator << (ostream &stream, Operation::Operator op);

} // end of namespace Preprocessor
} // end of namespace Barf

#endif // !defined(_BARF_PREPROCESSOR_AST_HPP_)
