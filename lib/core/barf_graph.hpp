// ///////////////////////////////////////////////////////////////////////////
// barf_graph.hpp by Victor Dods, created 2006/10/07
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_GRAPH_HPP_)
#define _BARF_GRAPH_HPP_

#include "barf.hpp"

#include <set>
#include <vector>

namespace Barf {

// There will never be more than 2^8 different Graph::Transition types.  In
// fact, there are 4 ::Barf::Regex's transition type enums, and 4 ::Trison
// transition type enums.  ::Reflex uses the regex transition type enums.
typedef Uint8 TransitionType;

class Graph
{
public:

    struct Color
    {
        explicit Color (Uint32 hex_value) : m_hex_value(hex_value) { assert(m_hex_value <= 0xFFFFFF); }

    private:

        Uint32 m_hex_value;

        friend ostream &operator << (ostream &stream, Color const &color);
    }; // end of struct Graph::Color

    // Transition is meant to be subclassed for use, but the subclasses should
    // really only be frontends for constructors -- they shouldn't have any member
    // variables, and they shouldn't do anything in the destructor (since the
    // subclass destructor won't be called).  TODO -- change this scheme to
    // have functions which construct custom Transitions.
    struct Transition
    {
        typedef string (*Stringify)(Transition const &);
        typedef vector<Uint32> DataArray;

        static Uint32 const ms_no_target_index = UINT32_UPPER_BOUND;

        bool HasTarget () const { return m_target_index != ms_no_target_index; }
        TransitionType Type () const { return m_transition_type; }
//         DataArray const &Data () const { return m_data_array; }
        Uint32 DataCount () const { return m_data_array.size(); }
        Uint32 Data (Uint32 index) const { return (index < m_data_array.size()) ? m_data_array[index] : 0; }
        Uint32 TargetIndex () const { return m_target_index; }
        string const &Label () const { return m_label; }
        Color const &DotGraphColor () const { return m_dot_graph_color; }

        struct Order
        {
            bool operator () (Transition const &t0, Transition const &t1)
            {
                return t0.Type() < t1.Type()
                    ||
                    t0.Type() == t1.Type() &&
                    t0.m_data_array < t1.m_data_array
                    ||
                    t0.m_data_array == t1.m_data_array &&
                    t0.TargetIndex() < t1.TargetIndex();
            }
        }; // end of struct Graph::Transition::Order

    protected:

        Transition (
            TransitionType type,
            Uint32 data_count,
            Uint32 target_index,
            string const &label,
            Color const &dot_graph_color = Color(0x000000))
            :
            m_transition_type(type),
            m_data_array(data_count, 0),
            m_target_index(target_index),
            m_label(label),
            m_dot_graph_color(dot_graph_color)
        { }
        Transition (
            TransitionType type,
            DataArray const &data_array,
            Uint32 target_index,
            string const &label,
            Color const &dot_graph_color = Color(0x000000))
            :
            m_transition_type(type),
            m_data_array(data_array),
            m_target_index(target_index),
            m_label(label),
            m_dot_graph_color(dot_graph_color)
        { }

        void SetData (Uint32 index, Uint32 data) { assert(index < m_data_array.size()); m_data_array[index] = data; }
        void SetLabel (string const &label) { m_label = label; }

    private:

        TransitionType m_transition_type;
        DataArray m_data_array;
        Uint32 m_target_index;
        string m_label;
        Color m_dot_graph_color;

        friend struct Order;
    }; // end of struct Graph::Transition

    typedef set<Transition, Transition::Order> TransitionSet;

    class Node
    {
    public:

        // interface class for Graph::Node data
        struct Data
        {
            virtual ~Data () { }
            virtual string GetAsText (Uint32 node_index) const = 0;
            virtual Color DotGraphColor (Uint32 node_index) const = 0;
            virtual Uint32 GetNodePeripheries (Uint32 node_index) const { return 1; }
        };

        Node (Data const *data = NULL) : m_data(data) { }

        bool GetHasData () const { return m_data != NULL; }
        Data const &GetData () const { assert(GetHasData()); return *m_data; }
        template <typename DataType>
        DataType const &GetDataAs () const { assert(GetHasData()); return *Dsc<DataType const *>(m_data); }
        Uint32 GetTransitionCount () const { return m_transition_set.size(); }
        TransitionSet::const_iterator GetTransitionSetBegin () const { return m_transition_set.begin(); }
        TransitionSet::const_iterator GetTransitionSetEnd () const { return m_transition_set.end(); }

        inline void AddTransition (Transition const &transition) { m_transition_set.insert(transition); }

    private:

        TransitionSet m_transition_set;
        Data const *m_data;
    }; // end of class Graph::Node

    Uint32 GetNodeCount () const { return m_node_array.size(); }
    Node const &GetNode (Uint32 index) const
    {
        assert(index < m_node_array.size());
        return m_node_array[index];
    }

    Uint32 AddNode (Node::Data const *node_data = NULL);
    void AddTransition (Uint32 source_index, Transition const &transition);
    void PrintDotGraph (ostream &stream, string const &graph_name) const;

private:

    typedef vector<Node> NodeArray;

    NodeArray m_node_array;
}; // end of class Graph

ostream &operator << (ostream &stream, Graph::Color const &color);

struct Automaton
{
    Graph m_graph;
    vector<Uint32> m_start_state_index;
}; // end of struct Automaton

} // end of namespace Barf

#endif // !defined(_BARF_GRAPH_HPP_)
