// ///////////////////////////////////////////////////////////////////////////
// barf_regex_dfa.cpp by Victor Dods, created 2006/12/31
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_regex_dfa.hpp"

#include <map>
#include <set>

#include "barf_graph.hpp"
#include "barf_regex_graph.hpp"
#include "barf_regex_nfa.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace Regex {

// a DFA state is really a set of NFA states
typedef set<Uint32> DfaState;
// the DfaStateMap stores the index of already-explored DfaStates
typedef map<DfaState, Uint32> DfaStateMap;
// for ordering Conditional instances
struct ConditionalOrder
{
    bool operator () (Conditional const &c0, Conditional const &c1)
    {
        return c0.m_mask < c1.m_mask || c0.m_mask == c1.m_mask && c0.m_flags < c1.m_flags;
    }
};
// for mapping sets of target states to a particular Conditional
typedef map<Conditional, DfaState, ConditionalOrder> TargetStateMap;

struct GraphContext
{
    Graph const &m_nfa_graph;
    Uint32 m_nfa_accept_state_count;
    Graph &m_dfa_graph;
    DfaStateMap &m_dfa_state_map;

    GraphContext (Graph const &nfa_graph, Uint32 nfa_accept_state_count, Graph &dfa_graph, DfaStateMap &dfa_state_map)
        :
        m_nfa_graph(nfa_graph),
        m_nfa_accept_state_count(nfa_accept_state_count),
        m_dfa_graph(dfa_graph),
        m_dfa_state_map(dfa_state_map)
    { }
}; // end of struct GraphContext

void PerformTransitionClosure (GraphContext const &graph_context, Uint32 nfa_state, Conditional conditional, TargetStateMap &target_state_map, TargetStateMap &visited)
{
    // early-out if we've already visited this conditional+nfa_state
    {
        TargetStateMap::const_iterator it;
        if (Contains(visited, conditional, it) && Contains(it->second, nfa_state))
            return;
    }
    // add this conditional+nfa_state to the visited set
    visited[conditional].insert(nfa_state);

    Graph::Node const &node = graph_context.m_nfa_graph.GetNode(nfa_state);

    // if this nfa state is an accept-node, or if it has no
    // transitions, add this nfa state to the target map.
    if (node.GetTransitionCount() == 0 ||
        GetNodeData(graph_context.m_nfa_graph, nfa_state).m_is_accept_node)
    {
        target_state_map[conditional].insert(nfa_state);
    }

    // iterate over all transitions
    for (Graph::Node::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
                                                    it_end = node.GetTransitionSetEnd();
         it != it_end;
         ++it)
    {
        Graph::Transition const &transition = *it;
        switch (transition.m_transition_type)
        {
            case TT_INPUT_ATOM:
            case TT_INPUT_ATOM_RANGE:
                // if there are input-atom transitions, add this nfa state to the target map.
                target_state_map[conditional].insert(nfa_state);
                break;

            case TT_EPSILON:
                // if it's an epsilon transition, recurse.
                PerformTransitionClosure(graph_context, transition.m_target_index, conditional, target_state_map, visited);
                break;

            case TT_CONDITIONAL:
            {
                // get the Conditional value from transition.m_transition_type
                Conditional transition_conditional(GetConditionalFromConditionalType(transition.m_data_0));
                // check if it conflicts with the passed-in conditional
                if (conditional.ConflictsWith(transition_conditional))
                    throw string("conditional ") + GetConditionalTypeString(transition.m_data_0) + " conflicts with previous adjacent conditionals";
                // compose this transition's conditional with the one passed-in.
                Conditional transitioned_conditional(
                    conditional.m_mask|transition_conditional.m_mask,
                    conditional.m_flags|transition_conditional.m_flags);
                // recurse to the state indicated by this transition, composing
                // this transition's conditional with the current.
                PerformTransitionClosure(graph_context, transition.m_target_index, transitioned_conditional, target_state_map, visited);
                break;
            }
        }
    }
}

void PerformTransitionClosure (GraphContext const &graph_context, Uint32 nfa_state, TargetStateMap &target_state_map)
{
    TargetStateMap visited;
    PerformTransitionClosure(graph_context, nfa_state, Conditional(0, 0), target_state_map, visited);
}

void PerformEpsilonClosure (GraphContext const &graph_context, Uint32 nfa_state, DfaState &dfa_state, DfaState &visited)
{
    // early-out if we've already visited this nfa_state
    if (Contains(visited, nfa_state))
        return;
    // add this nfa_state to the visited set
    visited.insert(nfa_state);

    Graph::Node const &node = graph_context.m_nfa_graph.GetNode(nfa_state);

    // if this nfa state is an accept-node, a start-node, or if
    // it has no transitions, add this nfa state to the target map.
    if (node.GetTransitionCount() == 0 ||
        GetNodeData(graph_context.m_nfa_graph, nfa_state).m_is_start_node ||
        GetNodeData(graph_context.m_nfa_graph, nfa_state).m_is_accept_node)
    {
        dfa_state.insert(nfa_state);
    }

    // iterate over all transitions
    for (Graph::Node::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
                                                    it_end = node.GetTransitionSetEnd();
         it != it_end;
         ++it)
    {
        Graph::Transition const &transition = *it;
        // if it's an epsilon transition, recurse.
        if (transition.m_transition_type == TT_EPSILON)
            PerformEpsilonClosure(graph_context, transition.m_target_index, dfa_state, visited);
        // otherwise add it to the closed dfa state
        else
            dfa_state.insert(nfa_state);
    }
}

void PerformEpsilonClosure (GraphContext const &graph_context, DfaState &dfa_state)
{
    if (dfa_state.empty())
        return;

    DfaState new_dfa_state;
    DfaState visited;
    for (DfaState::const_iterator it = dfa_state.begin(), it_end = dfa_state.end();
         it != it_end;
         ++it)
    {
        PerformEpsilonClosure(graph_context, *it, new_dfa_state, visited);
    }
    dfa_state = new_dfa_state;
}

NodeData *CreateNodeDataFromDfaState (GraphContext const &graph_context, DfaState const &dfa_state)
{
    string node_label(" : DFA\n");
    DfaState::const_iterator it = dfa_state.begin(), it_end = dfa_state.end();
    while (it != it_end)
    {
        node_label += GetNodeData(graph_context.m_nfa_graph, *it).GetAsText(*it);
        if (++it != it_end)
            node_label += ", "; // use newline instead?
    }

    Uint32 accept_handler_index = graph_context.m_nfa_accept_state_count;
    bool is_start_node = false;
    bool is_accept_node = false;
    for (DfaState::const_iterator it = dfa_state.begin(), it_end = dfa_state.end();
         it != it_end;
         ++it)
    {
        if (GetNodeData(graph_context.m_nfa_graph, *it).m_is_start_node)
            is_start_node = true;
        if (GetNodeData(graph_context.m_nfa_graph, *it).m_is_accept_node)
        {
            is_accept_node = true;
            // record the lowest accept handler index
            assert(*it < graph_context.m_nfa_accept_state_count);
            if (*it < accept_handler_index)
                accept_handler_index = *it;
        }
    }

    return new NodeData(is_start_node ? IS_START_NODE : NOT_START_NODE, is_accept_node ? IS_ACCEPT_NODE : NOT_ACCEPT_NODE, node_label, accept_handler_index);
}

Uint32 GetDfaStateIndex (GraphContext &graph_context, DfaState const &dfa_state, bool disallow_transition_closure)
{
    assert(graph_context.m_dfa_graph.GetNodeCount() == graph_context.m_dfa_state_map.size());
    assert(!dfa_state.empty());
    DfaState closed_dfa_state(dfa_state);
    PerformEpsilonClosure(graph_context, closed_dfa_state);
    assert(!closed_dfa_state.empty());

    // if the requested state is already in the map, return the index
    DfaStateMap::iterator it = graph_context.m_dfa_state_map.find(closed_dfa_state);
    if (it != graph_context.m_dfa_state_map.end())
        return it->second;

    // add the requested state to the state map so that any requests for it
    // which happen before the end of this call don't cause infinite recursion
    Uint32 dfa_state_index = graph_context.m_dfa_state_map.size();
    graph_context.m_dfa_state_map[closed_dfa_state] = dfa_state_index;
    Uint32 current_state_index = graph_context.m_dfa_graph.AddNode(CreateNodeDataFromDfaState(graph_context, closed_dfa_state));
    assert(current_state_index == dfa_state_index);

    // generate the requested state
    {
        // perform closure on current state, OR'ing a bitmask of conditionals
        // encountered and keeping a bitset of chars encountered.
        TargetStateMap target_state_map;
        for (DfaState::const_iterator it = closed_dfa_state.begin(), it_end = closed_dfa_state.end();
             it != it_end;
             ++it)
        {
            PerformTransitionClosure(graph_context, *it, target_state_map);
        }

        // if no conditionals were hit, target_state_map should have a single element
        // mapped to key Conditional(0, 0).  in this case, no conditional
        // transitions have to be made from this state -- we can proceed directly
        // with input-atom transitions.
        TargetStateMap::const_iterator target_it = target_state_map.find(Conditional(0, 0));
        if ((target_state_map.size() == 1 || disallow_transition_closure)
            &&
            target_it != target_state_map.end())
        {
            DfaState const &closed_dfa_state = target_it->second;
            // map of transition atoms onto transitioned-to DFA states
            map<Uint8, DfaState> transition_map;
            for (DfaState::const_iterator it = closed_dfa_state.begin(), it_end = closed_dfa_state.end();
                 it != it_end;
                 ++it)
            {
                Graph::Node const &from_nfa_node = graph_context.m_nfa_graph.GetNode(*it);
                for (Graph::Node::TransitionSet::const_iterator trans_it = from_nfa_node.GetTransitionSetBegin(),
                                                                trans_it_end = from_nfa_node.GetTransitionSetEnd();
                     trans_it != trans_it_end;
                     ++trans_it)
                {
                    Graph::Transition const &transition = *trans_it;
                    if (transition.m_transition_type == TT_INPUT_ATOM)
                        transition_map[transition.m_data_0].insert(transition.m_target_index);
                    else if (transition.m_transition_type == TT_INPUT_ATOM_RANGE)
                        for (Uint32 atom = transition.m_data_0; atom <= transition.m_data_1; ++atom)
                            transition_map[atom].insert(transition.m_target_index);
                }
            }
            // add the transitions from the above-made map
            map<Uint8, DfaState>::const_iterator it = transition_map.begin(), it_end = transition_map.end();
            while (it != it_end)
            {
                map<Uint8, DfaState>::const_iterator section_it_end = it;
                Uint8 starting_atom = it->first;
                Uint8 most_recent_atom = it->first;
                DfaState const &target_dfa_state = it->second;
                while (section_it_end != it_end && section_it_end->first - most_recent_atom <= 1 && section_it_end->second == target_dfa_state)
                {
                    most_recent_atom = section_it_end->first;
                    ++section_it_end;
                }
                assert(it != section_it_end);
                // if the atom range was a single value, use TT_INPUT_ATOM
                if (most_recent_atom == starting_atom)
                    graph_context.m_dfa_graph.AddTransition(
                        current_state_index,
                        InputAtomTransition(
                            starting_atom,
                            GetDfaStateIndex(graph_context, target_dfa_state, false)));
                // otherwise they spanned a range, so use TT_INPUT_ATOM_RANGE
                else
                    graph_context.m_dfa_graph.AddTransition(
                        current_state_index,
                        InputAtomRangeTransition(
                            starting_atom,
                            most_recent_atom,
                            GetDfaStateIndex(graph_context, target_dfa_state, false)));
                it = section_it_end;
            }
        }
        // otherwise we have to make conditional transitions.  all transitions
        // from the nodes being transitioned to here will have only input-atom
        // transitions.
        else
        {
            // calculate a comprehensive mask which include all target
            // conditionals' masks.
            Conditional current_condition(0, 0);
            for (TargetStateMap::const_iterator it = target_state_map.begin(),
                                                it_end = target_state_map.end();
                 it != it_end;
                 ++it)
            {
                Conditional const &target_condition = it->first;
                current_condition.m_mask |= target_condition.m_mask;
            }

            // for every possible combination recorded in target_state_map, make a
            // transition to the set of DFA states (each being a set of NFA states)
            // which are accepted by the calculated conditional.
            current_condition.m_flags = 0;
            do
            {
                DfaState cumulative_target_dfa_state;
                for (TargetStateMap::const_iterator it = target_state_map.begin(),
                                                    it_end = target_state_map.end();
                    it != it_end;
                    ++it)
                {
                    Conditional const &target_condition = it->first;
                    if (!current_condition.ConflictsWith(target_condition))
                    {
                        DfaState const &target_dfa_state = it->second;
                        cumulative_target_dfa_state.insert(target_dfa_state.begin(), target_dfa_state.end());
                    }
                }
                if (!cumulative_target_dfa_state.empty())
                    graph_context.m_dfa_graph.AddTransition(
                        current_state_index,
                        DfaConditionalTransition(
                            current_condition.m_mask,
                            current_condition.m_flags,
                            GetDfaStateIndex(graph_context, cumulative_target_dfa_state, true)));

                ++current_condition;
            }
            while (current_condition.m_flags != 0);
        }
    }

    // return the generated state's index
    return dfa_state_index;
}

void GenerateDfa (Graph const &nfa_graph, Uint32 nfa_accept_state_count, vector<Uint32> const &nfa_start_state, Graph &dfa_graph, vector<Uint32> &dfa_start_state)
{
    assert(nfa_graph.GetNodeCount() >= nfa_accept_state_count);
    assert(nfa_graph.GetNodeCount() >= nfa_start_state.size());
    assert(dfa_start_state.empty());
    assert(dfa_graph.GetNodeCount() == 0);

    DfaStateMap dfa_state_map;
    GraphContext graph_context(nfa_graph, nfa_accept_state_count, dfa_graph, dfa_state_map);

    for (Uint32 i = 0; i < nfa_start_state.size(); ++i)
    {
        assert(nfa_start_state[i] < nfa_graph.GetNodeCount());

        DfaState dfa_state;
        dfa_state.insert(nfa_start_state[i]);
        dfa_start_state.push_back(GetDfaStateIndex(graph_context, dfa_state, false));
    }

    assert(dfa_start_state.size() == nfa_start_state.size());
}

} // end of namespace Regex
} // end of namespace Barf
