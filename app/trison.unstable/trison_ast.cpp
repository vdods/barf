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

#include <fstream>
#include <sstream>

#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_symboltable.hpp"
#include "trison_graph.hpp"
#include "trison_npda.hpp"

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
        "AT_REPRESENTATION",
        "AT_RULE",
        "AT_RULE_LIST",
        "AT_RULE_TOKEN",
        "AT_RULE_TOKEN_LIST",
        "AT_TOKEN",
        "AT_TOKEN_LIST",
        "AT_TOKEN_MAP"
    };

    assert(ast_type < AT_COUNT);
    if (ast_type < CommonLang::AT_START_CUSTOM_TYPES_HERE_)
        return CommonLang::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-CommonLang::AT_START_CUSTOM_TYPES_HERE_];
}

void Token::SetAssignedType (string const &assigned_type)
{
    assert(m_assigned_type.empty() && "already has an assigned type");
    m_assigned_type = assigned_type;
}

void Token::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    AstCommon::Ast::Print(stream, Stringify, indent_level);
    if (m_id != NULL)
        m_id->Print(stream, Stringify, indent_level+1);
    else
        m_char->Print(stream, Stringify, indent_level+1);
}

void RuleToken::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    AstCommon::Ast::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "token id: " << m_token_id << endl;
    if (!m_assigned_id.empty())
        stream << Tabs(indent_level+1) << "assigned id: " << m_assigned_id << endl;
}

string Rule::GetAsText (Uint32 stage) const
{
    assert(m_owner_nonterminal != NULL);

    ostringstream out;
    out << m_owner_nonterminal->m_id << " <-";
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

void Rule::SetAcceptStateIndex (Uint32 accept_state_index) const
{
    assert(m_accept_state_index == UINT32_UPPER_BOUND);
    m_accept_state_index = accept_state_index;
}

void Rule::PopulateAcceptHandlerCodeArraySymbol (
    string const &target_language_id,
    Preprocessor::ArraySymbol *accept_handler_code_symbol) const
{
    assert(accept_handler_code_symbol != NULL);
    CommonLang::RuleHandler const *rule_handler = m_rule_handler_map->GetElement(target_language_id);
    if (rule_handler != NULL)
    {
        AstCommon::CodeBlock const *rule_handler_code_block = rule_handler->m_rule_handler_code_block;
        assert(rule_handler_code_block != NULL);
        accept_handler_code_symbol->AppendArrayElement(
            new Preprocessor::Body(
                rule_handler_code_block->GetText(),
                rule_handler_code_block->GetFiLoc()));
    }
    else
        accept_handler_code_symbol->AppendArrayElement(new Preprocessor::Body("", FiLoc::ms_invalid));
}

void Rule::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    AstCommon::Ast::Print(stream, Stringify, indent_level);
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

void Nonterminal::PopulateAcceptHandlerCodeArraySymbol (
    string const &target_language_id,
    Preprocessor::ArraySymbol *accept_handler_code_symbol) const
{
    assert(accept_handler_code_symbol != NULL);

    for (RuleList::const_iterator it = m_rule_list->begin(),
                                  it_end = m_rule_list->end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        rule->PopulateAcceptHandlerCodeArraySymbol(target_language_id, accept_handler_code_symbol);
    }
}

void Nonterminal::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    AstCommon::Ast::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "id: " << m_id << endl;
    if (!m_assigned_type.empty())
        stream << Tabs(indent_level+1) << "assigned type: " << GetStringLiteral(m_assigned_type) << endl;
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
    AstCommon::Ast::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "precedence id: " << m_precedence_id << endl;
    stream << Tabs(indent_level+1) << "precedence associativity: " << m_precedence_associativity << endl;
    stream << Tabs(indent_level+1) << "precedence level: " << m_precedence_level << endl;
}

Uint32 Representation::GetTokenIndex (string const &token_id) const
{
    Uint32 index = 0x100;
    if (token_id == "END_")
        return index;
    ++index;
    for (TokenMap::const_iterator it = m_token_map->begin(), it_end = m_token_map->end();
         it != it_end;
         ++it)
    {
        Token const *token = it->second;
        assert(token != NULL);
        if (token_id == token->GetSourceText())
            return index;
        if (token->GetIsId())
            ++index;
    }
    for (NonterminalMap::const_iterator it = m_nonterminal_map->begin(), it_end = m_nonterminal_map->end();
         it != it_end;
         ++it)
    {
        Nonterminal const *nonterminal = it->second;
        assert(nonterminal != NULL);
        if (token_id == nonterminal->m_id)
            return index;
        ++index;
    }
    assert(false && "invalid token id");
    return UINT32_UPPER_BOUND;
}

Rule const *Representation::GetRule (Uint32 rule_index) const
{
    assert(rule_index < GetRuleCount());
    for (Uint32 i = 0; i < m_nonterminal_list->size(); ++i)
    {
        Nonterminal const *nonterminal = m_nonterminal_list->GetElement(i);
        assert(nonterminal != NULL);
        if (rule_index < nonterminal->m_rule_list->size())
            return nonterminal->m_rule_list->GetElement(rule_index);
        else
            rule_index -= nonterminal->m_rule_list->size();
    }
    assert(false && "this should never happen");
    return NULL;
}

void Representation::GenerateNpdaAndDpda () const
{
    // generate the NPDA
    {
        GenerateNpda(*this, m_npda_graph);
    }

    // generate the DPDA
}

void Representation::PrintNpdaGraph (string const &filename, string const &graph_name) const
{
    // don't print anything if no filename was specified.
    if (filename.empty())
        return;

    if (filename == "-")
        m_npda_graph.PrintDotGraph(cout, graph_name);
    else
    {
        ofstream file(filename.c_str());
        if (file.is_open())
            m_npda_graph.PrintDotGraph(file, graph_name);
        else
            EmitWarning("could not open file \"" + filename + "\" for writing");
    }
}

void Representation::PrintDpdaGraph (string const &filename, string const &graph_name) const
{
    // don't print anything if no filename was specified.
    if (filename.empty())
        return;

    if (filename == "-")
        m_dpda_graph.PrintDotGraph(cout, graph_name);
    else
    {
        ofstream file(filename.c_str());
        if (file.is_open())
            m_dpda_graph.PrintDotGraph(file, graph_name);
        else
            EmitWarning("could not open file \"" + filename + "\" for writing");
    }
}

void Representation::GenerateAutomatonSymbols (
    Preprocessor::SymbolTable &symbol_table) const
{
    assert(m_npda_graph.GetNodeCount() > 0);
//     assert(m_dpda_graph.GetNodeCount() > 0);

    // _start -- value of %start -- the name of the default start nonterminal
    {
        Preprocessor::ScalarSymbol *symbol =
            symbol_table.DefineScalarSymbol("_start", FiLoc::ms_invalid);
        symbol->SetScalarBody(
            new Preprocessor::Body(
                m_start_nonterminal_id,
                FiLoc::ms_invalid));
    }

    // _terminal_index_list[terminal count] -- list of terminal token values
    // (END_ is 0x100, then all non-char terminals are next contiguously)
    //
    // _terminal_name_list[terminal count] -- list of terminal token names
    // (END_ and then all non-char terminals)
    //
    // _nonterminal_index_list[nonterminal count] -- list of nonterminal token values;
    // they start where _terminal_index_list left off.
    //
    // _nonterminal_name_list[nonterminal count] -- list of nonterminal token names
    {
        Preprocessor::ArraySymbol *terminal_index_list_symbol =
            symbol_table.DefineArraySymbol("_terminal_index_list", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *terminal_name_list_symbol =
            symbol_table.DefineArraySymbol("_terminal_name_list", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nonterminal_index_list_symbol =
            symbol_table.DefineArraySymbol("_nonterminal_index_list", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nonterminal_name_list_symbol =
            symbol_table.DefineArraySymbol("_nonterminal_name_list", FiLoc::ms_invalid);

        Uint32 token_index = 0x100;

        terminal_index_list_symbol->AppendArrayElement(
            new Preprocessor::Body(
                Sint32(token_index++),
                FiLoc::ms_invalid));
        terminal_name_list_symbol->AppendArrayElement(
            new Preprocessor::Body(
                "END_",
                FiLoc::ms_invalid));

        for (TokenMap::const_iterator it = m_token_map->begin(), it_end = m_token_map->end();
             it != it_end;
             ++it)
        {
            Token const *token = it->second;
            assert(token != NULL);
            if (token->GetIsId())
            {
                terminal_index_list_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(token_index++),
                        FiLoc::ms_invalid));
                terminal_name_list_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        token->GetSourceText(),
                        FiLoc::ms_invalid));
            }
        }

        nonterminal_index_list_symbol->AppendArrayElement(
            new Preprocessor::Body(0, FiLoc::ms_invalid));
        nonterminal_name_list_symbol->AppendArrayElement(
            new Preprocessor::Body("none_", FiLoc::ms_invalid));

        for (NonterminalMap::const_iterator it = m_nonterminal_map->begin(), it_end = m_nonterminal_map->end();
             it != it_end;
             ++it)
        {
            Nonterminal const *nonterminal = it->second;
            assert(nonterminal != NULL);
            nonterminal_index_list_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(token_index++),
                    FiLoc::ms_invalid));
            nonterminal_name_list_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    nonterminal->m_id,
                    FiLoc::ms_invalid));
        }
    }

    // _npda_initial_node_index[nonterminal name] -- maps nonterminal name => node index
    {
        Preprocessor::MapSymbol *npda_initial_node_index_symbol =
            symbol_table.DefineMapSymbol("_npda_initial_node_index", FiLoc::ms_invalid);
        for (NonterminalMap::const_iterator it = m_nonterminal_map->begin(),
                                            it_end = m_nonterminal_map->end();
             it != it_end;
             ++it)
        {
            string const &nonterminal_name = it->first;
            Nonterminal const *nonterminal = it->second;
            assert(nonterminal != NULL);
            npda_initial_node_index_symbol->SetMapElement(
                nonterminal_name,
                new Preprocessor::Body(
                    Sint32(nonterminal->GetNpdaGraphStartState()),
                    FiLoc::ms_invalid));
        }
    }

    // _npda_precedence_count -- gives the number of precedences
    //
    // _npda_precedence_index[precedence name] -- maps precedence name => precedence index
    // it should be noted that there is always a "DEFAULT_" precedence with index 0
    //
    // _npda_precedence_name[_npda_precedence_count] -- the name of this precedence
    //
    // _npda_precedence_level[_npda_precedence_count] -- the numeric precedence level; higher
    // level indicates higher precedence.  the "DEFAULT_" precedence has level 0.
    //
    // _npda_precedence_associativity_index[_npda_precedence_count] -- the numeric value of
    // the associativity of this precedence (or 1 if no explicit associativity was
    // supplied). the values are 0 for %left, 1 for %nonassoc, and 2 for %right.
    //
    // _npda_precedence_associativity_name[_npda_precedence_count] -- the named associativity
    // of this precedence (or "NONASSOC" if no explicit associativity was supplied).
    // the names are "LEFT" for %left, "NONASSOC" for %nonassoc, and "RIGHT" for
    // %right.
    {
        Preprocessor::ScalarSymbol *npda_precedence_count_symbol =
            symbol_table.DefineScalarSymbol("_npda_precedence_count", FiLoc::ms_invalid);
        npda_precedence_count_symbol->SetScalarBody(
            new Preprocessor::Body(
                Sint32(m_precedence_list->size()),
                FiLoc::ms_invalid));

        Preprocessor::MapSymbol *npda_precedence_index_symbol =
            symbol_table.DefineMapSymbol("_npda_precedence_index", FiLoc::ms_invalid);
        for (PrecedenceMap::const_iterator it = m_precedence_map->begin(),
                                           it_end = m_precedence_map->end();
             it != it_end;
             ++it)
        {
            string const &precedence_id = it->first;
            assert(it->second != NULL);
            Precedence const &precedence = *it->second;
            // the level and index are the same at the time trison runs
            npda_precedence_index_symbol->SetMapElement(
                precedence_id,
                new Preprocessor::Body(
                    precedence.m_precedence_level,
                    FiLoc::ms_invalid));
        }

        Preprocessor::ArraySymbol *npda_precedence_name_symbol =
            symbol_table.DefineArraySymbol("_npda_precedence_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_precedence_level_symbol =
            symbol_table.DefineArraySymbol("_npda_precedence_level", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_precedence_associativity_index_symbol =
            symbol_table.DefineArraySymbol("_npda_precedence_associativity_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_precedence_associativity_name_symbol =
            symbol_table.DefineArraySymbol("_npda_precedence_associativity_name", FiLoc::ms_invalid);
        for (PrecedenceList::const_iterator it = m_precedence_list->begin(),
                                            it_end = m_precedence_list->end();
             it != it_end;
             ++it)
        {
            assert(*it != NULL);
            Precedence const &precedence = **it;
            npda_precedence_name_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    precedence.m_precedence_id,
                    FiLoc::ms_invalid));
            npda_precedence_level_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    precedence.m_precedence_level,
                    FiLoc::ms_invalid));
            npda_precedence_associativity_index_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(precedence.m_precedence_associativity),
                    FiLoc::ms_invalid));
            npda_precedence_associativity_name_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    precedence.m_precedence_associativity == A_LEFT ? "LEFT" : (precedence.m_precedence_associativity == A_NONASSOC ? "NONASSOC" : "RIGHT"),
                    FiLoc::ms_invalid));
        }
    }

    // _rule_count -- gives the number of accept handlers.
    //
    // _npda_rule_reduction_nonterminal_index[_rule_count] -- the index of the nonterminal
    // which this rule reduces to.
    //
    // _npda_rule_reduction_nonterminal_name[_rule_count] -- the name of the nonterminal
    // which this rule reduces to.
    //
    // _npda_rule_precedence_index[_rule_count] -- the numeric value of the
    // precedence of the rule which this state is a part of (0 if no explicit
    // precedence was supplied).
    //
    // _npda_rule_precedence_name[_rule_count] -- the named precedence of the
    // rule which this state is a part of ("DEFAULT_" if no explicit precedence was supplied).
    //
    // _npda_rule_token_count[_rule_count] -- the number of tokens in this rule
    //
    // _npda_rule_description[_rule_count] -- the textual description of
    // this rule (e.g. "exp <- exp '+' exp")
    {
        Preprocessor::ScalarSymbol *rule_count_symbol =
            symbol_table.DefineScalarSymbol("_rule_count", FiLoc::ms_invalid);
        rule_count_symbol->SetScalarBody(
            new Preprocessor::Body(
                Sint32(GetRuleCount()),
                FiLoc::ms_invalid));

        Preprocessor::ArraySymbol *npda_rule_reduction_nonterminal_index_symbol =
            symbol_table.DefineArraySymbol("_npda_rule_reduction_nonterminal_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_rule_reduction_nonterminal_name_symbol =
            symbol_table.DefineArraySymbol("_npda_rule_reduction_nonterminal_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_rule_precedence_index_symbol =
            symbol_table.DefineArraySymbol("_npda_rule_precedence_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_rule_precedence_name_symbol =
            symbol_table.DefineArraySymbol("_npda_rule_precedence_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_rule_token_count_symbol =
            symbol_table.DefineArraySymbol("_npda_rule_token_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_rule_description_symbol =
            symbol_table.DefineArraySymbol("_npda_rule_description", FiLoc::ms_invalid);

        for (Uint32 i = 0; i < GetRuleCount(); ++i)
        {
            Rule const &rule = *GetRule(i);

            npda_rule_reduction_nonterminal_index_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(GetTokenIndex(rule.m_owner_nonterminal->m_id)),
                    FiLoc::ms_invalid));
            npda_rule_reduction_nonterminal_name_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    rule.m_owner_nonterminal->m_id,
                    FiLoc::ms_invalid));

            Precedence const *rule_precedence = m_precedence_map->GetElement(rule.m_rule_precedence_id);
            assert(rule_precedence != NULL);
            npda_rule_precedence_index_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(rule_precedence->m_precedence_level),
                    FiLoc::ms_invalid));
            npda_rule_precedence_name_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    rule.m_rule_precedence_id,
                    FiLoc::ms_invalid));

            npda_rule_token_count_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(rule.m_rule_token_list->size()),
                    FiLoc::ms_invalid));

            npda_rule_description_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    GetStringLiteral(rule.GetAsText()),
                    FiLoc::ms_invalid));
        }
    }

    // _npda_node_count -- the number of nodes in the nondeterministic pushdown automaton (NPDA)
    {
        Preprocessor::ScalarSymbol *npda_node_count_symbol =
            symbol_table.DefineScalarSymbol("_npda_node_count", FiLoc::ms_invalid);
        npda_node_count_symbol->SetScalarBody(
            new Preprocessor::Body(
                Sint32(m_npda_graph.GetNodeCount()),
                FiLoc::ms_invalid));
    }

    // _npda_node_rule_index[_npda_node_count] -- the index of the rule which this state
    // is a part of, or _rule_count if this state is not part of a rule.
    //
    // _npda_node_rule_stage[_npda_node_count] -- the stage (e.g. 0 indicates no tokens have
    // been accepted, 1 indicates 1 token, etc) of the rule which this state is a
    // part of, or 0 (really is undefined) if this state is not part of a rule.
    //
    // _npda_node_description[_npda_node_count] -- the description of this node (e.g.
    // "start using xxx" for a start state, "return xxx" for a return state,
    // "exp <- exp . '+' exp" if a part of a rule, or "head of xxx" for a nonterminal
    // head state).
    //
    // _npda_node_nonterminal_index[_rule_count] -- the index of the nonterminal
    // which this node is a part of, or 0 if there is no associated nonterminal.
    //
    // _npda_node_nonterminal_name[_rule_count] -- the name of the nonterminal
    // which this node is a part of, or "none_" if there is no associated nonterminal.
    //
    // _npda_node_transition_offset[_npda_node_count] -- gives the first index of the
    // contiguous set of transitions for this node.
    //
    // _npda_node_transition_count[_npda_node_count] -- gives the number of transitions for
    // this node (the number of contiguous transitions which apply to this node).
    //
    // _npda_transition_count -- gives the number of transitions in this NPDA.
    //
    // _npda_transition_type_index[_npda_transition_count] -- gives the integer value
    // of the transition type.  valid values are 0 (TT_EPSILON), 1 (TT_REDUCE),
    // 2 (TT_RETURN), and 3 (TT_SHIFT)
    //
    // _npda_transition_type_name[_npda_transition_count] -- gives the text name
    // of the transition type (i.e. "TT_EPSILON", etc).
    //
    // _npda_transition_data_index[_npda_transition_count] gives the numeric value of the
    // token which this transition accepts if the transition type is TT_SHIFT,
    // otherwise this value is undefined.
    //
    // _npda_transition_data_name[_npda_transition_count] gives the name of the
    // token which this transition accepts if the transition type is TT_SHIFT,
    // otherwise this value is undefined.
    //
    // _npda_transition_target_node_index[_npda_transition_count] gives the index of
    // the node which to transition to if this transition is exercised, or -1 if
    // not applicable.
    {
        Preprocessor::ArraySymbol *npda_node_rule_index_symbol =
            symbol_table.DefineArraySymbol("_npda_node_rule_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_node_rule_stage_symbol =
            symbol_table.DefineArraySymbol("_npda_node_rule_stage", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_node_description_symbol =
            symbol_table.DefineArraySymbol("_npda_node_description", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_node_nonterminal_index_symbol =
            symbol_table.DefineArraySymbol("_npda_node_nonterminal_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_node_nonterminal_name_symbol =
            symbol_table.DefineArraySymbol("_npda_node_nonterminal_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_node_is_return_state_symbol =
            symbol_table.DefineArraySymbol("_npda_node_is_return_state", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_node_is_epsilon_closure_state_symbol =
            symbol_table.DefineArraySymbol("_npda_node_is_epsilon_closure_state", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_node_transition_offset_symbol =
            symbol_table.DefineArraySymbol("_npda_node_transition_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_node_transition_count_symbol =
            symbol_table.DefineArraySymbol("_npda_node_transition_count", FiLoc::ms_invalid);
        Preprocessor::ScalarSymbol *npda_transition_count_symbol =
            symbol_table.DefineScalarSymbol("_npda_transition_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_transition_type_index_symbol =
            symbol_table.DefineArraySymbol("_npda_transition_type_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_transition_type_name_symbol =
            symbol_table.DefineArraySymbol("_npda_transition_type_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_transition_data_index_symbol =
            symbol_table.DefineArraySymbol("_npda_transition_data_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_transition_data_name_symbol =
            symbol_table.DefineArraySymbol("_npda_transition_data_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_transition_target_node_index_symbol =
            symbol_table.DefineArraySymbol("_npda_transition_target_node_index", FiLoc::ms_invalid);

        Sint32 total_transition_count = 0;

        for (Uint32 node_index = 0; node_index < m_npda_graph.GetNodeCount(); ++node_index)
        {
            Graph::Node const &node = m_npda_graph.GetNode(node_index);
            NpdaNodeData const &node_data = node.GetDataAs<NpdaNodeData>();
            Sint32 node_transition_offset = total_transition_count;
            Sint32 node_transition_count = 0;

            for (Graph::Node::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
                                                            it_end = node.GetTransitionSetEnd();
                 it != it_end;
                 ++it)
            {
                Graph::Transition const &transition = *it;

                npda_transition_type_index_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.Type()),
                        FiLoc::ms_invalid));
                npda_transition_type_name_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        GetTransitionTypeString(transition.Type()),
                        FiLoc::ms_invalid));
                npda_transition_data_index_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.Data0()),
                        FiLoc::ms_invalid));
                npda_transition_data_name_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        transition.Label(),
                        FiLoc::ms_invalid));
                npda_transition_target_node_index_symbol->AppendArrayElement(
                    new Preprocessor::Body(
                        Sint32(transition.TargetIndex()),
                        FiLoc::ms_invalid));

                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++node_transition_count;
                assert(total_transition_count < SINT32_UPPER_BOUND);
                ++total_transition_count;
            }

            // figure out all the values for _npda_node_*
            Nonterminal const *nonterminal = node_data.GetAssociatedNonterminal();
            Rule const *rule = node_data.GetAssociatedRule();
            npda_node_rule_index_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    rule != NULL ? Sint32(rule->GetAcceptStateIndex()) : GetRuleCount(),
                    FiLoc::ms_invalid));
            npda_node_rule_stage_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    rule != NULL ? Sint32(node_data.GetRuleStage()) : 0,
                    FiLoc::ms_invalid));
            npda_node_description_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    GetStringLiteral(node_data.GetDescription()),
                    FiLoc::ms_invalid));
            npda_node_nonterminal_index_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(nonterminal != NULL ? GetTokenIndex(nonterminal->m_id) : 0),
                    FiLoc::ms_invalid));
            npda_node_nonterminal_name_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    nonterminal != NULL ? nonterminal->m_id : "none_",
                    FiLoc::ms_invalid));
            npda_node_is_return_state_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(node_data.IsReturnState() ? 1 : 0),
                    FiLoc::ms_invalid));
            npda_node_is_epsilon_closure_state_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(node_data.IsEpsilonClosureState() ? 1 : 0),
                    FiLoc::ms_invalid));
            npda_node_transition_offset_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(node_transition_offset),
                    FiLoc::ms_invalid));
            npda_node_transition_count_symbol->AppendArrayElement(
                new Preprocessor::Body(
                    Sint32(node_transition_count),
                    FiLoc::ms_invalid));
        }

        npda_transition_count_symbol->SetScalarBody(
            new Preprocessor::Body(
                Sint32(total_transition_count),
                FiLoc::ms_invalid));
    }
}

void Representation::GenerateTargetLanguageDependentSymbols (
    string const &target_language_id,
    Preprocessor::SymbolTable &symbol_table) const
{
    // _accept_handler_code[_rule_count] -- specifies code for all accept handlers.
    {
        Preprocessor::ArraySymbol *accept_handler_code_symbol =
            symbol_table.DefineArraySymbol("_accept_handler_code", FiLoc::ms_invalid);

        for (NonterminalMap::const_iterator it = m_nonterminal_map->begin(),
                                            it_end = m_nonterminal_map->end();
             it != it_end;
             ++it)
        {
            Nonterminal const *nonterminal = it->second;
            assert(nonterminal != NULL);
            nonterminal->PopulateAcceptHandlerCodeArraySymbol(
                target_language_id,
                accept_handler_code_symbol);
        }
    }
}

void Representation::AssignRuleIndices ()
{
    for (Uint32 i = 0, s = GetRuleCount(); i < s; ++i)
    {
        Rule const *rule = GetRule(i);
        assert(rule != NULL);
        rule->SetAcceptStateIndex(i);
    }
}

void Representation::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, GetAstTypeString, indent_level);
}

void Representation::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    AstCommon::Ast::Print(stream, Stringify, indent_level);
    m_target_language_map->Print(stream, Stringify, indent_level+1);
    m_token_map->Print(stream, Stringify, indent_level+1);
    m_precedence_map->Print(stream, Stringify, indent_level+1);
    stream << Tabs(indent_level+1) << "start nonterminal: " << m_start_nonterminal_id << endl;
    m_nonterminal_list->Print(stream, Stringify, indent_level+1);
}

} // end of namespace Trison
