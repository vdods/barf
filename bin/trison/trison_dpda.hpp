// ///////////////////////////////////////////////////////////////////////////
// trison_dpda.hpp by Victor Dods, created 2006/11/30
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(TRISON_DPDA_HPP_)
#define TRISON_DPDA_HPP_

#include "trison.hpp"

#include <set>
#include <string>
#include <vector>

#include "trison_ast.hpp"
#include "trison_graph.hpp"

namespace Trison {

// a set of npda states constitutes a single dpda state
typedef set<Uint32> DpdaState;

struct ConflictResolution
{
    string m_annotation;
    vector<Rule const *> m_shift_rules;
    Rule const *m_reduce_rule;

    ConflictResolution (string const &annotation, vector<Rule const *> const &shift_rules, Rule const *reduce_rule)
        : m_annotation(annotation)
        , m_shift_rules(shift_rules)
        , m_reduce_rule(reduce_rule)
    { }
};

struct DpdaNodeData : public Graph::Node::Data
{
    DpdaNodeData (Graph const &npda_graph, DpdaState const &dpda_state);

    virtual string AsText (Uint32 node_index) const;
    virtual Graph::Color DotGraphColor (Uint32 node_index) const;
    virtual Uint32 NodePeripheries (Uint32 node_index) const;

    DpdaState const &GetDpdaState () const { return m_dpda_state; }
    string const &Description () const { return m_description; }
    vector<ConflictResolution *> const &ConflictResolutions () const { return m_conflict_resolutions; }

    void AddConflictResolution (ConflictResolution *conflict_resolution);

private:

    DpdaState const m_dpda_state;
    string m_description;
    bool m_is_start_state;
    bool m_is_return_state;
    vector<ConflictResolution *> m_conflict_resolutions;
}; // end of struct DpdaNodeData

void GenerateDpda (PrimarySource const &primary_source, Graph const &npda_graph, Graph &dpda_graph, Uint32 &lalr_lookahead_count);
void PrintDpdaStatesFile (PrimarySource const &primary_source, Graph const &npda_graph, Graph const &dpda_state, Uint32 lalr_lookahead_count, ostream &stream);

} // end of namespace Trison

#endif // !defined(TRISON_DPDA_HPP_)
