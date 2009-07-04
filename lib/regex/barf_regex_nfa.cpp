// ///////////////////////////////////////////////////////////////////////////
// barf_regex_nfa.cpp by Victor Dods, created 2006/12/27
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_regex_nfa.hpp"

#include <stack>

#include "barf_graph.hpp"
#include "barf_regex_ast.hpp"
#include "barf_regex_graph.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace Regex {

struct GraphContext
{
    Graph &m_graph;

    bool IsCaseSensitive () const { assert(!m_case_sensitivity_stack.empty()); return m_case_sensitivity_stack.top(); }
    void PushCaseSensitivity (bool is_case_sensitive) { m_case_sensitivity_stack.push(is_case_sensitive); }
    void ModifyCaseSensitivity (bool is_case_sensitive) { assert(m_case_sensitivity_stack.size() >= 1); m_case_sensitivity_stack.top() = is_case_sensitive; }
    void PopCaseSensitivity () { assert(m_case_sensitivity_stack.size() > 1); m_case_sensitivity_stack.pop(); }

    GraphContext (Graph &graph)
        :
        m_graph(graph)
    {
        m_case_sensitivity_stack.push(true);
    }
    ~GraphContext ()
    {
        assert(m_case_sensitivity_stack.size() == 1);
    }

private:

    stack<bool> m_case_sensitivity_stack;
}; // end of struct GraphContext

NodeData const &GetNodeData (Graph const &graph, Uint32 node_index)
{
    Graph::Node const &node = graph.GetNode(node_index);
    assert(node.HasData());
    return node.DataAs<NodeData>();
}

void GenerateNfa (Atom const &atom, GraphContext &graph_context, Uint32 start_index, Uint32 end_index);

void GenerateNfa (Piece const &piece, GraphContext &graph_context, Uint32 start_index, Uint32 end_index)
{
    assert(piece.GetAtom() != NULL);

    Uint32 end_of_required_repetitions_index;
    if (piece.GetBound()->m_lower_bound == piece.GetBound()->m_upper_bound)
        end_of_required_repetitions_index = end_index;
    else if (piece.GetBound()->m_lower_bound == 0 && !piece.GetBound()->HasNoUpperBound())
        end_of_required_repetitions_index = start_index;
    else
        end_of_required_repetitions_index = graph_context.m_graph.AddNode(new NodeData(NOT_START_NODE, NOT_ACCEPT_NODE));

    // transitions to match the minimum number of repetitions (i.e. the first 3 in "a{3,5}")
    Sint16 repetition_count = piece.GetBound()->m_lower_bound;

    // special case for 0-required-repetitions
    if (repetition_count == 0)
    {
        graph_context.m_graph.AddTransition(start_index, EpsilonTransition(end_of_required_repetitions_index));
    }
    // normal iteration for everything else
    else
    {
        for (Uint32 local_start_index = start_index,
                    local_end_index = (repetition_count <= 1) ? end_of_required_repetitions_index : graph_context.m_graph.AddNode(new NodeData(NOT_START_NODE, NOT_ACCEPT_NODE));
             repetition_count > 0;
             --repetition_count,
             local_start_index = local_end_index,
             local_end_index = (repetition_count <= 1) ? end_of_required_repetitions_index : graph_context.m_graph.AddNode(new NodeData(NOT_START_NODE, NOT_ACCEPT_NODE)))
        {
            GenerateNfa(*piece.GetAtom(), graph_context, local_start_index, local_end_index);
        }
    }

    // transitions for the optional repetitions (i.e. the final infinity in "a{3,}" or final 2 in "a{3,5}")

    // special case for no-upper-bound -- "a{3,}"
    if (piece.GetBound()->HasNoUpperBound())
    {
        GenerateNfa(*piece.GetAtom(), graph_context, end_of_required_repetitions_index, end_of_required_repetitions_index);
        graph_context.m_graph.AddTransition(end_of_required_repetitions_index, EpsilonTransition(end_index));
    }
    // normal iteration for everything else
    else
    {
        repetition_count = piece.GetBound()->m_upper_bound - piece.GetBound()->m_lower_bound;
        assert(0 <= repetition_count && repetition_count <= 255);

        for (Uint32 local_start_index = end_of_required_repetitions_index,
                    local_end_index = (repetition_count <= 1) ? end_index : graph_context.m_graph.AddNode(new NodeData(NOT_START_NODE, NOT_ACCEPT_NODE));
             repetition_count > 0;
             --repetition_count,
             local_start_index = local_end_index,
             local_end_index = (repetition_count <= 1) ? end_index : graph_context.m_graph.AddNode(new NodeData(NOT_START_NODE, NOT_ACCEPT_NODE)))
        {
            graph_context.m_graph.AddTransition(local_start_index, EpsilonTransition(end_index));
            GenerateNfa(*piece.GetAtom(), graph_context, local_start_index, local_end_index);
        }
    }
}

void GenerateNfa (Branch const &branch, GraphContext &graph_context, Uint32 start_index, Uint32 end_index)
{
    Sint16 piece_count = branch.size();
    // special case for empty piece lists
    if (piece_count == 0)
    {
        graph_context.m_graph.AddTransition(start_index, EpsilonTransition(end_index));
    }
    // normal iteration for everything else
    else
    {
        Ast::AstList<Piece>::const_iterator it;
        Uint32 local_start_index;
        Uint32 local_end_index;
        for (it = branch.begin(),
             local_start_index = start_index,
             local_end_index = (piece_count <= 1) ? end_index : graph_context.m_graph.AddNode(new NodeData(NOT_START_NODE, NOT_ACCEPT_NODE));
             piece_count > 0;
             --piece_count,
             ++it,
             local_start_index = local_end_index,
             local_end_index = (piece_count <= 1) ? end_index : graph_context.m_graph.AddNode(new NodeData(NOT_START_NODE, NOT_ACCEPT_NODE)))
        {
            Piece const *piece = *it;
            assert(piece != NULL);
            GenerateNfa(*piece, graph_context, local_start_index, local_end_index);
        }
    }
}

void GenerateNfa (RegularExpression const &regular_expression, GraphContext &graph_context, Uint32 start_index, Uint32 end_index)
{
    // push the case sensitivity stack with its current top value
    graph_context.PushCaseSensitivity(graph_context.IsCaseSensitive());

    // special case for empty branch lists
    if (regular_expression.empty())
    {
        graph_context.m_graph.AddTransition(start_index, EpsilonTransition(end_index));
    }
    // normal iteration for everything else
    else
    {
        Uint32 branching_state_index = graph_context.m_graph.AddNode(new NodeData(NOT_START_NODE, NOT_ACCEPT_NODE));
        graph_context.m_graph.AddTransition(start_index, EpsilonTransition(branching_state_index));

        for (Ast::List<Branch>::const_iterator it = regular_expression.begin(),
                                               it_end = regular_expression.end();
             it != it_end;
             ++it)
        {
            Branch const *branch = *it;
            assert(branch != NULL);
            GenerateNfa(*branch, graph_context, branching_state_index, end_index);
        }
    }

    // pop the case sensitivity stack
    graph_context.PopCaseSensitivity();
}

void GenerateNfa (Char const &ch, GraphContext &graph_context, Uint32 start_index, Uint32 end_index)
{
    graph_context.m_graph.AddTransition(start_index, InputAtomTransition(ch.GetChar(), end_index));
    // if we're case insensitive and the char has case, then add its opposite case
    if (!graph_context.IsCaseSensitive() && ch.GetChar() != SwitchCase(ch.GetChar()))
        graph_context.m_graph.AddTransition(start_index, InputAtomTransition(SwitchCase(ch.GetChar()), end_index));
}

void GenerateNfa (ConditionalChar const &ch, GraphContext &graph_context, Uint32 start_index, Uint32 end_index)
{
    graph_context.m_graph.AddTransition(start_index, NfaConditionalTransition(ch.GetConditionalType(), end_index));
}

void GenerateNfa (BakedControlChar const &ch, GraphContext &graph_context, Uint32 start_index, Uint32 end_index)
{
    graph_context.m_graph.AddTransition(start_index, EpsilonTransition(end_index));
    switch (ch.GetBakedControlCharType())
    {
        case BCCT_CASE_SENSITIVITY_DISABLE:
            graph_context.ModifyCaseSensitivity(false);
            break;
            
        case BCCT_CASE_SENSITIVITY_ENABLE:
            graph_context.ModifyCaseSensitivity(true);
            break;

        default:
            assert(false && "invalid BakedControlCharType");
            break;
    }
}

void GenerateNfa (BracketCharSet const &bracket_char_set, GraphContext &graph_context, Uint32 start_index, Uint32 end_index)
{
    Uint16 c0 = 0;
    Uint16 c1;
    while (c0 < 256)
    {
        if (bracket_char_set.IsCharInSet(c0, graph_context.IsCaseSensitive()))
        {
            c1 = c0;
            while (c1 < 255 && bracket_char_set.IsCharInSet(c1+1, graph_context.IsCaseSensitive()))
                ++c1;

            graph_context.m_graph.AddTransition(start_index, InputAtomRangeTransition(c0, c1, end_index));

            c0 = c1;
        }
        ++c0;
    }
}

// this is to provide the "polymorphism" for the Atom class Nfa generation.
void GenerateNfa (Atom const &atom, GraphContext &graph_context, Uint32 start_index, Uint32 end_index)
{
    switch (atom.GetAstType())
    {
        case AST_BAKED_CONTROL_CHAR: return GenerateNfa(static_cast<BakedControlChar const &>(atom),  graph_context, start_index, end_index);
        case AST_BRACKET_CHAR_SET:   return GenerateNfa(static_cast<BracketCharSet const &>(atom),    graph_context, start_index, end_index);
        case AST_CHAR:               return GenerateNfa(static_cast<Char const &>(atom),              graph_context, start_index, end_index);
        case AST_CONDITIONAL_CHAR:   return GenerateNfa(static_cast<ConditionalChar const &>(atom),   graph_context, start_index, end_index);
        case AST_REGULAR_EXPRESSION: return GenerateNfa(static_cast<RegularExpression const &>(atom), graph_context, start_index, end_index);
        default: assert(false && "invalid atom type");
    }
}

void GenerateNfa (RegularExpression const &regular_expression, Graph &nfa_graph, Uint32 start_index, Uint32 end_index)
{
    GraphContext graph_context(nfa_graph);
    GenerateNfa(regular_expression, graph_context, start_index, end_index);
}

} // end of namespace Regex
} // end of namespace Barf
