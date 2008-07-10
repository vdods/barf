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
#include "trison_dpda.hpp"
#include "trison_graph.hpp"
#include "trison_npda.hpp"

namespace Trison {

void PopulateRuleCodeArraySymbol (Rule const &rule, string const &target_id, Preprocessor::ArraySymbol *rule_code)
{
    assert(rule_code != NULL);
    CommonLang::RuleHandler const *rule_handler = rule.m_rule_handler_map->GetElement(target_id);
    if (rule_handler != NULL)
    {
        Ast::CodeBlock const *rule_handler_code_block = rule_handler->m_rule_handler_code_block;
        assert(rule_handler_code_block != NULL);
        rule_code->AppendArrayElement(
            new Preprocessor::Body(
                rule_handler_code_block->GetText(),
                rule_handler_code_block->GetFiLoc()));
    }
    else
        rule_code->AppendArrayElement(new Preprocessor::Body(""));
}

void GenerateGeneralAutomatonSymbols (PrimarySource const &primary_source, Preprocessor::SymbolTable &symbol_table)
{
    // _rule_count -- gives the total number of rules (from all nonterminals combined).
    //
    // _rule_total_token_count -- specifies the total number of rule tokens from all
    // rules, summed together.
    //
    // _rule_token_assigned_id[_rule_total_token_count] -- a contiguous array of
    // all the token identifiers for the rule tokens in all rules.  i.e.
    // elements 0 - m will correspond to the rule token ids in rule 0, elements
    // m+1 - n will correspond to the rule token ids in rule 1, etc.  if
    // no identifier was given for a particular token, it will be the empty string.
    //
    // _rule_token_table_offset[_rule_count]
    //
    // _rule_token_table_count[_rule_count]
    {
        {
            Preprocessor::ScalarSymbol *rule_count =
                symbol_table.DefineScalarSymbol("_rule_count", FiLoc::ms_invalid);
            rule_count->SetScalarBody(
                new Preprocessor::Body(Sint32(primary_source.GetRuleCount())));
        }

        Preprocessor::ScalarSymbol *rule_total_token_count =
            symbol_table.DefineScalarSymbol("_rule_total_token_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *rule_token_assigned_id =
            symbol_table.DefineArraySymbol("_rule_token_assigned_id", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *rule_token_table_offset =
            symbol_table.DefineArraySymbol("_rule_token_table_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *rule_token_table_count =
            symbol_table.DefineArraySymbol("_rule_token_table_count", FiLoc::ms_invalid);


        Uint32 rule_total_token_count_value = primary_source.GetRuleTokenCount();
        rule_total_token_count->SetScalarBody(
            new Preprocessor::Body(Sint32(rule_total_token_count_value)));
        for (Uint32 i = 0; i < rule_total_token_count_value; ++i)
        {
            RuleToken const *rule_token = primary_source.GetRuleToken(i);
            assert(rule_token != NULL);
            rule_token_assigned_id->AppendArrayElement(
                new Preprocessor::Body(rule_token->m_assigned_id));
        }

        Uint32 rule_count = primary_source.GetRuleCount();
        Uint32 token_table_offset = 0;
        for (Uint32 i = 0; i < rule_count; ++i)
        {
            Rule const *rule = primary_source.GetRule(i);
            assert(rule != NULL);
            assert(rule->m_rule_token_list != NULL);
            rule_token_table_offset->AppendArrayElement(
                new Preprocessor::Body(Sint32(token_table_offset)));
            rule_token_table_count->AppendArrayElement(
                new Preprocessor::Body(Sint32(rule->m_rule_token_list->size())));
            token_table_offset += rule->m_rule_token_list->size();
        }
    }

    // _default_parse_nonterminal -- value of %default_parse_nonterminal -- the
    // name of the default default parse nonterminal
    {
        Preprocessor::ScalarSymbol *symbol =
            symbol_table.DefineScalarSymbol("_default_parse_nonterminal", FiLoc::ms_invalid);
        symbol->SetScalarBody(
            new Preprocessor::Body(primary_source.m_default_parse_nonterminal_id));
    }

    // _terminal_index_list[terminal count] -- list of terminal token values
    // (END_ is 0x100, ERROR_ is 0x101, then all non-char terminals are next
    // contiguously)
    //
    // _terminal_name_list[terminal count] -- list of terminal token names
    // (END_, _ERROR, and then all non-char terminals)
    //
    // _nonterminal_index_list[nonterminal count] -- list of nonterminal token values;
    // they start where _terminal_index_list left off.
    //
    // _nonterminal_name_list[nonterminal count] -- list of nonterminal token names
    {
        Preprocessor::ArraySymbol *terminal_index_list =
            symbol_table.DefineArraySymbol("_terminal_index_list", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *terminal_name_list =
            symbol_table.DefineArraySymbol("_terminal_name_list", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nonterminal_index_list =
            symbol_table.DefineArraySymbol("_nonterminal_index_list", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *nonterminal_name_list =
            symbol_table.DefineArraySymbol("_nonterminal_name_list", FiLoc::ms_invalid);

        // these asserts aren't really necessary, but we want to make sure the special
        // "END_" and "ERROR_" terminals were added with specific token_index values (though
        // the specific values aren't actually that important either).
        assert(primary_source.m_terminal_map->GetElement("END_") != NULL);
        assert(primary_source.m_terminal_map->GetElement("END_")->m_token_index == 0x100);
        assert(primary_source.m_terminal_map->GetElement("ERROR_") != NULL);
        assert(primary_source.m_terminal_map->GetElement("ERROR_")->m_token_index == 0x101);

        for (TerminalList::const_iterator it = primary_source.m_terminal_list->begin(), it_end = primary_source.m_terminal_list->end();
             it != it_end;
             ++it)
        {
            Terminal const *terminal = *it;
            assert(terminal != NULL);
            if (terminal->m_is_id)
            {
                terminal_index_list->AppendArrayElement(
                    new Preprocessor::Body(terminal->m_token_index));
                terminal_name_list->AppendArrayElement(
                    new Preprocessor::Body(terminal->GetText()));
            }
        }

        // this assert isn't really necessary, but we want to make sure the special
        // "none_" nonterminal was added with specific token_index value of 0 (this
        // value is important).
        assert(primary_source.m_nonterminal_map->GetElement("none_") != NULL);
        assert(primary_source.m_nonterminal_map->GetElement("none_")->m_token_index == 0);

        for (NonterminalList::const_iterator it = primary_source.m_nonterminal_list->begin(), it_end = primary_source.m_nonterminal_list->end();
             it != it_end;
             ++it)
        {
            Nonterminal const *nonterminal = *it;
            assert(nonterminal != NULL);
            nonterminal_index_list->AppendArrayElement(
                new Preprocessor::Body(nonterminal->m_token_index));
            nonterminal_name_list->AppendArrayElement(
                new Preprocessor::Body(nonterminal->GetText()));
        }
    }

    // _precedence_count -- gives the number of precedences
    //
    // _precedence_index[precedence name] -- maps precedence name => precedence index
    // it should be noted that there is always a "DEFAULT_" precedence with index 0
    //
    // _precedence_name[_precedence_count] -- the name of this precedence
    //
    // _precedence_level[_precedence_count] -- the numeric precedence level; higher
    // level indicates higher precedence.  the "DEFAULT_" precedence has level 0.
    //
    // _precedence_associativity_index[_precedence_count] -- the numeric value of
    // the associativity of this precedence (or 1 if no explicit associativity was
    // supplied). the values are 0 for %left, 1 for %nonassoc, and 2 for %right.
    //
    // _precedence_associativity_name[_precedence_count] -- the named associativity
    // of this precedence (or "NONASSOC" if no explicit associativity was supplied).
    // the names are "LEFT" for %left, "NONASSOC" for %nonassoc, and "RIGHT" for
    // %right.
    {
        Preprocessor::ScalarSymbol *precedence_count =
            symbol_table.DefineScalarSymbol("_precedence_count", FiLoc::ms_invalid);
        precedence_count->SetScalarBody(
            new Preprocessor::Body(Sint32(primary_source.m_precedence_list->size())));

        Preprocessor::MapSymbol *precedence_index =
            symbol_table.DefineMapSymbol("_precedence_index", FiLoc::ms_invalid);
        for (PrecedenceMap::const_iterator it = primary_source.m_precedence_map->begin(),
                                           it_end = primary_source.m_precedence_map->end();
             it != it_end;
             ++it)
        {
            string const &precedence_id = it->first;
            assert(it->second != NULL);
            Precedence const &precedence = *it->second;
            // the level and index are the same at the time trison runs
            precedence_index->SetMapElement(
                precedence_id,
                new Preprocessor::Body(precedence.m_precedence_level));
        }

        Preprocessor::ArraySymbol *precedence_name =
            symbol_table.DefineArraySymbol("_precedence_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *precedence_level =
            symbol_table.DefineArraySymbol("_precedence_level", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *precedence_associativity_index =
            symbol_table.DefineArraySymbol("_precedence_associativity_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *precedence_associativity_name =
            symbol_table.DefineArraySymbol("_precedence_associativity_name", FiLoc::ms_invalid);
        for (PrecedenceList::const_iterator it = primary_source.m_precedence_list->begin(),
                                            it_end = primary_source.m_precedence_list->end();
             it != it_end;
             ++it)
        {
            assert(*it != NULL);
            Precedence const &precedence = **it;
            precedence_name->AppendArrayElement(
                new Preprocessor::Body(precedence.m_precedence_id));
            precedence_level->AppendArrayElement(
                new Preprocessor::Body(precedence.m_precedence_level));
            precedence_associativity_index->AppendArrayElement(
                new Preprocessor::Body(Sint32(precedence.m_precedence_associativity)));
            precedence_associativity_name->AppendArrayElement(
                new Preprocessor::Body(precedence.m_precedence_associativity == A_LEFT ? "LEFT" : (precedence.m_precedence_associativity == A_NONASSOC ? "NONASSOC" : "RIGHT")));
        }
    }

    // _rule_reduction_nonterminal_index[_rule_count] -- the index of the nonterminal
    // which this rule reduces to.
    //
    // _rule_reduction_nonterminal_name[_rule_count] -- the name of the nonterminal
    // which this rule reduces to.
    //
    // _rule_precedence_index[_rule_count] -- the numeric value of the
    // precedence of the rule which this state is a part of (0 if no explicit
    // precedence was supplied).
    //
    // _rule_precedence_name[_rule_count] -- the named precedence of the
    // rule which this state is a part of ("DEFAULT_" if no explicit precedence was supplied).
    //
    // _rule_token_count[_rule_count] -- the number of tokens in this rule
    //
    // _rule_description[_rule_count] -- the textual description of
    // this rule (e.g. "exp <- exp '+' exp")
    {
        Preprocessor::ArraySymbol *rule_reduction_nonterminal_index =
            symbol_table.DefineArraySymbol("_rule_reduction_nonterminal_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *rule_reduction_nonterminal_name =
            symbol_table.DefineArraySymbol("_rule_reduction_nonterminal_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *rule_precedence_index =
            symbol_table.DefineArraySymbol("_rule_precedence_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *rule_precedence_name =
            symbol_table.DefineArraySymbol("_rule_precedence_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *rule_token_count =
            symbol_table.DefineArraySymbol("_rule_token_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *rule_description =
            symbol_table.DefineArraySymbol("_rule_description", FiLoc::ms_invalid);

        for (Uint32 i = 0; i < primary_source.GetRuleCount(); ++i)
        {
            Rule const &rule = *primary_source.GetRule(i);

            rule_reduction_nonterminal_index->AppendArrayElement(
                new Preprocessor::Body(Sint32(rule.m_owner_nonterminal->m_token_index)));
            rule_reduction_nonterminal_name->AppendArrayElement(
                new Preprocessor::Body(rule.m_owner_nonterminal->GetText()));

            rule_precedence_index->AppendArrayElement(
                new Preprocessor::Body(Sint32(rule.m_rule_precedence->m_precedence_level)));
            rule_precedence_name->AppendArrayElement(
                new Preprocessor::Body(rule.m_rule_precedence->m_precedence_id));

            rule_token_count->AppendArrayElement(
                new Preprocessor::Body(Sint32(rule.m_rule_token_list->size())));

            rule_description->AppendArrayElement(
                new Preprocessor::Body(GetStringLiteral(rule.GetAsText())));
        }
    }

}

void GenerateNpdaSymbols (PrimarySource const &primary_source, Graph const &npda_graph, Preprocessor::SymbolTable &symbol_table)
{
    assert(npda_graph.GetNodeCount() > 0);

    // _npda_nonterminal_start_state_index[nonterminal name] -- maps nonterminal name => node index
    {
        Preprocessor::MapSymbol *npda_nonterminal_start_state_index =
            symbol_table.DefineMapSymbol("_npda_nonterminal_start_state_index", FiLoc::ms_invalid);
        for (NonterminalMap::const_iterator it = primary_source.m_nonterminal_map->begin(),
                                            it_end = primary_source.m_nonterminal_map->end();
             it != it_end;
             ++it)
        {
            string const &nonterminal_name = it->first;
            Nonterminal const *nonterminal = it->second;
            assert(nonterminal != NULL);
            // we don't want to add the special "none_" nonterminal here.
            if (nonterminal->GetText() != "none_")
                npda_nonterminal_start_state_index->SetMapElement(
                    nonterminal_name,
                    new Preprocessor::Body(Sint32(nonterminal->GetNpdaGraphStartState())));
        }
    }

    // _npda_state_count -- the number of nodes in the nondeterministic pushdown automaton (NPDA)
    //
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
        Preprocessor::ScalarSymbol *npda_state_count =
            symbol_table.DefineScalarSymbol("_npda_state_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_rule_index =
            symbol_table.DefineArraySymbol("_npda_state_rule_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_rule_stage =
            symbol_table.DefineArraySymbol("_npda_state_rule_stage", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_description =
            symbol_table.DefineArraySymbol("_npda_state_description", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_nonterminal_index =
            symbol_table.DefineArraySymbol("_npda_state_nonterminal_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_nonterminal_name =
            symbol_table.DefineArraySymbol("_npda_state_nonterminal_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_transition_offset =
            symbol_table.DefineArraySymbol("_npda_state_transition_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_state_transition_count =
            symbol_table.DefineArraySymbol("_npda_state_transition_count", FiLoc::ms_invalid);

        Preprocessor::ScalarSymbol *npda_transition_count =
            symbol_table.DefineScalarSymbol("_npda_transition_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_transition_type_index =
            symbol_table.DefineArraySymbol("_npda_transition_type_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_transition_type_name =
            symbol_table.DefineArraySymbol("_npda_transition_type_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_transition_data_index =
            symbol_table.DefineArraySymbol("_npda_transition_data_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_transition_data_name =
            symbol_table.DefineArraySymbol("_npda_transition_data_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *npda_transition_target_node_index =
            symbol_table.DefineArraySymbol("_npda_transition_target_node_index", FiLoc::ms_invalid);

        npda_state_count->SetScalarBody(
            new Preprocessor::Body(Sint32(npda_graph.GetNodeCount())));

        Sint32 total_transition_count = 0;

        for (Uint32 node_index = 0; node_index < npda_graph.GetNodeCount(); ++node_index)
        {
            Graph::Node const &node = npda_graph.GetNode(node_index);
            NpdaNodeData const &node_data = node.GetDataAs<NpdaNodeData>();
            Sint32 node_transition_offset = total_transition_count;
            Sint32 node_transition_count = 0;

            for (Graph::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
                                                      it_end = node.GetTransitionSetEnd();
                 it != it_end;
                 ++it)
            {
                Graph::Transition const &transition = *it;

                npda_transition_type_index->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Type())));
                npda_transition_type_name->AppendArrayElement(
                    new Preprocessor::Body(GetTransitionTypeString(transition.Type())));
                npda_transition_data_index->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data(0))));
                npda_transition_data_name->AppendArrayElement(
                    new Preprocessor::Body(transition.Label()));
                npda_transition_target_node_index->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.TargetIndex())));

                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++node_transition_count;
                assert(total_transition_count < SINT32_UPPER_BOUND);
                ++total_transition_count;
            }

            // figure out all the values for _npda_state_*
            Nonterminal const *nonterminal = node_data.GetAssociatedNonterminal();
            Rule const *rule = node_data.GetAssociatedRule();
            npda_state_rule_index->AppendArrayElement(
                new Preprocessor::Body(rule != NULL ? Sint32(rule->m_rule_index) : primary_source.GetRuleCount()));
            npda_state_rule_stage->AppendArrayElement(
                new Preprocessor::Body(rule != NULL ? Sint32(node_data.GetRuleStage()) : 0));
            npda_state_description->AppendArrayElement(
                new Preprocessor::Body(GetStringLiteral(node_data.GetOneLineDescription())));
            npda_state_nonterminal_index->AppendArrayElement(
                new Preprocessor::Body(Sint32(nonterminal != NULL ? nonterminal->m_token_index : 0)));
            npda_state_nonterminal_name->AppendArrayElement(
                new Preprocessor::Body(nonterminal != NULL ? nonterminal->GetText() : "none_"));
            npda_state_transition_offset->AppendArrayElement(
                new Preprocessor::Body(Sint32(node_transition_offset)));
            npda_state_transition_count->AppendArrayElement(
                new Preprocessor::Body(Sint32(node_transition_count)));
        }

        npda_transition_count->SetScalarBody(
            new Preprocessor::Body(Sint32(total_transition_count)));
    }
}

void GenerateDpdaSymbols (PrimarySource const &primary_source, Graph const &dpda_graph, Preprocessor::SymbolTable &symbol_table)
{
    assert(dpda_graph.GetNodeCount() > 0);

    // _dpda_lalr_lookahead_count -- the number of lookaheads required for this LALR grammar
    {
//         Preprocessor::ScalarSymbol *dpda_lalr_lookahead_count =
//             symbol_table.DefineScalarSymbol("_dpda_lalr_lookahead_count", FiLoc::ms_invalid);
//         npda_state_count->SetScalarBody(
//             new Preprocessor::Body(Sint32(// TODOnpda_graph.GetNodeCount() )));
    }

    // _dpda_nonterminal_start_state_index[nonterminal name] -- maps nonterminal name => node index
    {
        Preprocessor::MapSymbol *dpda_nonterminal_start_state_index =
            symbol_table.DefineMapSymbol("_dpda_nonterminal_start_state_index", FiLoc::ms_invalid);
        for (NonterminalMap::const_iterator it = primary_source.m_nonterminal_map->begin(),
                                            it_end = primary_source.m_nonterminal_map->end();
             it != it_end;
             ++it)
        {
            string const &nonterminal_name = it->first;
            Nonterminal const *nonterminal = it->second;
            assert(nonterminal != NULL);
            // we don't want to add the special "none_" nonterminal here.
            if (nonterminal->GetText() != "none_")
                dpda_nonterminal_start_state_index->SetMapElement(
                    nonterminal_name,
                    new Preprocessor::Body(Sint32(nonterminal->GetDpdaGraphStartState())));
        }
    }

    // _dpda_state_count -- the number of nodes in the deterministic pushdown automaton (DPDA)
    //
    // _dpda_state_description[_dpda_state_count] -- the description of this node (e.g.
    // "start using xxx" for a start state, "return xxx" for a return state,
    // "exp <- exp . '+' exp" if a part of a rule, or "head of xxx" for a nonterminal
    // head state).
    //
    // _dpda_state_transition_offset[_dpda_state_count] -- gives the first index of the
    // contiguous set of transitions for this node, with the "default" transition
    // first.
    //
    // _dpda_state_transition_count[_dpda_state_count] -- gives the number of transitions for
    // this node (the number of contiguous transitions which apply to this node), which
    // always includes the "default" transition (so it is at least 1).
    //
    // _dpda_lookahead_count -- number of elements in _dpda_lookahead_index and _dpda_lookahead_name
    //
    // _dpda_lookahead_name[_dpda_lookahead_count] -- gives the identifier name (e.g.
    // '+', END_, etc) of the lookahead sequences.  this is a big array into which
    // the transitions point for their lookahead sequences.
    //
    // _dpda_lookahead_index[_dpda_lookahead_count] -- just like _dpda_lookahead_name
    // but uses the numerical index of the lookahead tokens.
    //
    // _dpda_transition_count -- gives the number of transitions in this DPDA.
    //
    // _dpda_transition_type_index[_dpda_transition_count] -- gives the integer value
    // of the transition type.  valid values are 1 (TT_REDUCE), 2 (TT_RETURN),
    // 3 (TT_SHIFT), 4 (TT_ERROR_PANIC) -- defined in trison_graph.hpp
    //
    // _dpda_transition_type_name[_dpda_transition_count] -- gives the text name
    // of the transition type (i.e. "TT_EPSILON", etc).
    //
    // _dpda_transition_data[_dpda_transition_count] -- if the transition type is
    // TT_SHIFT, then this is the target state index.  if the transition type is
    // TT_REDUCE, then this is the reduction rule index.  otherwise this is -1.
    //
    // _dpda_transition_lookahead_offset[_dpda_transition_count] -- gives the index
    // (into _dpda_lookahead_index and _dpda_lookahead_name) of the lookahead
    // sequence (contiguously).
    //
    // _dpda_transition_lookahead_count[_dpda_transition_count] -- gives the number
    // of lookaheads required by this transition; i.e. the number of lookaheads pointed
    // to by _dpda_transition_lookahead_offset
    {
        Preprocessor::ScalarSymbol *dpda_state_count =
            symbol_table.DefineScalarSymbol("_dpda_state_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dpda_state_description =
            symbol_table.DefineArraySymbol("_dpda_state_description", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dpda_state_transition_offset =
            symbol_table.DefineArraySymbol("_dpda_state_transition_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dpda_state_transition_count =
            symbol_table.DefineArraySymbol("_dpda_state_transition_count", FiLoc::ms_invalid);

        Preprocessor::ScalarSymbol *dpda_lookahead_count =
            symbol_table.DefineScalarSymbol("_dpda_lookahead_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dpda_lookahead_name =
            symbol_table.DefineArraySymbol("_dpda_lookahead_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dpda_lookahead_index =
            symbol_table.DefineArraySymbol("_dpda_lookahead_index", FiLoc::ms_invalid);

        Preprocessor::ScalarSymbol *dpda_transition_count =
            symbol_table.DefineScalarSymbol("_dpda_transition_count", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dpda_transition_type_index =
            symbol_table.DefineArraySymbol("_dpda_transition_type_index", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dpda_transition_type_name =
            symbol_table.DefineArraySymbol("_dpda_transition_type_name", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dpda_transition_data =
            symbol_table.DefineArraySymbol("_dpda_transition_data", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dpda_transition_lookahead_offset =
            symbol_table.DefineArraySymbol("_dpda_transition_lookahead_offset", FiLoc::ms_invalid);
        Preprocessor::ArraySymbol *dpda_transition_lookahead_count =
            symbol_table.DefineArraySymbol("_dpda_transition_lookahead_count", FiLoc::ms_invalid);

        dpda_state_count->SetScalarBody(
            new Preprocessor::Body(Sint32(dpda_graph.GetNodeCount())));

        Sint32 total_lookahead_count = 0;
        Sint32 total_transition_count = 0;

        for (Uint32 node_index = 0; node_index < dpda_graph.GetNodeCount(); ++node_index)
        {
            cerr << "codespecsymbols'ing dpda node: " << node_index << endl;
            Graph::Node const &node = dpda_graph.GetNode(node_index);
            DpdaNodeData const &node_data = node.GetDataAs<DpdaNodeData>();
            Sint32 node_transition_offset = total_transition_count;
            Sint32 node_transition_count = 0;

            for (Graph::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
                                                      it_end = node.GetTransitionSetEnd();
                 it != it_end;
                 ++it)
            {
                Graph::Transition const &transition = *it;

                cerr << "  transition: " << GetTransitionTypeString(transition.Type());

                dpda_transition_type_index->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Type())));
                dpda_transition_type_name->AppendArrayElement(
                    new Preprocessor::Body(GetTransitionTypeString(transition.Type())));
                dpda_transition_data->AppendArrayElement(
                    new Preprocessor::Body(Sint32(transition.Data(0))));
                dpda_transition_lookahead_offset->AppendArrayElement(
                    new Preprocessor::Body(total_lookahead_count));

                if (transition.Type() == TT_SHIFT)
                {
                    dpda_transition_lookahead_count->AppendArrayElement(
                        new Preprocessor::Body(Sint32(transition.DataCount())));
                    for (Uint32 i = 0; i < transition.DataCount(); ++i)
                    {
                        cerr << ' ' << primary_source.GetTokenId(transition.Data(i));
                        dpda_lookahead_name->AppendArrayElement(
                            new Preprocessor::Body(primary_source.GetTokenId(transition.Data(i))));
                        dpda_lookahead_index->AppendArrayElement(
                            new Preprocessor::Body(Sint32(transition.Data(i))));
                        ++total_lookahead_count;
                    }
                }
                else if (transition.Type() == TT_REDUCE)
                {
                    // -1 because the reduce lookaheads come after element 0.
                    dpda_transition_lookahead_count->AppendArrayElement(
                        new Preprocessor::Body(Sint32(transition.DataCount())-1));
                    for (Uint32 i = 1; i < transition.DataCount(); ++i)
                    {
                        cerr << ' ' << primary_source.GetTokenId(transition.Data(i));
                        dpda_lookahead_name->AppendArrayElement(
                            new Preprocessor::Body(primary_source.GetTokenId(transition.Data(i))));
                        dpda_lookahead_index->AppendArrayElement(
                            new Preprocessor::Body(Sint32(transition.Data(i))));
                        ++total_lookahead_count;
                    }
                }
                else
                {
                    // dummy value in dpda_transition_lookahead_count
                    dpda_transition_lookahead_count->AppendArrayElement(
                        new Preprocessor::Body(Sint32(0)));
                }
                cerr << endl;

                assert(node_transition_count < SINT32_UPPER_BOUND);
                ++node_transition_count;
                assert(total_transition_count < SINT32_UPPER_BOUND);
                ++total_transition_count;
            }

            // figure out all the values for _dpda_state_*
            dpda_state_description->AppendArrayElement(
                new Preprocessor::Body(GetStringLiteral(node_data.GetDescription())));
            dpda_state_transition_offset->AppendArrayElement(
                new Preprocessor::Body(Sint32(node_transition_offset)));
            dpda_state_transition_count->AppendArrayElement(
                new Preprocessor::Body(Sint32(node_transition_count)));
        }

        dpda_lookahead_count->SetScalarBody(
            new Preprocessor::Body(Sint32(total_lookahead_count)));
        dpda_transition_count->SetScalarBody(
            new Preprocessor::Body(Sint32(total_transition_count)));
    }
}

void GenerateTargetDependentSymbols (PrimarySource const &primary_source, string const &target_id, Preprocessor::SymbolTable &symbol_table)
{
    // _rule_code[_rule_count] -- specifies code for each rule.
    {
        Preprocessor::ArraySymbol *rule_code =
            symbol_table.DefineArraySymbol("_rule_code", FiLoc::ms_invalid);

        for (Uint32 rule_index = 0; rule_index < primary_source.GetRuleCount(); ++rule_index)
        {
            Rule const *rule = primary_source.GetRule(rule_index);
            assert(rule != NULL);
            PopulateRuleCodeArraySymbol(*rule, target_id, rule_code);
        }
    }

    // _rule_token_assigned_type[_rule_total_token_count] -- a contiguous array of
    // all the target-dependent types for the rule tokens in all rules.  i.e.
    // elements 0 - m will correspond to the rule tokens in rule 0, elements
    // m+1 - n will correspond to the rule tokens in rule 1, etc.  the value will
    // be the empty string if no overridden type was given, and therefore the
    // default type should be used.
    {
        Preprocessor::ArraySymbol *rule_token_assigned_type =
            symbol_table.DefineArraySymbol("_rule_token_assigned_type", FiLoc::ms_invalid);

        Uint32 rule_total_token_count = primary_source.GetRuleTokenCount();
        for (Uint32 i = 0; i < rule_total_token_count; ++i)
        {
            RuleToken const *rule_token = primary_source.GetRuleToken(i);
            assert(rule_token != NULL);
            rule_token_assigned_type->AppendArrayElement(
                new Preprocessor::Body(primary_source.GetAssignedType(rule_token->m_token_id, target_id)));
        }
    }
}

} // end of namespace Trison
