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

string const &GetAstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AT_COUNT-CommonLang::AT_START_CUSTOM_TYPES_HERE_] =
    {
        "AT_NONTERMINAL",
        "AT_NONTERMINAL_LIST",
        "AT_NONTERMINAL_MAP",
        "AT_PRECEDENCE",
        "AT_PRECEDENCE_LIST",
        "AT_PRECEDENCE_MAP",
        "AT_PRIMARY_SOURCE",
        "AT_RULE",
        "AT_RULE_LIST",
        "AT_RULE_TOKEN",
        "AT_RULE_TOKEN_LIST",
        "AT_TERMINAL",
        "AT_TERMINAL_LIST",
        "AT_TERMINAL_MAP",
        "AT_TOKEN_ID",
        "AT_TYPE_MAP"
    };

    assert(ast_type < AT_COUNT);
    if (ast_type < CommonLang::AT_START_CUSTOM_TYPES_HERE_)
        return CommonLang::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-CommonLang::AT_START_CUSTOM_TYPES_HERE_];
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

string Rule::GetAsText (Uint32 stage) const
{
    assert(m_owner_nonterminal != NULL);

    ostringstream out;
    out << m_owner_nonterminal->GetText() << " <-";
    for (Uint32 s = 0; s < min(stage, m_rule_token_list->size()); ++s)
        out << ' ' << m_rule_token_list->GetElement(s)->m_token_id;
    if (stage <= m_rule_token_list->size())
    {
        out << " .";
        for (Uint32 s = stage; s < m_rule_token_list->size(); ++s)
            out << ' ' << m_rule_token_list->GetElement(s)->m_token_id;
    }
    return out.str();
}

void Rule::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Base::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "precedence: " << m_rule_precedence_id << endl;
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
    assert(!GetIsNpdaGraphed() && "already has states");
    m_npda_graph_start_state = npda_graph_start_state;
    m_npda_graph_head_state = npda_graph_head_state;
    m_npda_graph_return_state = npda_graph_return_state;
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

Uint32 NonterminalList::GetRuleCount () const
{
    Uint32 rule_count = 0;
    for (Uint32 i = 0; i < size(); ++i)
    {
        Nonterminal const *nonterminal = GetElement(i);
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

Uint32 PrimarySource::GetTokenIndex (string const &token_id) const
{
    assert(!token_id.empty());

    Uint32 index = 0x100;

    // special terminals (see also GenerateGeneralAutomatonSymbols in trison_codespecsymbols.cpp)
    if (token_id == "END_")
        return index;
    ++index;

    if (token_id == "ERROR_")
        return index;
    ++index;

    for (TerminalMap::const_iterator it = m_terminal_map->begin(), it_end = m_terminal_map->end();
         it != it_end;
         ++it)
    {
        Terminal const *terminal = it->second;
        assert(terminal != NULL);
        if (token_id == terminal->GetText())
            return index;
        if (terminal->m_is_id)
            ++index;
    }
    for (NonterminalMap::const_iterator it = m_nonterminal_map->begin(), it_end = m_nonterminal_map->end();
         it != it_end;
         ++it)
    {
        Nonterminal const *nonterminal = it->second;
        assert(nonterminal != NULL);
        if (token_id == nonterminal->GetText())
            return index;
        ++index;
    }
    assert(false && "invalid token id");
    return UINT32_UPPER_BOUND;
}

Rule const *PrimarySource::GetRule (Uint32 rule_index) const
{
    assert(rule_index < GetRuleCount());
    for (Uint32 i = 0; i < m_nonterminal_list->size(); ++i)
    {
        Nonterminal const *nonterminal = m_nonterminal_list->GetElement(i);
        assert(nonterminal != NULL);
        assert(nonterminal->m_rule_list != NULL);
        if (rule_index < nonterminal->m_rule_list->size())
            return nonterminal->m_rule_list->GetElement(rule_index);
        else
            rule_index -= nonterminal->m_rule_list->size();
    }
    assert(false && "this should never happen");
    return NULL;
}

Uint32 PrimarySource::GetRuleTokenCount () const
{
    Uint32 rule_token_count = 0;
    for (Uint32 i = 0; i < m_nonterminal_list->size(); ++i)
    {
        Nonterminal const *nonterminal = m_nonterminal_list->GetElement(i);
        assert(nonterminal != NULL);
        assert(nonterminal->m_rule_list != NULL);
        for (Uint32 j = 0; j < nonterminal->m_rule_list->size(); ++j)
        {
            Rule const *rule = nonterminal->m_rule_list->GetElement(j);
            assert(rule != NULL);
            assert(rule->m_rule_token_list != NULL);
            rule_token_count += rule->m_rule_token_list->size();
        }
    }
    return rule_token_count;
}

RuleToken const *PrimarySource::GetRuleToken (Uint32 rule_token_index) const
{
    assert(rule_token_index < GetRuleTokenCount());
    for (Uint32 i = 0; i < m_nonterminal_list->size(); ++i)
    {
        Nonterminal const *nonterminal = m_nonterminal_list->GetElement(i);
        assert(nonterminal != NULL);
        assert(nonterminal->m_rule_list != NULL);
        for (Uint32 j = 0; j < nonterminal->m_rule_list->size(); ++j)
        {
            Rule const *rule = nonterminal->m_rule_list->GetElement(j);
            assert(rule != NULL);
            assert(rule->m_rule_token_list != NULL);

            if (rule_token_index < rule->m_rule_token_list->size())
                return rule->m_rule_token_list->GetElement(rule_token_index);
            else
                rule_token_index -= rule->m_rule_token_list->size();
        }
    }
    assert(false && "this should never happen");
    return NULL;
}

string const &PrimarySource::GetAssignedType (string const &token_id, string const &target_id) const
{
    assert(!token_id.empty());
    assert(!target_id.empty());
    Terminal const *terminal = m_terminal_map->GetElement(token_id);
    Nonterminal const *nonterminal = m_nonterminal_map->GetElement(token_id);
    assert(terminal != NULL && nonterminal == NULL || terminal == NULL && nonterminal != NULL);
    TypeMap const *assigned_type_map = terminal != NULL ? terminal->GetAssignedTypeMap() : nonterminal->m_assigned_type_map;
    assert(assigned_type_map != NULL);
    Ast::String const *assigned_type = assigned_type_map->GetElement(target_id);
    return assigned_type != NULL ? assigned_type->GetText() : g_empty_string;
}

void PrimarySource::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, GetAstTypeString, indent_level);
}

void PrimarySource::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Base::Print(stream, Stringify, indent_level);
    m_target_map->Print(stream, Stringify, indent_level+1);
    m_terminal_map->Print(stream, Stringify, indent_level+1);
    m_precedence_map->Print(stream, Stringify, indent_level+1);
    stream << Tabs(indent_level+1) << "default parse nonterminal: " << m_default_parse_nonterminal_id << endl;
    m_nonterminal_list->Print(stream, Stringify, indent_level+1);
}

} // end of namespace Trison
