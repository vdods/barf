// 2006.11.30 - Copyright Victor Dods - Licensed under Apache 2.0

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
    this_is_an_error_terminal = terminal.m_token_index == graph_context.m_primary_source.GetTokenIndex("ERROR_");
}

void GenerateNpda (
    RuleToken const &rule_token,
    GraphContext &graph_context,
    Uint32 start_index,
    Uint32 end_index,
    Nonterminal const *owner_nonterminal,
    bool &this_is_an_error_terminal,
    TokenSpecifierList const *&error_until_lookaheads)
{
    this_is_an_error_terminal = false;
    error_until_lookaheads = NULL;

    Terminal const *terminal = graph_context.m_primary_source.m_terminal_map->Element(rule_token.m_token_id);
    Nonterminal const *nonterminal = graph_context.m_primary_source.m_nonterminal_map->Element(rule_token.m_token_id);
    assert(terminal == NULL || nonterminal == NULL);
    if (terminal == NULL && nonterminal == NULL)
    {
        EmitError("undeclared token \"" + rule_token.m_token_id + "\"", rule_token.GetFiLoc());
        return;
    }
    assert((terminal != NULL && nonterminal == NULL) || (terminal == NULL && nonterminal != NULL));
    if (terminal != NULL)
    {
        GenerateNpda(*terminal, graph_context, start_index, end_index, this_is_an_error_terminal);
        if (this_is_an_error_terminal)
        {
            ErrorDirective const *error_directive = dynamic_cast<ErrorDirective const *>(&rule_token);
            assert(error_directive != NULL);
            assert(error_directive->m_acceptable_tokens != NULL && "this should be impossible");
            error_until_lookaheads = error_directive->m_acceptable_tokens;
        }
    }
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

    assert(this_is_an_error_terminal == (error_until_lookaheads != NULL));
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
    TokenSpecifierList const *previous_error_acceptable_tokens = NULL;
    TokenSpecifierList const *current_error_acceptable_tokens = NULL;
    graph_context.m_npda_graph.AddTransition(start_index, NpdaEpsilonTransition(end_index));
    start_index = end_index;
    ++stage;

    // Add transitions for each rule token.
    for (RuleTokenList::const_iterator it = rule.m_rule_token_list->begin(),
                                       it_end = rule.m_rule_token_list->end();
        it != it_end;
        ++it)
    {
        RuleToken const *rule_token = *it;
        assert(rule_token != NULL);
        end_index = graph_context.m_npda_graph.AddNode(new RuleNpdaNodeData(&rule, stage));

        GenerateNpda(*rule_token, graph_context, start_index, end_index, (stage == 1) ? &owner_nonterminal : NULL, current_token_is_error, current_error_acceptable_tokens);

        if (previous_token_was_error)
        {
            assert(previous_error_acceptable_tokens != NULL);
            assert(previous_error_acceptable_tokens->m_is_inverted);
            for (TokenSpecifierList::const_iterator acceptable_token_it = previous_error_acceptable_tokens->begin(),
                                                    acceptable_token_it_end = previous_error_acceptable_tokens->end();
                 acceptable_token_it != acceptable_token_it_end;
                 ++acceptable_token_it)
            {
                assert(*acceptable_token_it != NULL);
                Ast::Id const &acceptable_token = **acceptable_token_it;
                graph_context.m_npda_graph.AddTransition(start_index, NpdaPopStackTransition(graph_context.m_primary_source.GetTokenIndex(acceptable_token.GetText()), acceptable_token.GetText(), 2));
            }
            graph_context.m_npda_graph.AddTransition(start_index, NpdaDiscardLookaheadTransition(graph_context.m_primary_source.GetTokenIndex("none_"), "default"));
        }
        else
        {
            if (!current_token_is_error)
                graph_context.m_npda_graph.AddTransition(start_index, NpdaPopStackTransition(graph_context.m_primary_source.GetTokenIndex("ERROR_"), "ERROR_", 1));
            graph_context.m_npda_graph.AddTransition(start_index, NpdaInsertLookaheadErrorTransition(graph_context.m_primary_source.GetTokenIndex("none_"), "default"));
        }

        start_index = end_index;
        ++stage;
        previous_token_was_error = current_token_is_error;
        previous_error_acceptable_tokens = current_error_acceptable_tokens;
    }

    // add the reduce transition at the tail of the rule states
    if (rule.m_lookahead_directive != NULL)
    {
        // If there's a %lookahead directive, add transitions for that.

        std::set<string> used_terminals;

        // TODO: Could potentially simplify this whole section of code; but only after it's tested
        // and works as a reference.

        // The way the transitions are constructed depends on if the lookahead's list is inverted.
        if (rule.m_lookahead_directive->m_lookaheads->m_is_inverted)
        {
            // For the example rule
            //
            //     rule 2: nt <- t1 t2 t3 . %lookahead[!(a|b|c)]
            //
            // where t3 is not an %error token and none of a, b, or c are %error tokens,
            // there would be transitions
            //
            //     a: INSERT_LOOKAHEAD_ERROR
            //     b: INSERT_LOOKAHEAD_ERROR
            //     c: INSERT_LOOKAHEAD_ERROR
            //     ERROR_: POP_STACK 1
            //     default: REDUCE 2
            //
            // If any of a, b, or c are %error tokens, then the `ERROR_: POP_STACK 1`
            // transition would not be added.
            if (!previous_token_was_error)
            {
                // Add error-handling transitions for the terminals in the inverted lookahead terminal list.
                for (TokenSpecifierList::const_iterator lookahead_it = rule.m_lookahead_directive->m_lookaheads->begin(),
                                                        lookahead_it_end = rule.m_lookahead_directive->m_lookaheads->end();
                     lookahead_it != lookahead_it_end;
                     ++lookahead_it)
                {
                    assert(*lookahead_it != NULL);
                    Ast::Id const &lookahead = **lookahead_it;
                    // Only add the transition if there is no transition for it yet.
                    if (used_terminals.find(lookahead.GetText()) == used_terminals.end())
                    {
                        graph_context.m_npda_graph.AddTransition(start_index, NpdaInsertLookaheadErrorTransition(graph_context.m_primary_source.GetTokenIndex(lookahead.GetText()), lookahead.GetText()));
                        used_terminals.insert(lookahead.GetText());
                    }
                }
            }
            // For the example rule
            //
            //     rule 4: nt <- t1 t2 %error[!(c|x|y|%end)] . %lookahead[!(a|b|c)]
            //
            // An error lookahead couldn't be inserted because there's already an error on the stack.
            // Thus the transitions would be
            //
            //     // terminals in the negated %error terminal list
            //     c: POP_STACK 2
            //     x: POP_STACK 2
            //     y: POP_STACK 2
            //     END_: POP_STACK 2
            //     // then the terminals in the negated %lookahead terminal list, minus the ones above
            //     a: DISCARD_LOOKAHEAD
            //     b: DISCARD_LOOKAHEAD
            //     // then the default transition
            //     default: REDUCE 4
            //
            // Note that the terminal c is in the %error but not the %lookahead directive, which means
            // that the %error won't absorb it and the %lookahead doesn't accept it, meaning that it
            // should result in error handling (in this case, POP_STACK 2).  Because the %error token can't
            // accept x, y, and %end (and those aren't included in the %lookahead), those must cause
            // error-handling actions (in this case, POP_STACK 2).  The fallthrough is to reduce the
            // rule.
            else
            {
                // Add the transitions relating to the previous %error token
                assert(previous_error_acceptable_tokens != NULL);
                assert(previous_error_acceptable_tokens->m_is_inverted);
                for (TokenSpecifierList::const_iterator acceptable_token_it = previous_error_acceptable_tokens->begin(),
                                                        acceptable_token_it_end = previous_error_acceptable_tokens->end();
                     acceptable_token_it != acceptable_token_it_end;
                     ++acceptable_token_it)
                {
                    assert(*acceptable_token_it != NULL);
                    Ast::Id const &acceptable_token = **acceptable_token_it;
                    // Only add the transition if there is no transition for it yet.
                    if (used_terminals.find(acceptable_token.GetText()) == used_terminals.end())
                    {
                        graph_context.m_npda_graph.AddTransition(start_index, NpdaPopStackTransition(graph_context.m_primary_source.GetTokenIndex(acceptable_token.GetText()), acceptable_token.GetText(), 2));
                        used_terminals.insert(acceptable_token.GetText());
                    }
                }

                // Add the transitions to filter out the (inverted) lookaheads.
                for (TokenSpecifierList::const_iterator lookahead_it = rule.m_lookahead_directive->m_lookaheads->begin(),
                                                        lookahead_it_end = rule.m_lookahead_directive->m_lookaheads->end();
                    lookahead_it != lookahead_it_end;
                    ++lookahead_it)
                {
                    assert(*lookahead_it != NULL);
                    Ast::Id const &lookahead = **lookahead_it;
                    // Only add the transition if there is no transition for it yet.
                    if (used_terminals.find(lookahead.GetText()) == used_terminals.end())
                    {
                        graph_context.m_npda_graph.AddTransition(start_index, NpdaDiscardLookaheadTransition(graph_context.m_primary_source.GetTokenIndex(lookahead.GetText()), lookahead.GetText()));
                        used_terminals.insert(lookahead.GetText());
                    }
                }
            }

            // Add the default transition (in this case, reduce); the above transitions all act as a filter.
            graph_context.m_npda_graph.AddTransition(start_index, NpdaReduceTransition(graph_context.m_primary_source.GetTokenIndex("none_"), "default", rule.m_rule_index));
        }
        else // !rule.m_lookahead_directive->m_lookaheads->m_is_inverted
        {
            // For the example rule
            //
            //     rule 1: nt <- t1 t2 t3 . %lookahead[a|b|c]
            //
            // where t3 is not an %error token, there would be transitions
            //
            //     a: REDUCE 1
            //     b: REDUCE 1
            //     c: REDUCE 1
            //     // If neither a, b, nor c are %error, then add a transition to handle ERROR_
            //     // in order to avoid an infinite loop of INSERT_LOOKAHEAD_ERROR.
            //     ERROR_: POP_STACK 1
            //     default: INSERT_LOOKAHEAD_ERROR
            if (!previous_token_was_error)
            {
                bool at_least_one_lookahead_was_error = false;
                // Add reduce transitions for the terminals in the lookahead terminal list.
                for (TokenSpecifierList::const_iterator lookahead_it = rule.m_lookahead_directive->m_lookaheads->begin(),
                                                        lookahead_it_end = rule.m_lookahead_directive->m_lookaheads->end();
                     lookahead_it != lookahead_it_end;
                     ++lookahead_it)
                {
                    assert(*lookahead_it != NULL);
                    Ast::Id const &lookahead = **lookahead_it;
                    if (lookahead.GetText() == "ERROR_")
                        at_least_one_lookahead_was_error = true;
                    // Only add the transition if there is no transition for it yet.
                    if (used_terminals.find(lookahead.GetText()) == used_terminals.end())
                    {
                        graph_context.m_npda_graph.AddTransition(start_index, NpdaReduceTransition(graph_context.m_primary_source.GetTokenIndex(lookahead.GetText()), lookahead.GetText(), rule.m_rule_index));
                        used_terminals.insert(lookahead.GetText());
                    }
                }
                // If none of the lookaheads was %error, then add a rule for ERROR_ to pop the stack,
                // in order to avoid an infinite loop of INSERT_LOOKAHEAD_ERROR (since the default
                // transition is INSERT_LOOKAHEAD_ERROR).
                if (!at_least_one_lookahead_was_error)
                    graph_context.m_npda_graph.AddTransition(start_index, NpdaPopStackTransition(graph_context.m_primary_source.GetTokenIndex("ERROR_"), "ERROR_", 1));
                // Add the default transition (in this case, INSERT_LOOKAHEAD_ERROR); the above transitions all act as a filter.
                graph_context.m_npda_graph.AddTransition(start_index, NpdaInsertLookaheadErrorTransition(graph_context.m_primary_source.GetTokenIndex("none_"), "default"));
            }
            // For the example rule
            //
            //     rule 3: nt <- t1 t2 %error[!(c|x|y|%end)] . %lookahead[a|b|c]
            //
            // An error lookahead couldn't be inserted because there's already an error on the stack.
            // Thus the transitions would be
            //
            //     // positive %lookahead terminals first
            //     a: REDUCE 3
            //     b: REDUCE 3
            //     c: REDUCE 3
            //     // then the terminals in the negated %error terminal list, minus the ones above
            //     x: POP_STACK 2
            //     y: POP_STACK 2
            //     END_: POP_STACK 2
            //     // then the default transition
            //     default: DISCARD_LOOKAHEAD
            //
            // Note that the terminal c is in both the %error and the %lookahead directive, which means
            // that the %error won't absorb it, but the %lookahead requires it, meaning that it should
            // reduce upon c.  Because the %error token can't accept x, y, and %end (and those aren't
            // included in the %lookahead), those must cause error-handling actions (in this case,
            // POP_STACK 2).  The fallthrough is the ordinary error handling, which is to discard the
            // lookahead.
            else // previous_token_was_error
            {
                assert(previous_error_acceptable_tokens != NULL);
                assert(previous_error_acceptable_tokens->m_is_inverted);

                // Add reduce transitions for the terminals in the lookahead terminal list.
                for (TokenSpecifierList::const_iterator lookahead_it = rule.m_lookahead_directive->m_lookaheads->begin(),
                                                        lookahead_it_end = rule.m_lookahead_directive->m_lookaheads->end();
                     lookahead_it != lookahead_it_end;
                     ++lookahead_it)
                {
                    assert(*lookahead_it != NULL);
                    Ast::Id const &lookahead = **lookahead_it;
                    // Only add the transition if there is no transition for it yet.
                    if (used_terminals.find(lookahead.GetText()) == used_terminals.end())
                    {
                        graph_context.m_npda_graph.AddTransition(start_index, NpdaReduceTransition(graph_context.m_primary_source.GetTokenIndex(lookahead.GetText()), lookahead.GetText(), rule.m_rule_index));
                        used_terminals.insert(lookahead.GetText());
                    }
                }
                // Add the transitions relating to the previous %error token
                for (TokenSpecifierList::const_iterator acceptable_token_it = previous_error_acceptable_tokens->begin(),
                                                        acceptable_token_it_end = previous_error_acceptable_tokens->end();
                     acceptable_token_it != acceptable_token_it_end;
                     ++acceptable_token_it)
                {
                    assert(*acceptable_token_it != NULL);
                    Ast::Id const &acceptable_token = **acceptable_token_it;
                    // Only add the transition if there is no transition for it yet.
                    if (used_terminals.find(acceptable_token.GetText()) == used_terminals.end())
                    {
                        graph_context.m_npda_graph.AddTransition(start_index, NpdaPopStackTransition(graph_context.m_primary_source.GetTokenIndex(acceptable_token.GetText()), acceptable_token.GetText(), 2));
                        used_terminals.insert(acceptable_token.GetText());
                    }
                }

                // Add the default transition (in this case, INSERT_LOOKAHEAD_ERROR); the above transitions all act as a filter.
                graph_context.m_npda_graph.AddTransition(start_index, NpdaDiscardLookaheadTransition(graph_context.m_primary_source.GetTokenIndex("none_"), "default"));
            }
        }
    }
    else // rule.m_lookahead_directive == NULL
    {
        assert(rule.m_lookahead_directive == NULL); // pedantic
        // This is the same as the pre-%lookahead-feature behavior of trison
        if (previous_token_was_error)
        {
            assert(previous_error_acceptable_tokens->m_is_inverted);
            for (TokenSpecifierList::const_iterator acceptable_token_it = previous_error_acceptable_tokens->begin(),
                                                    acceptable_token_it_end = previous_error_acceptable_tokens->end();
                 acceptable_token_it != acceptable_token_it_end;
                 ++acceptable_token_it)
            {
                assert(*acceptable_token_it != NULL);
                Ast::Id const &acceptable_token = **acceptable_token_it;
                graph_context.m_npda_graph.AddTransition(start_index, NpdaReduceTransition(graph_context.m_primary_source.GetTokenIndex(acceptable_token.GetText()), acceptable_token.GetText(), rule.m_rule_index));
            }
            graph_context.m_npda_graph.AddTransition(start_index, NpdaDiscardLookaheadTransition(graph_context.m_primary_source.GetTokenIndex("none_"), "default"));
        }
        else
            graph_context.m_npda_graph.AddTransition(start_index, NpdaReduceTransition(graph_context.m_primary_source.GetTokenIndex("none_"), "default", rule.m_rule_index));
    }
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
    graph_context.m_npda_graph.AddTransition(graph_return_state, NpdaReturnTransition(graph_context.m_primary_source.GetTokenIndex("none_"), "default"));
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

    // First generate the fallback state which results in the TT_ABORT action.
    Uint32 graph_fallback_state = graph_context.m_npda_graph.AddNode(new GenericNpdaNodeData("FALLBACK", Graph::Color(0xFFA793), false, false));
    assert(graph_fallback_state == 0);
    graph_context.m_npda_graph.AddTransition(graph_fallback_state, NpdaAbortTransition(graph_context.m_primary_source.GetTokenIndex("none_"), "default"));

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

void PrintNpdaStatesFile (PrimarySource const &primary_source, Graph const &npda_graph, ostream &stream)
{
    assert(primary_source.RuleCount() > 0);
    Uint32 max_rule_index_width = FORMAT(primary_source.RuleCount()-1).length();

    stream << "//////////////////////////////////////////////////////////////////////////////" << endl
           << "// GRAMMAR" << endl
           << "//////////////////////////////////////////////////////////////////////////////" << endl
           << endl;
    for (Uint32 i = 0; i < primary_source.RuleCount(); ++i)
    {
        stream << "    rule ";
        stream.width(max_rule_index_width);
        stream.setf(ios_base::right);
        stream << i << ": " << primary_source.GetRule(i)->AsText() << endl;
    }
    stream << endl;

    assert(npda_graph.NodeCount() > 0);

    stream << "//////////////////////////////////////////////////////////////////////////////" << endl
           << "// NPDA STATE MACHINE - " << npda_graph.NodeCount() << " STATES" << endl
           << "//////////////////////////////////////////////////////////////////////////////" << endl
           << endl;
    for (Uint32 i = 0; i < npda_graph.NodeCount(); ++i)
    {
        // print the state index and corresponding NPDA state indices
        Graph::Node const &npda_node = npda_graph.GetNode(i);
        NpdaNodeData const &npda_node_data = npda_node.DataAs<NpdaNodeData>();
        stream << "state " << i << ':' << endl;

        std::string npda_node_full_description(npda_node_data.FullDescription());
        // Indent the description string in order to have nicer formatting.
        Barf::ReplaceAllInString(&npda_node_full_description, "\n", "\n    ");
        stream << "    " << npda_node_full_description << endl;
        stream << endl;

        // TODO -- figure out the justification width of the SHIFT transition
        // lookaheads so the transition printout can be all nice and justified

        // print the transitions
        for (Graph::TransitionSet::const_iterator it = npda_node.TransitionSetBegin(),
                                                  it_end = npda_node.TransitionSetEnd();
             it != it_end;
             ++it)
        {
            Graph::Transition const &transition = *it;
            // print transition label (which includes the type and action)
            stream << "    " << transition.Label() << endl;
        }
        stream << endl;
    }
}

} // end of namespace Trison
