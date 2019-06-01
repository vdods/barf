// ///////////////////////////////////////////////////////////////////////////
// trison_ast.cpp by Victor Dods, created 2006/11/21
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_ast.hpp"

#include <sstream>

namespace Trison {

string const &AstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AST_COUNT-CommonLang::AST_START_CUSTOM_TYPES_HERE_] =
    {
        "AST_ERROR_DIRECTIVE",
        "AST_LOOKAHEAD_DIRECTIVE",
        "AST_NONTERMINAL",
        "AST_NONTERMINAL_LIST",
        "AST_NONTERMINAL_MAP",
        "AST_PRECEDENCE",
        "AST_PRECEDENCE_LIST",
        "AST_PRECEDENCE_MAP",
        "AST_PRIMARY_SOURCE",
        "AST_RULE",
        "AST_RULE_LIST",
        "AST_RULE_TOKEN",
        "AST_RULE_TOKEN_LIST",
        "AST_TERMINAL",
        "AST_TERMINAL_LIST",
        "AST_TERMINAL_MAP",
        "AST_TOKEN_ID",
        "AST_TOKEN_SPECIFIER_LIST",
        "AST_TYPE_MAP"
    };

    assert(ast_type < AST_COUNT);
    if (ast_type < CommonLang::AST_START_CUSTOM_TYPES_HERE_)
        return CommonLang::AstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-CommonLang::AST_START_CUSTOM_TYPES_HERE_];
}

void Terminal::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(m_assigned_type_map != NULL);
    TokenId::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "assigned types: " << endl;
    m_assigned_type_map->Print(stream, Stringify, indent_level+2);
}

void Terminal::SetAssignedTypeMap (TypeMap const *assigned_type_map)
{
    assert(m_assigned_type_map == NULL);
    assert(assigned_type_map != NULL);
    m_assigned_type_map = assigned_type_map;
}

void RuleToken::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Base::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "token id: " << m_token_id << endl;
    if (!m_assigned_id.empty())
        stream << Tabs(indent_level+1) << "assigned id: " << m_assigned_id << endl;
}

bool TokenSpecifierList::Contains (std::string const &token_specifier) const
{
    bool found_in_list = false;
    for (const_iterator it = begin(), it_end = end(); it != it_end; ++it)
    {
        assert(*it != NULL);
        Ast::Id const &t = **it;
        if (t.GetText() == token_specifier)
        {
            found_in_list = true;
            break;
        }
    }

    if (m_is_inverted)
        return !found_in_list;
    else
        return found_in_list;
}

void TokenSpecifierList::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Base::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "inverted: " << std::boolalpha << m_is_inverted << endl;
}

void ErrorDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    RuleToken::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "lookheads:\n";
    m_acceptable_tokens->Print(stream, Stringify, indent_level+2);
}

string LookaheadDirective::AsText () const
{
    ostringstream out;

    out << "%lookahead[";
    if (m_lookaheads->m_is_inverted)
        out << "![";
    for (TokenSpecifierList::const_iterator it = m_lookaheads->begin(), it_end = m_lookaheads->end(); it != it_end; ++it)
    {
        assert(*it != NULL);
        Ast::Id const &token_specifier = **it;
        out << token_specifier.GetText();
        TokenSpecifierList::const_iterator next_it = it;
        ++next_it;
        if (next_it != it_end)
            out << '|';
    }
    if (m_lookaheads->m_is_inverted)
        out << ']';
    out << ']';

    return out.str();
}

void LookaheadDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    RuleToken::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "lookheads:\n";
    m_lookaheads->Print(stream, Stringify, indent_level+2);
}

string Rule::AsText (Uint32 stage) const
{
    assert(m_owner_nonterminal != NULL);

    ostringstream out;
    out << m_owner_nonterminal->GetText() << " <-";
    for (Uint32 s = 0; s < min(stage, Uint32(m_rule_token_list->size())); ++s)
        out << ' ' << m_rule_token_list->Element(s)->m_token_id;
    if (stage <= m_rule_token_list->size())
    {
        out << " .";
        // TODO: Need to generalize this to account for ErrorDirective
        for (Uint32 s = stage; s < m_rule_token_list->size(); ++s)
            out << ' ' << m_rule_token_list->Element(s)->m_token_id;
    }
    if (m_lookahead_directive != NULL)
        out << ' ' << m_lookahead_directive->AsText();
    return out.str();
}

void Rule::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Base::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << endl;
    m_rule_precedence->Print(stream, Stringify, indent_level+1);
    m_rule_token_list->Print(stream, Stringify, indent_level+1);
    m_rule_handler_map->Print(stream, Stringify, indent_level+1);
}

void Nonterminal::SetRuleList (RuleList *rule_list)
{
    assert(m_rule_list == NULL && "already has a rule list");
    assert(rule_list != NULL);
    for (RuleList::iterator it = rule_list->begin(),
                            it_end = rule_list->end();
         it != it_end;
         ++it)
    {
        Rule *rule = *it;
        assert(rule != NULL);
        rule->m_owner_nonterminal = this;
    }
    m_rule_list = rule_list;
}

void Nonterminal::SetNpdaGraphStates (Uint32 npda_graph_start_state, Uint32 npda_graph_head_state, Uint32 npda_graph_return_state) const
{
    assert(npda_graph_start_state != UINT32_UPPER_BOUND);
    assert(npda_graph_head_state != UINT32_UPPER_BOUND);
    assert(npda_graph_return_state != UINT32_UPPER_BOUND);
    assert(!IsNpdaGraphed() && "already has states");
    m_npda_graph_start_state = npda_graph_start_state;
    m_npda_graph_head_state = npda_graph_head_state;
    m_npda_graph_return_state = npda_graph_return_state;
}

void Nonterminal::SetDpdaGraphStates (Uint32 dpda_graph_start_state) const
{
    assert(dpda_graph_start_state != UINT32_UPPER_BOUND);
    assert(!IsDpdaGraphed() && "already has states");
    m_dpda_graph_start_state = dpda_graph_start_state;
}

void Nonterminal::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Base::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "id: " << GetText() << endl;
    if (m_assigned_type_map != NULL)
    {
        stream << Tabs(indent_level+1) << "assigned types: " << endl;
        m_assigned_type_map->Print(stream, Stringify, indent_level+2);
    }
    m_rule_list->Print(stream, Stringify, indent_level);
}

Uint32 NonterminalList::RuleCount () const
{
    Uint32 rule_count = 0;
    for (Uint32 i = 0; i < size(); ++i)
    {
        Nonterminal const *nonterminal = Element(i);
        assert(nonterminal != NULL);
        rule_count += nonterminal->m_rule_list->size();
    }
    return rule_count;
}

void Precedence::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Base::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "precedence id: " << m_precedence_id << endl;
    stream << Tabs(indent_level+1) << "precedence associativity: " << m_precedence_associativity << endl;
    stream << Tabs(indent_level+1) << "precedence level: " << m_precedence_level << endl;
}

Rule const *PrimarySource::GetRule (Uint32 rule_index) const
{
    assert(rule_index < RuleCount());
    for (Uint32 i = 0; i < m_nonterminal_list->size(); ++i)
    {
        Nonterminal const *nonterminal = m_nonterminal_list->Element(i);
        assert(nonterminal != NULL);
        assert(nonterminal->m_rule_list != NULL);
        if (rule_index < nonterminal->m_rule_list->size())
            return nonterminal->m_rule_list->Element(rule_index);
        else
            rule_index -= nonterminal->m_rule_list->size();
    }
    assert(false && "this should never happen");
    return NULL;
}

Uint32 PrimarySource::RuleTokenCount () const
{
    Uint32 rule_token_count = 0;
    for (Uint32 i = 0; i < m_nonterminal_list->size(); ++i)
    {
        Nonterminal const *nonterminal = m_nonterminal_list->Element(i);
        assert(nonterminal != NULL);
        assert(nonterminal->m_rule_list != NULL);
        for (Uint32 j = 0; j < nonterminal->m_rule_list->size(); ++j)
        {
            Rule const *rule = nonterminal->m_rule_list->Element(j);
            assert(rule != NULL);
            assert(rule->m_rule_token_list != NULL);
            rule_token_count += rule->m_rule_token_list->size();
        }
    }
    return rule_token_count;
}

RuleToken const *PrimarySource::GetRuleToken (Uint32 rule_token_index) const
{
    assert(rule_token_index < RuleTokenCount());
    for (Uint32 i = 0; i < m_nonterminal_list->size(); ++i)
    {
        Nonterminal const *nonterminal = m_nonterminal_list->Element(i);
        assert(nonterminal != NULL);
        assert(nonterminal->m_rule_list != NULL);
        for (Uint32 j = 0; j < nonterminal->m_rule_list->size(); ++j)
        {
            Rule const *rule = nonterminal->m_rule_list->Element(j);
            assert(rule != NULL);
            assert(rule->m_rule_token_list != NULL);

            if (rule_token_index < rule->m_rule_token_list->size())
                return rule->m_rule_token_list->Element(rule_token_index);
            else
                rule_token_index -= rule->m_rule_token_list->size();
        }
    }
    assert(false && "this should never happen");
    return NULL;
}

string const &PrimarySource::AssignedType (string const &token_id, string const &target_id) const
{
    assert(!token_id.empty());
    assert(!target_id.empty());
    Terminal const *terminal = m_terminal_map->Element(token_id);
    Nonterminal const *nonterminal = m_nonterminal_map->Element(token_id);
    assert((terminal != NULL && nonterminal == NULL) || (terminal == NULL && nonterminal != NULL));
    TypeMap const *assigned_type_map = terminal != NULL ? terminal->AssignedTypeMap() : nonterminal->m_assigned_type_map;
    assert(assigned_type_map != NULL);
    Ast::String const *assigned_type = assigned_type_map->Element(target_id);
    return assigned_type != NULL ? assigned_type->GetText() : g_empty_string;
}

string PrimarySource::GetTokenId (Uint32 token_index) const
{
    if (token_index < 0x100)
        return CharLiteral(token_index);

    TokenIdMap::const_iterator it = m_token_id_map.find(token_index);
    assert(it != m_token_id_map.end());
    return it->second;
}

Uint32 PrimarySource::GetTokenIndex (string const &token_id) const
{
    TokenIndexMap::const_iterator it = m_token_index_map.find(token_id);
    assert(it != m_token_index_map.end());
    return it->second;
}

void PrimarySource::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, AstTypeString, indent_level);
}

void PrimarySource::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(m_target_map != NULL && "no target map set");

    Ast::Base::Print(stream, Stringify, indent_level);
    m_target_map->Print(stream, Stringify, indent_level+1);
    m_terminal_map->Print(stream, Stringify, indent_level+1);
    m_precedence_map->Print(stream, Stringify, indent_level+1);
    stream << Tabs(indent_level+1) << "default parse nonterminal: " << m_default_parse_nonterminal_id << endl;
    m_nonterminal_list->Print(stream, Stringify, indent_level+1);
}

} // end of namespace Trison
