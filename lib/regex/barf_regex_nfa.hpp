// ///////////////////////////////////////////////////////////////////////////
// barf_regex_nfa.hpp by Victor Dods, created 2006/12/27
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_REGEX_NFA_HPP_)
#define BARF_REGEX_NFA_HPP_

#include "barf_regex.hpp"

#include <sstream>

namespace Barf {

class Graph;

namespace Regex {

// ///////////////////////////////////////////////////////////////////////////
// Graph::Node::Data implementations for Nonterminal and Rule (and generic)
// ///////////////////////////////////////////////////////////////////////////

enum IsStartNode
{
    NOT_START_NODE,
    IS_START_NODE
};

enum IsAcceptNode
{
    NOT_ACCEPT_NODE,
    IS_ACCEPT_NODE
};

struct NodeData : public Graph::Node::Data
{
    bool const m_is_start_node;
    bool const m_is_accept_node;
    string const m_node_label;
    Uint32 const m_dfa_accept_handler_index;

    NodeData (IsStartNode is_start_node, IsAcceptNode is_accept_node, string const &node_label = g_empty_string, Uint32 dfa_accept_handler_index = 0)
        :
        m_is_start_node(is_start_node == IS_START_NODE),
        m_is_accept_node(is_accept_node == IS_ACCEPT_NODE),
        m_node_label(node_label),
        m_dfa_accept_handler_index(dfa_accept_handler_index)
    { }

    virtual string GetAsText (Uint32 node_index) const
    {
        ostringstream out;
        out << node_index;
        if (!m_node_label.empty())
            out << ':' << m_node_label;
        return out.str();
    }
    virtual Graph::Color DotGraphColor (Uint32 node_index) const
    {
        if (m_is_accept_node)
            return Graph::Color(0xB6FFAE);
        else
            return Graph::Color(0xFCFFAE);
    }
    virtual Uint32 GetNodePeripheries (Uint32 node_index) const
    {
        return m_is_start_node ? 2 : 1;
    }
}; // end of struct NodeData

NodeData const &GetNodeData (Graph const &graph, Uint32 node_index);

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

class RegularExpression;

void GenerateNfa (RegularExpression const &regular_expression, Graph &nfa_graph, Uint32 start_index, Uint32 end_index);

} // end of namespace Regex
} // end of namespace Barf

#endif // !defined(BARF_REGEX_NFA_HPP_)
