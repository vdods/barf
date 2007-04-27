// ///////////////////////////////////////////////////////////////////////////
// barf_graph.cpp by Victor Dods, created 2006/10/07
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_graph.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "barf_util.hpp"

namespace Barf {

// ///////////////////////////////////////////////////////////////////////////
// Graph
// ///////////////////////////////////////////////////////////////////////////

Uint32 Graph::AddNode (Node::Data const *node_data)
{
    m_node_array.push_back(Node(node_data));
    assert(m_node_array.size() >= 1);
    return m_node_array.size() - 1;
}

void Graph::AddTransition (Uint32 source_index, Transition const &transition)
{
    assert(source_index < m_node_array.size());
    m_node_array[source_index].AddTransition(transition);
}

void Graph::PrintDotGraph (ostream &stream, string const &graph_name) const
{
    stream << "digraph " << GetStringLiteral(graph_name) << " {" << endl
           << "    fontname=courier;" << endl
           << "    subgraph cluster {" << endl
           << "        label=" << GetStringLiteral(graph_name) << ";" << endl;

    for (Uint32 i = 0; i < m_node_array.size(); ++i)
    {
        Node const &node = m_node_array[i];
        if (node.GetHasData())
            stream << "        node [label=" << GetStringLiteral(node.GetData().GetAsText(i))
                   << ", style=filled, fillcolor=\"#" << node.GetData().GetNodeColor(i)
                   << "\", shape=box, fontname=courier, peripheries="
                   << node.GetData().GetNodePeripheries(i) << "];" << endl;
        else
            stream << "        node [label=\"\\N\", style=solid, shape=box, fontname=courier, peripheries=1];" << endl;
        stream << "        " << i << endl;
    }

    stream << endl;

    for (Uint32 i = 0; i < m_node_array.size(); ++i)
    {
        Node const &node = m_node_array[i];

        for (Node::TransitionSet::const_iterator it = node.GetTransitionSetBegin(),
                                                 it_end = node.GetTransitionSetEnd();
             it != it_end;
             ++it)
        {
            Transition const &transition = *it;

            string hex_color(FORMAT(setfill('0') << setw(6) << hex << transition.HexColor()));
            stream << "        edge [label=" << GetStringLiteral(transition.Label())
                   << ", fontname=courier, color=\"#" << hex_color << "\", fontcolor=\"#" << hex_color << "\", dir="
                   << (transition.HasTarget() ? "forward" : "none") << "];" << endl;
            stream << "        " << i << " -> " << (transition.HasTarget() ? transition.TargetIndex() : i) << endl;
        }
    }
    stream << "    }" << endl
           << "}" << endl;
}

} // end of namespace Barf
