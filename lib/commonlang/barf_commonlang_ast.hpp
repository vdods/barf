// ///////////////////////////////////////////////////////////////////////////
// barf_commonlang_ast.hpp by Victor Dods, created 2006/10/22
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_COMMONLANG_AST_HPP_)
#define _BARF_COMMONLANG_AST_HPP_

#include "barf_commonlang.hpp"

#include <vector>

#include "barf_ast.hpp"

namespace Barf {

namespace Targetspec {

class AddCodespec;
class AddDirective;
class Parser;
class Specification;

} // end of namespace Targetspec

namespace Preprocessor {

class Body;
class Parser;
class SymbolTable;

} // end of namespace Preprocessor

namespace CommonLang {

enum
{
    AT_RULE_HANDLER = Ast::AT_START_CUSTOM_TYPES_HERE_,
    AT_RULE_HANDLER_MAP,
    AT_TARGET,
    AT_TARGET_DIRECTIVE,
    AT_TARGET_MAP,

    AT_START_CUSTOM_TYPES_HERE_
};

string const &GetAstTypeString (AstType ast_type);

class TargetDirective : public Ast::Directive
{
public:

    Ast::Id const *const m_target_id;
    Ast::Id const *const m_directive_id;
    Ast::TextBase const *const m_directive_value;

    TargetDirective (
        Ast::Id const *target_id,
        Ast::Id const *directive_id,
        Ast::TextBase const *directive_value)
        :
        Directive("%target", target_id->GetFiLoc(), AT_TARGET_DIRECTIVE),
        m_target_id(target_id),
        m_directive_id(directive_id),
        m_directive_value(directive_value)
    {
        assert(m_target_id != NULL);
        assert(m_directive_id != NULL);
        // m_directive_value can be NULL
    }

    virtual string GetDirectiveString () const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class TargetDirective

class Target : public Ast::AstMap<TargetDirective>
{
public:

    string const m_target_id;

    Target (string const &target_id);

    // this is called on a Target which appears in %targets
    // and will enable specific error checking required for later code generation
    void EnableCodeGeneration () { m_is_enabled_for_code_generation = true; }
    // sets the primary source path which was used to generate this target
    void SetSourcePath (string const &source_path);
    // attempts to add a target directive, but will warn and not add if this
    // target is not enabled for code generation.
    void Add (TargetDirective *target_directive);
    // parses the targetspec file corresponding to this target.
    void ParseTargetspec (string const &tool_prefix, Targetspec::Parser &parser) const;
    // parses all the codespecs specified in m_targetspec and adds the
    // parsed Preprocessor::Body instances to m_codespec_body_list.
    void ParseCodespecs (string const &tool_prefix, Preprocessor::Parser &parser) const;
    // iterates through all codespecs and generates code.
    void GenerateCode (Preprocessor::SymbolTable const &symbol_table) const;

private:

    // checks the target directives against the given Targetspec::Specification.
    void CheckAgainstTargetspec (Targetspec::Specification const &specification) const;
    // checks a Targetspec::AddDirective against a TargetDirective
    void CheckAgainstAddDirective (
        Targetspec::AddDirective const &add_directive,
        TargetDirective const *target_directive) const;
    // adds target directives -- specific to this target
    void GenerateTargetSymbols (Preprocessor::SymbolTable &symbol_table) const;

    struct ParsedTargetspec
    {
        Targetspec::Specification const *m_specification;
        string m_source_path;
    }; // end of struct Target::ParsedTargetspec

    struct ParsedCodespec
    {
        Targetspec::AddCodespec const *m_add_codespec;
        Preprocessor::Body const *m_codespec_body;
        string m_source_path;

        ParsedCodespec (
            Targetspec::AddCodespec const *add_codespec,
            Preprocessor::Body const *codespec_body,
            string const &source_path)
            :
            m_add_codespec(add_codespec),
            m_codespec_body(codespec_body),
            m_source_path(source_path)
        {
            assert(m_add_codespec != NULL);
            assert(m_codespec_body != NULL);
            assert(!m_source_path.empty());
        }
    }; // end of struct Target::ParsedCodespec

    typedef vector<ParsedCodespec> ParsedCodespecList;

    bool m_is_enabled_for_code_generation;
    string m_source_path;
    mutable ParsedTargetspec m_targetspec;
    mutable ParsedCodespecList m_codespec_list;

    using Ast::AstMap<TargetDirective>::Add;
}; // end of class Target

struct TargetMap : public Ast::AstMap<Target>
{
    TargetMap () : Ast::AstMap<Target>(AT_TARGET_MAP) { }

    // sets the path of the primary source file on each Target
    void SetSourcePath (string const &source_path);
    // adds the given TargetDirective to the corresponding Target if
    // the Target exists, otherwise it creates the Target before
    // adding the TargetDirective.
    void AddTargetDirective (TargetDirective *target_directive);

    using Ast::AstMap<Target>::Add;
}; // end of struct TargetMap

struct RuleHandler : public Ast::Base
{
    Ast::Id const *const m_target_id;
    Ast::CodeBlock const *const m_rule_handler_code_block;

    RuleHandler (
        Ast::Id const *target_id,
        Ast::CodeBlock const *rule_handler_code_block)
        :
        Ast::Base(
            (target_id != NULL) ?
            target_id->GetFiLoc() :
            rule_handler_code_block->GetFiLoc(),
            AT_RULE_HANDLER),
        m_target_id(target_id),
        m_rule_handler_code_block(rule_handler_code_block)
    {
        assert(m_target_id != NULL);
        assert(m_rule_handler_code_block != NULL);
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct RuleHandler

struct RuleHandlerMap : public Ast::AstMap<RuleHandler>
{
    RuleHandlerMap () : Ast::AstMap<RuleHandler>(AT_RULE_HANDLER_MAP) { }
}; // end of struct RuleHandlerMap

} // end of namespace CommonLang
} // end of namespace Barf

#endif // !defined(_BARF_COMMONLANG_AST_HPP_)
