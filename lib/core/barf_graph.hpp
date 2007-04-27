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

    struct Transition
    {
        typedef string (*Stringify)(Transition const &);

        static Uint32 const ms_no_target_index = UINT32_UPPER_BOUND;

        bool HasTarget () const { return m_target_index != ms_no_target_index; }
        TransitionType Type () const { return m_transition_type; }
        Uint32 Data0 () const { return m_data_0; }
        Uint32 Data1 () const { return m_data_1; }
        Uint32 TargetIndex () const { return m_target_index; }
        string const &Label () const { return m_label; }
        Uint32 HexColor () const { return m_hex_color; }

    protected:

        Transition (TransitionType type, Uint32 data_0, Uint32 data_1, Uint32 target_index, string const &label, Uint32 hex_color = 0x000000)
            :
            m_transition_type(type),
            m_data_0(data_0),
            m_data_1(data_1),
            m_target_index(target_index),
            m_label(label),
            m_hex_color(hex_color)
        {
            assert(m_hex_color <= 0xFFFFFF);
        }
        
        void SetLabel (string const &label) { m_label = label; }
        
    private:
    
        TransitionType m_transition_type;
        Uint32 m_data_0;
        Uint32 m_data_1;
        Uint32 m_target_index;
        string m_label;
        Uint32 m_hex_color;
    }; // end of struct Transition

    struct TransitionOrder
    {
        bool operator () (Transition const &t0, Transition const &t1)
        {
            return t0.Type() < t1.Type()
                   ||
                   t0.Type() == t1.Type() &&
                   t0.Data0() < t1.Data0()
                   ||
                   t0.Data0() == t1.Data0() &&
                   t0.Data1() < t1.Data1()
                   ||
                   t0.Data1() == t1.Data1() &&
                   t0.TargetIndex() < t1.TargetIndex();
        }
    };

    class Node
    {
    public:

        typedef set<Transition, TransitionOrder> TransitionSet;

        // interface class for Graph::Node data
        struct Data
        {
            virtual ~Data () { }
            virtual string GetAsText (Uint32 node_index) const = 0;
            virtual string GetNodeColor (Uint32 node_index) const = 0;
            virtual Uint32 GetNodePeripheries (Uint32 node_index) const { return 1; }
        };

        Node (Data const *data = NULL)
            :
            m_data(data)
        { }

        bool GetHasData () const { return m_data != NULL; }
        Data const &GetData () const { assert(GetHasData()); return *m_data; }
        template <typename DataType>
        DataType const &GetDataAs () const
        {
            assert(GetHasData());
            return *DStaticCast<DataType const *>(m_data);
        }
        Uint32 GetTransitionCount () const { return m_transition_set.size(); }
        TransitionSet::const_iterator GetTransitionSetBegin () const { return m_transition_set.begin(); }
        TransitionSet::const_iterator GetTransitionSetEnd () const { return m_transition_set.end(); }

        inline void AddTransition (Transition const &transition) { m_transition_set.insert(transition); }

    private:

        TransitionSet m_transition_set;
        Data const *m_data;
    }; // end of class Node

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

} // end of namespace Barf

#endif // !defined(_BARF_GRAPH_HPP_)
