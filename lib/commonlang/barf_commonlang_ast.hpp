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

namespace LangSpec {

class AddCodeSpec;
class AddDirective;
class Parser;
class Specification;

} // end of namespace LangSpec

namespace Preprocessor {

class Body;
class Parser;
class SymbolTable;

} // end of namespace Preprocessor

namespace CommonLang {

enum
{
    AT_LANGUAGE_DIRECTIVE = Ast::AT_START_CUSTOM_TYPES_HERE_,
    AT_TARGET,
    AT_TARGET_MAP,
    AT_RULE_HANDLER,
    AT_RULE_HANDLER_MAP,

    AT_START_CUSTOM_TYPES_HERE_
};

string const &GetAstTypeString (AstType ast_type);

class LanguageDirective : public Ast::Directive
{
public:

    Ast::Id const *const m_language_id;
    Ast::Id const *const m_directive_id;
    Ast::TextBase const *const m_directive_value;

    LanguageDirective (
        Ast::Id const *language_id,
        Ast::Id const *directive_id,
        Ast::TextBase const *directive_value)
        :
        Directive("%language", language_id->GetFiLoc(), AT_LANGUAGE_DIRECTIVE),
        m_language_id(language_id),
        m_directive_id(directive_id),
        m_directive_value(directive_value)
    {
        assert(m_language_id != NULL);
        assert(m_directive_id != NULL);
        // m_directive_value can be NULL
    }

    virtual string GetDirectiveString () const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class LanguageDirective

class Target : public Ast::AstMap<LanguageDirective>
{
public:

    string const m_language_id;

    Target (string const &language_id);

    // this is called on a Target which appears in %targets
    // and will enable specific error checking required for later code generation
    void EnableCodeGeneration () { m_is_enabled_for_code_generation = true; }
    // sets the primary source path which was used to generate this target
    void SetSourcePath (string const &source_path);
    // attempts to add a language directive, but will warn and not add if this
    // target is not enabled for code generation.
    void Add (LanguageDirective *language_directive);
    // parses the langspec file corresponding to this target.
    void ParseLangSpec (string const &tool_prefix, LangSpec::Parser &parser) const;
    // parses all the codespecs specified in m_lang_spec and adds the
    // parsed Preprocessor::Body instances to m_codespec_body_list.
    void ParseCodeSpecs (string const &tool_prefix, Preprocessor::Parser &parser) const;
    // iterates through all codespecs and generates code.
    void GenerateCode (Preprocessor::SymbolTable const &symbol_table) const;

private:

    // checks the language directives against the given LangSpec::Specification.
    void CheckAgainstLangSpec (LangSpec::Specification const &specification) const;
    // checks a LangSpec::AddDirective against a LanguageDirective
    void CheckAgainstAddDirective (
        LangSpec::AddDirective const &add_directive,
        LanguageDirective const *language_directive) const;
    // adds language directives -- specific to this target
    void GenerateTargetSymbols (Preprocessor::SymbolTable &symbol_table) const;

    struct ParsedLangSpec
    {
        LangSpec::Specification const *m_specification;
        string m_source_path;
    }; // end of struct Target::ParsedLangSpec

    struct ParsedCodeSpec
    {
        LangSpec::AddCodeSpec const *m_add_codespec;
        Preprocessor::Body const *m_codespec_body;
        string m_source_path;

        ParsedCodeSpec (
            LangSpec::AddCodeSpec const *add_codespec,
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
    }; // end of struct Target::ParsedCodeSpec

    typedef vector<ParsedCodeSpec> ParsedCodeSpecList;

    bool m_is_enabled_for_code_generation;
    string m_source_path;
    mutable ParsedLangSpec m_lang_spec;
    mutable ParsedCodeSpecList m_code_spec_list;

    using Ast::AstMap<LanguageDirective>::Add;
}; // end of class Target

struct TargetMap : public Ast::AstMap<Target>
{
    TargetMap () : Ast::AstMap<Target>(AT_TARGET_MAP) { }

    // sets the path of the primary source file on each Target
    void SetSourcePath (string const &source_path);
    // adds the given LanguageDirective to the corresponding Target if
    // the Target exists, otherwise it creates the Target before
    // adding the LanguageDirective.
    void AddLanguageDirective (LanguageDirective *language_directive);

    using Ast::AstMap<Target>::Add;
}; // end of struct TargetMap

struct RuleHandler : public Ast::Base
{
    Ast::Id const *const m_language_id;
    Ast::CodeBlock const *const m_rule_handler_code_block;

    RuleHandler (
        Ast::Id const *language_id,
        Ast::CodeBlock const *rule_handler_code_block)
        :
        Ast::Base(
            (language_id != NULL) ?
            language_id->GetFiLoc() :
            rule_handler_code_block->GetFiLoc(),
            AT_RULE_HANDLER),
        m_language_id(language_id),
        m_rule_handler_code_block(rule_handler_code_block)
    {
        assert(m_language_id != NULL);
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
