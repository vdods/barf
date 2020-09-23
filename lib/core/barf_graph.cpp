// 2006.10.07 - Copyright Victor Dods - Licensed under Apache 2.0

#include "barf_graph.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

#include "barf_util.hpp"

namespace Barf {

// ///////////////////////////////////////////////////////////////////////////
// Graph::Color
// ///////////////////////////////////////////////////////////////////////////

Graph::Color const Graph::Color::ms_black(0x000000);
Graph::Color const Graph::Color::ms_white(0xFFFFFF);
Graph::Color const Graph::Color::ms_orange(0xFF8C00);
Graph::Color const Graph::Color::ms_red(0xEF280E);
Graph::Color const Graph::Color::ms_green(0x008800);
Graph::Color const Graph::Color::ms_blue(0x0000FF);

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
    stream << "digraph " << StringLiteral(graph_name) << " {" << endl
           << "    fontname=courier;" << endl
           << "    subgraph cluster {" << endl
           << "        label=" << StringLiteral(graph_name) << ";" << endl;

    for (Uint32 i = 0; i < m_node_array.size(); ++i)
    {
        Node const &node = m_node_array[i];
        if (node.HasData())
            stream << "        node [label=" << StringLiteral(node.GetData().AsText(i))
                   << ", style=filled, fillcolor=\"#" << node.GetData().DotGraphColor(i)
                   << "\", shape=box, fontname=courier, peripheries="
                   << node.GetData().NodePeripheries(i) << "];" << endl;
        else
            stream << "        node [label=\"\\N\", style=solid, shape=box, fontname=courier, peripheries=1];" << endl;
        stream << "        " << i << endl;
    }

    stream << endl;

    for (Uint32 i = 0; i < m_node_array.size(); ++i)
    {
        Node const &node = m_node_array[i];

        for (TransitionSet::const_iterator it = node.TransitionSetBegin(),
                                           it_end = node.TransitionSetEnd();
             it != it_end;
             ++it)
        {
            Transition const &transition = *it;

            stream << "        edge [label=" << StringLiteral(transition.Label())
                   << ", fontname=courier, color=\"#" << transition.DotGraphColor()
                   << "\", fontcolor=\"#" << transition.DotGraphColor()
                   << "\", dir=" << (transition.HasTarget() ? "forward" : "none") << "];" << endl;
            stream << "        " << i << " -> " << (transition.HasTarget() ? transition.TargetIndex() : i) << endl;
        }
    }
    stream << "    }" << endl
           << "}" << endl;
}

ostream &operator << (ostream &stream, Graph::Color const &color)
{
    return stream << FORMAT(setfill('0') << setw(6) << hex << color.m_hex_value);
}

} // end of namespace Barf
