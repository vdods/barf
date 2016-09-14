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
// NpdaNodeData
// ///////////////////////////////////////////////////////////////////////////

string NpdaNodeData::FullDescription (Uint32 min_width) const
{
    ostringstream out;
    out.width(min_width);
    out.setf(ios_base::left);
    out << OneLineDescription();
    return out.str();
}

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
    virtual string AsText (Uint32 node_index) const
    {
        ostringstream out;
        out << "state " << node_index << endl;
        out << "rule " << m_rule->m_rule_index << ": " << m_rule->AsText(m_stage);
        return out.str();
    }
    virtual Graph::Color DotGraphColor (Uint32 node_index) const { return Graph::Color(0xB6FFAE); }

    // NpdaNodeData interface methods
    virtual Nonterminal const *AssociatedNonterminal () const { return m_rule->m_owner_nonterminal; }
    virtual Rule const *AssociatedRule () const { return m_rule; }
    virtual Uint32 RuleStage () const { return m_stage; }
    virtual string OneLineDescription () const { return FORMAT("rule " << m_rule->m_rule_index << ": " << m_rule->AsText(m_stage)); }
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
    virtual string AsText (Uint32 node_index) const
    {
        Uint32 max_width = 0;
        for (RuleList::const_iterator it = m_nonterminal->m_rule_list->begin(),
                                      it_end = m_nonterminal->m_rule_list->end();
             it != it_end;
             ++it)
        {
            Rule const *rule = *it;
            assert(rule != NULL);
            max_width = max(max_width, Uint32(rule->AsText(0).length()));
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
            out << "rule " << rule->m_rule_index << ": ";
            out.width(max_width);
            out.setf(ios_base::left);
            out << rule->AsText(0) << endl;
        }
        return out.str();
    }
    virtual Graph::Color DotGraphColor (Uint32 node_index) const { return Graph::Color(0xAEF5FF); }

    // NpdaNodeData interface methods
    virtual Nonterminal const *AssociatedNonterminal () const { return m_nonterminal; }
    virtual string OneLineDescription () const { return "head of: " + m_nonterminal->GetText(); }
    virtual string FullDescription (Uint32 min_width) const
    {
        ostringstream out;
        for (RuleList::const_iterator it = m_nonterminal->m_rule_list->begin(),
                                      it_end = m_nonterminal->m_rule_list->end();
             it != it_end;
             ++it)
        {
            Rule const *rule = *it;
            assert(rule != NULL);
            out.width(min_width);
            out.setf(ios_base::left);
            out << FORMAT("rule " << rule->m_rule_index << ": " << rule->AsText(0));
            RuleList::const_iterator next_it = it;
            ++next_it;
            if (next_it != it_end)
                out << endl;
        }
        return out.str();
    }
}; // end of class NonterminalHeadNpdaNodeData

struct GenericNpdaNodeData : public NpdaNodeData
{
    string const m_text;
    Graph::Color const m_dot_graph_color;

    GenericNpdaNodeData (string const &text, Graph::Color const &dot_graph_color, bool is_start_state = false, bool is_return_state = false)
        :
        m_text(text),
        m_dot_graph_color(dot_graph_color),
        m_is_start_state(is_start_state),
        m_is_return_state(is_return_state)
    {
        assert(!m_text.empty());
    }

    // Graph::Node::Data interface methods
    virtual string AsText (Uint32 node_index) const
    {
        ostringstream out;
        out << "state " << node_index << endl;
        out << m_text;
        return out.str();
    }
    virtual Graph::Color DotGraphColor (Uint32 node_index) const { return m_dot_graph_color; }

    // NpdaNodeData interface methods
    virtual string OneLineDescription () const { return m_text; }
    virtual bool IsStartState () const { return m_is_start_state; }
    virtual bool IsReturnState () const { return m_is_return_state; }

private:

    bool const m_is_start_state;
    bool const m_is_return_state;
}; // end of class GenericNpdaNodeData

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

struct GraphContext
{
    PrimarySource const &m_primary_source;
    Graph &m_npda_graph;

    GraphContext (PrimarySource const &primary_source, Graph &npda_graph)
        :
        m_primary_source(primary_source),
        m_npda_graph(npda_graph)
    { }
}; // end of struct GraphContext

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void EnsureGeneratedNpda (
    Nonterminal const &nonterminal,
    GraphContext &graph_context);

void GenerateNpda (
    Terminal const &terminal,
    GraphContext &graph_context,
    Uint32 start_index,
    Uint32 end_index,
    bool &this_is_an_error_terminal)
{
    Uint32 transition_input = terminal.m_is_id ? terminal.m_token_index : terminal.m_char;
    graph_context.m_npda_graph.AddTransition(start_index, NpdaShiftTransition(transition_input, terminal.GetText(), end_index));
    // If the terminal isn't ERROR_, then add a transition for ERROR_ which is POP
    this_is_an_error_terminal = terminal.m_token_index == graph_context.m_primary_source.m_terminal_map->Element("ERROR_")->m_token_index;
}

void GenerateNpda (
    RuleToken const &rule_token,
    GraphContext &graph_context,
    Uint32 start_index,
    Uint32 end_index,
    Nonterminal const *owner_nonterminal,
    bool &this_is_an_error_terminal)
{
    Terminal const *terminal = graph_context.m_primary_source.m_terminal_map->Element(rule_token.m_token_id);
    Nonterminal const *nonterminal = graph_context.m_primary_source.m_nonterminal_map->Element(rule_token.m_token_id);
    if (terminal == NULL && nonterminal == NULL)
    {
        EmitError("undeclared token \"" + rule_token.m_token_id + "\"", rule_token.GetFiLoc());
        return;
    }
    assert((terminal != NULL && nonterminal == NULL) || (terminal == NULL && nonterminal != NULL));
    if (terminal != NULL)
        GenerateNpda(*terminal, graph_context, start_index, end_index, this_is_an_error_terminal);
    else
    {
        // add a transition using the nonterminal's token index (which will
        // be encountered directly after a rule reduction).
        Uint32 transition_input = nonterminal->m_token_index;
        graph_context.m_npda_graph.AddTransition(start_index, NpdaShiftTransition(transition_input, nonterminal->GetText(), end_index));
        // generate the nonterminal's subgraph if not already generated
        EnsureGeneratedNpda(*nonterminal, graph_context);
        // add a transition to the nonterminal's own head state (but only if
        // it's a different nonterminal than the one owning this rule,
        // otherwise we'll have an epsilon-transition cycle)
        if (nonterminal != owner_nonterminal)
            graph_context.m_npda_graph.AddTransition(start_index, NpdaEpsilonTransition(nonterminal->NpdaGraphHeadState()));
    }
}

void GenerateNpda (
    Rule const &rule,
    GraphContext &graph_context,
    Uint32 start_index,
    Nonterminal const &owner_nonterminal)
{
    // add the first state in the rule's state sequence
    Uint32 stage = 0;
    Uint32 end_index = graph_context.m_npda_graph.AddNode(new RuleNpdaNodeData(&rule, stage));
    bool previous_token_was_error = false;
    bool current_token_is_error = false;
    graph_context.m_npda_graph.AddTransition(start_index, NpdaEpsilonTransition(end_index));
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
        end_index = graph_context.m_npda_graph.AddNode(new RuleNpdaNodeData(&rule, stage));

        GenerateNpda(*rule_token, graph_context, start_index, end_index, (stage == 1) ? &owner_nonterminal : NULL, current_token_is_error);

        if (previous_token_was_error)
        {
            graph_context.m_npda_graph.AddTransition(start_index, NpdaPopStackTransition(graph_context.m_primary_source.m_terminal_map->Element("END_")->m_token_index, "END_", 2));
            graph_context.m_npda_graph.AddTransition(start_index, NpdaDiscardLookaheadTransition(graph_context.m_primary_source.m_nonterminal_map->Element("none_")->m_token_index, "default"));
        }
        else
        {
            if (!current_token_is_error)
                graph_context.m_npda_graph.AddTransition(start_index, NpdaPopStackTransition(graph_context.m_primary_source.m_terminal_map->Element("ERROR_")->m_token_index, "ERROR_", 1));
            graph_context.m_npda_graph.AddTransition(start_index, NpdaInsertLookaheadErrorTransition(graph_context.m_primary_source.m_nonterminal_map->Element("none_")->m_token_index, "default"));
        }

        start_index = end_index;
        ++stage;
        previous_token_was_error = current_token_is_error;
    }

    // add the reduce transition at the tail of the rule states
    if (current_token_is_error)
    {
        graph_context.m_npda_graph.AddTransition(start_index, NpdaReduceTransition(graph_context.m_primary_source.m_terminal_map->Element("END_")->m_token_index, "END_", rule.m_rule_index));
        graph_context.m_npda_graph.AddTransition(start_index, NpdaDiscardLookaheadTransition(graph_context.m_primary_source.m_nonterminal_map->Element("none_")->m_token_index, "default"));
    }
    else
        graph_context.m_npda_graph.AddTransition(start_index, NpdaReduceTransition(graph_context.m_primary_source.m_nonterminal_map->Element("none_")->m_token_index, "default", rule.m_rule_index));
}

void EnsureGeneratedNpda (
    Nonterminal const &nonterminal,
    GraphContext &graph_context)
{
    // if it's already graphed, we don't need to do anything
    if (nonterminal.IsNpdaGraphed())
        return;

    // create the start, head and return states for this nonterminal
    Uint32 graph_start_state = graph_context.m_npda_graph.AddNode(new GenericNpdaNodeData("START " + nonterminal.GetText(), Graph::Color(0xAEC3FF), true, false));
    Uint32 graph_return_state = graph_context.m_npda_graph.AddNode(new GenericNpdaNodeData("RETURN " + nonterminal.GetText(), Graph::Color(0xC9AEFF), false, true));
    Uint32 graph_head_state = graph_context.m_npda_graph.AddNode(new NonterminalHeadNpdaNodeData(&nonterminal));
    // create the transitions from the start state to the head and return states
    graph_context.m_npda_graph.AddTransition(graph_start_state, NpdaEpsilonTransition(graph_head_state));
    graph_context.m_npda_graph.AddTransition(graph_start_state, NpdaShiftTransition(nonterminal.m_token_index, nonterminal.GetText(), graph_return_state));
    // create the return transition
    graph_context.m_npda_graph.AddTransition(graph_return_state, NpdaReturnTransition(graph_context.m_primary_source.m_nonterminal_map->Element("none_")->m_token_index, "default"));
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

void GenerateNpda (PrimarySource const &primary_source, Graph &npda_graph)
{
    assert(npda_graph.NodeCount() == 0 && "must start with an empty graph");

    GraphContext graph_context(primary_source, npda_graph);
    for (NonterminalList::const_iterator it = primary_source.m_nonterminal_list->begin(), it_end = primary_source.m_nonterminal_list->end();
         it != it_end;
         ++it)
    {
        Nonterminal const *nonterminal = *it;
        assert(nonterminal != NULL);
        // we don't want to graph the special "none_" nonterminal
        if (nonterminal->GetText() != "none_")
            EnsureGeneratedNpda(*nonterminal, graph_context);
    }
}

} // end of namespace Trison
