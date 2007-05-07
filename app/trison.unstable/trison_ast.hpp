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

#include "barf_ast.hpp"
#include "barf_commonlang_ast.hpp"
#include "barf_util.hpp"
#include "trison_enums.hpp"

namespace Trison {

struct Nonterminal;
struct PrimarySource;

enum
{
    AT_NONTERMINAL = CommonLang::AT_START_CUSTOM_TYPES_HERE_,
    AT_NONTERMINAL_LIST,
    AT_NONTERMINAL_MAP,
    AT_PRECEDENCE,
    AT_PRECEDENCE_LIST,
    AT_PRECEDENCE_MAP,
    AT_PRIMARY_SOURCE,
    AT_RULE,
    AT_RULE_LIST,
    AT_RULE_TOKEN,
    AT_RULE_TOKEN_LIST,
    AT_TERMINAL,
    AT_TERMINAL_LIST,
    AT_TERMINAL_MAP,
    AT_TOKEN_ID,

    AT_COUNT
};

string const &GetAstTypeString (AstType ast_type);

struct TokenId : public Ast::TextBase
{
    TokenId (string const &text, FiLoc const &filoc, AstType ast_type = AT_TOKEN_ID)
        :
        TextBase(text, filoc, ast_type)
    {
        assert(!text.empty());
    }
}; // end of struct TokenId

struct Terminal : public TokenId
{
    bool const m_is_id;
    Uint8 const m_char;

    Terminal (Ast::Id const *id)
        :
        TokenId(id->GetText(), id->GetFiLoc(), AT_TERMINAL),
        m_is_id(true),
        m_char('\0')
    {
        delete id;
    }
    Terminal (Ast::Char const *ch)
        :
        TokenId(GetCharLiteral(ch->GetChar()), ch->GetFiLoc(), AT_TERMINAL),
        m_is_id(false),
        m_char(ch->GetChar())
    {
        delete ch;
    }

    string const &GetAssignedType () const { return m_assigned_type; }

    void SetAssignedType (string const &assigned_type);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    string m_assigned_type;
}; // end of struct Terminal

struct TerminalList : public Ast::AstList<Terminal>
{
    TerminalList () : Ast::AstList<Terminal>(AT_TERMINAL_LIST) { }
}; // end of struct TerminalList

struct TerminalMap : public Ast::AstMap<Terminal>
{
    TerminalMap () : Ast::AstMap<Terminal>(AT_TERMINAL_MAP) { }
}; // end of struct TerminalMap

struct RuleToken : public Ast::Base
{
    string const m_token_id;
    string const m_assigned_id;

    RuleToken (string const &token_id, FiLoc const &filoc, string const &assigned_id = gs_empty_string)
        :
        Ast::Base(filoc, AT_RULE_TOKEN),
        m_token_id(token_id),
        m_assigned_id(assigned_id)
    {
        assert(!m_token_id.empty());
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct RuleToken

struct RuleTokenList : public Ast::AstList<RuleToken>
{
    RuleTokenList () : Ast::AstList<RuleToken>(AT_RULE_TOKEN_LIST) { }
}; // end of struct RuleTokenList

struct Rule : public Ast::Base
{
    Nonterminal const *m_owner_nonterminal;
    RuleTokenList const *const m_rule_token_list;
    string const m_rule_precedence_id;
    CommonLang::RuleHandlerMap const *m_rule_handler_map;
    Uint32 const m_rule_index;

    Rule (RuleTokenList const *rule_token_list, string const &rule_precedence_id, Uint32 rule_index)
        :
        Ast::Base(rule_token_list->GetFiLoc(), AT_RULE),
        m_owner_nonterminal(NULL),
        m_rule_token_list(rule_token_list),
        m_rule_precedence_id(rule_precedence_id),
        m_rule_handler_map(NULL),
        m_rule_index(rule_index)
    {
        assert(m_rule_token_list != NULL);
    }

    string GetAsText (Uint32 stage = UINT32_UPPER_BOUND) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct Rule

struct RuleList : public Ast::AstList<Rule>
{
    RuleList () : Ast::AstList<Rule>(AT_RULE_LIST) { }
}; // end of struct RuleList

struct Nonterminal : public TokenId
{
    string const m_assigned_type;
    RuleList const *m_rule_list;

    Nonterminal (
        string const &id,
        FiLoc const &filoc,
        string const &assigned_type = gs_empty_string)
        :
        TokenId(id, filoc, AT_NONTERMINAL),
        m_assigned_type(assigned_type),
        m_rule_list(NULL),
        m_npda_graph_start_state(UINT32_UPPER_BOUND),
        m_npda_graph_head_state(UINT32_UPPER_BOUND),
        m_npda_graph_return_state(UINT32_UPPER_BOUND)
    {
        assert(!id.empty());
    }

    bool GetHasAssignedType () const { return !m_assigned_type.empty(); }
    bool GetIsNpdaGraphed () const { return m_npda_graph_start_state != UINT32_UPPER_BOUND; }
    Uint32 GetNpdaGraphStartState () const { assert(GetIsNpdaGraphed()); return m_npda_graph_start_state; }
    Uint32 GetNpdaGraphHeadState () const { assert(GetIsNpdaGraphed()); return m_npda_graph_head_state; }
    Uint32 GetNpdaGraphReturnState () const { assert(GetIsNpdaGraphed()); return m_npda_graph_return_state; }

    void SetRuleList (RuleList *rule_list);
    void SetNpdaGraphStates (Uint32 npda_graph_start_state, Uint32 npda_graph_head_state, Uint32 npda_graph_return_state) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    // the separate start state (with epsilon transition into the head state)
    mutable Uint32 m_npda_graph_start_state;
    // the first state for all rules contained within this nonterminal
    mutable Uint32 m_npda_graph_head_state;
    // the return state for this nonterminal
    mutable Uint32 m_npda_graph_return_state;
}; // end of struct Nonterminal

struct NonterminalList : public Ast::AstList<Nonterminal>
{
    NonterminalList () : Ast::AstList<Nonterminal>(AT_NONTERMINAL_LIST) { }

    Uint32 GetRuleCount () const;
}; // end of struct NonterminalList

struct NonterminalMap : public Ast::AstMap<Nonterminal>
{
    NonterminalMap () : Ast::AstMap<Nonterminal>(AT_NONTERMINAL_MAP) { }
}; // end of struct NonterminalMap

struct Precedence : public Ast::Base
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
        Ast::Base(filoc, AT_PRECEDENCE),
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

struct PrecedenceMap : public Ast::AstMap<Precedence>
{
    PrecedenceMap () : Ast::AstMap<Precedence>(AT_PRECEDENCE_MAP) { }
}; // end of struct PrecedenceMap

struct PrecedenceList : public Ast::AstList<Precedence>
{
    PrecedenceList () : Ast::AstList<Precedence>(AT_PRECEDENCE_LIST) { }
}; // end of struct PrecedenceList

struct PrimarySource : public Ast::Base
{
    CommonLang::TargetMap const *const m_target_map;
    TerminalMap const *const m_terminal_map;
    PrecedenceMap const *const m_precedence_map;
    PrecedenceList const *const m_precedence_list;
    string const m_default_parse_nonterminal_id;
    NonterminalMap const *const m_nonterminal_map;
    NonterminalList const *const m_nonterminal_list;

    PrimarySource (
        CommonLang::TargetMap const *target_map,
        TerminalMap const *terminal_map,
        PrecedenceMap const *precedence_map,
        PrecedenceList const *precedence_list,
        string const &default_parse_nonterminal_id,
        FiLoc const &filoc,
        NonterminalMap const *nonterminal_map,
        NonterminalList const *nonterminal_list)
        :
        Ast::Base(filoc, AT_PRIMARY_SOURCE),
        m_target_map(target_map),
        m_terminal_map(terminal_map),
        m_precedence_map(precedence_map),
        m_precedence_list(precedence_list),
        m_default_parse_nonterminal_id(default_parse_nonterminal_id),
        m_nonterminal_map(nonterminal_map),
        m_nonterminal_list(nonterminal_list)
    {
        assert(m_target_map != NULL);
        assert(m_terminal_map != NULL);
        assert(m_precedence_map != NULL);
        assert(m_precedence_list != NULL);
        assert(!m_default_parse_nonterminal_id.empty());
        assert(m_nonterminal_map != NULL);
        assert(m_nonterminal_list != NULL);
    }

    Uint32 GetTokenIndex (string const &token_id) const;
    Uint32 GetRuleCount () const { return m_nonterminal_list->GetRuleCount(); }
    Rule const *GetRule (Uint32 rule_index) const;

    // this is the non-virtual, top-level Print method, not
    // to be confused with Ast::Base::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct PrimarySource

} // end of namespace Trison

#endif // !defined(_TRISON_AST_HPP_)
