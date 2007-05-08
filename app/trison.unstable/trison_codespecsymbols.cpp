// ///////////////////////////////////////////////////////////////////////////
// trison_codespecsymbols.cpp by Victor Dods, created 2007/05/06
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_codespecsymbols.hpp"

#include "barf_graph.hpp"
#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_symboltable.hpp"
#include "trison_ast.hpp"
#include "trison_graph.hpp"
#include "trison_npda.hpp"

namespace Trison {

void PopulateRuleCodeArraySymbol (Rule const &rule, string const &target_id, Preprocessor::ArraySymbol *rule_code_symbol)
{
    assert(rule_code_symbol != NULL);
    CommonLang::RuleHandler const *rule_handler = rule.m_rule_handler_map->GetElement(target_id);
    if (rule_handler != NULL)
    {
        Ast::CodeBlock const *rule_handler_code_block = rule_handler->m_rule_handler_code_block;
        assert(rule_handler_code_block != NULL);
        rule_code_symbol->AppendArrayElement(
            new Preprocessor::Body(
                rule_handler_code_block->GetText(),
                rule_handler_code_block->GetFiLoc()));
    }
    else
        rule_code_symbol->AppendArrayElement(new Preprocessor::Body(""));
}

void PopulateRuleCodeArraySymbol (Nonterminal const &nonterminal, string const &target_id, Preprocessor::ArraySymbol *rule_code_symbol)
{
    assert(rule_code_symbol != NULL);

    for (RuleList::const_iterator it = nonterminal.m_rule_list->begin(),
                                  it_end = nonterminal.m_rule_list->end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        PopulateRuleCodeArraySymbol(*rule, target_id, rule_code_symbol);
    }
}

void GenerateGeneralAutomatonSymbols (PrimarySource const &primary_source, Preprocessor::SymbolTable &symbol_table)
{
    // _rule_count -- gives the total number of rules (from all nonterminals combined).
    Preprocessor::ScalarSymbol *rule_count_symbol =
        symbol_table.DefineScalarSymbol("_rule_count", FiLoc::ms_invalid);
    rule_count_symbol->SetScalarBody(
        new Preprocessor::Body(Sint32(primary_source.GetRuleCount())));
}

void GenerateNpdaSymbols (PrimarySource const &primary_source, Graph const &npda_graph, Preprocessor::SymbolTable &symbol_table)
{
    assert(npda_graph.GetNodeCount() > 0);

    // _default_parse_nonterminal -- value of %default_parse_nonterminal -- the name of the default default parse nonterminal
    {
        Preprocessor::ScalarSymbol *symbol =
            symbol_table.DefineScalarSymbol("_default_parse_nonterminal", FiLoc::ms_invalid);
        symbol->SetScalarBody(
            new Preprocessor::Body(primary_source.m_default_parse_nonterminal_id));
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

        // add the special END_ terminal token
        terminal_index_list_symbol->AppendArrayElement(
            new Preprocessor::Body(Sint32(token_index++)));
        terminal_name_list_symbol->AppendArrayElement(
            new Preprocessor::Body("END_"));

        for (TerminalMap::const_iterator it = primary_source.m_terminal_map->begin(), it_end = primary_source.m_terminal_map->end();
             it != it_end;
             ++it)
        {
            Terminal const *terminal = it->second;
            assert(terminal != NULL);
            if (terminal->m_is_id)
            {
                terminal_index_list_symbol->AppendArrayElement(
                    new Preprocessor::Body(Sint32(token_index++)));
                terminal_name_list_symbol->AppendArrayElement(
                    new Preprocessor::Body(terminal->GetText()));
            }
        }

        nonterminal_index_list_symbol->AppendArrayElement(
            new Preprocessor::Body(Sint32(0)));
        nonterminal_name_list_symbol->AppendArrayElement(
            new Preprocessor::Body("none_"));

        for (NonterminalMap::const_iterator it = primary_source.m_nonterminal_map->begin(), it_end = primary_source.m_nonterminal_map->end();
             it != it_end;
             ++it)
        {
            Nonterminal const *nonterminal = it->second;
            assert(nonterminal != NULL);
            nonterminal_index_list_symbol->AppendArrayElement(
                new Preprocessor::Body(Sint32(token_index++)));
            nonterminal_name_list_symbol->AppendArrayElement(
                new Preprocessor::Body(nonterminal->GetText()));
        }
    }

    // _npda_nonterminal_start_state_index[nonterminal name] -- maps nonterminal name => node index
    {
        Preprocessor::MapSymbol *npda_nonterminal_start_state_index_symbol =
            symbol_table.DefineMapSymbol("_npda_nonterminal_start_state_index", FiLoc::ms_invalid);
        for (NonterminalMap::const_iterator it = primary_source.m_nonterminal_map->begin(),
                                            it_end = primary_source.m_nonterminal_map->end();
             it != it_end;
             ++it)
        {
            string const &nonterminal_name = it->first;
            Nonterminal const *nonterminal = it->second;
            assert(nonterminal != NULL);
            npda_nonterminal_start_state_index_symbol->SetMapElement(
                nonterminal_name,
                new Preprocessor::Body(Sint32(nonterminal->GetNpdaGraphStartState())));
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
            new Preprocessor::Body(Sint32(primary_source.m_precedence_list->size())));

        Preprocessor::MapSymbol *npda_precedence_index_symbol =
            symbol_table.DefineMapSymbol("_npda_precedence_index", FiLoc::ms_invalid);
        for (PrecedenceMap::const_iterator it = primary_source.m_precedence_map->begin(),
                                           it_end = primary_source.m_precedence_map->end();
             it != it_end;
             ++it)
        {
            string const &precedence_id = it->first;
            assert(it->second != NULL);
            Precedence const &precedence = *it->second;
            // the level and index are the same at the time trison runs
            npda_precedence_index_symbol->SetMapElement(
                precedence_id,
                new Preprocessor::Body(precedence.m_precedence_level));
        }

        Preprocessor::ArraySymbol *npda_precedence_name_symbol =
            symbol_table.DefineArraySymbol("_npda_precedence_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_precedence_level_symbol =
            symbol_table.DefineArraySymbol("_npda_precedence_level", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_precedence_associativity_index_symbol =
            symbol_table.DefineArraySymbol("_npda_precedence_associativity_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_precedence_associativity_name_symbol =
            symbol_table.DefineArraySymbol("_npda_precedence_associativity_name", FiLoc::ms_invalid);
        for (PrecedenceList::const_iterator it = primary_source.m_precedence_list->begin(),
                                            it_end = primary_source.m_precedence_list->end();
             it != it_end;
             ++it)
        {
            assert(*it != NULL);
            Precedence const &precedence = **it;
            npda_precedence_name_symbol->AppendArrayElement(
                new Preprocessor::Body(precedence.m_precedence_id));
            npda_precedence_level_symbol->AppendArrayElement(
                new Preprocessor::Body(precedence.m_precedence_level));
            npda_precedence_associativity_index_symbol->AppendArrayElement(
                new Preprocessor::Body(Sint32(precedence.m_precedence_associativity)));
            npda_precedence_associativity_name_symbol->AppendArrayElement(
                new Preprocessor::Body(precedence.m_precedence_associativity == A_LEFT ? "LEFT" : (precedence.m_precedence_associativity == A_NONASSOC ? "NONASSOC" : "RIGHT")));
        }
    }

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

        for (Uint32 i = 0; i < primary_source.GetRuleCount(); ++i)
        {
            Rule const &rule = *primary_source.GetRule(i);

            npda_rule_reduction_nonterminal_index_symbol->AppendArrayElement(
                new Preprocessor::Body(Sint32(primary_source.GetTokenIndex(rule.m_owner_nonterminal->GetText()))));
            npda_rule_reduction_nonterminal_name_symbol->AppendArrayElement(
                new Preprocessor::Body(rule.m_owner_nonterminal->GetText()));

            Precedence const *rule_precedence = primary_source.m_precedence_map->GetElement(rule.m_rule_precedence_id);
            assert(rule_precedence != NULL);
            npda_rule_precedence_index_symbol->AppendArrayElement(
                new Preprocessor::Body(Sint32(rule_precedence->m_precedence_level)));
            npda_rule_precedence_name_symbol->AppendArrayElement(
                new Preprocessor::Body(rule.m_rule_precedence_id));

            npda_rule_token_count_symbol->AppendArrayElement(
                new Preprocessor::Body(Sint32(rule.m_rule_token_list->size())));

            npda_rule_description_symbol->AppendArrayElement(
                new Preprocessor::Body(GetStringLiteral(rule.GetAsText())));
        }
    }

    // _npda_state_count -- the number of nodes in the nondeterministic pushdown automaton (NPDA)
    {
        Preprocessor::ScalarSymbol *npda_state_count_symbol =
            symbol_table.DefineScalarSymbol("_npda_state_count", FiLoc::ms_invalid);
        npda_state_count_symbol->SetScalarBody(
            new Preprocessor::Body(Sint32(npda_graph.GetNodeCount())));
    }

    // _npda_state_rule_index[_npda_state_count] -- the index of the rule which this state
    // is a part of, or _rule_count if this state is not part of a rule.
    //
    // _npda_state_rule_stage[_npda_state_count] -- the stage (e.g. 0 indicates no tokens have
    // been accepted, 1 indicates 1 token, etc) of the rule which this state is a
    // part of, or 0 (really is undefined) if this state is not part of a rule.
    //
    // _npda_state_description[_npda_state_count] -- the description of this node (e.g.
    // "start using xxx" for a start state, "return xxx" for a return state,
    // "exp <- exp . '+' exp" if a part of a rule, or "head of xxx" for a nonterminal
    // head state).
    //
    // _npda_state_nonterminal_index[_rule_count] -- the index of the nonterminal
    // which this node is a part of, or 0 if there is no associated nonterminal.
    //
    // _npda_state_nonterminal_name[_rule_count] -- the name of the nonterminal
    // which this node is a part of, or "none_" if there is no associated nonterminal.
    //
    // _npda_state_transition_offset[_npda_state_count] -- gives the first index of the
    // contiguous set of transitions for this node.
    //
    // _npda_state_transition_count[_npda_state_count] -- gives the number of transitions for
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
        Preprocessor::ArraySymbol *npda_state_rule_index_symbol =
            symbol_table.DefineArraySymbol("_npda_state_rule_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_rule_stage_symbol =
            symbol_table.DefineArraySymbol("_npda_state_rule_stage", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_description_symbol =
            symbol_table.DefineArraySymbol("_npda_state_description", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_nonterminal_index_symbol =
            symbol_table.DefineArraySymbol("_npda_state_nonterminal_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_nonterminal_name_symbol =
            symbol_table.DefineArraySymbol("_npda_state_nonterminal_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_transition_offset_symbol =
            symbol_table.DefineArraySymbol("_npda_state_transition_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_transition_count_symbol =
            symbol_table.DefineArraySymbol("_npda_state_transition_count", FiLoc::ms_invalid);
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

        for (Uint32 node_index = 0; node_index < npda_graph.GetNodeCount(); ++node_index)
        {
            Graph::Node const &node = npda_graph.GetNode(node_index);
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
                    new Preprocessor::Body(Sint32(transition.Type())));
                npda_transition_type_name_symbol->AppendArrayElement(
                    new Preprocessor::Body(GetTransitionTypeString(transition.Type())));
                npda_transition_data_index_symbol->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data0())));
                npda_transition_data_name_symbol->AppendArrayElement(
                    new Preprocessor::Body(transition.Label()));
                npda_transition_target_node_index_symbol->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.TargetIndex())));

                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++node_transition_count;
                assert(total_transition_count < SINT32_UPPER_BOUND);
                ++total_transition_count;
            }

            // figure out all the values for _npda_state_*
            Nonterminal const *nonterminal = node_data.GetAssociatedNonterminal();
            Rule const *rule = node_data.GetAssociatedRule();
            npda_state_rule_index_symbol->AppendArrayElement(
                new Preprocessor::Body(rule != NULL ? Sint32(rule->m_rule_index) : primary_source.GetRuleCount()));
            npda_state_rule_stage_symbol->AppendArrayElement(
                new Preprocessor::Body(rule != NULL ? Sint32(node_data.GetRuleStage()) : 0));
            npda_state_description_symbol->AppendArrayElement(
                new Preprocessor::Body(GetStringLiteral(node_data.GetDescription())));
            npda_state_nonterminal_index_symbol->AppendArrayElement(
                new Preprocessor::Body(Sint32(nonterminal != NULL ? primary_source.GetTokenIndex(nonterminal->GetText()) : 0)));
            npda_state_nonterminal_name_symbol->AppendArrayElement(
                new Preprocessor::Body(nonterminal != NULL ? nonterminal->GetText() : "none_"));
            npda_state_transition_offset_symbol->AppendArrayElement(
                new Preprocessor::Body(Sint32(node_transition_offset)));
            npda_state_transition_count_symbol->AppendArrayElement(
                new Preprocessor::Body(Sint32(node_transition_count)));
        }

        npda_transition_count_symbol->SetScalarBody(
            new Preprocessor::Body(Sint32(total_transition_count)));
    }
}

void GenerateDpdaSymbols (PrimarySource const &primary_source, Graph const &dpda_graph, Preprocessor::SymbolTable &symbol_table)
{
    // not implemented yet
}

void GenerateTargetDependentSymbols(PrimarySource const &primary_source, string const &target_id, Preprocessor::SymbolTable &symbol_table)
{
    // _rule_code[_rule_count] -- specifies code for each rule.
    Preprocessor::ArraySymbol *rule_code_symbol =
        symbol_table.DefineArraySymbol("_rule_code", FiLoc::ms_invalid);

    for (NonterminalMap::const_iterator it = primary_source.m_nonterminal_map->begin(),
                                        it_end = primary_source.m_nonterminal_map->end();
         it != it_end;
         ++it)
    {
        Nonterminal const *nonterminal = it->second;
        assert(nonterminal != NULL);
        PopulateRuleCodeArraySymbol(*nonterminal, target_id, rule_code_symbol);
    }
}

} // end of namespace Trison
