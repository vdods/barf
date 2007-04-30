// ///////////////////////////////////////////////////////////////////////////
// barf_langspec_ast.hpp by Victor Dods, created 2006/10/22
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_LANGSPEC_AST_HPP_)
#define _BARF_LANGSPEC_AST_HPP_

#include "barf_langspec.hpp"

#include "barf_astcommon.hpp"

namespace Barf {
namespace LangSpec {

/*

// ///////////////////////////////////////////////////////////////////////////
// class hierarchy
// ///////////////////////////////////////////////////////////////////////////

AstCommon::Ast (abstract)
    Specification
    AstCommon::Directive
        AddDirective
        AddCodeSpec
    ParamSpec
    ParamType
    Bound

// ///////////////////////////////////////////////////////////////////////////
// class composition
// ///////////////////////////////////////////////////////////////////////////

Specification
    Identifier (%target_language)
    AstCommon::CodeBlock (%run_before_code_generation)
    AstCommon::DirectiveList
        AstCommon::Directive[]

AddDirective
    std::string (directive identifier)
    ParamSpec
        AstType (param type)
        Bound

AddCodeSpec
    AstCommon::String (filename)

ParamType
    AstType (param type)

Bound
    Sint32 lower_bound
    Sint32 upper_bound (if -1, no upper bound)

*/

enum
{
    AT_SPECIFICATION_MAP = AstCommon::AT_START_CUSTOM_TYPES_HERE_,
    AT_ADD_CODESPEC_LIST,
    AT_ADD_DIRECTIVE_MAP,
    AT_SPECIFICATION,
    AT_ADD_CODESPEC,
    AT_ADD_DIRECTIVE,
    AT_PARAM_TYPE,

    AT_COUNT
};

string const &GetAstTypeString (AstType ast_type);

struct AddCodeSpec : public AstCommon::Directive
{
    AstCommon::String const *const m_filename;
    AstCommon::Identifier const *const m_filename_directive_identifier;

    AddCodeSpec (AstCommon::String const *filename, AstCommon::Identifier const *filename_directive_identifier)
        :
        AstCommon::Directive("%add_codespec", filename->GetFiLoc(), AT_ADD_CODESPEC),
        m_filename(filename),
        m_filename_directive_identifier(filename_directive_identifier)
    {
        assert(m_filename != NULL);
        assert(m_filename_directive_identifier != NULL);
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class AddCodeSpec

struct AddCodeSpecList : public AstCommon::AstList<AddCodeSpec>
{
    AddCodeSpecList () : AstCommon::AstList<AddCodeSpec>(AT_ADD_CODESPEC_LIST) { }
}; // end of class AddCodeSpecList

struct AddDirective : public AstCommon::Directive
{
    AstCommon::Identifier const *const m_directive_to_add_identifier;
    AstType const m_param_type;

    AddDirective (AstCommon::Identifier const *directive_to_add_identifier, AstType param_type, string const &directive_identifier)
        :
        AstCommon::Directive(directive_identifier, directive_to_add_identifier->GetFiLoc(), AT_ADD_DIRECTIVE),
        m_directive_to_add_identifier(directive_to_add_identifier),
        m_param_type(param_type)
    {
        assert(m_directive_to_add_identifier != NULL);
        assert(m_param_type == AstCommon::AT_IDENTIFIER ||
               m_param_type == AstCommon::AT_STRING ||
               m_param_type == AstCommon::AT_DUMB_CODE_BLOCK ||
               m_param_type == AstCommon::AT_STRICT_CODE_BLOCK ||
               m_param_type == AstCommon::AT_NONE);
    }

    virtual bool GetIsRequired () const { return false; }
    virtual AstCommon::TextBase const *GetDefaultValue () const { return NULL; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class AddDirective

struct AddDirectiveMap : public AstCommon::AstMap<AddDirective>
{
    AddDirectiveMap () : AstCommon::AstMap<AddDirective>(AT_ADD_DIRECTIVE_MAP) { }
}; // end of class AddDirectiveMap

struct AddRequiredDirective : public AddDirective
{
    AddRequiredDirective (AstCommon::Identifier *directive_to_add_identifier, AstType param_type)
        :
        AddDirective(directive_to_add_identifier, param_type, "%add_required_directive")
    { }

    virtual bool GetIsRequired () const { return true; }
}; // end of class AddRequiredDirective

struct AddOptionalDirective : public AddDirective
{
    AstCommon::TextBase const *const m_default_value;

    AddOptionalDirective (AstCommon::Identifier *directive_to_add_identifier, AstType param_type, AstCommon::TextBase const *default_value)
        :
        AddDirective(directive_to_add_identifier, param_type, "%add_optional_directive"),
        m_default_value(default_value)
    { }

    virtual AstCommon::TextBase const *GetDefaultValue () const { return m_default_value; }
}; // end of class AddOptionalDirective

struct ParamType : public AstCommon::Ast
{
    AstType const m_param_type;

    ParamType (AstType param_type)
        :
        AstCommon::Ast(FiLoc::ms_invalid, AT_PARAM_TYPE),
        m_param_type(param_type)
    {
        assert(m_param_type == AstCommon::AT_IDENTIFIER ||
               m_param_type == AstCommon::AT_STRING ||
               m_param_type == AstCommon::AT_DUMB_CODE_BLOCK ||
               m_param_type == AstCommon::AT_STRICT_CODE_BLOCK ||
               m_param_type == AstCommon::AT_NONE);
    }

    static string const &GetParamTypeString (AstType ast_type);
}; // end of class ParamType

struct Specification : public AstCommon::Ast
{
    AstCommon::Identifier const *const m_target_language_identifier;
    AddCodeSpecList const *const m_add_codespec_list;
    AddDirectiveMap const *const m_add_directive_map;

    Specification (
        AstCommon::Identifier const *target_language_identifier,
        AddCodeSpecList const *add_codespec_list,
        AddDirectiveMap const *add_directive_map)
        :
        AstCommon::Ast(target_language_identifier->GetFiLoc(), AT_SPECIFICATION),
        m_target_language_identifier(target_language_identifier),
        m_add_codespec_list(add_codespec_list),
        m_add_directive_map(add_directive_map)
    {
        assert(m_target_language_identifier != NULL);
        assert(m_add_directive_map != NULL);
        assert(m_add_codespec_list != NULL);
    }

    // this is the non-virtual, top-level Print method, not
    // to be confused with AstCommon::Ast::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class Specification

struct SpecificationMap : public AstCommon::AstMap<Specification>
{
    SpecificationMap () : AstCommon::AstMap<Specification>(AT_SPECIFICATION_MAP) { }
}; // end of class SpecificationMap

} // end of namespace LangSpec
} // end of namespace Barf

#endif // !defined(_BARF_LANGSPEC_AST_HPP_)
