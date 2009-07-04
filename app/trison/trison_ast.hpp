// ///////////////////////////////////////////////////////////////////////////
// trison_ast.hpp by Victor Dods, created 2006/11/21
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(TRISON_AST_HPP_)
#define TRISON_AST_HPP_

#include "trison.hpp"

#include "barf_ast.hpp"
#include "barf_commonlang_ast.hpp"
#include "barf_util.hpp"
#include "trison_enums.hpp"

namespace Trison {

struct Nonterminal;
struct Precedence;
struct PrimarySource;

enum
{
    AST_NONTERMINAL = CommonLang::AST_START_CUSTOM_TYPES_HERE_,
    AST_NONTERMINAL_LIST,
    AST_NONTERMINAL_MAP,
    AST_PRECEDENCE,
    AST_PRECEDENCE_LIST,
    AST_PRECEDENCE_MAP,
    AST_PRIMARY_SOURCE,
    AST_RULE,
    AST_RULE_LIST,
    AST_RULE_TOKEN,
    AST_RULE_TOKEN_LIST,
    AST_TERMINAL,
    AST_TERMINAL_LIST,
    AST_TERMINAL_MAP,
    AST_TOKEN_ID,
    AST_TYPE_MAP,

    AST_COUNT
};

string const &AstTypeString (AstType ast_type);

struct TypeMap : public Ast::AstMap<Ast::String>
{
    TypeMap () : Ast::AstMap<Ast::String>(AST_TYPE_MAP) { }
}; // end of struct TypeMap

struct TokenId : public Ast::TextBase
{
    Uint32 const m_token_index;

    TokenId (string const &text, Uint32 token_index, FiLoc const &filoc, AstType ast_type = AST_TOKEN_ID)
        :
        TextBase(text, filoc, ast_type),
        m_token_index(token_index)
    {
        assert(!text.empty());
    }
}; // end of struct TokenId

struct Terminal : public TokenId
{
    bool const m_is_id;
    Uint8 const m_char;

    Terminal (Ast::Id const *id, Uint32 token_index)
        :
        TokenId(id->GetText(), token_index, id->GetFiLoc(), AST_TERMINAL),
        m_is_id(true),
        m_char('\0'),
        m_assigned_type_map(NULL)
    {
        delete id;
    }
    Terminal (Ast::Char const *ch)
        :
        TokenId(CharLiteral(ch->GetChar()), (Uint32)ch->GetChar(), ch->GetFiLoc(), AST_TERMINAL),
        m_is_id(false),
        m_char(ch->GetChar()),
        m_assigned_type_map(NULL)
    {
        delete ch;
    }

    TypeMap const *AssignedTypeMap () const { assert(m_assigned_type_map != NULL); return m_assigned_type_map; }

    void SetAssignedTypeMap (TypeMap const *assigned_type_map);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    TypeMap const *m_assigned_type_map; // TODO: move into TokenId (baseclass)
}; // end of struct Terminal

struct TerminalList : public Ast::AstList<Terminal>
{
    TerminalList () : Ast::AstList<Terminal>(AST_TERMINAL_LIST) { }
}; // end of struct TerminalList

struct TerminalMap : public Ast::AstMap<Terminal>
{
    TerminalMap () : Ast::AstMap<Terminal>(AST_TERMINAL_MAP) { }
}; // end of struct TerminalMap

struct RuleToken : public Ast::Base
{
    string const m_token_id;
    string const m_assigned_id;

    RuleToken (string const &token_id, FiLoc const &filoc, string const &assigned_id = g_empty_string)
        :
        Ast::Base(filoc, AST_RULE_TOKEN),
        m_token_id(token_id),
        m_assigned_id(assigned_id)
    {
        assert(!m_token_id.empty());
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct RuleToken

struct RuleTokenList : public Ast::AstList<RuleToken>
{
    RuleTokenList () : Ast::AstList<RuleToken>(AST_RULE_TOKEN_LIST) { }
}; // end of struct RuleTokenList

struct Rule : public Ast::Base
{
    Nonterminal const *m_owner_nonterminal;
    RuleTokenList const *const m_rule_token_list;
    Precedence const *const m_rule_precedence;
    CommonLang::RuleHandlerMap const *m_rule_handler_map;
    Uint32 const m_rule_index;

    Rule (RuleTokenList const *rule_token_list, Precedence const *rule_precedence, Uint32 rule_index)
        :
        Ast::Base(rule_token_list->GetFiLoc(), AST_RULE),
        m_owner_nonterminal(NULL),
        m_rule_token_list(rule_token_list),
        m_rule_precedence(rule_precedence),
        m_rule_handler_map(NULL),
        m_rule_index(rule_index)
    {
        assert(m_rule_token_list != NULL);
        assert(m_rule_precedence != NULL);
    }

    string AsText (Uint32 stage = UINT32_UPPER_BOUND) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct Rule

struct RuleList : public Ast::AstList<Rule>
{
    RuleList () : Ast::AstList<Rule>(AST_RULE_LIST) { }
}; // end of struct RuleList

struct Nonterminal : public TokenId
{
    TypeMap const *const m_assigned_type_map; // TODO: move into TokenId (baseclass)
    RuleList const *m_rule_list;

    Nonterminal (
        string const &id,
        Uint32 token_index,
        FiLoc const &filoc,
        TypeMap const *assigned_type_map = NULL)
        :
        TokenId(id, token_index, filoc, AST_NONTERMINAL),
        m_assigned_type_map(assigned_type_map),
        m_rule_list(NULL),
        m_npda_graph_start_state(UINT32_UPPER_BOUND),
        m_npda_graph_head_state(UINT32_UPPER_BOUND),
        m_npda_graph_return_state(UINT32_UPPER_BOUND),
        m_dpda_graph_start_state(UINT32_UPPER_BOUND)
    {
        assert(!id.empty());
    }

    // TODO: these shouldn't be in here, they should be in some separate thing associated with the npda/dpda graphs
    bool IsNpdaGraphed () const { return m_npda_graph_start_state != UINT32_UPPER_BOUND; }
    Uint32 NpdaGraphStartState () const { assert(IsNpdaGraphed()); return m_npda_graph_start_state; }
    Uint32 NpdaGraphHeadState () const { assert(IsNpdaGraphed()); return m_npda_graph_head_state; }
    Uint32 NpdaGraphReturnState () const { assert(IsNpdaGraphed()); return m_npda_graph_return_state; }
    bool IsDpdaGraphed () const { return m_dpda_graph_start_state != UINT32_UPPER_BOUND; }
    Uint32 DpdaGraphStartState () const { assert(IsDpdaGraphed()); return m_dpda_graph_start_state; }

    void SetRuleList (RuleList *rule_list);
    void SetNpdaGraphStates (Uint32 npda_graph_start_state, Uint32 npda_graph_head_state, Uint32 npda_graph_return_state) const;
    void SetDpdaGraphStates (Uint32 dpda_graph_start_state) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    // the separate npda start state (with epsilon transition into the head state)
    mutable Uint32 m_npda_graph_start_state;
    // the first npda state for all rules contained within this nonterminal
    mutable Uint32 m_npda_graph_head_state;
    // the return npda state for this nonterminal
    mutable Uint32 m_npda_graph_return_state;

    // the dpda start state for this nonterminal
    mutable Uint32 m_dpda_graph_start_state;
}; // end of struct Nonterminal

struct NonterminalList : public Ast::AstList<Nonterminal>
{
    NonterminalList () : Ast::AstList<Nonterminal>(AST_NONTERMINAL_LIST) { }

    Uint32 RuleCount () const;
}; // end of struct NonterminalList

struct NonterminalMap : public Ast::AstMap<Nonterminal>
{
    NonterminalMap () : Ast::AstMap<Nonterminal>(AST_NONTERMINAL_MAP) { }
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
        Ast::Base(filoc, AST_PRECEDENCE),
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
    PrecedenceMap () : Ast::AstMap<Precedence>(AST_PRECEDENCE_MAP) { }
}; // end of struct PrecedenceMap

struct PrecedenceList : public Ast::AstList<Precedence>
{
    PrecedenceList () : Ast::AstList<Precedence>(AST_PRECEDENCE_LIST) { }
}; // end of struct PrecedenceList

struct PrimarySource : public Ast::Base
{
    TerminalList const *const m_terminal_list;
    TerminalMap const *const m_terminal_map;
    PrecedenceMap const *const m_precedence_map;
    PrecedenceList const *const m_precedence_list;
    string const m_default_parse_nonterminal_id;
    NonterminalList const *const m_nonterminal_list;
    NonterminalMap const *const m_nonterminal_map;

    PrimarySource (
        TerminalList const *terminal_list,
        TerminalMap const *terminal_map,
        PrecedenceMap const *precedence_map,
        PrecedenceList const *precedence_list,
        string const &default_parse_nonterminal_id,
        FiLoc const &filoc,
        NonterminalList const *nonterminal_list,
        NonterminalMap const *nonterminal_map)
        :
        Ast::Base(filoc, AST_PRIMARY_SOURCE),
        m_terminal_list(terminal_list),
        m_terminal_map(terminal_map),
        m_precedence_map(precedence_map),
        m_precedence_list(precedence_list),
        m_default_parse_nonterminal_id(default_parse_nonterminal_id),
        m_nonterminal_list(nonterminal_list),
        m_nonterminal_map(nonterminal_map),
        m_target_map(NULL)
    {
        assert(m_terminal_list != NULL);
        assert(m_terminal_map != NULL);
        assert(m_precedence_map != NULL);
        assert(m_precedence_list != NULL);
        assert(!m_default_parse_nonterminal_id.empty());
        assert(m_nonterminal_list != NULL);
        assert(m_nonterminal_map != NULL);

        for (Uint32 i = 0; i < m_terminal_list->size(); ++i)
        {
            Terminal const *terminal = m_terminal_list->Element(i);
            assert(terminal != NULL);
            assert(m_token_id_map.find(terminal->m_token_index) == m_token_id_map.end());
            m_token_id_map[terminal->m_token_index] = terminal->GetText();
        }
        for (Uint32 i = 0; i < m_nonterminal_list->size(); ++i)
        {
            Nonterminal const *nonterminal = m_nonterminal_list->Element(i);
            assert(nonterminal != NULL);
            assert(m_token_id_map.find(nonterminal->m_token_index) == m_token_id_map.end());
            m_token_id_map[nonterminal->m_token_index] = nonterminal->GetText();
        }
    }

    Uint32 RuleCount () const { return m_nonterminal_list->RuleCount(); }
    Rule const *GetRule (Uint32 rule_index) const;
    Uint32 RuleTokenCount () const;
    RuleToken const *GetRuleToken (Uint32 rule_token_index) const;
    string const &AssignedType (string const &token_id, string const &target_id) const;
    string GetTokenId (Uint32 token_index) const;
    CommonLang::TargetMap const &GetTargetMap () const
    {
        assert(m_target_map != NULL && "no target map set");
        return *m_target_map;
    }

    void SetTargetMap (CommonLang::TargetMap const *target_map)
    {
        assert(m_target_map == NULL && "target map already set");
        assert(target_map != NULL);
        m_target_map = target_map;
    }

    // this is the non-virtual, top-level Print method, not
    // to be confused with Ast::Base::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    typedef map<Uint32, string> TokenIdMap;
    TokenIdMap m_token_id_map;
    CommonLang::TargetMap const *m_target_map;
}; // end of struct PrimarySource

} // end of namespace Trison

#endif // !defined(TRISON_AST_HPP_)
