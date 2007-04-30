// ///////////////////////////////////////////////////////////////////////////
// trison_npda.cpp by Victor Dods, created 2006/11/30
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_npda.hpp"

#include <sstream>

#include "barf_util.hpp"
#include "trison_ast.hpp"
#include "trison_graph.hpp"

namespace Trison {

// ///////////////////////////////////////////////////////////////////////////
// Graph::Node::Data implementations for Nonterminal and Rule (and generic)
// ///////////////////////////////////////////////////////////////////////////

struct RuleNpdaNodeData : public NpdaNodeData
{
    Rule const *const m_rule;
    Uint32 const m_stage;

    RuleNpdaNodeData (Rule const *rule, Uint32 stage)
        :
        m_rule(rule),
        m_stage(stage)
    {
        assert(m_rule != NULL);
        assert(stage <= m_rule->m_rule_token_list->size());
    }

    // Graph::Node::Data interface methods
    virtual string GetAsText (Uint32 node_index) const
    {
        ostringstream out;
        out << "state " << node_index << endl;
        out << m_rule->GetAsText(m_stage);
        return out.str();
    }
    virtual Graph::Color DotGraphColor (Uint32 node_index) const { return Graph::Color(0xB6FFAE); }

    // NpdaNodeData interface methods
    virtual bool IsEpsilonClosureState () const { return m_stage == 0; }
    virtual Nonterminal const *GetAssociatedNonterminal () const { return m_rule->m_owner_nonterminal; }
    virtual Rule const *GetAssociatedRule () const { return m_rule; }
    virtual Uint32 GetRuleStage () const { return m_stage; }
    virtual string GetDescription () const { return m_rule->GetAsText(m_stage); }
}; // end of struct RuleNpdaNodeData

struct NonterminalHeadNpdaNodeData : public NpdaNodeData
{
    Nonterminal const *const m_nonterminal;

    NonterminalHeadNpdaNodeData (Nonterminal const *nonterminal)
        :
        m_nonterminal(nonterminal)
    {
        assert(m_nonterminal != NULL);
    }

    // Graph::Node::Data interface methods
    virtual string GetAsText (Uint32 node_index) const
    {
        Uint32 max_width = 0;
        for (RuleList::const_iterator it = m_nonterminal->m_rule_list->begin(),
                                      it_end = m_nonterminal->m_rule_list->end();
             it != it_end;
             ++it)
        {
            Rule const *rule = *it;
            assert(rule != NULL);
            max_width = max(max_width, Uint32(rule->GetAsText(0).length()));
        }

        ostringstream out;
        out << "state " << node_index << endl;
        for (RuleList::const_iterator it = m_nonterminal->m_rule_list->begin(),
                                      it_end = m_nonterminal->m_rule_list->end();
             it != it_end;
             ++it)
        {
            Rule const *rule = *it;
            assert(rule != NULL);
            assert(rule->GetHasAcceptStateIndex());
            out.width(max_width);
            out.setf(ios_base::left);
            out << rule->GetAsText(0) << endl;
        }
        return out.str();
    }
    virtual Graph::Color DotGraphColor (Uint32 node_index) const { return Graph::Color(0xAEF5FF); }

    // NpdaNodeData interface methods
    virtual bool IsEpsilonClosureState () const { return true; }
    virtual Nonterminal const *GetAssociatedNonterminal () const { return m_nonterminal; }
    virtual string GetDescription () const { return "head of: " + m_nonterminal->m_id; }
}; // end of class NonterminalHeadNpdaNodeData

struct GenericNpdaNodeData : public NpdaNodeData
{
    string const m_text;
    Graph::Color const m_dot_graph_color;
    bool const m_is_return_state;

    GenericNpdaNodeData (string const &text, Graph::Color const &dot_graph_color, bool is_return_state = false)
        :
        m_text(text),
        m_dot_graph_color(dot_graph_color),
        m_is_return_state(is_return_state)
    {
        assert(!m_text.empty());
    }

    // Graph::Node::Data interface methods
    virtual string GetAsText (Uint32 node_index) const
    {
        ostringstream out;
        out << "state " << node_index << endl;
        out << m_text;
        return out.str();
    }
    virtual Graph::Color DotGraphColor (Uint32 node_index) const { return m_dot_graph_color; }

    // NpdaNodeData interface methods
    virtual bool IsReturnState () const { return m_is_return_state; }
    virtual string GetDescription () const { return m_text; }
}; // end of class GenericNpdaNodeData

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

struct GraphContext
{
    Representation const &m_representation;
    Graph &m_graph;

    GraphContext (Representation const &representation, Graph &graph)
        :
        m_representation(representation),
        m_graph(graph)
    { }
}; // end of struct GraphContext

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void EnsureGeneratedNpda (
    Nonterminal const &nonterminal,
    GraphContext &graph_context);

void GenerateNpda (
    Token const &token,
    GraphContext &graph_context,
    Uint32 start_index,
    Uint32 end_index)
{
    if (token.m_id != NULL)
    {
        Uint32 transition_input = graph_context.m_representation.GetTokenIndex(token.m_id->GetText());
        graph_context.m_graph.AddTransition(start_index, ShiftTransition(transition_input, token.m_id->GetText(), end_index));
    }
    else
    {
        Uint32 transition_input = token.m_character->GetCharacter();
        graph_context.m_graph.AddTransition(start_index, ShiftTransition(transition_input, GetCharacterLiteral(transition_input), end_index));
    }
}

void GenerateNpda (
    RuleToken const &rule_token,
    GraphContext &graph_context,
    Uint32 start_index,
    Uint32 end_index,
    Nonterminal const *owner_nonterminal)
{
    Token const *terminal = graph_context.m_representation.m_token_map->GetElement(rule_token.m_token_id);
    Nonterminal const *nonterminal = graph_context.m_representation.m_nonterminal_map->GetElement(rule_token.m_token_id);
    assert(terminal != NULL && nonterminal == NULL || terminal == NULL && nonterminal != NULL);
    if (terminal != NULL)
        GenerateNpda(*terminal, graph_context, start_index, end_index);
    else
    {
        // add a transition using the nonterminal's token index (which will
        // be encountered directly after a rule reduction).
        Uint32 transition_input = graph_context.m_representation.GetTokenIndex(nonterminal->m_id);
        graph_context.m_graph.AddTransition(start_index, ShiftTransition(transition_input, nonterminal->m_id, end_index));
        // generate the nonterminal's subgraph if not already generated
        EnsureGeneratedNpda(*nonterminal, graph_context);
        // add a transition to the nonterminal's own head state (but only if
        // it's a different nonterminal than the one owning this rule,
        // otherwise we'll have an epsilon-transition cycle)
        if (nonterminal != owner_nonterminal)
            graph_context.m_graph.AddTransition(start_index, EpsilonTransition(nonterminal->GetNpdaGraphHeadState()));
    }
}

void GenerateNpda (
    Rule const &rule,
    GraphContext &graph_context,
    Uint32 start_index,
    Nonterminal const &owner_nonterminal)
{
    assert(rule.GetHasAcceptStateIndex());

    // minimal graphing
    {
        // add all the shift transitions
        Uint32 stage = 0;
        for (RuleTokenList::const_iterator it = rule.m_rule_token_list->begin(),
                                           it_end = rule.m_rule_token_list->end();
            it != it_end;
            ++it)
        {
            RuleToken const *rule_token = *it;
            assert(rule_token != NULL);
            Uint32 end_index = graph_context.m_graph.AddNode(new RuleNpdaNodeData(&rule, ++stage));
            GenerateNpda(*rule_token, graph_context, start_index, end_index, (stage == 1) ? &owner_nonterminal : NULL);
            start_index = end_index;
        }

        // add the reduce transition at the tail of the rule states
        graph_context.m_graph.AddTransition(start_index, ReduceTransition(rule.GetAcceptStateIndex(), FORMAT(rule.GetAcceptStateIndex())));
    }
/*
    // separate-rule-style graphing
    {
        // add the first state in the rule's state sequence
        Uint32 stage = 0;
        Uint32 end_index = graph_context.m_graph.AddNode(new RuleNpdaNodeData(&rule, stage));
        graph_context.m_graph.AddTransition(start_index, EpsilonTransition(end_index));
        start_index = end_index;
        ++stage;

        // add all the shift transitions
        for (RuleTokenList::const_iterator it = rule.m_rule_token_list->begin(),
                                        it_end = rule.m_rule_token_list->end();
            it != it_end;
            ++it)
        {
            RuleToken const *rule_token = *it;
            assert(rule_token != NULL);
            end_index = graph_context.m_graph.AddNode(new RuleNpdaNodeData(&rule, stage));
            GenerateNpda(*rule_token, graph_context, start_index, end_index, (stage == 1) ? &owner_nonterminal : NULL);
            start_index = end_index;
            ++stage;
        }

        // add the reduce transition at the tail of the rule states
        graph_context.m_graph.AddTransition(start_index, ReduceTransition(rule.GetAcceptStateIndex(), FORMAT(rule.GetAcceptStateIndex())));
    }
*/
}

void EnsureGeneratedNpda (
    Nonterminal const &nonterminal,
    GraphContext &graph_context)
{
    // if it's already graphed, we don't need to do anything
    if (nonterminal.GetIsNpdaGraphed())
        return;

    // create the start, head and return states for this nonterminal
    Uint32 graph_start_state = graph_context.m_graph.AddNode(new GenericNpdaNodeData("start: " + nonterminal.m_id, Graph::Color(0xAEC3FF)));
    Uint32 graph_return_state = graph_context.m_graph.AddNode(new GenericNpdaNodeData("return: " + nonterminal.m_id, Graph::Color(0xC9AEFF), true));
    Uint32 graph_head_state = graph_context.m_graph.AddNode(new NonterminalHeadNpdaNodeData(&nonterminal));
    // create the transitions from the start state to the head and return states
    graph_context.m_graph.AddTransition(graph_start_state, EpsilonTransition(graph_head_state));
    graph_context.m_graph.AddTransition(graph_start_state, ShiftTransition(graph_context.m_representation.GetTokenIndex(nonterminal.m_id), nonterminal.m_id, graph_return_state));
    // create the return transition
    graph_context.m_graph.AddTransition(graph_return_state, ReturnTransition(nonterminal.m_id));
    // record the start, head and return states
    nonterminal.SetNpdaGraphStates(graph_start_state, graph_head_state, graph_return_state);
    // the rules are effectively or'ed together
    for (RuleList::const_iterator it = nonterminal.m_rule_list->begin(),
                                  it_end = nonterminal.m_rule_list->end();
        it != it_end;
        ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        GenerateNpda(*rule, graph_context, graph_head_state, nonterminal);
    }
}

void GenerateNpda (Representation const &representation, Graph &npda_graph)
{
    assert(npda_graph.GetNodeCount() == 0 && "must start with an empty graph");

    // the entire graph is generated starting with the start nonterminal
    Nonterminal const *start_nonterminal = representation.m_nonterminal_map->GetElement(representation.m_start_nonterminal_id);
    assert(start_nonterminal != NULL);
    GraphContext graph_context(representation, npda_graph);
    EnsureGeneratedNpda(*start_nonterminal, graph_context);

    // TODO: iterate through nonterminals and emit warnings for unused ones
}

} // end of namespace Trison
