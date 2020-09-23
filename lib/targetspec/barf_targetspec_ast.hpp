// 2006.10.22 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_TARGETSPEC_AST_HPP_)
#define BARF_TARGETSPEC_AST_HPP_

#include "barf_targetspec.hpp"

#include "barf_ast.hpp"

namespace Barf {
namespace Targetspec {

/*

// ///////////////////////////////////////////////////////////////////////////
// class hierarchy
// ///////////////////////////////////////////////////////////////////////////

Ast::Base (abstract)
    Specification
    Ast::Directive
        AddDirective
        AddCodespec
    ParamSpec
    ParamType
    Bound

// ///////////////////////////////////////////////////////////////////////////
// class composition
// ///////////////////////////////////////////////////////////////////////////

Specification
    Id (%target)
    Ast::CodeBlock (%run_before_code_generation)
    Ast::DirectiveList
        Ast::Directive[]

AddDirective
    std::string (directive id)
    ParamSpec
        AstType (param type)
        Bound

AddCodespec
    Ast::String (filename)

ParamType
    AstType (param type)

Bound
    Sint32 lower_bound
    Sint32 upper_bound (if -1, no upper bound)

*/

enum
{
    AST_ADD_CODESPEC = Ast::AST_START_CUSTOM_TYPES_HERE_,
    AST_ADD_CODESPEC_LIST,
    AST_ADD_DIRECTIVE,
    AST_ADD_DIRECTIVE_MAP,
    AST_PARAM_TYPE,
    AST_SPECIFICATION,
    AST_SPECIFICATION_MAP,

    AST_COUNT
};

string const &AstTypeString (AstType ast_type);

struct AddCodespec : public Ast::Directive
{
    Ast::String const *const m_filename;
    Ast::Id const *const m_filename_directive_id;

    AddCodespec (Ast::String const *filename, Ast::Id const *filename_directive_id)
        :
        Ast::Directive("%add_codespec", filename->GetFiLoc(), AST_ADD_CODESPEC),
        m_filename(filename),
        m_filename_directive_id(filename_directive_id)
    {
        assert(m_filename != NULL);
        assert(m_filename_directive_id != NULL);
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class AddCodespec

struct AddCodespecList : public Ast::AstList<AddCodespec>
{
    AddCodespecList () : Ast::AstList<AddCodespec>(AST_ADD_CODESPEC_LIST) { }
}; // end of class AddCodespecList

struct AddDirective : public Ast::Directive
{
    Ast::Id const *const m_directive_to_add_id;
    AstType const m_param_type;

    AddDirective (Ast::Id const *directive_to_add_id, AstType param_type, string const &directive_id)
        :
        Ast::Directive(directive_id, directive_to_add_id->GetFiLoc(), AST_ADD_DIRECTIVE),
        m_directive_to_add_id(directive_to_add_id),
        m_param_type(param_type)
    {
        assert(m_directive_to_add_id != NULL);
        assert(m_param_type == Ast::AST_ID ||
               m_param_type == Ast::AST_STRING ||
               m_param_type == Ast::AST_DUMB_CODE_BLOCK ||
               m_param_type == Ast::AST_STRICT_CODE_BLOCK ||
               m_param_type == Ast::AST_NONE);
    }

    virtual bool IsRequired () const { return false; }
    virtual Ast::TextBase const *DefaultValue () const { return NULL; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class AddDirective

struct AddDirectiveMap : public Ast::AstMap<AddDirective>
{
    AddDirectiveMap () : Ast::AstMap<AddDirective>(AST_ADD_DIRECTIVE_MAP) { }
}; // end of class AddDirectiveMap

struct AddRequiredDirective : public AddDirective
{
    AddRequiredDirective (Ast::Id *directive_to_add_id, AstType param_type)
        :
        AddDirective(directive_to_add_id, param_type, "%add_required_directive")
    { }

    virtual bool IsRequired () const { return true; }
}; // end of class AddRequiredDirective

struct AddOptionalDirective : public AddDirective
{
    Ast::TextBase const *const m_default_value;

    AddOptionalDirective (Ast::Id *directive_to_add_id, AstType param_type, Ast::TextBase const *default_value)
        :
        AddDirective(directive_to_add_id, param_type, "%add_optional_directive"),
        m_default_value(default_value)
    { }

    virtual Ast::TextBase const *DefaultValue () const { return m_default_value; }
}; // end of class AddOptionalDirective

struct ParamType : public Ast::Base
{
    AstType const m_param_type;

    ParamType (AstType param_type)
        :
        Ast::Base(FiLoc::ms_invalid, AST_PARAM_TYPE),
        m_param_type(param_type)
    {
        assert(m_param_type == Ast::AST_ID ||
               m_param_type == Ast::AST_STRING ||
               m_param_type == Ast::AST_DUMB_CODE_BLOCK ||
               m_param_type == Ast::AST_STRICT_CODE_BLOCK ||
               m_param_type == Ast::AST_NONE);
    }

    static string const &ParamTypeString (AstType ast_type);
}; // end of class ParamType

struct Specification : public Ast::Base
{
    Ast::Id const *const m_target_id;
    AddCodespecList const *const m_add_codespec_list;
    AddDirectiveMap const *const m_add_directive_map;

    Specification (
        Ast::Id const *target_id,
        AddCodespecList const *add_codespec_list,
        AddDirectiveMap const *add_directive_map)
        :
        Ast::Base(target_id->GetFiLoc(), AST_SPECIFICATION),
        m_target_id(target_id),
        m_add_codespec_list(add_codespec_list),
        m_add_directive_map(add_directive_map)
    {
        assert(m_target_id != NULL);
        assert(m_add_directive_map != NULL);
        assert(m_add_codespec_list != NULL);
    }

    // this is the non-virtual, top-level Print method, not
    // to be confused with Ast::Base::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class Specification

struct SpecificationMap : public Ast::AstMap<Specification>
{
    SpecificationMap () : Ast::AstMap<Specification>(AST_SPECIFICATION_MAP) { }
}; // end of class SpecificationMap

} // end of namespace Targetspec
} // end of namespace Barf

#endif // !defined(BARF_TARGETSPEC_AST_HPP_)
