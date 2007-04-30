// ///////////////////////////////////////////////////////////////////////////
// trison_ast.hpp by Victor Dods, created 2006/11/21
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_AST_HPP_)
#define _TRISON_AST_HPP_

#include "trison.hpp"

#include "barf_astcommon.hpp"
#include "barf_commonlang_ast.hpp"
#include "barf_graph.hpp"
#include "barf_preprocessor_symboltable.hpp"
#include "barf_util.hpp"
#include "trison_enums.hpp"

namespace Trison {

struct Nonterminal;
struct Representation;

enum
{
    AT_TOKEN = CommonLang::AT_START_CUSTOM_TYPES_HERE_,
    AT_TOKEN_LIST,
    AT_TOKEN_MAP,
    AT_RULE_TOKEN,
    AT_RULE_TOKEN_LIST,
    AT_RULE,
    AT_RULE_LIST,
    AT_NONTERMINAL,
    AT_NONTERMINAL_LIST,
    AT_NONTERMINAL_MAP,
    AT_PRECEDENCE,
    AT_PRECEDENCE_LIST,
    AT_PRECEDENCE_MAP,
    AT_REPRESENTATION,

    AT_COUNT
};

string const &GetAstTypeString (AstType ast_type);

struct Token : public AstCommon::Ast
{
    AstCommon::Id const *const m_id;
    AstCommon::Char const *const m_char;

    Token (AstCommon::Id const *id)
        :
        AstCommon::Ast(id->GetFiLoc(), AT_TOKEN),
        m_id(id),
        m_char(NULL)
    {
        assert(m_id != NULL);
    }
    Token (AstCommon::Char const *ch)
        :
        AstCommon::Ast(ch->GetFiLoc(), AT_TOKEN),
        m_id(NULL),
        m_char(ch)
    {
        assert(m_char != NULL);
    }
    ~Token ()
    {
        delete m_id;
        delete m_char;
    }

    bool GetIsId () const { return m_id != NULL; }
    string GetSourceText () const { return m_id != NULL ? m_id->GetText() : GetCharLiteral(m_char->GetChar()); }
    string const &GetIdString () const { assert(m_id != NULL); return m_id->GetText(); }
    Uint8 GetCharChar () const { assert(m_char != NULL); return m_char->GetChar(); }
    string const &GetAssignedType () const { return m_assigned_type; }

    void SetAssignedType (string const &assigned_type);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    string m_assigned_type;
}; // end of struct Token

struct TokenList : public AstCommon::AstList<Token>
{
    TokenList () : AstCommon::AstList<Token>(AT_TOKEN_LIST) { }
}; // end of struct TokenList

struct TokenMap : public AstCommon::AstMap<Token>
{
    TokenMap () : AstCommon::AstMap<Token>(AT_TOKEN_MAP) { }
}; // end of struct TokenMap

struct RuleToken : public AstCommon::Ast
{
    string const m_token_id;
    string const m_assigned_id;

    RuleToken (string const &token_id, FiLoc const &filoc, string const &assigned_id = gs_empty_string)
        :
        AstCommon::Ast(filoc, AT_RULE_TOKEN),
        m_token_id(token_id),
        m_assigned_id(assigned_id)
    {
        assert(!m_token_id.empty());
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct RuleToken

struct RuleTokenList : public AstCommon::AstList<RuleToken>
{
    RuleTokenList () : AstCommon::AstList<RuleToken>(AT_RULE_TOKEN_LIST) { }
}; // end of struct RuleTokenList

struct Rule : public AstCommon::Ast
{
    Nonterminal const *m_owner_nonterminal;
    RuleTokenList const *const m_rule_token_list;
    string const m_rule_precedence_id;
    CommonLang::RuleHandlerMap const *m_rule_handler_map;

    Rule (RuleTokenList const *rule_token_list, string const &rule_precedence_id)
        :
        AstCommon::Ast(rule_token_list->GetFiLoc(), AT_RULE),
        m_owner_nonterminal(NULL),
        m_rule_token_list(rule_token_list),
        m_rule_precedence_id(rule_precedence_id),
        m_rule_handler_map(NULL),
        m_accept_state_index(UINT32_UPPER_BOUND)
    {
        assert(m_rule_token_list != NULL);
    }

    bool GetHasAcceptStateIndex () const { return m_accept_state_index != UINT32_UPPER_BOUND; }
    Uint32 GetAcceptStateIndex () const { assert(GetHasAcceptStateIndex()); return m_accept_state_index; }
    string GetAsText (Uint32 stage = UINT32_UPPER_BOUND) const;

    void SetAcceptStateIndex (Uint32 accept_state_index) const;

    void PopulateAcceptHandlerCodeArraySymbol (
        string const &target_language_id,
        Preprocessor::ArraySymbol *accept_handler_code_symbol) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    mutable Uint32 m_accept_state_index;
}; // end of struct Rule

struct RuleList : public AstCommon::AstList<Rule>
{
    RuleList () : AstCommon::AstList<Rule>(AT_RULE_LIST) { }
}; // end of struct RuleList

struct Nonterminal : public AstCommon::Ast
{
    string const m_id;
    string const m_assigned_type;
    RuleList const *m_rule_list;

    Nonterminal (
        string const &id,
        FiLoc const &filoc,
        string const &assigned_type = gs_empty_string)
        :
        AstCommon::Ast(filoc, AT_NONTERMINAL),
        m_id(id),
        m_assigned_type(assigned_type),
        m_rule_list(NULL),
        m_npda_graph_start_state(UINT32_UPPER_BOUND),
        m_npda_graph_head_state(UINT32_UPPER_BOUND),
        m_npda_graph_return_state(UINT32_UPPER_BOUND)
    {
        assert(!m_id.empty());
    }

    bool GetHasAssignedType () const { return !m_assigned_type.empty(); }
    bool GetIsNpdaGraphed () const { return m_npda_graph_start_state != UINT32_UPPER_BOUND; }
    Uint32 GetNpdaGraphStartState () const { assert(GetIsNpdaGraphed()); return m_npda_graph_start_state; }
    Uint32 GetNpdaGraphHeadState () const { assert(GetIsNpdaGraphed()); return m_npda_graph_head_state; }
    Uint32 GetNpdaGraphReturnState () const { assert(GetIsNpdaGraphed()); return m_npda_graph_return_state; }

    void SetRuleList (RuleList *rule_list);
    void SetNpdaGraphStates (Uint32 npda_graph_start_state, Uint32 npda_graph_head_state, Uint32 npda_graph_return_state) const;

    void PopulateAcceptHandlerCodeArraySymbol (
        string const &target_language_id,
        Preprocessor::ArraySymbol *accept_handler_code_symbol) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    // the separate start state (with epsilon transition into the head state)
    mutable Uint32 m_npda_graph_start_state;
    // the first state for all rules contained within this nonterminal
    mutable Uint32 m_npda_graph_head_state;
    // the return state for this nonterminal
    mutable Uint32 m_npda_graph_return_state;
}; // end of struct Nonterminal

struct NonterminalList : public AstCommon::AstList<Nonterminal>
{
    NonterminalList () : AstCommon::AstList<Nonterminal>(AT_NONTERMINAL_LIST) { }

    Uint32 GetRuleCount () const;
}; // end of struct NonterminalList

struct NonterminalMap : public AstCommon::AstMap<Nonterminal>
{
    NonterminalMap () : AstCommon::AstMap<Nonterminal>(AT_NONTERMINAL_MAP) { }
}; // end of struct NonterminalMap

struct Precedence : public AstCommon::Ast
{
    enum
    {
        DEFAULT_PRECEDENCE_LEVEL = -1
    };

    string const m_precedence_id;
    Associativity const m_precedence_associativity;
    Sint32 const m_precedence_level;

    Precedence (
        string const &precedence_id,
        Associativity precedence_associativity,
        FiLoc const &filoc,
        Sint32 precedence_level = DEFAULT_PRECEDENCE_LEVEL)
        :
        AstCommon::Ast(filoc, AT_PRECEDENCE),
        m_precedence_id(precedence_id),
        m_precedence_associativity(precedence_associativity),
        m_precedence_level(precedence_level)
    {
        assert(!m_precedence_id.empty());
        assert(m_precedence_associativity == A_LEFT ||
               m_precedence_associativity == A_NONASSOC ||
               m_precedence_associativity == A_RIGHT);
        assert(m_precedence_level >= DEFAULT_PRECEDENCE_LEVEL);
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct Precedence

struct PrecedenceMap : public AstCommon::AstMap<Precedence>
{
    PrecedenceMap () : AstCommon::AstMap<Precedence>(AT_PRECEDENCE_MAP) { }
}; // end of struct PrecedenceMap

struct PrecedenceList : public AstCommon::AstList<Precedence>
{
    PrecedenceList () : AstCommon::AstList<Precedence>(AT_PRECEDENCE_LIST) { }
}; // end of struct PrecedenceList

struct Representation : public AstCommon::Ast
{
    CommonLang::TargetLanguageMap const *const m_target_language_map;
    TokenMap const *const m_token_map;
    PrecedenceMap const *const m_precedence_map;
    PrecedenceList const *const m_precedence_list;
    string const m_start_nonterminal_id;
    NonterminalMap const *const m_nonterminal_map;
    NonterminalList const *const m_nonterminal_list;

    Representation (
        CommonLang::TargetLanguageMap const *target_language_map,
        TokenMap const *token_map,
        PrecedenceMap const *precedence_map,
        PrecedenceList const *precedence_list,
        string const &start_nonterminal_id,
        FiLoc const &filoc,
        NonterminalMap const *nonterminal_map,
        NonterminalList const *nonterminal_list)
        :
        AstCommon::Ast(filoc, AT_REPRESENTATION),
        m_target_language_map(target_language_map),
        m_token_map(token_map),
        m_precedence_map(precedence_map),
        m_precedence_list(precedence_list),
        m_start_nonterminal_id(start_nonterminal_id),
        m_nonterminal_map(nonterminal_map),
        m_nonterminal_list(nonterminal_list)
    {
        assert(m_target_language_map != NULL);
        assert(m_token_map != NULL);
        assert(m_precedence_map != NULL);
        assert(m_precedence_list != NULL);
        assert(!m_start_nonterminal_id.empty());
        assert(m_nonterminal_map != NULL);
        assert(m_nonterminal_list != NULL);
        AssignRuleIndices();
    }

    Uint32 GetTokenIndex (string const &token_id) const;
    Uint32 GetRuleCount () const { return m_nonterminal_list->GetRuleCount(); }
    Rule const *GetRule (Uint32 rule_index) const;

    void GenerateNpdaAndDpda () const;
    void PrintNpdaGraph (string const &filename, string const &graph_name) const;
    void PrintDpdaGraph (string const &filename, string const &graph_name) const;
    void GenerateAutomatonSymbols (
        Preprocessor::SymbolTable &symbol_table) const;
    void GenerateTargetLanguageDependentSymbols (
        string const &target_language_id,
        Preprocessor::SymbolTable &symbol_table) const;
    void AssignRuleIndices ();
    // this is the non-virtual, top-level Print method, not
    // to be confused with AstCommon::Ast::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    mutable Graph m_npda_graph;
    mutable Graph m_dpda_graph;
}; // end of struct Representation

} // end of namespace Trison

#endif // !defined(_TRISON_AST_HPP_)
