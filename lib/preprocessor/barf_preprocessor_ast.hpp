// 2006.10.15 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_PREPROCESSOR_AST_HPP_)
#define BARF_PREPROCESSOR_AST_HPP_

#include "barf_preprocessor.hpp"

#include <vector>

#include "barf_ast.hpp"
#include "barf_preprocessor_symboltable.hpp"

namespace Barf {
namespace Preprocessor {

class Body;
class BodyList;
class Conditional;
class Define;
class Dereference;
class Directive;
class DumpSymbolTable;
class Executable;
class ExecutableAst;
class ExecutableAstList;
class Expression;
class Integer;
class IntegerCast;
class IsDefined;
class Loop;
class Operation;
class Sizeof;
class StringCast;
class Text;
class Textifier;
class Undefine;

enum DereferenceType
{
    DEREFERENCE_ALWAYS = 0,
    DEREFERENCE_IFF_DEFINED
}; // end of enum DereferenceType

string const &DereferenceTypeString (DereferenceType dereference_type);

enum
{
    AST_BODY = Ast::AST_START_CUSTOM_TYPES_HERE_,
    AST_BODY_LIST,
    AST_CONDITIONAL,
    AST_DECLARE_ARRAY,
    AST_DECLARE_MAP,
    AST_DEFINE,
    AST_DEFINE_ARRAY_ELEMENT,
    AST_DEFINE_MAP_ELEMENT,
    AST_DEREFERENCE,
    AST_DUMP_SYMBOL_TABLE,
    AST_FOR_EACH,
    AST_INCLUDE,
    AST_INTEGER,
    AST_INTEGER_CAST,
    AST_IS_DEFINED,
    AST_LOOP,
    AST_MESSAGE,
    AST_OPERATION,
    AST_SIZEOF,
    AST_STRING_CAST,
    AST_TEXT,
    AST_UNDEFINE,

    AST_COUNT
};

string const &AstTypeString (AstType ast_type);

class Executable
{
public:

    virtual ~Executable () { }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const = 0;
}; // end of class Executable

class ExecutableAst : public Ast::Base, public Executable
{
public:

    ExecutableAst (FiLoc const &filoc, AstType ast_type)
        :
        Ast::Base(filoc, ast_type),
        Executable()
    { }
}; // end of class ExecutableAst

class ExecutableAstList : public Ast::AstList<ExecutableAst>
{
public:

    ExecutableAstList (AstType ast_type) : Ast::AstList<ExecutableAst>(ast_type) { }
}; // end of class ExecutableAstList

class Body : public ExecutableAstList, public Executable
{
public:

    Body () : ExecutableAstList(AST_BODY), Executable() { }
    Body (string const &body_text, FiLoc const &source_filoc = FiLoc::ms_invalid);
    Body (Sint32 body_integer, FiLoc const &source_filoc = FiLoc::ms_invalid);

    bool IsNativeIntegerValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;

    // this is the non-virtual, top-level Print method, not
    // to be confused with Ast::Base::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    using ExecutableAstList::Print;
}; // end of class Body

class BodyList : public Ast::AstList<Body>
{
public:

    BodyList () : Ast::AstList<Body>(AST_BODY_LIST) { }
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

    virtual bool IsNativeIntegerValue (SymbolTable &symbol_table) const = 0;

    virtual Sint32 IntegerValue (SymbolTable &symbol_table) const = 0;
    virtual string TextValue (SymbolTable &symbol_table) const = 0;
}; // end of class Expression

class Conditional : public ExecutableAst
{
public:

    Conditional (Expression const *if_expression)
        :
        ExecutableAst(if_expression->GetFiLoc(), AST_CONDITIONAL),
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

    DumpSymbolTable () : ExecutableAst(FiLoc::ms_invalid, AST_DUMP_SYMBOL_TABLE) { }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
}; // end of class DumpSymbolTable

class DeclareArray : public Directive
{
public:

    DeclareArray (Ast::Id const *id)
        :
        Directive(id->GetFiLoc(), AST_DECLARE_ARRAY),
        m_id(id)
    {
        assert(m_id != NULL);
    }
    virtual ~DeclareArray ();

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Ast::Id const *const m_id;
}; // end of class DeclareArray

class DeclareMap : public Directive
{
public:

    DeclareMap (Ast::Id const *id)
        :
        Directive(id->GetFiLoc(), AST_DECLARE_ARRAY),
        m_id(id)
    {
        assert(m_id != NULL);
    }
    virtual ~DeclareMap ();

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Ast::Id const *const m_id;
}; // end of class DeclareMap

class Define : public Directive
{
public:

    Define (Ast::Id *id)
        :
        Directive(id->GetFiLoc(), AST_DEFINE),
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

    Define (Ast::Id *id, AstType ast_type)
        :
        Directive(id->GetFiLoc(), ast_type),
        m_id(id),
        m_body(NULL)
    {
        assert(m_id != NULL);
    }

    Ast::Id *const m_id;
    Body *m_body;
}; // end of class Define

class DefineArrayElement : public Define
{
public:

    DefineArrayElement (Ast::Id *id)
        :
        Define(id, AST_DEFINE_ARRAY_ELEMENT)
    { }

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
}; // end of class DefineArrayElement

class DefineMapElement : public Define
{
public:

    DefineMapElement (Ast::Id *id, Text const *key)
        :
        Define(id, AST_DEFINE_ARRAY_ELEMENT),
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

    Undefine (Ast::Id *id)
        :
        Directive(id->GetFiLoc(), AST_UNDEFINE),
        m_id(id)
    {
        assert(m_id != NULL);
    }
    virtual ~Undefine ();

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Ast::Id *const m_id;
}; // end of class Undefine

class Loop : public Directive
{
public:

    Loop (
        Ast::Id *iterator_id,
        Expression *iteration_count_expression)
        :
        Directive(iterator_id->GetFiLoc(), AST_LOOP),
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

    Ast::Id *const m_iterator_id;
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
        Ast::Id *key_id,
        Ast::Id *map_id)
        :
        Directive(key_id->GetFiLoc(), AST_FOR_EACH),
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

    Ast::Id *const m_key_id;
    Ast::Id *const m_map_id;
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
        Directive(include_filename_expression->GetFiLoc(), AST_INCLUDE),
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
        Directive(message_expression->GetFiLoc(), AST_MESSAGE),
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

    // TODO: deprecate this once the reflex scanner is implemented
    Text (string const &text, FiLoc const &filoc)
        :
        Expression(filoc, AST_TEXT),
        m_text(text)
    { }
    Text (char const *s, Uint32 char_count, FiLoc const &filoc)
        :
        Expression(filoc, AST_TEXT),
        m_text(s, char_count)
    { }

    string const &GetText () const { return m_text; }
    void SetText (string const &text) { m_text = text; }
    void AppendText (string const &text) { m_text += text; }
    void AppendText (char const *s, Uint32 char_count)
    {
        assert(s != NULL);
        for (Uint32 i = 0; i < char_count; ++i)
            assert(s[i] != '\0');
        m_text.append(s, char_count);
    }
    void AppendChar (Uint8 c) { m_text += c; }

    virtual bool IsNativeIntegerValue (SymbolTable &symbol_table) const { return false; }

    virtual Sint32 IntegerValue (SymbolTable &symbol_table) const;
    virtual string TextValue (SymbolTable &symbol_table) const;

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
        Expression(filoc, AST_INTEGER),
        m_value(value)
    { }

    Sint32 Value () const { return m_value; }
    void SetValue (Sint32 value) { m_value = value; }

    virtual bool IsNativeIntegerValue (SymbolTable &symbol_table) const { return true; }

    virtual Sint32 IntegerValue (SymbolTable &symbol_table) const;
    virtual string TextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Sint32 m_value;
}; // end of class Integer

class Sizeof : public Expression
{
public:

    Sizeof (Ast::Id *id)
        :
        Expression(id->GetFiLoc(), AST_SIZEOF),
        m_id(id)
    {
        assert(m_id != NULL);
    }
    virtual ~Sizeof ();

    virtual bool IsNativeIntegerValue (SymbolTable &symbol_table) const { return true; }

    virtual Sint32 IntegerValue (SymbolTable &symbol_table) const;
    virtual string TextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Ast::Id *const m_id;
}; // end of class Sizeof

class Dereference : public Expression
{
public:

    Dereference (Ast::Id *id, Expression *element_index_expression, DereferenceType dereference_type)
        :
        Expression(id->GetFiLoc(), AST_DEREFERENCE),
        m_id(id),
        m_element_index_expression(element_index_expression),
        m_dereference_type(dereference_type)
    {
        assert(m_id != NULL);
        // m_element_index_expression can be NULL
        assert(m_dereference_type == DEREFERENCE_IFF_DEFINED || m_dereference_type == DEREFERENCE_ALWAYS);
    }
    virtual ~Dereference ();

    virtual bool IsNativeIntegerValue (SymbolTable &symbol_table) const;

    virtual Sint32 IntegerValue (SymbolTable &symbol_table) const;
    virtual string TextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    Dereference (Ast::Id *id, Expression *element_index_expression, DereferenceType dereference_type, AstType ast_type)
        :
        Expression(id->GetFiLoc(), ast_type),
        m_id(id),
        m_element_index_expression(element_index_expression),
        m_dereference_type(dereference_type)
    {
        assert(m_id != NULL);
        // m_element_index_expression can be NULL
    }

    Body const *DereferencedBody (SymbolTable &symbol_table) const;

    Ast::Id *const m_id;
    Expression *const m_element_index_expression;
    DereferenceType const m_dereference_type;
}; // end of class Dereference

class IsDefined : public Dereference
{
public:

    IsDefined (Ast::Id *id, Expression *element_index_expression)
        :
        Dereference(id, element_index_expression, DEREFERENCE_ALWAYS, AST_IS_DEFINED)
    { }

    virtual bool IsNativeIntegerValue (SymbolTable &symbol_table) const { return true; }

    virtual Sint32 IntegerValue (SymbolTable &symbol_table) const;
    virtual string TextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
}; // end of class IsDefined

class Operation : public Expression
{
public:

    enum Operator
    {
        CONCATENATE = 0,
        DIVIDE,
        EQUAL,
        GREATER_THAN,
        GREATER_THAN_OR_EQUAL,
        INT_CAST,
        LESS_THAN,
        LESS_THAN_OR_EQUAL,
        LOGICAL_AND,
        LOGICAL_NOT,
        LOGICAL_OR,
        MINUS,
        MULTIPLY,
        NEGATIVE,
        NOT_EQUAL,
        PLUS,
        REMAINDER,
        STRING_CAST,
        STRING_LENGTH,
        TO_CHARACTER_LITERAL,
        TO_STRING_LITERAL,

        OPERATOR_COUNT
    }; // end of enum Operation::Operator

    Operation (
        Expression const *left_expression,
        Operator op,
        Expression const *right_expression)
        :
        Expression(left_expression->GetFiLoc(), AST_OPERATION),
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
        Expression(right_expression->GetFiLoc(), AST_OPERATION),
        m_op(op),
        m_left(NULL),
        m_right(right_expression)
    {
        ValidateOperands();
    }
    virtual ~Operation ();

    virtual bool IsNativeIntegerValue (SymbolTable &symbol_table) const;

    virtual Sint32 IntegerValue (SymbolTable &symbol_table) const;
    virtual string TextValue (SymbolTable &symbol_table) const;

    virtual void Execute (Textifier &textifier, SymbolTable &symbol_table) const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    bool IsTextOperation () const
    {
        return m_op == CONCATENATE ||
               m_op == STRING_CAST ||
               m_op == TO_CHARACTER_LITERAL ||
               m_op == TO_STRING_LITERAL;
    }

    void ValidateOperands ();

    Operator const m_op;
    Expression const *const m_left;
    Expression const *const m_right;
}; // end of class Operation

ostream &operator << (ostream &stream, Operation::Operator op);

} // end of namespace Preprocessor
} // end of namespace Barf

#endif // !defined(BARF_PREPROCESSOR_AST_HPP_)
