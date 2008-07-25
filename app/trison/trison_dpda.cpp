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
#include <vector>

#include "barf_list.hpp"
#include "barf_weakreference.hpp"
#include "trison_npda.hpp"

// #define DEBUG_CODE(x) if (true) { x }
#define DEBUG_CODE(x) { }

namespace Trison {

ostream &operator << (ostream &stream, DpdaState const &dpda_state)
{
    stream << '(';
    for (DpdaState::const_iterator it = dpda_state.begin(), it_end = dpda_state.end();
         it != it_end;
         ++it)
    {
        stream << *it;
        DpdaState::const_iterator next_it = it;
        ++next_it;
        if (next_it != it_end)
            stream << ' ';
    }
    stream << ')';
    return stream;
}

DpdaNodeData::DpdaNodeData (Graph const &npda_graph, DpdaState const &dpda_state)
    :
    m_dpda_state(dpda_state),
    m_is_start_state(false),
    m_is_return_state(false)
{
    // first figure out the max width of all the lines of the output
    Uint32 max_width = 0;
    for (DpdaState::const_iterator it = dpda_state.begin(), it_end = dpda_state.end();
            it != it_end;
            ++it)
    {
        NpdaNodeData const &npda_node_data = npda_graph.GetNode(*it).GetDataAs<NpdaNodeData>();
        string const text(npda_node_data.GetFullDescription(0));
        Uint32 line_width_count = 0;
        for (string::const_iterator str_it = text.begin(), str_it_end = text.end();
                str_it != str_it_end;
                ++str_it)
        {
            if (*str_it == '\n')
            {
                max_width = max(max_width, line_width_count);
                line_width_count = 0;
            }
            else
                ++line_width_count;
        }
        max_width = max(max_width, line_width_count);
    }

    // generate the justified description text
    for (DpdaState::const_iterator it = dpda_state.begin(), it_end = dpda_state.end();
         it != it_end;
         ++it)
    {
        NpdaNodeData const &npda_node_data = npda_graph.GetNode(*it).GetDataAs<NpdaNodeData>();
        m_description += npda_node_data.GetFullDescription(max_width);
        DpdaState::const_iterator next_it = it;
        ++next_it;
        if (next_it != it_end)
            m_description += '\n';
    }

    // figure out if this is a start/return state
    for (DpdaState::const_iterator it = dpda_state.begin(), it_end = dpda_state.end();
            it != it_end;
            ++it)
    {
        NpdaNodeData const &npda_node_data = npda_graph.GetNode(*it).GetDataAs<NpdaNodeData>();
        m_is_start_state = m_is_start_state || npda_node_data.IsStartState();
        m_is_return_state = m_is_return_state || npda_node_data.IsReturnState();
    }
}

string DpdaNodeData::GetAsText (Uint32 node_index) const
{
    return FORMAT("state " << node_index << ' ' << m_dpda_state << '\n' << m_description);
}

Graph::Color DpdaNodeData::DotGraphColor (Uint32 node_index) const
{
    if (m_is_return_state)
        return Graph::Color(0xB6FFAE);
    else
        return Graph::Color(0xFCFFAE);
}

Uint32 DpdaNodeData::GetNodePeripheries (Uint32 node_index) const
{
    return m_is_start_state ? 2 : 1;
}

typedef map<DpdaState, Uint32> GeneratedDpdaStateMap;

struct GraphContext
{
    PrimarySource const &m_primary_source;
    Graph const &m_npda_graph;

    GraphContext (PrimarySource const &primary_source, Graph const &npda_graph, Graph &dpda_graph)
        :
        m_primary_source(primary_source),
        m_npda_graph(npda_graph),
        m_dpda_graph(dpda_graph),
        m_lalr_lookahead_count(0)
    { }

    Uint32 DpdaStateIndex (DpdaState const &dpda_state)
    {
        assert(DpdaStateIsGenerated(dpda_state));
        return m_generated_dpda_state_map[dpda_state];
    }
    bool DpdaStateIsGenerated (DpdaState const &dpda_state)
    {
        assert(!dpda_state.empty());
        return m_generated_dpda_state_map.find(dpda_state) != m_generated_dpda_state_map.end();
    }
    Uint32 LalrLookaheadCount () const { return m_lalr_lookahead_count; }
    void RecordLalrLookaheadCount (Uint32 lalr_lookahead_count)
    {
        m_lalr_lookahead_count = max(m_lalr_lookahead_count, lalr_lookahead_count);
    }
    void AddDpdaState (DpdaState const &dpda_state, DpdaNodeData const *dpda_node_data)
    {
        assert(dpda_node_data != NULL);
        assert(!DpdaStateIsGenerated(dpda_state));
        m_generated_dpda_state_map[dpda_state] = m_dpda_graph.AddNode(dpda_node_data);
    }
    void AddTransition (DpdaState const &source_dpda_state, Graph::Transition const &transition)
    {
        // record the number of lookaheads required by this LALR parser
        if (transition.Type() == TT_SHIFT)
            RecordLalrLookaheadCount(transition.DataCount());
        else if (transition.Type() == TT_REDUCE)
            // -1 because the first one is the reduction rule.
            RecordLalrLookaheadCount(transition.DataCount()-1);

        m_dpda_graph.AddTransition(DpdaStateIndex(source_dpda_state), transition);
    }
    Uint32 TransitionCount (DpdaState const &source_dpda_state)
    {
        assert(DpdaStateIsGenerated(source_dpda_state));
        return m_dpda_graph.GetNode(DpdaStateIndex(source_dpda_state)).GetTransitionCount();
    }

private:

    Graph &m_dpda_graph;
    GeneratedDpdaStateMap m_generated_dpda_state_map;
    Uint32 m_lalr_lookahead_count;
}; // end of struct GraphContext

string GetLookaheadSequenceString (PrimarySource const &primary_source, Graph::Transition::DataArray const &lookahead_sequence)
{
    string ret;
    for (Graph::Transition::DataArray::const_iterator it = lookahead_sequence.begin(), it_end = lookahead_sequence.end();
         it != it_end;
         ++it)
    {
        ret += primary_source.GetTokenId(*it);
        Graph::Transition::DataArray::const_iterator next_it = it;
        ++next_it;
        if (next_it != it_end)
            ret += ' ';
    }
    return ret;
}

enum IterateFlags
{
    IT_EPSILON_TRANSITIONS           = 1 << 0,
    IT_REDUCE_TRANSITIONS            = 1 << 1,
    IT_RETURN_TRANSITIONS            = 1 << 2,
    IT_SHIFT_TERMINAL_TRANSITIONS    = 1 << 3,
    IT_SHIFT_NONTERMINAL_TRANSITIONS = 1 << 4
}; // end of enum IterateFlags

// the lifetime of an instance of TransitionIterator should never exceed
// that of the npda_graph or dpda_state passed into it (because it stores
// references to them).
class TransitionIterator
{
public:

    TransitionIterator (GraphContext const &graph_context, DpdaState const &dpda_state, IterateFlags flags)
        :
        m_npda_graph(graph_context.m_npda_graph),
        m_dpda_state(dpda_state),
        m_flags(flags)
    {
        assert(graph_context.m_primary_source.m_nonterminal_list->size() >= 2);
        assert(graph_context.m_primary_source.m_nonterminal_list->GetElement(0)->m_token_index == 0);
        assert(graph_context.m_primary_source.m_nonterminal_list->GetElement(0)->GetText() == "none_");
        m_lowest_nonterminal_index = graph_context.m_primary_source.m_nonterminal_list->GetElement(1)->m_token_index;

        assert(m_lowest_nonterminal_index > 0x101); // 0x100 and 0x101 are END_ and ERROR_ respectively
        // start the TransitionIterator's major iterator pointing at the first
        // Graph::Transition of the first nonempty DpdaState Graph::Node (i.e. a
        // node in the npda graph).
        m_dpda_state_it = m_dpda_state.begin();
        SkipEmpties();
        // increment until we're pointing at a transition which satisfies the flags,
        // stopping if there is no more iterating to do.
        while (!IsDone() && !SatisfiesFlags())
            PrivateIncrement();
    }

    Graph::Transition const &operator * () const
    {
        assert(!IsDone() && "can't dereference a 'done' TransitionIterator");
        return *m_transition_it;
    }
    Graph::Transition const *operator -> () const
    {
        assert(!IsDone() && "can't dereference a 'done' TransitionIterator");
        return &*m_transition_it;
    }
    void operator ++ ()
    {
        assert(!IsDone() && "can't increment a 'done' TransitionIterator");
        do
            PrivateIncrement();
        while (!IsDone() && !SatisfiesFlags());
    }
    bool IsDone () const
    {
        // we're done only when the major iteration is done.
        return m_dpda_state_it == m_dpda_state.end();
    }

private:

    bool SatisfiesFlags () const
    {
        assert(!IsDone());
        bool satisfies_flags = false;
        if ((m_flags&IT_EPSILON_TRANSITIONS) != 0)
            satisfies_flags |= operator*().Type() == TT_EPSILON;
        if ((m_flags&IT_REDUCE_TRANSITIONS) != 0)
            satisfies_flags |= operator*().Type() == TT_REDUCE;
        if ((m_flags&IT_RETURN_TRANSITIONS) != 0)
            satisfies_flags |= operator*().Type() == TT_RETURN;
        if ((m_flags&IT_SHIFT_TERMINAL_TRANSITIONS) != 0)
            satisfies_flags |= operator*().Type() == TT_SHIFT && operator*().Data(0) < m_lowest_nonterminal_index;
        if ((m_flags&IT_SHIFT_NONTERMINAL_TRANSITIONS) != 0)
            satisfies_flags |= operator*().Type() == TT_SHIFT && operator*().Data(0) >= m_lowest_nonterminal_index;
        return satisfies_flags;
    }
    void PrivateIncrement ()
    {
        assert(!IsDone());
        // increment the minor iterator (m_transition_it)
        ++m_transition_it;
        // if the minor iterator reached the end of the minor iteration,
        // then increment the major iterator (m_dpda_state_it) and set
        // the minor iterator to the beginning of the minor iteration.
        if (m_transition_it == m_npda_graph.GetNode(*m_dpda_state_it).GetTransitionSetEnd())
        {
            // increment the major iterator and skip all empty DpdaState Graph::Nodes
            ++m_dpda_state_it;
            SkipEmpties();
        }
    }
    void SkipEmpties ()
    {
        while (m_dpda_state_it != m_dpda_state.end() &&
               m_npda_graph.GetNode(*m_dpda_state_it).GetTransitionSetBegin() == m_npda_graph.GetNode(*m_dpda_state_it).GetTransitionSetEnd())
        {
            ++m_dpda_state_it;
        }
        // set the minor iterator
        if (m_dpda_state_it != m_dpda_state.end())
            m_transition_it = m_npda_graph.GetNode(*m_dpda_state_it).GetTransitionSetBegin();
    }

    Graph const &m_npda_graph;
    DpdaState const m_dpda_state;
    IterateFlags const m_flags;
    Uint32 m_lowest_nonterminal_index;
    // major iterator ("outer loop")
    DpdaState::const_iterator m_dpda_state_it;
    // minor iterator ("inner loop")
    Graph::TransitionSet::const_iterator m_transition_it;
}; // end of class TransitionIterator

enum ActionType
{
    AT_NONE = 0,
    AT_SHIFT,
    AT_REDUCE,
    AT_RETURN,
    AT_ERROR_PANIC,

    AT_COUNT
}; // end of enum ActionType

class ActionSpec
{
public:

    ActionSpec () : m_type(AT_NONE), m_data(0) { }
    ActionSpec (ActionType type, Uint32 data = 0) : m_type(type), m_data(data) { }

    ActionType Type () const { return m_type; }
    Uint32 Data () const { return m_data; }

private:

    ActionType m_type;
    Uint32 m_data;
}; // end of class ActionSpec

ostream &operator << (ostream &stream, ActionType const &action_type)
{
    static string const s_action_type_string[AT_COUNT] =
    {
        "AT_NONE",
        "AT_SHIFT",
        "AT_REDUCE",
        "AT_RETURN",
        "AT_ERROR_PANIC"
    };
    assert(action_type >= 0 && action_type < AT_COUNT);
    return stream << s_action_type_string[action_type];
}

// Npda is the actual automaton which simulates a parser.
class Npda
{
private:

    enum BranchListType { BLT_PARSER = 0, BLT_ACTION, BLT_COUNT };

    struct Action;
    struct ActionBranch : public ListNode<ActionBranch> { protected: ActionBranch () { } };
    struct ParserBranch : public ListNode<ParserBranch> { protected: ParserBranch () { } };
    struct Reduce;
    struct Shift;

    typedef vector<Uint32> StateStack;
    typedef Uint32 TokenIndex;
    enum { none__ = 0 }; // single known value for built-in nonterminal

    struct ReduceReduceConflict
    {
        static bool IsHigherPriority (Rule const &left, Rule const &right)
        {
            // order by higher precedence, and then lower rule index
            return left.m_rule_precedence->m_precedence_level > right.m_rule_precedence->m_precedence_level
                   ||
                   left.m_rule_precedence->m_precedence_level == right.m_rule_precedence->m_precedence_level &&
                   left.m_rule_index < right.m_rule_index;
        }
    }; // end of struct Npda::ReduceReduceConflict
    struct ShiftReduceConflict
    {
        static bool IsHigherPriority (Rule const &left, Rule const &right)
        {
            return left.m_rule_precedence->m_precedence_level > right.m_rule_precedence->m_precedence_level;
        }
        static bool IsEqualPriority (Rule const &left, Rule const &right)
        {
            return left.m_rule_precedence->m_precedence_level == right.m_rule_precedence->m_precedence_level;
        }

        struct Order
        {
            bool operator () (Rule const *left, Rule const *right)
            {
                // prefer NULL over non-NULL rules, and then go by Npda::ShiftReduceConflict::IsHigherPriority
                if (left == NULL && right == NULL)
                    return false;
                else if (left == NULL && right != NULL)
                    return true;
                else if (left != NULL && right == NULL)
                    return false;
                else
                    return ShiftReduceConflict::IsHigherPriority(*left, *right);
            }
        }; // end of struct Npda::ShiftReduceConflict::Order
    }; // end of struct Npda::ShiftReduceConflict
    struct TreeNode
    {
        enum TreeNodeType { ACTION = 0, BRANCH, REDUCE, SHIFT };
        TreeNodeType const m_tree_node_type;
        Action *m_parent;

        TreeNode (TreeNodeType tree_node_type)
            :
            m_tree_node_type(tree_node_type),
            m_parent(NULL)
        { }
    }; // end of struct Npda::TreeNode
    typedef WeakReference<Shift> ShiftReference;
    typedef map<Rule const *, ShiftReference, ShiftReduceConflict::Order> ShiftReferenceMap;
    typedef pair<Shift const *, Rule const *> ShiftRulePair;
    typedef pair<Rule const *, ShiftReference> RuleShiftReferencePair;
    typedef map<ShiftRulePair, ShiftReference> DeepCopyShiftReferenceMap;
    struct Branch : public TreeNode, public ActionBranch, public ParserBranch
    {
        typedef vector<RuleShiftReferencePair> ShiftReferenceList;
        ShiftReferenceList m_shift_reference_list;
        StateStack m_state_stack;
        TokenIndex m_lookahead_nonterminal_token_index;
        bool m_is_epsilon_closed;

        Branch (Uint32 starting_state)
            :
            TreeNode(BRANCH),
            m_lookahead_nonterminal_token_index(none__),
            m_is_epsilon_closed(false)
        {
            m_state_stack.push_back(starting_state);
        }
        ~Branch ()
        {
//             cerr << "~Branch(); this = " << this << endl;
            Action *parent = m_parent;
            if (parent != NULL) // a NULL parent is possible with floaty branches
            {
                parent->RemoveChild(this);
                assert(m_parent == NULL);
            }
            // iterate until we're out of parents, we hit the root node,
            // or there are other children which rely on the parent to live.
            while (parent != NULL && parent->m_parent != NULL && !parent->HasChildren())
            {
                TreeNode *node = parent;
                parent = node->m_parent;
                parent->RemoveChild(node);
                assert(node != this);
                delete node;
            }
            // release all the rule references.  if the ref count is about to go to
            // 1, inform the Shift (if it exists) that it should resolve conflicts.
            for (ShiftReferenceList::iterator it = m_shift_reference_list.begin(), it_end = m_shift_reference_list.end();
                 it != it_end;
                 ++it)
            {
                ShiftReference &shift_reference = it->second;
                assert(shift_reference.IsValid());

                if (shift_reference.ReferenceCount() == 2 && shift_reference.InstanceIsValid())
                {
                    Shift *shift = &*shift_reference;
                    assert(shift != NULL);
                    assert(shift->m_parent != NULL);
                    shift_reference.Release();
                    shift->DestroyAllUnusedInRuleRange();
                    shift->m_parent->ResolveShiftReduceConflicts();
                    // DO NOT use shift after this because ResolveShiftReduceConflicts
                    // may delete it.
                }
            }
        }

        Branch *DeepCopy (DeepCopyShiftReferenceMap &deep_copy_shift_reference_map) const
        {
            Branch *copied_branch = new Branch(m_state_stack, m_lookahead_nonterminal_token_index);
            copied_branch->m_is_epsilon_closed = m_is_epsilon_closed;
            DeepCopyGutsInto(*copied_branch, deep_copy_shift_reference_map);
            return copied_branch;
        }
        void DeepCopyGutsInto (Branch &copied_branch, DeepCopyShiftReferenceMap &deep_copy_shift_reference_map) const
        {
            copied_branch.m_is_epsilon_closed = m_is_epsilon_closed;
        }
        void DeepCopyShiftReferencesInto (Branch &copied_branch, DeepCopyShiftReferenceMap &deep_copy_shift_reference_map) const
        {
            for (ShiftReferenceList::const_iterator it = m_shift_reference_list.begin(), it_end = m_shift_reference_list.end();
                 it != it_end;
                 ++it)
            {
                RuleShiftReferencePair const &rule_shift_reference_pair = *it;
                assert(deep_copy_shift_reference_map.find(ShiftRulePair(&*rule_shift_reference_pair.second, rule_shift_reference_pair.first)) != deep_copy_shift_reference_map.end());
                copied_branch.m_shift_reference_list.push_back(
                    RuleShiftReferencePair(
                        rule_shift_reference_pair.first,
                        deep_copy_shift_reference_map[ShiftRulePair(&*rule_shift_reference_pair.second, rule_shift_reference_pair.first)]));
            }
        }

        Uint32 Top () const
        {
            assert(!m_state_stack.empty());
            return m_state_stack.back();
        }
        Branch *Clone () const
        {
            Branch *cloned = new Branch(m_state_stack, m_lookahead_nonterminal_token_index);
            assert(cloned->m_parent == NULL);
            // duplicate the shift reference list
            for (ShiftReferenceList::const_iterator it = m_shift_reference_list.begin(),
                                                    it_end = m_shift_reference_list.end();
                 it != it_end;
                 ++it)
            {
                ShiftReference const &shift_reference = it->second;
                assert(shift_reference.IsValid());
                assert(shift_reference.ReferenceCount() >= 1);
                // only copy the reference if it's valid
                if (shift_reference.InstanceIsValid())
                {
                    assert(&*shift_reference != NULL);
                    cloned->m_shift_reference_list.push_back(RuleShiftReferencePair(it->first, shift_reference));
                }
            }
            return cloned;
        }
        void RemoveFromBranchList (BranchListType branch_list_type)
        {
            assert(branch_list_type == BLT_PARSER || branch_list_type == BLT_ACTION);
            if (branch_list_type == BLT_PARSER)
                ParserBranch::Remove();
            else // branch_list_type == BLT_ACTION
                ActionBranch::Remove();
        }

        void Print (ostream &stream, GraphContext const &graph_context, Uint32 indent_level) const
        {
            stream << string(2*indent_level, ' ') << (m_is_epsilon_closed ? 'c' : '_') << " branch " << this;
            for (StateStack::const_iterator it = m_state_stack.begin(), it_end = m_state_stack.end();
                 it != it_end;
                 ++it)
            {
                Uint32 stack_state = *it;
                stream << ' ' << stack_state;
            }
            if (m_lookahead_nonterminal_token_index != none__)
                stream << " (" << m_lookahead_nonterminal_token_index << ')';
            NpdaNodeData const &top_state_npda_node_data = graph_context.m_npda_graph.GetNode(Top()).GetDataAs<NpdaNodeData>();
            stream << " \"" << top_state_npda_node_data.GetOneLineDescription() << '\"' << endl;
            for (ShiftReferenceList::const_iterator it = m_shift_reference_list.begin(),
                                                    it_end = m_shift_reference_list.end();
                    it != it_end;
                    ++it)
            {
                ShiftReference const &shift_reference = it->second;
                assert(shift_reference.IsValid());
                // TODO: make into assert(shift_reference.ReferenceCount() > 1); -- changes in the ref count should cause cleanup in the rule range
                if (shift_reference.ReferenceCount() > 1)
                {
                    assert(shift_reference.InstanceIsValid());
                    stream << string(2*(indent_level+1), ' ') << "RuleShiftReferencePair -- rule = " << it->first << ", shift* = " << &*shift_reference << ", ref count = " << shift_reference.ReferenceCount() << endl;
                }
            }
        }

    private:

        // for use only by DeepCopy() and Clone()
        Branch (StateStack const &state_stack, TokenIndex lookahead_nonterminal_token_index)
            :
            TreeNode(BRANCH),
            m_state_stack(state_stack),
            m_lookahead_nonterminal_token_index(lookahead_nonterminal_token_index),
            m_is_epsilon_closed(false)
        {
            assert(!m_state_stack.empty());
        }
    }; // end of struct Npda::Branch
    struct ActionBranchList : public List<ActionBranch>
    {
        void Prepend (Branch *node) { List<ActionBranch>::Prepend(static_cast<ActionBranch *>(node)); }
        void Append (Branch *node) { List<ActionBranch>::Append(static_cast<ActionBranch *>(node)); }
    }; // end of struct Npda::ActionBranchList
    struct ParserBranchList : public List<ParserBranch>
    {
        void Prepend (Branch *node) { List<ParserBranch>::Prepend(static_cast<ParserBranch *>(node)); }
        void Append (Branch *node) { List<ParserBranch>::Append(static_cast<ParserBranch *>(node)); }
    }; // end of struct Npda::ParserBranchList
    struct Action : public TreeNode
    {
        // TODO: make non-public
        // there can only ever be up to 2 children, one of each type.
        Reduce *m_reduce_child;
        Shift *m_shift_child;
        ActionBranchList m_action_branch_list;

        Action (TreeNodeType tree_node_type)
            :
            TreeNode(tree_node_type),
            m_reduce_child(NULL),
            m_shift_child(NULL)
        { }
        ~Action ()
        {
//             cerr << "~Action(); this = " << this << endl;

            // delete this and all children out to the leaves
            if (m_parent != NULL)
                m_parent->RemoveChild(this);

            delete m_reduce_child;
            delete m_shift_child;

            ActionBranch *action_branch;
            while ((action_branch = m_action_branch_list.Front()) != NULL)
            {
                Branch *branch = static_cast<Branch *>(action_branch);
                branch->m_parent = NULL; // to stop ~Branch() from interfering
                delete branch;
                assert(m_action_branch_list.Front() != action_branch);
            }
        }

        Action *DeepCopy (DeepCopyShiftReferenceMap &deep_copy_shift_reference_map, Branch const *&source_parser_branch, ParserBranchList &target_parser_branch_list) const
        {
            Action *copied_action = new Action(m_tree_node_type);
            DeepCopyGutsInto(*copied_action, deep_copy_shift_reference_map, source_parser_branch, target_parser_branch_list);
            return copied_action;
        }
        void DeepCopyGutsInto (Action &copied_action, DeepCopyShiftReferenceMap &deep_copy_shift_reference_map, Branch const *&source_parser_branch, ParserBranchList &target_parser_branch_list) const
        {
            // we have to copy the action branches before the reduce/shift children
            for (ActionBranch const *action_branch = m_action_branch_list.Front();
                 action_branch != NULL;
                 action_branch = action_branch->Next())
            {
                Branch *copied_branch = static_cast<Branch const *>(action_branch)->DeepCopy(deep_copy_shift_reference_map);
                copied_action.m_action_branch_list.Append(copied_branch);
                copied_branch->m_parent = &copied_action;

                if (static_cast<Branch const *>(action_branch) == source_parser_branch)
                {
                    target_parser_branch_list.Append(copied_branch);
                    source_parser_branch = static_cast<Branch const *>(source_parser_branch->ParserBranch::Next());
                }
            }

            if (m_reduce_child != NULL)
            {
                copied_action.m_reduce_child = m_reduce_child->DeepCopy(deep_copy_shift_reference_map, source_parser_branch, target_parser_branch_list);
                copied_action.m_reduce_child->m_parent = &copied_action;
            }

            if (m_shift_child != NULL)
            {
                copied_action.m_shift_child = m_shift_child->DeepCopy(deep_copy_shift_reference_map, source_parser_branch, target_parser_branch_list);
                copied_action.m_shift_child->m_parent = &copied_action;
            }
        }
        void DeepCopyShiftReferencesInto (Action &copied_action, DeepCopyShiftReferenceMap &deep_copy_shift_reference_map) const
        {
            if (m_reduce_child != NULL)
            {
                assert(copied_action.m_reduce_child != NULL);
                m_reduce_child->DeepCopyShiftReferencesInto(*copied_action.m_reduce_child, deep_copy_shift_reference_map);
            }

            if (m_shift_child != NULL)
            {
                assert(copied_action.m_shift_child != NULL);
                m_shift_child->DeepCopyShiftReferencesInto(*copied_action.m_shift_child, deep_copy_shift_reference_map);
            }

            ActionBranch const *action_branch;
            ActionBranch *copied_action_branch;
            for (action_branch = m_action_branch_list.Front(), copied_action_branch = copied_action.m_action_branch_list.Front();
                 action_branch != NULL || copied_action_branch != NULL;
                 action_branch = action_branch->Next(), copied_action_branch = copied_action_branch->Next())
            {
                assert(action_branch != NULL && copied_action_branch != NULL);
                static_cast<Branch const *>(action_branch)->DeepCopyShiftReferencesInto(*static_cast<Branch *>(copied_action_branch), deep_copy_shift_reference_map);
            }
        }

        bool HasChildren () const { return m_reduce_child != NULL || m_shift_child != NULL || !m_action_branch_list.IsEmpty(); }
        bool AssertActionAndParserBranchListsAreConsistent (ParserBranchList const &parser_branch_list) const
        {
            ParserBranch const *parser_branch = parser_branch_list.Front();
            AssertActionAndParserBranchListsAreConsistent(parser_branch);
            return true; // dummy value so this can be wrapped in assert()
        }
        TreeNode *SingleChild ()
        {
            if (m_reduce_child != NULL && m_shift_child == NULL && m_action_branch_list.IsEmpty())
                return m_reduce_child;
            else if (m_reduce_child == NULL && m_shift_child != NULL && m_action_branch_list.IsEmpty())
                return m_shift_child;
            else if (m_reduce_child == NULL && m_shift_child == NULL && !m_action_branch_list.IsEmpty() && m_action_branch_list.Front() == m_action_branch_list.Back())
                return static_cast<Branch const *>(m_action_branch_list.Front());
            else
                return NULL;
        }
        ActionBranch *LastSubordinateBranch ()
        {
            if (m_shift_child != NULL)
            {
                ActionBranch *last_subordinate_branch = m_shift_child->LastSubordinateBranch();
                if (last_subordinate_branch != NULL)
                    return last_subordinate_branch;
            }
            if (m_reduce_child != NULL)
            {
                ActionBranch *last_subordinate_branch = m_reduce_child->LastSubordinateBranch();
                if (last_subordinate_branch != NULL)
                    return last_subordinate_branch;
            }
            {
                ActionBranch *last_subordinate_branch = m_action_branch_list.Back();
                if (last_subordinate_branch != NULL)
                    return last_subordinate_branch;
            }
            return NULL;
        }
        bool ReduceActionWouldSucceed (Rule const *reduction_rule) const
        {
            assert(reduction_rule != NULL);
            assert(m_shift_child == NULL && "this should never happen");
            return m_reduce_child == NULL || !ReduceReduceConflict::IsHigherPriority(*m_reduce_child->m_reduction_rule, *reduction_rule);
        }

        void AddChildBranch (Branch *child)
        {
            assert(child != NULL);
            assert(child->m_parent == NULL);
            assert(!child->ActionBranch::IsAnElement());
            assert(!child->ParserBranch::IsAnElement());
            assert(child->m_tree_node_type == BRANCH);
            child->m_parent = this;
            // no need to call ResolveShiftReduceConflicts() because it is
            // a no-op when the action branch list is non-empty.
        }
        void RemoveChild (TreeNode *child)
        {
            assert(child != NULL);
            assert(child->m_parent == this);
            switch (child->m_tree_node_type)
            {
                default:
                case ACTION: assert(false && "this should never happen"); break;
                case BRANCH: static_cast<Branch *>(child)->RemoveFromBranchList(BLT_ACTION); break;
                case REDUCE: assert(child == m_reduce_child); m_reduce_child = NULL; if (m_shift_child != NULL) m_shift_child->ClearRuleRange(); break;
                case SHIFT : assert(child == m_shift_child); m_shift_child = NULL; break;
            }
            child->m_parent = NULL;
            ResolveShiftReduceConflicts();
        }
        void AddEpsilonClosure (Branch &source_branch, StateStack &extra_state_stack)
        {
            assert(source_branch.m_parent == this);
            // TODO: here is where to check for branch collisions

            // clone the source branch, push the states contained in extra_state_stack.
            Branch *closed_branch = source_branch.Clone();
            assert(closed_branch->m_parent == NULL);
            closed_branch->m_is_epsilon_closed = true;
            closed_branch->m_state_stack.insert(closed_branch->m_state_stack.end(), extra_state_stack.begin(), extra_state_stack.end());
            // add the closed branch as a child
            AddChildBranch(closed_branch);
            // add the closed branch to the action and parser branch lists before source_branch
            source_branch.ActionBranch::InsertBefore(closed_branch);
            source_branch.ParserBranch::InsertBefore(closed_branch);
        }
        void AddReduce (Branch &source_branch, Rule const *reduction_rule)
        {
            assert(source_branch.m_parent == this);
            assert(m_shift_child == NULL);
            assert(reduction_rule != NULL);
            assert(ReduceActionWouldSucceed(reduction_rule) && "nice try, jerk");
            // three possible cases here: 1. no existing reduce child, so the
            // incoming branch gets added trivially.  2. the incoming reduction
            // rule has higher priority than that of the reduce child; so prune
            // the reduce child and use the incoming reduction rule instead.
            // 3. the incoming reduction rule is of equal priority as the existing
            // reduce child; so add the incoming branch to the reduce child.
            assert(m_reduce_child == NULL || !ReduceReduceConflict::IsHigherPriority(*m_reduce_child->m_reduction_rule, *reduction_rule));
            // in case 1 or 2, we must prune a possibly existing reduce child and recreate it.
            if (m_reduce_child == NULL || ReduceReduceConflict::IsHigherPriority(*reduction_rule, *m_reduce_child->m_reduction_rule))
            {
                delete m_reduce_child;
                m_reduce_child = new Reduce(reduction_rule);
                m_reduce_child->m_parent = this;
            }
            // clone the source branch and operate on it
            Branch *reduced_branch = source_branch.Clone();
            assert(reduced_branch != NULL);
            if (g_minimal_npda_graphing)
            {
                assert(reduced_branch->m_state_stack.size() > reduction_rule->m_rule_token_list->size() + 1);
                reduced_branch->m_state_stack.resize(reduced_branch->m_state_stack.size() - (reduction_rule->m_rule_token_list->size() + 1));
            }
            else
            {
                assert(reduced_branch->m_state_stack.size() > reduction_rule->m_rule_token_list->size() + 2);
                reduced_branch->m_state_stack.resize(reduced_branch->m_state_stack.size() - (reduction_rule->m_rule_token_list->size() + 2));
            }
            reduced_branch->m_lookahead_nonterminal_token_index = reduction_rule->m_owner_nonterminal->m_token_index;
            // add the reduced branch as a child of the reduce child
            m_reduce_child->AddChildBranch(reduced_branch);
            // add the reduced branch to the proper place in the action and parser
            // branch lists indicated by members in various action branch lists.
            if (m_reduce_child->m_action_branch_list.IsEmpty())
            {
                // if the reduce child's action branch list is empty, add the
                // reduced branch to the front of the empty reduce child's action
                // branch list, and to parser branch list corresponding to the end
                // of this node's action branch list.
                assert(!m_action_branch_list.IsEmpty());
                assert(m_action_branch_list.Back() != NULL);
                static_cast<Branch *>(m_action_branch_list.Back())->ParserBranch::InsertAfter(reduced_branch);
                m_reduce_child->m_action_branch_list.Prepend(reduced_branch);
            }
            else
            {
                // otherwise, we can add it directly before the first branch
                // belonging to the reduce child -- in both branch lists.
                assert(m_reduce_child->m_action_branch_list.Front() != NULL);
                static_cast<Branch *>(m_reduce_child->m_action_branch_list.Front())->ParserBranch::InsertBefore(reduced_branch);
                m_reduce_child->m_action_branch_list.Front()->InsertBefore(reduced_branch);
            }
        }
        void AddShift (GraphContext const &graph_context, Branch &source_branch, Uint32 target_state)
        {
            assert(source_branch.m_parent == this);
            // associated_rule may be NULL when shifting into a return state
            if (m_shift_child == NULL)
            {
                m_shift_child = new Shift();
                m_shift_child->m_parent = this;
            }
            // clone the source branch and operate on it
            Branch *shifted_branch = source_branch.Clone();
            assert(shifted_branch != NULL);
            shifted_branch->m_state_stack.push_back(target_state);
            shifted_branch->m_lookahead_nonterminal_token_index = none__;
//             cerr << "adding parser branch " << static_cast<ParserBranch *>(shifted_branch)
//                       << " which is action branch " << static_cast<ActionBranch *>(shifted_branch)
//                       << endl;
            // add the shifted branch as a child of the shift child
            m_shift_child->AddChildBranch(shifted_branch);
            // add the shifted branch to the proper place in the parser branch list
            // indicated by members in various action branch lists.
            if (m_shift_child->m_action_branch_list.IsEmpty())
            {
                // if the shift child's action branch list is empty, we'll have to
                // attempt to use elements of the reduce child to determine where to
                // add the branch into the branch lists.
                ActionBranch *last_reduce_subordinate_branch = m_reduce_child != NULL ? m_reduce_child->LastSubordinateBranch() : NULL;
                if (last_reduce_subordinate_branch == NULL)
                {
                    // if there are no reduce subordinate branches, we'll add it after the
                    // this node's last action branch list member.
                    assert(!m_action_branch_list.IsEmpty());
                    assert(m_action_branch_list.Back() != NULL);
                    static_cast<Branch *>(m_action_branch_list.Back())->ParserBranch::InsertAfter(shifted_branch);
                }
                else
                {
                    // otherwise, we can add it to the parser branch list directly after
                    // the branch corresponding to the last branch in the parser branch
                    // list subordinate to the reduce child.
                    static_cast<Branch *>(last_reduce_subordinate_branch)->ParserBranch::InsertAfter(shifted_branch);
                }
            }
            else
            {
                // otherwise, we can add it directly before the first branch
                // belonging to the shift child.  NOTE: this could be InsertAfter
                assert(m_shift_child->m_action_branch_list.Front() != NULL);
                static_cast<Branch *>(m_shift_child->m_action_branch_list.Front())->ParserBranch::InsertBefore(shifted_branch);
            }
            // add the shifted branch to the shift child's action branch list.
            m_shift_child->m_action_branch_list.Prepend(shifted_branch);
            // only add a shift reference if there could be a conflict
            if (m_reduce_child != NULL)
            {
                assert(graph_context.m_npda_graph.GetNode(target_state).GetHasData());
                NpdaNodeData const &npda_node_data = graph_context.m_npda_graph.GetNode(target_state).GetDataAs<NpdaNodeData>();
                ShiftReference shift_reference = m_shift_child->EnsureShiftReferenceExists(npda_node_data.GetAssociatedRule());
                assert(&*shift_reference == m_shift_child);
                shifted_branch->m_shift_reference_list.push_back(RuleShiftReferencePair(npda_node_data.GetAssociatedRule(), shift_reference));
            }
        }
        void ResolveShiftReduceConflicts ()
        {
            // the only time this should really be called is when adding/removing
            // a child or when the rule range on a shift child changes.

            // no conflicts can be resolved until there are no
            // pending child branches at this node.
            if (!m_action_branch_list.IsEmpty())
                return;

            // there is no conflict if there is zero or one child
            if (m_reduce_child == NULL || m_shift_child == NULL)
                return;

            // when we have both a reduce child and a shift child, it should be impossible
            // for the shift child to have shift references mapped to NULL rules (because
            // this can only happen while shifting into return states, which requires a
            // lookahead nonterminal.  but when this is the case, no reduces can happen.
            assert(!m_shift_child->IsRuleRangeEmpty());
            Rule const *high_shift_rule = m_shift_child->RangeRuleHigh();
            Rule const *low_shift_rule = m_shift_child->RangeRuleLow();
            Rule const *reduce_rule = m_reduce_child->m_reduction_rule;
            assert(high_shift_rule != NULL);
            assert(low_shift_rule != NULL);
            assert(reduce_rule != NULL);
            assert(!ShiftReduceConflict::IsHigherPriority(*low_shift_rule, *high_shift_rule));

            // 6 possibilities (the higher lines indicate higher priority.  same line
            // indicates equality).  there is always exactly one reduce, and at least
            // one shift.
            //
            // 1.     shift        2.     shift        3.
            //        shift               shift
            // reduce              reduce shift        reduce shift
            //
            // 4.                  5.                  6.
            //                                                shift
            // reduce shift        reduce              reduce shift
            //        shift               shift               shift
            //        shift               shift
            //
            // cases 1 and 5 can be trivially resolved -- by pruning the reduce
            // and by pruning the shift respectively.
            //
            // case 3 may be trivially resolved via rule associativity (LEFT causes the
            // shift to be pruned, RIGHT causes the reduce to be pruned, and NONASSOC
            // should cause an error).
            //
            // case 4 can only be resolved if the associativity of the reduction rule
            // is LEFT, otherwise no resolution can be reached at this point.
            //
            // case 2 can only be resolved if the associativity of the reduction rule
            // is RIGHT, otherwise no resolution can be reached at this point.
            //
            // case 6 can not be resolved at this point.

            if (ShiftReduceConflict::IsHigherPriority(*low_shift_rule, *reduce_rule))
            {   // case 1
                delete m_reduce_child;
                m_reduce_child = NULL;
                m_shift_child->ClearRuleRange();
            }
            else if (ShiftReduceConflict::IsHigherPriority(*reduce_rule, *high_shift_rule))
            {   // case 5
                delete m_shift_child;
                m_shift_child = NULL;
            }
            else if (ShiftReduceConflict::IsEqualPriority(*reduce_rule, *high_shift_rule) &&
                     ShiftReduceConflict::IsEqualPriority(*reduce_rule, *low_shift_rule))
            {   // case 3
                assert(low_shift_rule->m_rule_precedence->m_precedence_level == high_shift_rule->m_rule_precedence->m_precedence_level);
                assert(low_shift_rule->m_rule_precedence->m_precedence_associativity == high_shift_rule->m_rule_precedence->m_precedence_associativity);
                assert(reduce_rule->m_rule_precedence->m_precedence_level == high_shift_rule->m_rule_precedence->m_precedence_level);
                assert(reduce_rule->m_rule_precedence->m_precedence_associativity == high_shift_rule->m_rule_precedence->m_precedence_associativity);
                if (reduce_rule->m_rule_precedence->m_precedence_associativity == A_LEFT)
                {
                    delete m_shift_child;
                    m_shift_child = NULL;
                }
                else if (reduce_rule->m_rule_precedence->m_precedence_associativity == A_RIGHT)
                {
                    delete m_reduce_child;
                    m_reduce_child = NULL;
                    m_shift_child->ClearRuleRange();
                }
                else // reduce_rule->m_rule_precedence->m_precedence_associativity == A_NONASSOC
                {
                    assert(reduce_rule->m_rule_precedence->m_precedence_associativity == A_NONASSOC);
                    assert(false && "nonassoc error handling not implemented yet");
                }
            }
            else if (ShiftReduceConflict::IsEqualPriority(*reduce_rule, *high_shift_rule) &&
                     ShiftReduceConflict::IsHigherPriority(*reduce_rule, *low_shift_rule))
            {   // case 4
                assert(reduce_rule->m_rule_precedence->m_precedence_level == high_shift_rule->m_rule_precedence->m_precedence_level);
                assert(reduce_rule->m_rule_precedence->m_precedence_associativity == high_shift_rule->m_rule_precedence->m_precedence_associativity);
                if (reduce_rule->m_rule_precedence->m_precedence_associativity == A_LEFT)
                {
                    delete m_shift_child;
                    m_shift_child = NULL;
                }
            }
            else if (ShiftReduceConflict::IsEqualPriority(*reduce_rule, *low_shift_rule) &&
                     ShiftReduceConflict::IsHigherPriority(*high_shift_rule, *reduce_rule))
            {   // case 2
                assert(reduce_rule->m_rule_precedence->m_precedence_level == low_shift_rule->m_rule_precedence->m_precedence_level);
                assert(reduce_rule->m_rule_precedence->m_precedence_associativity == low_shift_rule->m_rule_precedence->m_precedence_associativity);
                if (reduce_rule->m_rule_precedence->m_precedence_associativity == A_RIGHT)
                {
                    delete m_reduce_child;
                    m_reduce_child = NULL;
                    m_shift_child->ClearRuleRange();
                }
            }
            else
            {   // case 6
                // nothing can be done at this point
            }
        }

        void Print (ostream &stream, GraphContext const &graph_context, Uint32 indent_level) const
        {
            // fake polymorphism here, because we don't want to use virtual inheritance
            if (m_parent == NULL) // special treatment for the root
                PrivatePrint(stream, graph_context, indent_level);
            else switch (m_tree_node_type)
            {
                default:
                case BRANCH:
                case ACTION: assert(false && "this should never happen"); break;
                case REDUCE: static_cast<Reduce const *>(this)->Print(stream, graph_context, indent_level); break;
                case SHIFT : static_cast<Shift const *>(this)->Print(stream, graph_context, indent_level); break;
            }
        }

    protected:

        void PrintChildNodes (ostream &stream, GraphContext const &graph_context, Uint32 indent_level) const
        {
            for (ActionBranch const *action_branch = m_action_branch_list.Front(); action_branch != NULL; action_branch = action_branch->Next())
                static_cast<Branch const *>(action_branch)->Print(stream, graph_context, indent_level);
            if (m_reduce_child != NULL)
                m_reduce_child->Print(stream, graph_context, indent_level);
            if (m_shift_child != NULL)
                m_shift_child->Print(stream, graph_context, indent_level);
        }

    private:

        void AssertActionAndParserBranchListsAreConsistent (ParserBranch const *&parser_branch) const
        {
            if (parser_branch == NULL)
                assert(!HasChildren());
            for (ActionBranch const *action_branch = m_action_branch_list.Front();
                 action_branch != NULL;
                 action_branch = action_branch->Next(), parser_branch = parser_branch != NULL ? parser_branch->Next() : parser_branch)
            {
//                 cerr << "checking parser branch " << static_cast<Branch const *>(parser_branch)
//                           << " against action branch " << static_cast<Branch const *>(action_branch)
//                           << endl;
                assert(parser_branch != NULL && static_cast<Branch const *>(parser_branch) == static_cast<Branch const *>(action_branch));
            }
            if (m_reduce_child != NULL)
                m_reduce_child->AssertActionAndParserBranchListsAreConsistent(parser_branch);
            if (m_shift_child != NULL)
                m_shift_child->AssertActionAndParserBranchListsAreConsistent(parser_branch);
        }
        void PrivatePrint (ostream &stream, GraphContext const &graph_context, Uint32 indent_level) const
        {
            assert(m_parent == NULL);
            stream << string(2*indent_level, ' ') << "root " << this << endl;
            PrintChildNodes(stream, graph_context, indent_level+1);
        }
    }; // end of struct Npda::Action
    struct Reduce : public Action
    {
        Rule const *m_reduction_rule;

        Reduce (Rule const *reduction_rule)
            :
            Action(REDUCE),
            m_reduction_rule(reduction_rule)
        {
            assert(m_reduction_rule != NULL);
        }

        Reduce *DeepCopy (DeepCopyShiftReferenceMap &deep_copy_shift_reference_map, Branch const *&source_parser_branch, ParserBranchList &target_parser_branch_list) const
        {
            Reduce *copied_reduce = new Reduce(m_reduction_rule);
            DeepCopyGutsInto(*copied_reduce, deep_copy_shift_reference_map, source_parser_branch, target_parser_branch_list);
            return copied_reduce;
        }
        void DeepCopyGutsInto (Reduce &copied_reduce, DeepCopyShiftReferenceMap &deep_copy_shift_reference_map, Branch const *&source_parser_branch, ParserBranchList &target_parser_branch_list) const
        {
            Action::DeepCopyGutsInto(copied_reduce, deep_copy_shift_reference_map, source_parser_branch, target_parser_branch_list);
        }
        void DeepCopyShiftReferencesInto (Reduce &copied_reduce, DeepCopyShiftReferenceMap &deep_copy_shift_reference_map) const
        {
            Action::DeepCopyShiftReferencesInto(copied_reduce, deep_copy_shift_reference_map);
        }

        void Print (ostream &stream, GraphContext const &graph_context, Uint32 indent_level) const
        {
            stream << string(2*indent_level, ' ') << (m_parent != NULL ? "reduce " : "root ") << this
                   << " \"" << m_reduction_rule->GetAsText() << '\"'
                   << ", precedence = " << m_reduction_rule->m_rule_precedence->m_precedence_level
                   << ", associativity = " << m_reduction_rule->m_rule_precedence->m_precedence_associativity
                   << ", rule index = " << m_reduction_rule->m_rule_index << endl;
            PrintChildNodes(stream, graph_context, indent_level+1);
        }
    }; // end of struct Npda::Reduce
    struct Shift : public Action
    {
        Shift () : Action(SHIFT) { }
        ~Shift () { ClearRuleRange(); }

        Shift *DeepCopy (DeepCopyShiftReferenceMap &deep_copy_shift_reference_map, Branch const *&source_parser_branch, ParserBranchList &target_parser_branch_list) const
        {
            Shift *copied_shift = new Shift();
            DeepCopyGutsInto(*copied_shift, deep_copy_shift_reference_map, source_parser_branch, target_parser_branch_list);
            return copied_shift;
        }
        void DeepCopyGutsInto (Shift &copied_shift, DeepCopyShiftReferenceMap &deep_copy_shift_reference_map, Branch const *&source_parser_branch, ParserBranchList &target_parser_branch_list) const
        {
            Action::DeepCopyGutsInto(copied_shift, deep_copy_shift_reference_map, source_parser_branch, target_parser_branch_list);
            for (ShiftReferenceMap::const_iterator it = m_rule_range.begin(), it_end = m_rule_range.end();
                 it != it_end;
                 ++it)
            {
                Rule const *rule = it->first;
                assert(&*it->second == this);
                assert(deep_copy_shift_reference_map.find(ShiftRulePair(this, rule)) == deep_copy_shift_reference_map.end());
                deep_copy_shift_reference_map[ShiftRulePair(this, rule)] = ShiftReference(&copied_shift);
            }
        }
        void DeepCopyShiftReferencesInto (Shift &copied_shift, DeepCopyShiftReferenceMap &deep_copy_shift_reference_map) const
        {
            Action::DeepCopyShiftReferencesInto(copied_shift, deep_copy_shift_reference_map);
            for (ShiftReferenceMap::const_iterator it = m_rule_range.begin(), it_end = m_rule_range.end();
                 it != it_end;
                 ++it)
            {
                Rule const *rule = it->first;
                ShiftReference const &shift_reference = it->second;
                assert(deep_copy_shift_reference_map.find(ShiftRulePair(&*shift_reference, rule)) != deep_copy_shift_reference_map.end());
                assert(&*deep_copy_shift_reference_map[ShiftRulePair(&*shift_reference, rule)] == &copied_shift);
                copied_shift.m_rule_range[rule] = deep_copy_shift_reference_map[ShiftRulePair(&*shift_reference, rule)];
            }
        }

        bool IsRuleRangeEmpty () const { return m_rule_range.empty(); }
        Rule const *RangeRuleHigh () const { assert(!m_rule_range.empty()); return m_rule_range.begin()->first; }
        Rule const *RangeRuleLow () const { assert(!m_rule_range.empty()); return m_rule_range.rbegin()->first; }

        ShiftReference EnsureShiftReferenceExists (Rule const *rule)
        {
            ShiftReferenceMap::iterator it = m_rule_range.find(rule);
            if (it == m_rule_range.end())
            {
                ShiftReference &shift_reference = m_rule_range[rule] = ShiftReference(this);
                assert(shift_reference.ReferenceCount() == 1);
                return shift_reference;
            }
            else
            {
                ShiftReference &shift_reference = it->second;
                assert(shift_reference.ReferenceCount() >= 1);
                return shift_reference;
            }
        }
        void ClearRuleRange ()
        {
            // go through all rule references and release the instances (so any
            // branches which refer to them can know that they're no longer active)
            for (ShiftReferenceMap::iterator it = m_rule_range.begin(), it_end = m_rule_range.end();
                 it != it_end;
                 ++it)
            {
                ShiftReference &shift_reference = it->second;
                assert(shift_reference.IsValid());
                assert(shift_reference.ReferenceCount() >= 1);
//                 assert(shift_reference.InstanceIsValid()); // NOTE: this might/might not be necessary
                shift_reference.InstanceRelease();
                assert(shift_reference.IsValid());
                assert(shift_reference.ReferenceCount() >= 1);
                assert(!shift_reference.InstanceIsValid());
            }
            m_rule_range.clear();
        }
        void DestroyAllUnusedInRuleRange ()
        {
            // go through all rule references and erase/destruct all references
            // that have a ref count of 1 (indicating we're the only ones using
            // them).  the wacky iteration is so we don't have to worry about
            // incrementing an erased iterator.
            for (ShiftReferenceMap::iterator next_it = m_rule_range.begin(),
                                             it_end = m_rule_range.end(),
                                             it = (next_it != it_end) ? next_it++ : next_it;
                 it != it_end;
                 it = (next_it != it_end) ? next_it++ : next_it)
            {
                ShiftReference &shift_reference = it->second;
                assert(shift_reference.IsValid());
                assert(shift_reference.ReferenceCount() >= 1);
                if (shift_reference.ReferenceCount() == 1)
                    m_rule_range.erase(it); // this should call ~ShiftReference()
            }
        }

        void Print (ostream &stream, GraphContext const &graph_context, Uint32 indent_level) const
        {
            stream << string(2*indent_level, ' ') << (m_parent != NULL ? "shift " : "root ") << this << endl;
            if (!m_rule_range.empty())
                stream << string(2*(indent_level+1), ' ') << "rule range" << endl;
            for (ShiftReferenceMap::const_iterator it = m_rule_range.begin(), it_end = m_rule_range.end();
                 it != it_end;
                 ++it)
            {
                Rule const *rule = it->first;
                ShiftReference const &shift_reference = it->second;
                assert(shift_reference.IsValid());
                assert(shift_reference.InstanceIsValid());
                // TODO: make into assert(shift_reference.ReferenceCount() > 1); -- changes in the ref count should cause cleanup in the rule range
                if (shift_reference.ReferenceCount() > 1)
                {
                    assert(shift_reference.InstanceIsValid());
                    stream << string(2*(indent_level+2), ' ') << rule;
                    if (rule != NULL)
                        stream << " \"" << rule->GetAsText() << '\"'
                               << ", precedence = " << rule->m_rule_precedence->m_precedence_level
                               << ", associativity = " << rule->m_rule_precedence->m_precedence_associativity
                               << ", rule index = " << rule->m_rule_index
                               << ", ref count = " << shift_reference.ReferenceCount()
                               << ", shift* = " << &*shift_reference;
                    stream << endl;
                }
            }
            // TODO: print rule range
            PrintChildNodes(stream, graph_context, indent_level+1);
        }

    private:

        ShiftReferenceMap m_rule_range;
    }; // end of struct Npda::Shift

public:

    enum { DONT_FORCE_IS_SHIFT_BLOCKED = false, FORCE_IS_SHIFT_BLOCKED = true };

    Npda (GraphContext const &graph_context, Nonterminal const *start_nonterminal)
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

        PerformEpsilonClosure(graph_context);
    }
    Npda (Npda const &npda, GraphContext const &graph_context)
    {
        // make a deep copy of the tree
        {
            {
                DeepCopyShiftReferenceMap deep_copy_shift_reference_map;
                Branch const *source_parser_branch = static_cast<Branch const *>(npda.m_parser_branch_list.Front());
                // TODO make real versoin of this
                switch (npda.m_tree_root->m_tree_node_type)
                {
                    default:
                    case TreeNode::BRANCH:
                        assert(false && "impossible/invalid case");
                        break;

                    case TreeNode::ACTION:
                        m_tree_root = npda.m_tree_root->DeepCopy(deep_copy_shift_reference_map, source_parser_branch, m_parser_branch_list);
                        npda.m_tree_root->DeepCopyShiftReferencesInto(*m_tree_root, deep_copy_shift_reference_map);
                        break;

                    case TreeNode::REDUCE:
                    {
                        Reduce *tree_root = static_cast<Reduce *>(npda.m_tree_root)->DeepCopy(deep_copy_shift_reference_map, source_parser_branch, m_parser_branch_list);
                        static_cast<Reduce *>(npda.m_tree_root)->DeepCopyShiftReferencesInto(*tree_root, deep_copy_shift_reference_map);
                        m_tree_root = static_cast<Action *>(tree_root);
                        break;
                    }

                    case TreeNode::SHIFT:
                    {
                        Shift *tree_root = static_cast<Shift *>(npda.m_tree_root)->DeepCopy(deep_copy_shift_reference_map, source_parser_branch, m_parser_branch_list);
                        static_cast<Shift *>(npda.m_tree_root)->DeepCopyShiftReferencesInto(*tree_root, deep_copy_shift_reference_map);
                        m_tree_root = static_cast<Action *>(tree_root);
                        break;
                    }
                }
                assert(source_parser_branch == NULL);
            }
/*
            cerr << "TESTING -- original m_tree_root:" << endl;
            npda.m_tree_root->Print(cerr, graph_context, 0);
            cerr << "TESTING -- deep copied m_tree_root:" << endl;
            m_tree_root->Print(cerr, graph_context, 0);
            cerr << endl << endl;

            cerr << "TESTING -- original m_parser_branch_list:" << endl;
            for (ParserBranch const *parser_branch = npda.m_parser_branch_list.Front();
                parser_branch != NULL;
                parser_branch = parser_branch->Next())
            {
                Branch const *branch = static_cast<Branch const *>(parser_branch);
                branch->Print(cerr, graph_context, 1);
            }
            cerr << "TESTING -- deep copied m_parser_branch_list:" << endl;
            for (ParserBranch const *parser_branch = m_parser_branch_list.Front();
                parser_branch != NULL;
                parser_branch = parser_branch->Next())
            {
                Branch const *branch = static_cast<Branch const *>(parser_branch);
                branch->Print(cerr, graph_context, 1);
            }
            cerr << endl;
*/
        }

        m_is_shift_blocked = npda.m_is_shift_blocked;
        m_reduce_transitions_were_performed = npda.m_reduce_transitions_were_performed;
        m_shift_transitions_were_performed = npda.m_shift_transitions_were_performed;
        m_nonassoc_error_encountered = npda.m_nonassoc_error_encountered;
    }
    ~Npda ()
    {
        delete m_tree_root;
    }

    DpdaState CurrentDpdaState () const
    {
        DpdaState current_dpda_state;

        for (ParserBranch const *parser_branch = m_parser_branch_list.Front();
             parser_branch != NULL;
             parser_branch = parser_branch->Next())
        {
            Branch const *branch = static_cast<Branch const *>(parser_branch);
            current_dpda_state.insert(branch->m_state_stack.back());
        }

        return current_dpda_state;
    }
    ActionSpec DefaultAction (GraphContext const &graph_context) const
    {
        assert(m_tree_root != NULL);

        // if a reduce action occurred, then that's the one.
        if (m_tree_root->m_reduce_child != NULL)
            return ActionSpec(AT_REDUCE, m_tree_root->m_reduce_child->m_reduction_rule->m_rule_index);

        TransitionIterator it(graph_context, CurrentDpdaState(), IT_RETURN_TRANSITIONS);
        // if there is a return transition, make sure it's the only one (theoretically,
        // it should only be possible to get to a single return transition at a time).
        if (!it.IsDone())
        {
            Uint32 return_nonterminal_token_index = it->Data(0);
            ++it;
            assert(it.IsDone() && "somehow more than one return transition was encountered");
            return ActionSpec(AT_RETURN, return_nonterminal_token_index);
        }

        // the last resort is to error panic.
        return ActionSpec(AT_ERROR_PANIC);
    }
    bool HasTrunk () const
    {
        assert(m_tree_root != NULL);
        // we can't have any action branches at the tree root
        if (!m_tree_root->m_action_branch_list.IsEmpty())
            return false;
        // there must either be only a reduce or only a shift child.
        return m_tree_root->m_reduce_child != NULL && m_tree_root->m_shift_child == NULL
               ||
               m_tree_root->m_reduce_child == NULL && m_tree_root->m_shift_child != NULL;
    }
    ActionSpec FirstTrunkAction (Uint32 lookahead_token_index) const
    {
        if (!HasTrunk())
            return ActionSpec(AT_NONE);
        else if (m_tree_root->m_reduce_child != NULL)
            return ActionSpec(AT_REDUCE, m_tree_root->m_reduce_child->m_reduction_rule->m_rule_index);
        else
            return ActionSpec(AT_SHIFT, lookahead_token_index);
    }
    void RemoveTrunk ()
    {
        Action *action = m_tree_root;

        while (true)
        {
            assert(action != NULL);

            // if we're at a branch or a fork, stop iterating
            TreeNode *single_child = action->SingleChild();
            if (single_child == NULL || single_child->m_tree_node_type == TreeNode::BRANCH)
                break;

            action = static_cast<Action *>(single_child);
        }

        // the trunk has been executed.  now delete it.
        while (m_tree_root != action)
        {
            TreeNode *single_child = m_tree_root->SingleChild();
            assert(single_child->m_tree_node_type == TreeNode::REDUCE ||
                   single_child->m_tree_node_type == TreeNode::SHIFT);
            m_tree_root->RemoveChild(single_child);
            switch (m_tree_root->m_tree_node_type)
            {
                default:
                case TreeNode::BRANCH:
                    assert(false && "this should never happen");
                    break;

                case TreeNode::ACTION:
//                     cerr << "deleting TreeNode::ACTION " << m_tree_root << endl;
                    delete m_tree_root;
                    break;

                case TreeNode::REDUCE:
//                     cerr << "deleting TreeNode::REDUCE " << static_cast<Reduce *>(m_tree_root) << endl;
                    delete static_cast<Reduce *>(m_tree_root);
                    break;

                case TreeNode::SHIFT:
//                     cerr << "deleting TreeNode::SHIFT " << static_cast<Shift *>(m_tree_root) << endl;
                    delete static_cast<Shift *>(m_tree_root);
                    break;
            }
            m_tree_root = static_cast<Action *>(single_child);
        }
    }

    // runs the npda, with no input, until nothing more can be done without more input, or HasTrunk()
    void Run (GraphContext const &graph_context)
    {
//         cerr << __PRETTY_FUNCTION__ << endl;
        Run(graph_context, Graph::Transition::DataArray());
    }
    // runs the npda, with a single lookahead token, until nothing more can be done without more input, or HasTrunk()
    void Run (GraphContext const &graph_context, TokenIndex lookahead_token_index)
    {
//         cerr << __PRETTY_FUNCTION__ << endl;
        Run(graph_context, Graph::Transition::DataArray(1, lookahead_token_index));
    }
    // runs the npda, with a single lookahead token, until nothing more can be done without more input, or HasTrunk()
    void RunUsingNonterminalLookahead (GraphContext const &graph_context, TokenIndex lookahead_nonterminal_token_index)
    {
//         cerr << __PRETTY_FUNCTION__ << endl;
        m_is_shift_blocked = true;
        for (ParserBranch *parser_branch = m_parser_branch_list.Front();
             parser_branch != NULL;
             parser_branch = parser_branch->Next())
        {
            Branch *branch = static_cast<Branch *>(parser_branch);
            branch->m_lookahead_nonterminal_token_index = lookahead_nonterminal_token_index;
        }
        Run(graph_context, Graph::Transition::DataArray());
    }
    // runs the npda, with a sequence of lookahead tokens, until nothing more can be done without more input, or HasTrunk()
    void Run (GraphContext const &graph_context, Graph::Transition::DataArray lookahead_sequence)
    {
//         cerr << __PRETTY_FUNCTION__ << ", lookahead_sequence: " << GetLookaheadSequenceString(graph_context.m_primary_source, lookahead_sequence) << endl;

        assert(m_tree_root != NULL);
        assert(!m_parser_branch_list.IsEmpty());

        DEBUG_CODE(cerr << "!!! start !!!" << endl;)
        DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
        assert(m_tree_root->AssertActionAndParserBranchListsAreConsistent(m_parser_branch_list));

        while (true)
        {
            DEBUG_CODE(cerr << endl << endl << "!!! iteration (lookahead_sequence: " << GetLookaheadSequenceString(graph_context.m_primary_source, lookahead_sequence) << ") !!!" << endl;)

            assert(!m_parser_branch_list.IsEmpty());
            assert(m_doomed_nonreturn_branch_list.IsEmpty());
            assert(m_doomed_return_branch_list.IsEmpty());

            // e-closure nodes should not be added for self-recursive rules.  e-closures
            // should only be done on non-closed branches.  the rule stack level should
            // be incremented on the transition after an e-closure.
            DEBUG_CODE(cerr << "*** e-close ***" << endl;)
            PerformEpsilonClosure(graph_context);
            DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
            // only schedule reduce transitions if no branches have nonterminal lookaheads
            // (equivalent to not being shift-blocked)
            m_reduce_transitions_were_performed = false;
            m_shift_transitions_were_performed = false;
            // we can only perform reduce transitions if we're not blocked with
            // lookahead nonterminals.  we must wait to shift the lookahead nonterminals
            // so all branches can stay in lock-step with the scanner input.
            if (!m_is_shift_blocked)
                PerformReduceTransitions(graph_context);

            if (m_reduce_transitions_were_performed)
            {
                DEBUG_CODE(cerr << "*** reduce transitions ***" << endl;)
                m_is_shift_blocked = true;
                DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
            }
            // we can only ever perform shift transitions if no reduce transitions were
            // scheduled, because we want to keep all branches' lookahead reading in sync.
            else
            {
                // if we're not shift-blocked and lookahead_sequence is empty, then
                // there is nothing more we can do without more input, so we're done
                // running the npda for now.
                if (!m_is_shift_blocked && lookahead_sequence.empty())
                    break;

                // if we are currently shift-blocked, only schedule shift transitions for
                // branches which have a nonterminal lookahead.  during shift-transition
                // scheduling, if a branch has no transitions, it is scheduled to be pruned.
                PerformShiftTransitions(graph_context, lookahead_sequence);
                if (m_shift_transitions_were_performed)
                {
                    DEBUG_CODE(if (m_is_shift_blocked)
                        cerr << "*** shift transitions (for lookahead nonterminal branches only) ***" << endl;
                    else
                        cerr << "*** shift transitions (for all branches) ***" << endl;)
                    DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
                }
                else
                {
                    DEBUG_CODE(cerr << "*** no shift transitions ***" << endl;)
                }

                m_is_shift_blocked = false;

                // if there were doomed branches
                if (!m_doomed_nonreturn_branch_list.IsEmpty() || !m_doomed_return_branch_list.IsEmpty())
                {
                    // if any shift transitions were performed, we can prune all
                    // doomed branches (because there was a valid transition)
                    if (m_shift_transitions_were_performed)
                    {
                        DEBUG_CODE(cerr << "*** pruning all doomed branches ***" << endl;)
                        assert(!m_parser_branch_list.IsEmpty());
                        // TODO: figure out if deleting these branch lists in this order guarantees
                        // no shift/reduce conflicts will be resolved incorrectly
                        PruneBranchList(graph_context, m_doomed_return_branch_list);
                        PruneBranchList(graph_context, m_doomed_nonreturn_branch_list);
                        DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
                        assert(m_tree_root->AssertActionAndParserBranchListsAreConsistent(m_parser_branch_list));
                    }
                    // if no shift transitions were performed but the doomed return branch
                    // list isn't empty, prune only the doomed non-return branches, keeping
                    // the return branches.  then return the single stack token.
                    else if (!m_doomed_return_branch_list.IsEmpty())
                    {
                        DEBUG_CODE(cerr << "*** pruning doomed non-return branches only ***" << endl;)
                        assert(m_parser_branch_list.IsEmpty());
                        PruneBranchList(graph_context, m_doomed_nonreturn_branch_list);
                        DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
                        assert(m_doomed_nonreturn_branch_list.IsEmpty());

                        // TODO technically there should only be one return branch at this
                        // point, but currently there's no way of guaranteeing branch uniqueness,
                        // so mutually recursive rules will produce redundant branches.
                        //assert(!m_doomed_return_branch_list.IsEmpty() && // TODO: enable this assert when appropriate
                        //       m_doomed_return_branch_list.Front() == m_doomed_return_branch_list.Back());

                        // no need to e-close here, since return branches will have no e-closure
                        break;
                    }
                    // if none of the above, go into error handling mode
                    else
                    {
                        assert(m_parser_branch_list.IsEmpty());
                        assert(m_doomed_return_branch_list.IsEmpty());
//                         m_parser_branch_list.List<ParserBranch>::Prepend(m_doomed_nonreturn_branch_list);
//                         assert(false && "this should never happen");
//                         assert(false && "error handling mode not implemented yet");

                        DEBUG_CODE(cerr << "*** quitting ***" << endl;)
                        DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
                        // no need to e-close here, since we're "dead" already
                        break;
                    }
                }

                assert(!m_parser_branch_list.IsEmpty());
                assert(m_doomed_nonreturn_branch_list.IsEmpty());
                assert(m_doomed_return_branch_list.IsEmpty());
            }
            // TODO: handle nonassoc errors
            assert(!m_nonassoc_error_encountered && "nonassoc error handling not implemented yet");
            m_nonassoc_error_encountered = false;

            // here is where the action tree trunk would be executed and then removed.
            assert(m_tree_root->AssertActionAndParserBranchListsAreConsistent(m_parser_branch_list));
            assert(!m_parser_branch_list.IsEmpty() || !m_doomed_nonreturn_branch_list.IsEmpty() || !m_doomed_return_branch_list.IsEmpty());

            // if there is an unequivocal action to take, we can stop running the npda.
            if (HasTrunk())
            {
                // we want to e-close before quitting,
                DEBUG_CODE(cerr << "*** e-close ***" << endl;)
                PerformEpsilonClosure(graph_context);
                DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
                break;
            }
        }
    }
    // runs the npda for a single action step -- a shift or a reduce.
    void Step (GraphContext const &graph_context, ActionSpec action)
    {
//         cerr << __PRETTY_FUNCTION__ << ", action: " << action.Type() << '(' << action.Data() << ')' << endl;

        assert(m_tree_root != NULL);
        assert(!m_parser_branch_list.IsEmpty());
        assert(!m_is_shift_blocked);

        DEBUG_CODE(cerr << "!!! step !!!" << endl;)
        DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
        assert(m_tree_root->AssertActionAndParserBranchListsAreConsistent(m_parser_branch_list));
        // e-closure nodes should not be added for self-recursive rules.  e-closures
        // should only be done on non-closed branches.  the rule stack level should
        // be incremented on the transition after an e-closure.
        DEBUG_CODE(cerr << "*** e-close ***" << endl;)
        PerformEpsilonClosure(graph_context);
        DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)

        switch (action.Type())
        {
            default:
            case AT_NONE:
            case AT_RETURN:
            case AT_ERROR_PANIC:
                assert(false && "can't Step() using this action type");
                return;

            case AT_SHIFT:
            {
                Graph::Transition::DataArray lookahead_sequence(1, action.Data());
                PerformShiftTransitions(graph_context, lookahead_sequence);
                DEBUG_CODE(cerr << "*** shift " << GetLookaheadSequenceString(graph_context.m_primary_source, lookahead_sequence) << " ***" << endl;)
                DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
                DEBUG_CODE(cerr << "*** pruning all doomed branches ***" << endl;)
                assert(!m_parser_branch_list.IsEmpty());
                // TODO: figure out if deleting these branch lists in this order guarantees
                // no shift/reduce conflicts will be resolved incorrectly
                PruneBranchList(graph_context, m_doomed_return_branch_list);
                PruneBranchList(graph_context, m_doomed_nonreturn_branch_list);
                DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
                assert(m_tree_root->AssertActionAndParserBranchListsAreConsistent(m_parser_branch_list));
                break;
            }

            case AT_REDUCE:
                PerformStepReduceTransitions(graph_context, action.Data());
                DEBUG_CODE(cerr << "*** reduce " << action.Data() << " ***" << endl;)
                DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)
                break;
        }

        DEBUG_CODE(cerr << "*** e-close ***" << endl;)
        PerformEpsilonClosure(graph_context);
        DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)

        DEBUG_CODE(cerr << "*** removing trunk ***" << endl;)
        RemoveTrunk();
        DEBUG_CODE(m_tree_root->Print(cerr, graph_context, 0);)

        // we also need to un-set the various Npda flags, as well as the
        // m_lookahead_nonterminal_token_index value on each branch.
        m_is_shift_blocked = false;
        m_reduce_transitions_were_performed = false;
        m_shift_transitions_were_performed = false;
        m_nonassoc_error_encountered = false; // NOTE: is this necessary?
        for (ParserBranch *parser_branch = m_parser_branch_list.Front();
             parser_branch != NULL;
             parser_branch = parser_branch->Next())
        {
            Branch *branch = static_cast<Branch *>(parser_branch);
            branch->m_lookahead_nonterminal_token_index = none__;
        }
    }

private:

    static bool TransitionAcceptsTokenId (Graph::Transition const &transition, TokenIndex token_index)
    {
        return transition.Type() == TT_SHIFT && transition.Data(0) == token_index;
    }

    void PerformEpsilonClosure (GraphContext const &graph_context)
    {
        // perform epsilon closure on the top stack states
        // of all non-epsilon-closed branches.
        StateStack extra_state_stack;
        for (ParserBranch *parser_branch = m_parser_branch_list.Front(); parser_branch != NULL; parser_branch = parser_branch->Next())
        {
            Branch *branch = static_cast<Branch *>(parser_branch);
            if (!branch->m_is_epsilon_closed)
            {
                branch->m_is_epsilon_closed = true;
                assert(extra_state_stack.empty());
                // this will add branches to the beginning of m_parser_branch_list,
                // so we don't need to worry about it interfering with iteration.
                PerformEpsilonClosure(graph_context, *branch, extra_state_stack);
                assert(extra_state_stack.empty());
            }
        }
    }
    void PerformEpsilonClosure (GraphContext const &graph_context, Branch &source_branch, StateStack &extra_state_stack)
    {
        // iterate through the transitions from the top state of the branch,
        // recursively epsilon-closing epsilon transitions.  also, remember
        // if we have encountered a non-epsilon transition; this indicates
        // that a branch should be cloned and added here.
        bool non_epsilon_transition_encountered = false;
        for (Graph::TransitionSet::const_iterator trans_it = (extra_state_stack.empty() ?
                                                              graph_context.m_npda_graph.GetNode(source_branch.Top()).GetTransitionSetBegin() :
                                                              graph_context.m_npda_graph.GetNode(extra_state_stack.back()).GetTransitionSetBegin()),
                                                  trans_it_end = (extra_state_stack.empty() ?
                                                                  graph_context.m_npda_graph.GetNode(source_branch.Top()).GetTransitionSetEnd() :
                                                                  graph_context.m_npda_graph.GetNode(extra_state_stack.back()).GetTransitionSetEnd());
             trans_it != trans_it_end;
             ++trans_it)
        {
            Graph::Transition const &transition = *trans_it;
            if (transition.Type() == TT_EPSILON)
            {
                // don't perform epsilon closure on this transition if the target
                // state has already been visited during this epsilon closure overall.
                assert(transition.HasTarget());
                bool transition_target_has_been_visited = source_branch.Top() == transition.TargetIndex();
                for (StateStack::iterator it = extra_state_stack.begin(), it_end = extra_state_stack.end();
                     !transition_target_has_been_visited && it != it_end;
                     ++it)
                {
                    if (transition.TargetIndex() == *it)
                        transition_target_has_been_visited = true;
                }
                if (!transition_target_has_been_visited)
                {
                    extra_state_stack.push_back(transition.TargetIndex());
                    PerformEpsilonClosure(graph_context, source_branch, extra_state_stack);
                    extra_state_stack.pop_back();
                }
            }
            else
                non_epsilon_transition_encountered = true;
        }

        // we don't clone the source branch if the extra state stack is empty
        // because it would be identical to the source branch which is already
        // contained in m_parser_branch_list.
        if (!extra_state_stack.empty() && non_epsilon_transition_encountered)
        {
            source_branch.m_parent->AddEpsilonClosure(source_branch, extra_state_stack);
            assert(m_tree_root->AssertActionAndParserBranchListsAreConsistent(m_parser_branch_list));
        }
    }
    void PerformReduceTransitions (GraphContext const &graph_context)
    {
        assert(!m_is_shift_blocked);
        assert(!m_reduce_transitions_were_performed);
        ParserBranch *parser_branch = m_parser_branch_list.Front();
        while (parser_branch != NULL)
        {
            Branch *branch = static_cast<Branch *>(parser_branch);
            parser_branch = parser_branch->Next();

            // only attempt to transition from non-transition-fromed branches
            if (branch->m_is_epsilon_closed)
            {
                bool reduce_transitions_were_performed_on_this_branch = false;
                Uint32 state = branch->Top();
                for (Graph::TransitionSet::const_iterator it = graph_context.m_npda_graph.GetNode(state).GetTransitionSetBegin(),
                                                          it_end = graph_context.m_npda_graph.GetNode(state).GetTransitionSetEnd();
                     it != it_end;
                     ++it)
                {
                    Graph::Transition const &transition = *it;
                    if (transition.Type() == TT_REDUCE)
                    {
                        PerformReduceTransition(graph_context, *branch, graph_context.m_primary_source.GetRule(transition.Data(0)));
                        reduce_transitions_were_performed_on_this_branch = true;
                    }
                }

                if (reduce_transitions_were_performed_on_this_branch)
                    delete branch;
            }
        }
    }
    void PerformStepReduceTransitions (GraphContext const &graph_context, Uint32 rule_index)
    {
        assert(!m_is_shift_blocked);
        assert(!m_reduce_transitions_were_performed);
        ParserBranch *parser_branch = m_parser_branch_list.Front();
        while (parser_branch != NULL)
        {
            Branch *branch = static_cast<Branch *>(parser_branch);
            parser_branch = parser_branch->Next();

            // only attempt to transition from non-transition-fromed branches
            if (branch->m_is_epsilon_closed)
            {
                bool reduce_transitions_were_performed_on_this_branch = false;
                Uint32 state = branch->Top();
                for (Graph::TransitionSet::const_iterator it = graph_context.m_npda_graph.GetNode(state).GetTransitionSetBegin(),
                                                          it_end = graph_context.m_npda_graph.GetNode(state).GetTransitionSetEnd();
                     it != it_end;
                     ++it)
                {
                    Graph::Transition const &transition = *it;
                    if (transition.Type() == TT_REDUCE && transition.Data(0) == rule_index)
                    {
                        PerformReduceTransition(graph_context, *branch, graph_context.m_primary_source.GetRule(transition.Data(0)));
                        reduce_transitions_were_performed_on_this_branch = true;
                    }
                }

                // we want to delete all old branches and leave none at the root
                // so there is a nontrivial trunk, consisting of exactly one reduce action.
                delete branch;
            }
        }
    }
    void PerformReduceTransition (GraphContext const &graph_context, Branch &source_branch, Rule const *reduction_rule)
    {
        assert(reduction_rule != NULL);
        assert(source_branch.m_lookahead_nonterminal_token_index == none__);
        assert(source_branch.m_parent != NULL);

        // only bother with a reduce action if it would actually succeed
        // (and not immediately be superceded by an existing reduce action.
        if (source_branch.m_parent->ReduceActionWouldSucceed(reduction_rule))
        {
            m_reduce_transitions_were_performed = true;
            source_branch.m_parent->AddReduce(source_branch, reduction_rule);
    //         m_tree_root->Print(cerr, graph_context, 0);
    //         PrintParserBranchList();
            assert(m_tree_root->AssertActionAndParserBranchListsAreConsistent(m_parser_branch_list));
        }
    }
    void PerformShiftTransitions (GraphContext const &graph_context, Graph::Transition::DataArray &lookahead_sequence)
    {
        assert(!m_reduce_transitions_were_performed);
        assert(m_doomed_nonreturn_branch_list.IsEmpty());

        TokenIndex dummy_lookahead_token;
        // only shift a lookahead token if we're not blocked waiting to
        // sync up the branch-wise token reading.
        TokenIndex const &lookahead_token = m_is_shift_blocked ? dummy_lookahead_token : lookahead_sequence.front();
        if (!m_is_shift_blocked)
            lookahead_sequence.erase(lookahead_sequence.begin());

        ParserBranch *parser_branch = m_parser_branch_list.Front();
        while (parser_branch != NULL)
        {
            Branch *branch = static_cast<Branch *>(parser_branch);
            parser_branch = parser_branch->Next();
    //         cerr << "branch: " << branch << endl;
            assert(m_is_shift_blocked || branch->m_lookahead_nonterminal_token_index == none__);
            // if we're shift-blocked, then we only want to transition those
            // branches that have a lookahead nonterminal token.
            if (m_is_shift_blocked && branch->m_lookahead_nonterminal_token_index == none__)
                continue;
            // if this branch is not epsilon closed, then it's already transitioned.
            if (!branch->m_is_epsilon_closed)
                continue;
            // otherwise iterate through the transitions at this state and transition
            // all that match the current lookahead.
            Uint32 state = branch->Top();
            bool return_state_encountered = false;
            for (Graph::TransitionSet::const_iterator it = graph_context.m_npda_graph.GetNode(state).GetTransitionSetBegin(),
                                                      it_end = graph_context.m_npda_graph.GetNode(state).GetTransitionSetEnd();
                 it != it_end;
                 ++it)
            {
                Graph::Transition const &transition = *it;
                // if we're shift-blocked, only schedule shift-transitions for branches with
                // nonterminal lookaheads, otherwise schedule any valid shift-transition.
                if (m_is_shift_blocked &&
                    branch->m_lookahead_nonterminal_token_index != none__ &&
                    TransitionAcceptsTokenId(transition, branch->m_lookahead_nonterminal_token_index)
                    ||
                    !m_is_shift_blocked &&
                    TransitionAcceptsTokenId(transition, lookahead_token))
                {
                    PerformShiftTransition(graph_context, *branch, transition.TargetIndex());
                }
                else if (transition.Type() == TT_RETURN)
                {
                    return_state_encountered = true;
                }
            }

            branch->RemoveFromBranchList(BLT_PARSER);
            if (return_state_encountered)
            {
//                 cerr << "%%% adding " << branch << " to doomed return branch list" << endl;
                m_doomed_return_branch_list.Append(branch);
            }
            else
            {
//                 cerr << "%%% adding " << branch << " to doomed nonreturn branch list" << endl;
                m_doomed_nonreturn_branch_list.Append(branch);
            }
        }
    }
    void PerformShiftTransition (GraphContext const &graph_context, Branch &source_branch, Uint32 target_state)
    {
        m_shift_transitions_were_performed = true;
        source_branch.m_parent->AddShift(graph_context, source_branch, target_state);
    //     m_tree_root->Print(cerr, graph_context, 0);
    //     PrintParserBranchList();
    }
    void PruneBranchList (GraphContext const &graph_context, ParserBranchList &branch_list)
    {
        // there can't be any pruning when un-shift-blocking because there's
        // no way for a branch with a lookahead nonterminal to fail to shift it.
        assert(!m_is_shift_blocked);
        // prune from the end of the list towards the front -- necessary to
        // prevent certain shift/reduce conflicts from being incorrectly resolved.
        // this is due to the order which branches are added to the Action
        // branch lists.
        ParserBranch *parser_branch;
        while ((parser_branch = branch_list.Back()) != NULL)
        {
//             cerr << "pruning branch " << static_cast<Branch *>(parser_branch) << endl;
            delete static_cast<Branch *>(parser_branch);
    //         m_tree_root->Print(cerr, graph_context, 0);
        }
    }
    void PrintParserBranchList (GraphContext const &graph_context) // TEMP (?)
    {
        cerr << "&&& parser branch list &&&" << endl;
        for (ParserBranch *parser_branch = m_parser_branch_list.Front(); parser_branch != NULL; parser_branch = parser_branch->Next())
            static_cast<Branch *>(parser_branch)->Print(cerr, graph_context, 1);
        cerr << "&&& end of parser branch list &&&" << endl;
    }

    Action *m_tree_root;
    ParserBranchList m_parser_branch_list;
    ParserBranchList m_doomed_nonreturn_branch_list;
    ParserBranchList m_doomed_return_branch_list;
    bool m_is_shift_blocked;
    bool m_reduce_transitions_were_performed;
    bool m_shift_transitions_were_performed;
    bool m_nonassoc_error_encountered;
}; // end of class Npda

void EnsureDpdaStateIsGenerated (GraphContext &graph_context, Npda const &npda);

void Recurse (GraphContext &graph_context, Npda const &source_npda, DpdaState const &source_dpda_state, Npda const &recurse_npda, ActionSpec const &default_action, Graph::Transition::DataArray &lookahead_sequence)
{
//     cerr << "Recurse(); lookahead_sequence: " << GetLookaheadSequenceString(graph_context.m_primary_source, lookahead_sequence) << endl;

    if (recurse_npda.CurrentDpdaState().empty())
        return;

    set<Uint32> transition_token_index;
    for (TransitionIterator it(graph_context, recurse_npda.CurrentDpdaState(), IT_SHIFT_TERMINAL_TRANSITIONS); !it.IsDone(); ++it)
        transition_token_index.insert(it->Data(0));

    for (set<Uint32>::const_iterator it = transition_token_index.begin(), it_end = transition_token_index.end();
         it != it_end;
         ++it)
    {
        ActionSpec action;
        // this block of code is entirely for the purpose of determining what lookaheads
        // are required for each transition.  e.g. in an LALR(3) grammar, this would
        // require a recursion to a depth of 3.
        {
            Npda child_npda(recurse_npda, graph_context);
            child_npda.Run(graph_context, *it);
            action = child_npda.FirstTrunkAction(lookahead_sequence.empty() ? *it : lookahead_sequence[0]);
            if (action.Type() == AT_NONE)
            {
                // add the iterator token to the lookahead sequence
                lookahead_sequence.push_back(*it);
                // recursively explore the states
                Recurse(graph_context, source_npda, source_dpda_state, child_npda, default_action, lookahead_sequence);
                // pop the iterator token
                lookahead_sequence.pop_back();
                // this continue statement is so child_npda and retry_npda (below)
                // don't both exist in memory at the same time.
                continue;
            }
        }
        // otherwise there was a real trunk action, so create a transition using it
        {
            assert(action.Type() == AT_REDUCE || action.Type() == AT_SHIFT);

            // create a fork of this npda and run it with the action determined above.
            Npda retry_npda(source_npda, graph_context);
            retry_npda.Step(graph_context, action);
            assert(!retry_npda.HasTrunk());
            // figure out what dpda state it's at
            DpdaState target_dpda_state(retry_npda.CurrentDpdaState());
            // make sure that the resulting npda's dpda state is generated
            EnsureDpdaStateIsGenerated(graph_context, retry_npda);

            // add the iterator token to the lookahead sequence
            lookahead_sequence.push_back(*it);
            // add a graph transition from this state to that, via the iterator terminal
            assert(graph_context.DpdaStateIsGenerated(source_dpda_state));

            // only add the action to the node if it doesn't match the default action,
            // since the default is a catch-all.
            if (action.Type() != default_action.Type() || action.Data() != default_action.Data())
            {
                switch (action.Type())
                {
                    default:
                    case AT_NONE:
                    case AT_RETURN:
                    case AT_ERROR_PANIC:
                        assert(false && "invalid trunk action");
                        break;

                    case AT_SHIFT:
                        DEBUG_CODE(cerr << "++ (at " << source_dpda_state << ") adding shift (" << target_dpda_state << ") transition with lookaheads: " << GetLookaheadSequenceString(graph_context.m_primary_source, lookahead_sequence) << endl;)
                        graph_context.AddTransition(
                            source_dpda_state,
                            DpdaShiftTransition(
                                lookahead_sequence,
                                GetLookaheadSequenceString(graph_context.m_primary_source, lookahead_sequence),
                                graph_context.DpdaStateIndex(target_dpda_state)));
                        break;

                    case AT_REDUCE:
                        DEBUG_CODE(cerr << "++ (at " << source_dpda_state << ") adding reduce \"" << graph_context.m_primary_source.GetRule(action.Data())->GetAsText() << "\" transition with lookaheads: " << GetLookaheadSequenceString(graph_context.m_primary_source, lookahead_sequence) << endl;)
                        assert(false && "i think the default action will necessarily preclude this case from ever happening");
                        break;
                }
            }

            // pop the iterator token
            lookahead_sequence.pop_back();
        }
    }
}

void EnsureDpdaStateIsGenerated (GraphContext &graph_context, Npda const &npda)
{
    assert(!npda.HasTrunk());

    DpdaState dpda_state(npda.CurrentDpdaState());
    // check if this dpda_state has already been generated
    if (graph_context.DpdaStateIsGenerated(dpda_state))
        return; // nothing needs to be done
//     cerr << "EnsureDpdaStateIsGenerated(); generating " << dpda_state << endl;
    // dpda_state is now considered "generated"
    graph_context.AddDpdaState(dpda_state, new DpdaNodeData(graph_context.m_npda_graph, dpda_state));

    // run the npda with no input, so we can decide what the default action is
    {
        Npda child_npda(npda, graph_context);
        child_npda.Run(graph_context);

        // if there is no action in the trunk, then the default action is error panic.
        // otherwise it is necessarily a reduce action (because there are no tokens
        // to shift yet.
        ActionSpec default_action(child_npda.DefaultAction(graph_context));
        assert(default_action.Type() == AT_ERROR_PANIC || default_action.Type() == AT_REDUCE || default_action.Type() == AT_RETURN);
        DEBUG_CODE(cerr << "++ (at " << dpda_state << ") adding default action: " << default_action.Type() << '(' << default_action.Data() << ')' << endl;)

        // the default transition better be the first one
        assert(graph_context.TransitionCount(dpda_state) == 0);

        // add the default action to the dpda graph
        switch (default_action.Type())
        {
            default:
            case AT_NONE:
            case AT_SHIFT:
                assert(false && "invalid default action");
                break;

            case AT_REDUCE:
                graph_context.AddTransition(
                    dpda_state,
                    DpdaReduceTransition(default_action.Data()));
                break;

            case AT_RETURN:
                graph_context.AddTransition(
                    dpda_state,
                    DpdaReturnTransition(
                        graph_context.m_primary_source.GetTokenId(default_action.Data()),
                        default_action.Data()));
                break;

            case AT_ERROR_PANIC:
                graph_context.AddTransition(
                    dpda_state,
                    DpdaErrorPanicTransition());
                break;
        }

        // now recurse, exploring the states resulting from all valid transitions.
        {
            Graph::Transition::DataArray lookahead_sequence; // empty
            Recurse(graph_context, npda, dpda_state, npda, default_action, lookahead_sequence);
            assert(lookahead_sequence.empty());
        }
    }

    // generate the shift transitions for the nonterminals at this state.  the fanciness
    // with the set<Uint32> is so we only attempt each transition once, and in order
    // by index.
    set<Uint32> nonterminal_token_index;
    for (TransitionIterator it(graph_context, dpda_state, IT_SHIFT_NONTERMINAL_TRANSITIONS); !it.IsDone(); ++it)
        nonterminal_token_index.insert(it->Data(0)); // Data(0) is the nonterminal token index

    for (set<Uint32>::const_iterator it = nonterminal_token_index.begin(), it_end = nonterminal_token_index.end();
         it != it_end;
         ++it)
    {
        // create a fork of this npda and run it with the iterator nonterminal
        Npda child_npda(npda, graph_context);
        child_npda.RunUsingNonterminalLookahead(graph_context, *it);
        assert(child_npda.HasTrunk());
        child_npda.RemoveTrunk();
        // figure out what dpda state it's at
        DpdaState target_dpda_state(child_npda.CurrentDpdaState());
        // make sure that dpda state is generated
        EnsureDpdaStateIsGenerated(graph_context, child_npda);
        // add a graph transition from this state to that, via the iterator nonterminal
        Graph::Transition::DataArray lookahead_sequence(1, *it);
        graph_context.AddTransition(
            dpda_state,
            DpdaShiftTransition(
                lookahead_sequence,
                GetLookaheadSequenceString(graph_context.m_primary_source, lookahead_sequence),
                graph_context.DpdaStateIndex(target_dpda_state)));
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
        // "none_" is a special nonterminal which isn't real, so skip it.
        if (nonterminal->GetText() == "none_")
            continue;
        Npda nonterminal_start_npda(graph_context, nonterminal);
        EnsureDpdaStateIsGenerated(graph_context, nonterminal_start_npda);
        nonterminal->SetDpdaGraphStates(graph_context.DpdaStateIndex(nonterminal_start_npda.CurrentDpdaState()));
    }

    EmitExecutionMessage(FORMAT("grammar is LALR(" << graph_context.LalrLookaheadCount() << ')'));
}

void PrintDpdaStatesFile (PrimarySource const &primary_source, Graph const &npda_graph, Graph const &dpda_graph, ostream &stream)
{
    assert(primary_source.GetRuleCount() > 0);
    Uint32 max_rule_index_width = FORMAT(primary_source.GetRuleCount()-1).length();

    stream << "//////////////////////////////////////////////////////////////////////////////" << endl
           << "// GRAMMAR" << endl
           << "//////////////////////////////////////////////////////////////////////////////" << endl
           << endl;
    for (Uint32 i = 0; i < primary_source.GetRuleCount(); ++i)
    {
        stream << "    rule ";
        stream.width(max_rule_index_width);
        stream.setf(ios_base::right);
        stream << i << ": " << primary_source.GetRule(i)->GetAsText() << endl;
    }
    stream << endl;

    assert(npda_graph.GetNodeCount() > 0);

    stream << "//////////////////////////////////////////////////////////////////////////////" << endl
           << "// DPDA STATE MACHINE - " << dpda_graph.GetNodeCount() << " STATES" << endl
           << "//////////////////////////////////////////////////////////////////////////////" << endl
           << endl;
    for (Uint32 i = 0; i < dpda_graph.GetNodeCount(); ++i)
    {
        // print the state index and corresponding NPDA state indices
        Graph::Node const &dpda_node = dpda_graph.GetNode(i);
        DpdaNodeData const &dpda_node_data = dpda_node.GetDataAs<DpdaNodeData>();
        stream << "State " << i << " - Corresponding NPDA states: " << dpda_node_data.GetDpdaState() << endl;

        // print the staged rules at this state
        for (DpdaState::const_iterator it = dpda_node_data.GetDpdaState().begin(),
                                       it_end = dpda_node_data.GetDpdaState().end();
             it != it_end;
             ++it)
        {
            NpdaNodeData const &npda_node_data = npda_graph.GetNode(*it).GetDataAs<NpdaNodeData>();

            stream << "    ";
            string npda_node_description(npda_node_data.GetFullDescription());
            char const *s = npda_node_description.c_str();
            while (*s != '\0')
            {
                if (*s == '\n')
                    stream << "\n    ";
                else
                    stream << *s;
                ++s;
            }

            stream << endl;
        }
        stream << endl;

        // TODO -- figure out the justification width of the SHIFT transition
        // lookaheads so the transition printout can be all nice and justified

        // print the transitions
        for (Graph::TransitionSet::const_iterator it = dpda_node.GetTransitionSetBegin(),
                                                  it_end = dpda_node.GetTransitionSetEnd();
             it != it_end;
             ++it)
        {
            Graph::Transition const &transition = *it;

            // indent
            stream << "   "; // 7 spaces, the 8th is '8' or ' ' depending
                                 // on if this is the default transition.
            // we want to indicate the default transition to avoid ambiguity.
            if (it == dpda_node.GetTransitionSetBegin())
                stream << "*Default transition: ";
            else
                stream << " ";
            // print the lookaheads and action
            stream << transition.Label();
            // if it's a shift transition, the label doesn't have the ": SHIFT blah blah" part
            if (transition.Type() == TT_SHIFT)
                stream << ": SHIFT " << GetLookaheadSequenceString(primary_source, Graph::Transition::DataArray(1, transition.Data(0))) << ", then push state " << transition.TargetIndex();
            stream << endl;
        }
        stream << endl;
    }
}


} // end of namespace Trison
