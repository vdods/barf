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

#include "barf_astcommon.hpp"

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
    AT_LANGUAGE_DIRECTIVE = AstCommon::AT_START_CUSTOM_TYPES_HERE_,
    AT_TARGET_LANGUAGE,
    AT_TARGET_LANGUAGE_MAP,
    AT_RULE_HANDLER,
    AT_RULE_HANDLER_MAP,

    AT_START_CUSTOM_TYPES_HERE_
};

string const &GetAstTypeString (AstType ast_type);

class LanguageDirective : public AstCommon::Directive
{
public:

    AstCommon::Identifier const *const m_language_identifier;
    AstCommon::Identifier const *const m_directive_identifier;
    AstCommon::TextBase const *const m_directive_value;

    LanguageDirective (
        AstCommon::Identifier const *language_identifier,
        AstCommon::Identifier const *directive_identifier,
        AstCommon::TextBase const *directive_value)
        :
        Directive("%language", language_identifier->GetFiLoc(), AT_LANGUAGE_DIRECTIVE),
        m_language_identifier(language_identifier),
        m_directive_identifier(directive_identifier),
        m_directive_value(directive_value)
    {
        assert(m_language_identifier != NULL);
        assert(m_directive_identifier != NULL);
        // m_directive_value can be NULL
    }

    virtual string GetDirectiveString () const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class LanguageDirective

class TargetLanguage : public AstCommon::AstMap<LanguageDirective>
{
public:

    string const m_language_identifier;

    TargetLanguage (string const &language_identifier);

    // this is called on a TargetLanguage which appears in %target_languages
    // and will enable specific error checking required for later code generation
    void EnableCodeGeneration () { m_is_enabled_for_code_generation = true; }
    // sets the primary source path which was used to generate this target language
    void SetSourcePath (string const &source_path);
    // attempts to add a language directive, but will warn and not add if this
    // target language is not enabled for code generation.
    void Add (LanguageDirective *language_directive);
    // parses the langspec file corresponding to this target language.
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
    // adds language directives -- specific to this target language
    void GenerateTargetLanguageSymbols (Preprocessor::SymbolTable &symbol_table) const;

    struct ParsedLangSpec
    {
        LangSpec::Specification const *m_specification;
        string m_source_path;
    }; // end of struct TargetLanguage::ParsedLangSpec

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
    }; // end of struct TargetLanguage::ParsedCodeSpec

    typedef vector<ParsedCodeSpec> ParsedCodeSpecList;

    bool m_is_enabled_for_code_generation;
    string m_source_path;
    mutable ParsedLangSpec m_lang_spec;
    mutable ParsedCodeSpecList m_code_spec_list;

    using AstCommon::AstMap<LanguageDirective>::Add;
}; // end of class TargetLanguage

struct TargetLanguageMap : public AstCommon::AstMap<TargetLanguage>
{
    TargetLanguageMap () : AstCommon::AstMap<TargetLanguage>(AT_TARGET_LANGUAGE_MAP) { }

    // sets the path of the primary source file on each TargetLanguage
    void SetSourcePath (string const &source_path);
    // adds the given LanguageDirective to the corresponding TargetLanguage if
    // the TargetLanguage exists, otherwise it creates the TargetLanguage before
    // adding the LanguageDirective.
    void AddLanguageDirective (LanguageDirective *language_directive);

    using AstCommon::AstMap<TargetLanguage>::Add;
}; // end of struct TargetLanguageMap

struct RuleHandler : public AstCommon::Ast
{
    AstCommon::Identifier const *const m_language_identifier;
    AstCommon::CodeBlock const *const m_rule_handler_code_block;

    RuleHandler (
        AstCommon::Identifier const *language_identifier,
        AstCommon::CodeBlock const *rule_handler_code_block)
        :
        AstCommon::Ast(
            (language_identifier != NULL) ?
            language_identifier->GetFiLoc() :
            rule_handler_code_block->GetFiLoc(),
            AT_RULE_HANDLER),
        m_language_identifier(language_identifier),
        m_rule_handler_code_block(rule_handler_code_block)
    {
        assert(m_language_identifier != NULL);
        assert(m_rule_handler_code_block != NULL);
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct RuleHandler

struct RuleHandlerMap : public AstCommon::AstMap<RuleHandler>
{
    RuleHandlerMap () : AstCommon::AstMap<RuleHandler>(AT_RULE_HANDLER_MAP) { }
}; // end of struct RuleHandlerMap

} // end of namespace CommonLang
} // end of namespace Barf

#endif // !defined(_BARF_COMMONLANG_AST_HPP_)
