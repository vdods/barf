// ///////////////////////////////////////////////////////////////////////////
// trison_dpda.cpp by Victor Dods, created 2006/11/30
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_dpda.hpp"

#include <map>
#include <set>
#include <vector>

namespace Trison {

// a set of npda states constitutes a single dpda state
typedef set<Uint32> DpdaState;
typedef map<DpdaState, Uint32> GeneratedDpdaStateMap;

struct GraphContext
{
    PrimarySource const &m_primary_source;
    Graph const &m_npda_graph;
    Graph &m_dpda_graph;
    GeneratedDpdaStateMap m_generated_dpda_state_map;

    GraphContext (PrimarySource const &primary_source, Graph const &npda_graph, Graph &dpda_graph)
        :
        m_primary_source(primary_source),
        m_npda_graph(npda_graph),
        m_dpda_graph(dpda_graph)
    { }

    bool DpdaStateIsGenerated (DpdaState const &dpda_state)
    {
        return m_generated_dpda_state_map.find(dpda_state) != m_generated_dpda_state_map.end();
    }
}; // end of struct GraphContext

class Action
{
public:

    enum Type
    {
        AT_NONE = 0,
        AT_SHIFT_AND_PUSH_STATE,
        AT_REDUCE,
        AT_ERROR_PANIC,

        AT_COUNT
    }; // end of enum Action::Type

    Action () : m_type(AT_NONE), m_data(0) { }
    Action (Type type, Uint32 data = 0) : m_type(type), m_data(data) { }

    Type Type () const { return m_type; }
    Uint32 Data () const { return m_data; }

private:

    Type m_type;
    Uint32 m_data;
};

// Npda is the actual automaton which simulates a parser.
class Npda
{
public:

    typedef Uint32 TokenId;
    typedef vector<TokenId> LookaheadSequence;

    Npda (Nonterminal const *start_nonterminal)
    {
        // create the tree root
        m_tree_root = new Action(TreeNode::ACTION);
        // create the initial branch with a rule stack level of 0
        Branch *initial_branch = new Branch(start_nonterminal->GetNpdaGraphStartState());
        // add the initial branch as a child to the tree
        m_tree_root->AddChildBranch(initial_branch);
        // add the initial branch to the action and parser branch lists
        m_tree_root->m_action_branch_list.Prepend(initial_branch);
        m_parser_branch_list.Prepend(initial_branch);
        // initialize other members
        m_is_shift_blocked = false;
        m_reduce_transitions_were_performed = false;
        m_shift_transitions_were_performed = false;
        m_nonassoc_error_encountered = false;

        PerformEpsilonClosure();
    }
    Npda (DpdaState const &dpda_state)
    {
        // create the tree root
        m_tree_root = new Action(TreeNode::ACTION);
        // add a branch for each npda_state in dpda_state
        for (DpdaState::const_iterator it = dpda_state.begin(), it_end = dpda_state.end();
             it != it_end;
             ++it)
        {
            // create the branch with a rule stack level of 0
            Branch *branch = new Branch(*it);
            // add the branch as a child to the tree
            m_tree_root->AddChildBranch(branch);
            // add the branch to the action and parser branch lists
            m_tree_root->m_action_branch_list.Prepend(branch);
            m_parser_branch_list.Prepend(branch);
        }
        // initialize other members
        m_is_shift_blocked = false;
        m_reduce_transitions_were_performed = false;
        m_shift_transitions_were_performed = false;
        m_nonassoc_error_encountered = false;

        PerformEpsilonClosure();
        assert(CurrentDpdaState() == dpda_state && "malformed dpda_state parameter");
    }
    Npda (Npda const &npda)
    {
        // TODO -- copy constructor -- deep copy of action tree, parse branches, etc

        // ...

        m_is_shift_blocked = npda.m_is_shift_blocked;
        m_reduce_transitions_were_performed = npda.m_reduce_transitions_were_performed;
        m_shift_transitions_were_performed = npda.m_shift_transitions_were_performed;
        m_nonassoc_error_encountered = npda.m_nonassoc_error_encountered;
    }
    ~Npda ()
    {
        delete m_tree_root;
    }

    DpdaState CurrentDpdaState () const { /* TODO */ }

private:

    Action *m_tree_root;
    ParserBranchList m_parser_branch_list;
    ParserBranchList m_doomed_nonreturn_branch_list;
    ParserBranchList m_doomed_return_branch_list;
    bool m_is_shift_blocked;
    bool m_reduce_transitions_were_performed;
    bool m_shift_transitions_were_performed;
    bool m_nonassoc_error_encountered;
};

void Recurse (GraphContext &graph_context, DpdaState const &source_dpda_state, Npda &npda, Action const &default_action, Npda::LookaheadSequence &lookahead_sequence)
{
/*
    for each valid npda.dpda_state terminal transition {
        child_context = context
        run child_context npda with transition
        if (action tree trunk is nontrivial)
        {
            first_action = first trunk action
            child_context = context
            run child_context npda with first_action and e-close it
            record first_action on source_dpda_state, as resulting from lookahead_sequence+transition, with child_context's dpda_state as the target
            EnsureDpdaStateIsGenerated(child_context's dpda_state);
        }
        else
            recurse(child_context, source_dpda_state, lookahead_sequence+transition)
    }
*/
    for (TransitionIterator it(dpda_state, TI_TERMINALS_ONLY); !it.IsDone(); ++it)
    {
        Action action;
        // this block of code is entirely for the purpose of determining what lookaheads
        // are required for each transition.  e.g. in an LALR(3) grammar, this would
        // require a recursion to a depth of 3.
        {
            Npda child_npda(npda);
            child_npda.Run(*it);
            action = child_npda.FirstTrunkAction();
            if (action.Type() == AT_NONE)
            {
                // add the iterator token to the lookahead sequence
                lookahead_sequence.push_back(*it);
                // recursively explore the states
                Recurse(graph_context, source_dpda_state, child_npda, default_action, lookahead_sequence);
                // pop the iterator token
                lookahead_sequence.pop_back();
                // this continue statement is so child_npda and retry_npda (below)
                // don't both exist in memory at the same time.
                continue;
            }
        }
        // otherwise there was a real trunk action, so create a transition using it
        {
            // create a fork of this npda and run it with the action determined above.
            Npda retry_npda(npda);
            retry_npda.Run(action);
            // figure out what dpda state it's at
            DpdaState target_dpda_state(retry_npda.CurrentDpdaState());
            // make sure that dpda state is generated
            EnsureDpdaStateIsGenerated(graph_context, target_dpda_state);
            // add a graph transition from this state to that, via the iterator terminal
            assert(graph_context.m_generated_dpda_state_map.find(source_dpda_state) != graph_context.m_generated_dpda_state_map.end());
            graph_context.m_dpda_graph.AddTransition(
                graph_context.m_generated_dpda_state_map[source_dpda_state],
                ShiftTransition(
                    *it, // TODO -- should use lookahead_sequence + *it as the transition token sequence
                    "TODO -- real label",
                    graph_context.m_generated_dpda_state_map[target_dpda_state]));
        }
    }
}

// TODO -- maybe make a version of this which accepts a Nonterminal const *
void EnsureDpdaStateIsGenerated (GraphContext &graph_context, DpdaState const &dpda_state)
{
/*
    construct context // action tree root, initial parse branch, token stack, lookahead queue, etc
    e-close context
    construct dpda_state // set of npda states, identifies each dpda state
    if (dpda_state exists in generated_dpda_state_map)
        return; // nothing to do
    add this dpda_state to generated_dpda_state_map

    figure out what the default transition for this dpda_state is // by running the npda without any input.  the default action can either be reduce or error panic
    recurse(context, dpda_state, []); // [] signifies the empty sequence

    for each valid npda.dpda_state nonterminal transition {
        child_context = context
        run child_context by shifting that nonterminal
        record the shift target state, as resulting from the nonterminal
    }
*/
    // check if this dpda_state has already been generated
    if (graph_context.m_generated_dpda_state_map.find(dpda_state) != graph_context.m_generated_dpda_state_map.end())
        return; // nothing needs to be done
    // dpda_state is now considered "generated"
    graph_context.m_generated_dpda_state_map[dpda_state] = graph_context.m_dpda_graph.AddNode(); // TODO -- do real NodeData

    // construct the npda with dpda_state as its initial conditions
    Npda npda(dpda_state);
    assert(npda.CurrentDpdaState() == dpda_state && "malformed dpda_state");

    // run the npda with no input, so we can decide what the default action is
    npda.Run();
    // if there is no action in the trunk, then the default action is error panic.
    // otherwise it is necessarily a reduce action (because there are no tokens
    // to shift yet.
    Action default_action(npda.FirstTrunkAction());
    assert(default_action.Type() == AT_ERROR_PANIC || default_action.Type() == AT_REDUCE);

    // now recurse, exploring the states resulting from all valid transitions.
    Npda::LookaheadSequence lookahead_sequence; // empty
    Recurse(graph_context, dpda_state, npda, default_action, lookahead_sequence);

    // generate the shift transitions for the nonterminals at this state
    for (TransitionIterator it(dpda_state, TI_NONTERMINALS_ONLY); !it.IsDone(); ++it)
    {
        // create a fork of this npda and run it with the iterator nonterminal
        Npda child_npda(npda);
        child_npda.Run(*it);
        // figure out what dpda state it's at
        DpdaState target_dpda_state(child_npda.CurrentDpdaState());
        // make sure that dpda state is generated
        EnsureDpdaStateIsGenerated(graph_context, target_dpda_state);
        // add a graph transition from this state to that, via the iterator nonterminal
        graph_context.m_dpda_graph.AddTransition(
            graph_context.m_generated_dpda_state_map[dpda_state],
            ShiftTransition(
                *it,
                "TODO -- real label",
                graph_context.m_generated_dpda_state_map[target_dpda_state]));
    }
}

void GenerateDpda (PrimarySource const &primary_source, Graph const &npda_graph, Graph &dpda_graph)
{
    assert(npda_graph.GetNodeCount() > 0 && "can't generate dpda_graph from an empty npda_graph");
    assert(dpda_graph.GetNodeCount() == 0 && "must start with an empty dpda_graph");

    // graph_context contains the "globals" of this whole dpda-generating algorithm
    GraphContext graph_context(primary_source, npda_graph, dpda_graph);

    // for each nonterminal (i.e. each "start" node in npda_graph),
    for (NonterminalList::const_iterator it = primary_source.m_nonterminal_list->begin(), it_end = primary_source.m_nonterminal_list->end();
         it != it_end;
         ++it)
    {
        Nonterminal const *nonterminal = *it;
        assert(nonterminal != NULL);
        // the Npda(nonterminal).CurrentDpdaState() part gives us the e-closed
        // dpda_state corresponding to the nonterminal's start state
        EnsureDpdaStateIsGenerated(Npda(nonterminal).CurrentDpdaState());
    }
}

} // end of namespace Trison
