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

#include "barf_list.hpp"
#include "barf_weakreference.hpp"

namespace Trison {

// a set of npda states constitutes a single dpda state
typedef set<Uint32> DpdaState;
typedef map<DpdaState, Uint32> GeneratedDpdaStateMap;

struct GraphContext
{
//     PrimarySource const &m_primary_source;
    Graph const &m_npda_graph;
    Graph &m_dpda_graph;
    GeneratedDpdaStateMap m_generated_dpda_state_map;

    GraphContext (/*PrimarySource const &primary_source, */Graph const &npda_graph, Graph &dpda_graph)
        :
//         m_primary_source(primary_source),
        m_npda_graph(npda_graph),
        m_dpda_graph(dpda_graph)
    { }

    bool DpdaStateIsGenerated (DpdaState const &dpda_state)
    {
        return m_generated_dpda_state_map.find(dpda_state) != m_generated_dpda_state_map.end();
    }
}; // end of struct GraphContext

// the lifetime of an instance of TransitionIterator should never exceed
// that of the npda_graph or dpda_state passed into it (because it stores
// references to them).
class TransitionIterator
{
public:

    enum IterateFlags
    {
        IT_NONE         = 0,
        IT_TERMINALS    = 1 << 0,
        IT_NONTERMINALS = 1 << 1,
        IT_ALL          = IT_TERMINALS|IT_NONTERMINALS
    }; // end of enum IterateOver

    TransitionIterator (Graph const &npda_graph, DpdaState const &dpda_state, IterateFlags flags)
        :
        m_npda_graph(npda_graph),
        m_dpda_state(dpda_state),
        m_flags(flags)
    {
        // start the TransitionIterator pointing at the first Graph::Transition
        // of the first DpdaState Graph::Node (i.e. a node in the npda graph).
        m_dpda_state_it = m_dpda_state.begin();
        if (m_dpda_state_it != m_dpda_state.end())
            m_transition_it = npda_graph.GetNode(*m_dpda_state_it).GetTransitionSetBegin();
    }

    void operator ++ ()
    {
        // increment the minor iterator (m_transition_it)
        ++m_transition_it;
        // if the minor iterator reached the end of the minor iteration,
        // then increment the major iterator (m_dpda_state_it) and set
        // the minor iterator to the beginning of the minor iteration.
        if (m_transition_it == npda_graph.GetNode(*m_dpda_state_it).GetTransitionSetEnd())
        {
            ++m_dpda_state_it;
            if (m_dpda_state_it != m_dpda_state.end())
                m_transition_it = npda_graph.GetNode(*m_dpda_state_it).GetTransitionSetBegin();
        }
    }
    bool IsDone () const
    {
        // we're done only when the major iteration is done.
        return m_dpda_state_it == m_dpda_state.end()
    }

private:

    Graph const &m_npda_graph;
    DpdaState const &m_dpda_state;
    IterateFlags const m_flags;
    // major iterator ("outer loop")
    DpdaState::const_iterator m_dpda_state_it;
    // minor iterator ("inner loop")
    Graph::Node::TransitionSet::const_iterator m_transition_it;
}; // end of class TransitionIterator

class ActionSpec
{
public:

    enum Type
    {
        AT_NONE = 0,
        AT_SHIFT_AND_PUSH_STATE,
        AT_REDUCE,
        AT_ERROR_PANIC,

        AT_COUNT
    }; // end of enum ActionSpec::Type

    ActionSpec () : m_type(AT_NONE), m_data(0) { }
    ActionSpec (Type type, Uint32 data = 0) : m_type(type), m_data(data) { }

    Type Type () const { return m_type; }
    Uint32 Data () const { return m_data; }

private:

    Type m_type;
    Uint32 m_data;
}; // end of class ActionSpec

// Npda is the actual automaton which simulates a parser.
class Npda
{
private:

    enum BranchListType { BLT_PARSER = 0, BLT_ACTION, BLT_COUNT };

    struct Action;
    struct ActionBranch : public ListNode<ActionBranch> { protected: ActionBranch () { } };
    struct ParserBranch : public ListNode<ParserBranch> { protected: ParserBranch () { } };
    struct Reduce;
//     struct Rule_;
    struct Shift;
//     struct State_;
//     struct Transition_;

    typedef std::vector<Uint32> StateStack;
    typedef Uint32 TokenId;
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
                    return Rule::ShiftReduceConflict::IsHigherPriority(*left, *right);
            }
        }; // end of struct Npda::ShiftReduceConflict::Order
    }; // end of struct Npda::ShiftReduceConflict
/*
    struct State_
    {
        Rule_ const *m_associated_rule_;
        Uint32_ m_associated_rule_stage_;
        TokenId m_associated_nonterminal_token_id_;
        Size_ m_transition_count_;
        Transition_ const *m_transition_table_;
        char const *m_description_;
    }; // end of struct Npda_::State_
    struct Transition_
    {
        enum Type_ { TT_EPSILON_ = 0, TT_REDUCE_, TT_RETURN_, TT_SHIFT_ };
        Uint8_ m_transition_type_;
        Uint32_ m_transition_data_;
        State_ const *m_target_state_;
    }; // end of struct Npda_::Transition_
    */
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
    }; // end of struct Npda_::TreeNode
    typedef WeakReference<Shift> ShiftReference;
    typedef std::map<Rule const *, ShiftReference, ShiftReduceConflict::Order> ShiftReferenceMap;
    struct Branch : public TreeNode, public ActionBranch, public ParserBranch
    {
        typedef std::vector<ShiftReference> ShiftReferenceList;
        ShiftReferenceList m_shift_reference_list;
        StateStack m_state_stack;
        TokenId m_lookahead_nonterminal_token_id;
        bool m_is_epsilon_closed;

        Branch (Uint32 starting_state)
            :
            TreeNode(BRANCH),
            m_lookahead_nonterminal_token_id(none__),
            m_is_epsilon_closed(false)
        {
            assert(starting_state != NULL);
            m_state_stack.push_back(starting_state);
        }
        ~Branch ()
        {
//             std::cerr << "~Branch(); this = " << this << std::endl;
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
                ShiftReference &shift_reference = *it;
                assert(shift_reference.IsValid());
                if (shift_reference.ReferenceCount() == 2 && shift_reference.InstanceIsValid())
                {
                    Shift *shift = &*shift_reference;
                    assert(shift != NULL);
                    assert(shift->m_parent != NULL);
                    shift_reference.Release();
                    shift->DestroyAllUnusedInRuleRange();
                    shift->m_parent->ResolveShiftReduceConflicts();
                }
            }
        }

        Uint32 Top () const
        {
            assert(!m_state_stack.empty());
            assert(m_state_stack.back() != NULL);
            return *m_state_stack.back();
        }
        Branch *Clone () const
        {
            Branch *cloned = new Branch(m_state_stack, m_lookahead_nonterminal_token_id);
            assert(cloned->m_parent == NULL);
            // duplicate the shift reference list
            for (ShiftReferenceList::const_iterator it = m_shift_reference_list.begin(),
                                                    it_end = m_shift_reference_list.end();
                 it != it_end;
                 ++it)
            {
                ShiftReference const &shift_reference = *it;
                assert(shift_reference.IsValid());
                assert(shift_reference.ReferenceCount() >= 1);
                // only copy the reference if it's valid
                if (shift_reference.InstanceIsValid())
                {
                    assert(&*shift_reference != NULL);
                    cloned->m_shift_reference_list.push_back(shift_reference);
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
/*
        void Print (std::ostream &stream, Rule_ const *rule_table, State_ const *state_table, Uint32_ indent_level) const
        {
            stream << std::string(2*indent_level, ' ') << (m_is_epsilon_closed ? 'c' : '_') << " branch " << this;
            for (StateStack::const_iterator it = m_state_stack.begin(), it_end = m_state_stack.end();
                 it != it_end;
                 ++it)
            {
                State_ const *stack_state = *it;
                assert(stack_state != NULL);
                stream << ' ' << (stack_state - state_table);
            }
            if (m_lookahead_nonterminal_token_id != none__)
                stream << " (" << m_lookahead_nonterminal_token_id << ')';
            stream << " \"" << Top().m_description_ << '\"' << std::endl;
        }
*/
    private:

        // for use only by Clone()
        Branch (StateStack const &state_stack, TokenId lookahead_nonterminal_token_id)
            :
            TreeNode(BRANCH),
            m_state_stack(state_stack),
            m_lookahead_nonterminal_token_id(lookahead_nonterminal_token_id),
            m_is_epsilon_closed(false)
        {
            assert(!m_state_stack.empty());
        }
    }; // end of struct Npda_::Branch
    struct ActionBranchList : public List_<ActionBranch>
    {
        void Prepend (Branch *node) { List_<ActionBranch>::Prepend(static_cast<ActionBranch *>(node)); }
        void Append (Branch *node) { List_<ActionBranch>::Append(static_cast<ActionBranch *>(node)); }
    }; // end of struct Npda_::ActionBranchList
    struct ParserBranchList : public List_<ParserBranch>
    {
        void Prepend (Branch *node) { List_<ParserBranch>::Prepend(static_cast<ParserBranch *>(node)); }
        void Append (Branch *node) { List_<ParserBranch>::Append(static_cast<ParserBranch *>(node)); }
    }; // end of struct Npda_::ParserBranchList
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
//             std::cerr << "~Action(); this = " << this << std::endl;

            // delete this and all children out to the leaves
            if (m_parent != NULL)
                m_parent->RemoveChild(this);

            delete m_reduce_child;
            delete m_shift_child;

            ActionBranch *action_branch;
            while ((action_branch = m_action_branch_list.Front()) != NULL)
            {
                Branch *branch = static_cast<Branch *>(static_cast<ActionBranch *>(action_branch));
                branch->m_parent = NULL; // to stop ~Branch() from interfering
                delete branch;
                assert(m_action_branch_list.Front() != action_branch);
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
//             reduced_branch->m_state_stack.resize(reduced_branch->m_state_stack.size() - (reduction_rule->m_rule_token_list->size() + 1)); // for condensed nonterminal head states
            reduced_branch->m_state_stack.resize(reduced_branch->m_state_stack.size() - (reduction_rule->m_rule_token_list->size() + 2)); // for uncondensed nonterminal head states


            // TODO start here
            // TODO start here
            // TODO start here


            reduced_branch->m_lookahead_nonterminal_token_id = reduction_rule->m_reduction_nonterminal_token_id_;
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
                static_cast<Branch *>(m_action_branch_list.Back())->ParserBranch::InsertAfter_(reduced_branch);
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
        void AddShift_ (Branch &source_branch, State_ const *target_state)
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
            shifted_branch->m_lookahead_nonterminal_token_id = none__;
//             std::cerr << "adding parser branch " << static_cast<ParserBranch *>(shifted_branch)
//                       << " which is action branch " << static_cast<ActionBranch *>(shifted_branch)
//                       << std::endl;
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
                    static_cast<Branch *>(m_action_branch_list.Back())->ParserBranch::InsertAfter_(shifted_branch);
                }
                else
                {
                    // otherwise, we can add it to the parser branch list directly after
                    // the branch corresponding to the last branch in the parser branch
                    // list subordinate to the reduce child.
                    static_cast<Branch *>(last_reduce_subordinate_branch)->ParserBranch::InsertAfter_(shifted_branch);
                }
            }
            else
            {
                // otherwise, we can add it directly before the first branch
                // belonging to the shift child.  NOTE: this could be InsertAfter_
                assert(m_shift_child->m_action_branch_list.Front() != NULL);
                static_cast<Branch *>(m_shift_child->m_action_branch_list.Front())->ParserBranch::InsertBefore(shifted_branch);
            }
            // add the shifted branch to the shift child's action branch list.
            m_shift_child->m_action_branch_list.Prepend(shifted_branch);
            // only add a shift reference if there could be a conflict
            if (m_reduce_child != NULL)
            {
                ShiftReference shift_reference = m_shift_child->EnsureShiftReferenceExists_(target_state->m_associated_rule_);
                assert(&*shift_reference == m_shift_child);
                shifted_branch->m_shift_reference_list.push_back(shift_reference);
            }
        }
        void ResolveShiftReduceConflicts ()
        {
//             std::cerr << "~ResolveShiftReduceConflicts(); this = " << this << ", blah = " << blah << std::endl;
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
            assert(!m_shift_child->IsRuleRangeEmpty_());
            Rule_ const *high_shift_rule = m_shift_child->RangeRuleHigh_();
            Rule_ const *low_shift_rule = m_shift_child->RangeRuleLow_();
            Rule_ const *reduce_rule = m_reduce_child->m_reduction_rule_;
            assert(high_shift_rule != NULL);
            assert(low_shift_rule != NULL);
            assert(reduce_rule != NULL);
            assert(!Rule_::ShiftReduceConflict_::IsHigherPriority(*low_shift_rule, *high_shift_rule));

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

            if (Rule_::ShiftReduceConflict_::IsHigherPriority(*low_shift_rule, *reduce_rule))
            {   // case 1
                delete m_reduce_child;
                m_reduce_child = NULL;
                m_shift_child->ClearRuleRange();
            }
            else if (Rule_::ShiftReduceConflict_::IsHigherPriority(*reduce_rule, *high_shift_rule))
            {   // case 5
                delete m_shift_child;
                m_shift_child = NULL;
            }
            else if (Rule_::ShiftReduceConflict_::IsEqualPriority_(*reduce_rule, *high_shift_rule) &&
                     Rule_::ShiftReduceConflict_::IsEqualPriority_(*reduce_rule, *low_shift_rule))
            {   // case 3
                assert(low_shift_rule->m_precedence_->m_level_ == high_shift_rule->m_precedence_->m_level_);
                assert(low_shift_rule->m_precedence_->m_associativity_ == high_shift_rule->m_precedence_->m_associativity_);
                assert(reduce_rule->m_precedence_->m_level_ == high_shift_rule->m_precedence_->m_level_);
                assert(reduce_rule->m_precedence_->m_associativity_ == high_shift_rule->m_precedence_->m_associativity_);
                if (reduce_rule->m_precedence_->m_associativity_ == ASSOCIATIVITY_LEFT_)
                {
                    delete m_shift_child;
                    m_shift_child = NULL;
                }
                else if (reduce_rule->m_precedence_->m_associativity_ == ASSOCIATIVITY_RIGHT_)
                {
                    delete m_reduce_child;
                    m_reduce_child = NULL;
                    m_shift_child->ClearRuleRange();
                }
                else // reduce_rule->m_precedence_->m_associativity_ == ASSOCIATIVITY_NONASSOC_
                {
                    assert(reduce_rule->m_precedence_->m_associativity_ == ASSOCIATIVITY_NONASSOC_);
                    assert(false && "nonassoc error handling not implemented yet");
                }
            }
            else if (Rule_::ShiftReduceConflict_::IsEqualPriority_(*reduce_rule, *high_shift_rule) &&
                     Rule_::ShiftReduceConflict_::IsHigherPriority(*reduce_rule, *low_shift_rule))
            {   // case 4
                assert(reduce_rule->m_precedence_->m_level_ == high_shift_rule->m_precedence_->m_level_);
                assert(reduce_rule->m_precedence_->m_associativity_ == high_shift_rule->m_precedence_->m_associativity_);
                if (reduce_rule->m_precedence_->m_associativity_ == ASSOCIATIVITY_LEFT_)
                {
                    delete m_shift_child;
                    m_shift_child = NULL;
                }
            }
            else if (Rule_::ShiftReduceConflict_::IsEqualPriority_(*reduce_rule, *low_shift_rule) &&
                     Rule_::ShiftReduceConflict_::IsHigherPriority(*high_shift_rule, *reduce_rule))
            {   // case 2
                assert(reduce_rule->m_precedence_->m_level_ == low_shift_rule->m_precedence_->m_level_);
                assert(reduce_rule->m_precedence_->m_associativity_ == low_shift_rule->m_precedence_->m_associativity_);
                if (reduce_rule->m_precedence_->m_associativity_ == ASSOCIATIVITY_RIGHT_)
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

        void Print (std::ostream &stream, Rule_ const *rule_table, State_ const *state_table, Uint32_ indent_level) const
        {
            // fake polymorphism here, because we don't want to use virtual inheritance
            if (m_parent == NULL) // special treatment for the root
                PrivatePrint_(stream, rule_table, state_table, indent_level);
            else switch (m_tree_node_type)
            {
                default:
                case BRANCH:
                case ACTION: assert(false && "this should never happen"); break;
                case REDUCE: static_cast<Reduce const *>(this)->Print(stream, rule_table, state_table, indent_level); break;
                case SHIFT : static_cast<Shift const *>(this)->Print(stream, rule_table, state_table, indent_level); break;
            }
        }

    protected:

        void PrintChildNodes_ (std::ostream &stream, Rule_ const *rule_table, State_ const *state_table, Uint32_ indent_level) const
        {
            for (ActionBranch const *action_branch = m_action_branch_list.Front(); action_branch != NULL; action_branch = action_branch->Next_())
                static_cast<Branch const *>(action_branch)->Print(stream, rule_table, state_table, indent_level);
            if (m_reduce_child != NULL)
                m_reduce_child->Print(stream, rule_table, state_table, indent_level);
            if (m_shift_child != NULL)
                m_shift_child->Print(stream, rule_table, state_table, indent_level);
        }

    private:

        void AssertActionAndParserBranchListsAreConsistent (ParserBranch const *&parser_branch) const
        {
            if (parser_branch == NULL)
                assert(!HasChildren());
            for (ActionBranch const *action_branch = m_action_branch_list.Front();
                 action_branch != NULL;
                 action_branch = action_branch->Next_(), parser_branch = parser_branch != NULL ? parser_branch->Next_() : parser_branch)
            {
//                 std::cerr << "checking parser branch " << static_cast<Branch const *>(parser_branch)
//                           << " against action branch " << static_cast<Branch const *>(action_branch)
//                           << std::endl;
                assert(parser_branch != NULL && static_cast<Branch const *>(parser_branch) == static_cast<Branch const *>(action_branch));
            }
            if (m_reduce_child != NULL)
                m_reduce_child->AssertActionAndParserBranchListsAreConsistent(parser_branch);
            if (m_shift_child != NULL)
                m_shift_child->AssertActionAndParserBranchListsAreConsistent(parser_branch);
        }
        void PrivatePrint_ (std::ostream &stream, Rule_ const *rule_table, State_ const *state_table, Uint32_ indent_level) const
        {
            assert(m_parent == NULL);
            stream << std::string(2*indent_level, ' ') << "root " << this << std::endl;
            PrintChildNodes_(stream, rule_table, state_table, indent_level+1);
        }
    }; // end of struct Npda_::Action
    struct Reduce : public Action
    {
        Rule_ const *m_reduction_rule_;

        Reduce (Rule_ const *reduction_rule)
            :
            Action(REDUCE),
            m_reduction_rule_(reduction_rule)
        {
            assert(m_reduction_rule_ != NULL);
        }

        void Print (std::ostream &stream, Rule_ const *rule_table, State_ const *state_table, Uint32_ indent_level) const
        {
            stream << std::string(2*indent_level, ' ') << (m_parent != NULL ? "reduce " : "root ") << this
                   << " \"" << m_reduction_rule_->m_description_ << '\"'
                   << ", precedence = " << m_reduction_rule_->m_precedence_->m_level_
                   << ", associativity = " << m_reduction_rule_->m_precedence_->m_associativity_
                   << ", rule index = " << m_reduction_rule_-rule_table << std::endl;
            PrintChildNodes_(stream, rule_table, state_table, indent_level+1);
        }
    }; // end of struct Npda_::Reduce
    struct Shift : public Action
    {
        Shift () : Action(SHIFT) { }
        ~Shift () { ClearRuleRange(); }

        bool IsRuleRangeEmpty_ () const { return m_rule_range_.empty(); }
        Rule_ const *RangeRuleHigh_ () const { assert(!m_rule_range_.empty()); return m_rule_range_.begin()->first; }
        Rule_ const *RangeRuleLow_ () const { assert(!m_rule_range_.empty()); return m_rule_range_.rbegin()->first; }

        ShiftReference EnsureShiftReferenceExists_ (Rule_ const *rule)
        {
            ShiftReferenceMap_::iterator it = m_rule_range_.find(rule);
            if (it == m_rule_range_.end())
            {
                ShiftReference &shift_reference = m_rule_range_[rule] = ShiftReference(this);
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
            for (ShiftReferenceMap_::iterator it = m_rule_range_.begin(), it_end = m_rule_range_.end();
                 it != it_end;
                 ++it)
            {
                ShiftReference &shift_reference = it->second;
                assert(shift_reference.IsValid());
                assert(shift_reference.ReferenceCount() >= 1);
                assert(shift_reference.InstanceIsValid());
                shift_reference.InstanceRelease_();
                assert(shift_reference.IsValid());
                assert(shift_reference.ReferenceCount() >= 1);
                assert(!shift_reference.InstanceIsValid());
            }
            m_rule_range_.clear();
        }
        void DestroyAllUnusedInRuleRange ()
        {
            // go through all rule references and erase/destruct all references
            // that have a ref count of 1 (indicating we're the only ones using
            // them).  the wacky iteration is so we don't have to worry about
            // incrementing an erased iterator.
            for (ShiftReferenceMap_::iterator next_it = m_rule_range_.begin(),
                                              it_end = m_rule_range_.end(),
                                              it = (next_it != it_end) ? next_it++ : next_it;
                 it != it_end;
                 it = (next_it != it_end) ? next_it++ : next_it)
            {
                ShiftReference &shift_reference = it->second;
                assert(shift_reference.IsValid());
                assert(shift_reference.ReferenceCount() >= 1);
                if (shift_reference.ReferenceCount() == 1)
                    m_rule_range_.erase(it); // this should call ~ShiftReference()
            }
        }

        void Print (std::ostream &stream, Rule_ const *rule_table, State_ const *state_table, Uint32_ indent_level) const
        {
            stream << std::string(2*indent_level, ' ') << (m_parent != NULL ? "shift " : "root ") << this << std::endl;
            if (!m_rule_range_.empty())
                stream << std::string(2*(indent_level+1), ' ') << "rule range" << std::endl;
            for (ShiftReferenceMap_::const_iterator it = m_rule_range_.begin(), it_end = m_rule_range_.end();
                 it != it_end;
                 ++it)
            {
                Rule_ const *rule = it->first;
                ShiftReference const &shift_reference = it->second;
                assert(shift_reference.IsValid());
                assert(shift_reference.InstanceIsValid());
                // TODO: make into assert(shift_reference.ReferenceCount() > 1); -- changes in the ref count should cause cleanup in the rule range
                if (shift_reference.ReferenceCount() > 1)
                {
                    stream << std::string(2*(indent_level+2), ' ') << rule;
                    if (rule != NULL)
                        stream << " \"" << rule->m_description_ << '\"'
                            << ", precedence = " << rule->m_precedence_->m_level_
                            << ", associativity = " << rule->m_precedence_->m_associativity_
                            << ", rule index = " << rule-rule_table
                            << ", ref count = " << shift_reference.ReferenceCount();
                    stream << std::endl;
                }
            }
            // TODO: print rule range
            PrintChildNodes_(stream, rule_table, state_table, indent_level+1);
        }

    private:

        ShiftReferenceMap_ m_rule_range_;
    }; // end of struct Npda_::Shift

public:

    typedef Uint32 TokenId;

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

    DpdaState CurrentDpdaState () const
    {
        return DpdaState(); // TODO: real code
    }

private:

    Action *m_tree_root;
    ParserBranchList m_parser_branch_list;
    ParserBranchList m_doomed_nonreturn_branch_list;
    ParserBranchList m_doomed_return_branch_list;
    bool m_is_shift_blocked;
    bool m_reduce_transitions_were_performed;
    bool m_shift_transitions_were_performed;
    bool m_nonassoc_error_encountered;
}; // end of class Npda

void Recurse (GraphContext &graph_context, DpdaState const &source_dpda_state, Npda &npda, ActionSpec const &default_action, Graph::Transition::DataArray &lookahead_sequence)
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
    for (TransitionIterator it(dpda_state, IT_TERMINALS); !it.IsDone(); ++it)
    {
        ActionSpec action;
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

            // add the iterator token to the lookahead sequence
            lookahead_sequence.push_back(*it);
            // add a graph transition from this state to that, via the iterator terminal
            assert(graph_context.m_generated_dpda_state_map.find(source_dpda_state) != graph_context.m_generated_dpda_state_map.end());
            // TODO -- check if this action matches the default action.  if so,
            // then there is no need to add this transition (because it will
            // happen by default)
            graph_context.m_dpda_graph.AddTransition(
                graph_context.m_generated_dpda_state_map[source_dpda_state],
                ShiftTransition(
                    lookahead_sequence,
                    "TODO -- real label",
                    graph_context.m_generated_dpda_state_map[target_dpda_state]));
            // pop the iterator token
            lookahead_sequence.pop_back();
        }
    }
}

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
    graph_context.m_generated_dpda_state_map[dpda_state] = graph_context.m_dpda_graph.AddNode(/* TODO -- real NodeData */);

    // construct the npda with dpda_state as its initial conditions
    Npda npda(dpda_state);
    assert(npda.CurrentDpdaState() == dpda_state && "malformed dpda_state");

    // run the npda with no input, so we can decide what the default action is
    npda.Run();
    // if there is no action in the trunk, then the default action is error panic.
    // otherwise it is necessarily a reduce action (because there are no tokens
    // to shift yet.
    ActionSpec default_action(npda.FirstTrunkAction());
    assert(default_action.Type() == AT_ERROR_PANIC || default_action.Type() == AT_REDUCE);

    // now recurse, exploring the states resulting from all valid transitions.
    Graph::Transition::DataArray lookahead_sequence; // empty
    Recurse(graph_context, dpda_state, npda, default_action, lookahead_sequence);

    // generate the shift transitions for the nonterminals at this state
    for (TransitionIterator it(dpda_state, IT_NONTERMINALS); !it.IsDone(); ++it)
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
