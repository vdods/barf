// ///////////////////////////////////////////////////////////////////////////
// reflex_ast.hpp by Victor Dods, created 2006/10/18
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_REFLEX_AST_HPP_)
#define _REFLEX_AST_HPP_

#include "reflex.hpp"

#include <map>
#include <vector>

#include "barf_commonlang_ast.hpp"
#include "barf_filoc.hpp"
#include "barf_graph.hpp"
#include "barf_regex_ast.hpp"
#include "barf_util.hpp"
#include "reflex_enums.hpp"

namespace Barf {
namespace Preprocessor {

class ArraySymbol;
class SymbolTable;

} // end of namespace Preprocessor

namespace Regex {

class RegularExpression;

} // end of namespace Regex
} // end of namespace Barf

namespace Reflex {

enum
{
    AT_START_DIRECTIVE = CommonLang::AT_START_CUSTOM_TYPES_HERE_,
    AT_RULE,
    AT_RULE_LIST,
    AT_SCANNER_STATE,
    AT_SCANNER_STATE_MAP,
    AT_REPRESENTATION,

    AT_COUNT
};

string const &GetAstTypeString (AstType ast_type);

struct StartDirective : public AstCommon::Directive
{
    AstCommon::Id const *const m_start_state_id;

    StartDirective (AstCommon::Id const *start_state_id)
        :
        AstCommon::Directive("%start", start_state_id->GetFiLoc(), AT_START_DIRECTIVE),
        m_start_state_id(start_state_id)
    {
        assert(m_start_state_id != NULL);
    }
    virtual ~StartDirective ()
    {
        delete m_start_state_id;
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class StartDirective

struct Rule : public AstCommon::Ast
{
    string const m_rule_regex_string;
    Regex::RegularExpression const *const m_rule_regex;
    CommonLang::RuleHandlerMap const *const m_rule_handler_map;

    Rule (
        string const &rule_regex_string,
        Regex::RegularExpression const *rule_regex,
        CommonLang::RuleHandlerMap const *rule_handler_map)
        :
        AstCommon::Ast(rule_regex->GetFiLoc(), AT_RULE),
        m_rule_regex_string(rule_regex_string),
        m_rule_regex(rule_regex),
        m_rule_handler_map(rule_handler_map)
    {
        assert(m_rule_regex != NULL);
        assert(m_rule_handler_map != NULL);
    }
    virtual ~Rule ()
    {
        delete m_rule_regex;
        delete m_rule_handler_map;
    }

    void GenerateNfa (Graph &graph, Uint32 start_index, Uint32 end_index) const;
    void PopulateAcceptHandlerCodeArraySymbol (
        string const &target_language_id,
        Preprocessor::ArraySymbol *accept_handler_code_symbol) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class Rule

struct RuleList : public AstCommon::AstList<Rule>
{
    RuleList () : AstCommon::AstList<Rule>(AT_RULE_LIST) { }
};

struct ScannerState : public AstCommon::Ast
{
    AstCommon::Id const *const m_scanner_state_id;
    RuleList const *const m_rule_list;

    ScannerState (
        AstCommon::Id const *scanner_state_id,
        RuleList *rule_list)
        :
        AstCommon::Ast(scanner_state_id->GetFiLoc(), AT_SCANNER_STATE),
        m_scanner_state_id(scanner_state_id),
        m_rule_list(rule_list)
    {
        assert(m_scanner_state_id != NULL);
        assert(m_rule_list != NULL);
    }
    virtual ~ScannerState ()
    {
        delete m_scanner_state_id;
        delete m_rule_list;
    }

    Uint32 GetAcceptHandlerCount () const { return m_rule_list->size(); }

    void GenerateNfa (
        Graph &nfa_graph,
        vector<Uint32> &start_state_index_array,
        Uint32 &next_accept_handler_index) const;
    void PopulateAcceptHandlerCodeArraySymbol (
        string const &target_language_id,
        Preprocessor::ArraySymbol *accept_handler_code_symbol) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class ScannerState

struct ScannerStateMap : public AstCommon::AstMap<ScannerState>
{
    ScannerStateMap () : AstCommon::AstMap<ScannerState>(AT_SCANNER_STATE_MAP) { }
};

class Representation : public AstCommon::Ast
{
public:

    CommonLang::TargetLanguageMap const *const m_target_language_map;
    Regex::RegularExpressionMap *const m_regex_macro_map; // this could technically go away
    StartDirective const *const m_start_directive;
    ScannerStateMap const *const m_scanner_state_map;

    Representation (
        CommonLang::TargetLanguageMap const *target_language_map,
        Regex::RegularExpressionMap *regex_macro_map,
        StartDirective const *start_directive,
        FiLoc const &end_preamble_filoc,
        ScannerStateMap const *scanner_state_map)
        :
        AstCommon::Ast(target_language_map->GetFiLoc(), AT_REPRESENTATION),
        m_target_language_map(target_language_map),
        m_regex_macro_map(regex_macro_map),
        m_start_directive(start_directive),
        m_scanner_state_map(scanner_state_map),
        m_next_accept_handler_index(0)
    {
        assert(m_target_language_map != NULL);
        assert(m_regex_macro_map != NULL);
        // m_start_directive can be NULL if an error happened
        assert(m_scanner_state_map != NULL);
    }

    CommonLang::TargetLanguageMap const &GetTargetLanguageMap () const { return *m_target_language_map; }
    Uint32 GetAcceptHandlerCount () const;
    Rule const *GetAcceptHandlerRule (Uint32 rule_index) const;
    Graph const &GetNfaGraph () const { return m_nfa_graph; }
    Graph const &GetDfaGraph () const { return m_dfa_graph; }

    void GenerateNfaAndDfa () const;
    void PrintNfaGraph (string const &filename, string const &graph_name) const;
    void PrintDfaGraph (string const &filename, string const &graph_name) const;
    void GenerateAutomatonSymbols (
        Preprocessor::SymbolTable &symbol_table) const;
    void GenerateTargetLanguageDependentSymbols (
        string const &target_language_id,
        Preprocessor::SymbolTable &symbol_table) const;
    // this is the non-virtual, top-level Print method, not
    // to be confused with AstCommon::Ast::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    mutable Graph m_nfa_graph;
    mutable vector<Uint32> m_nfa_start_state;
    mutable Graph m_dfa_graph;
    mutable vector<Uint32> m_dfa_start_state;
    mutable Uint32 m_next_accept_handler_index;
}; // end of class Representation

} // end of namespace Reflex

#endif // !defined(_REFLEX_AST_HPP_)
