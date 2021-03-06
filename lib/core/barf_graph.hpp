// 2006.10.07 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_GRAPH_HPP_)
#define BARF_GRAPH_HPP_

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
        static Color const ms_black;
        static Color const ms_white;
        static Color const ms_orange;
        static Color const ms_red;
        static Color const ms_green;
        static Color const ms_blue;

        explicit Color (Uint32 hex_value) : m_hex_value(hex_value) { assert(m_hex_value <= 0xFFFFFF); }

    private:

        Uint32 m_hex_value;

        friend ostream &operator << (ostream &stream, Color const &color);
    }; // end of struct Graph::Color

    struct Transition
    {
        typedef string (*Stringify)(Transition const &);
        typedef vector<Uint32> DataArray;

        static Uint32 const ms_no_target_index = UINT32_UPPER_BOUND;

        Transition (
            TransitionType type,
            Uint32 data_count,
            Uint32 target_index,
            string const &label,
            Color const &dot_graph_color = Color::ms_black)
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
            Color const &dot_graph_color = Color::ms_black)
            :
            m_transition_type(type),
            m_data_array(data_array),
            m_target_index(target_index),
            m_label(label),
            m_dot_graph_color(dot_graph_color)
        { }
        Transition (Transition const &transition)
            :
            m_transition_type(transition.m_transition_type),
            m_data_array(transition.m_data_array),
            m_target_index(transition.m_target_index),
            m_label(transition.m_label),
            m_dot_graph_color(transition.m_dot_graph_color)
        { }

        bool HasTarget () const { return m_target_index != ms_no_target_index; }
        TransitionType Type () const { return m_transition_type; }
        Uint32 DataCount () const { return m_data_array.size(); }
        Uint32 Data (Uint32 index) const { return (index < m_data_array.size()) ? m_data_array[index] : 0; }
        Uint32 TargetIndex () const { return m_target_index; }
        string const &Label () const { return m_label; }
        Color const &DotGraphColor () const { return m_dot_graph_color; }

        void SetData (Uint32 index, Uint32 data) { assert(index < m_data_array.size()); m_data_array[index] = data; }
        void SetLabel (string const &label) { m_label = label; }

        struct Order
        {
            bool operator () (Transition const &t0, Transition const &t1) const
            {
                if (t0.Type() < t1.Type())
                    return true;
                if (t0.Type() > t1.Type())
                    return false;

                if (t0.DataCount() < t1.DataCount())
                    return true;
                if (t0.DataCount() > t1.DataCount())
                    return false;

                if (t0.m_data_array < t1.m_data_array)
                    return true;
                if (t0.m_data_array > t1.m_data_array)
                    return false;

                if (t0.TargetIndex() < t1.TargetIndex())
                    return true;
                if (t0.TargetIndex() > t1.TargetIndex())
                    return false;

                return false;
            }
        }; // end of struct Graph::Transition::Order

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
            virtual string AsText (Uint32 node_index) const = 0;
            virtual Color DotGraphColor (Uint32 node_index) const { return Color::ms_white; }
            virtual Uint32 NodePeripheries (Uint32 node_index) const { return 1; }
        };

        Node (Data const *data = NULL) : m_data(data) { }

        bool HasData () const { return m_data != NULL; }
        Data const &GetData () const { assert(HasData()); return *m_data; }
        template <typename DataType>
        DataType const &DataAs () const { assert(HasData()); return *Dsc<DataType const *>(m_data); }
        Uint32 TransitionCount () const { return m_transition_set.size(); }
        TransitionSet::const_iterator TransitionSetBegin () const { return m_transition_set.begin(); }
        TransitionSet::const_iterator TransitionSetEnd () const { return m_transition_set.end(); }

        inline void AddTransition (Transition const &transition)
        {
            assert(m_transition_set.find(transition) == m_transition_set.end() && "you can't add the same transition twice");
            m_transition_set.insert(transition);
        }

    private:

        TransitionSet m_transition_set;
        Data const *m_data;
    }; // end of class Graph::Node

    Uint32 NodeCount () const { return m_node_array.size(); }
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

#endif // !defined(BARF_GRAPH_HPP_)
