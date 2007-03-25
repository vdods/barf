// ///////////////////////////////////////////////////////////////////////////
// trison_npda.hpp by Victor Dods, created 2006/11/30
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_NPDA_HPP_)
#define _TRISON_NPDA_HPP_

#include "trison.hpp"

#include "barf_graph.hpp"

namespace Trison {

struct Nonterminal;
struct Representation;
struct Rule;

struct NpdaNodeData : public Graph::Node::Data
{
    virtual bool IsReturnState () const { return false; }
    virtual bool IsEpsilonClosureState () const { return false; }
    virtual Nonterminal const *GetAssociatedNonterminal () const { return NULL; }
    virtual Rule const *GetAssociatedRule () const { return NULL; }
    virtual Uint32 GetRuleStage () const { return UINT32_UPPER_BOUND; }
    virtual string GetDescription () const = 0;
}; // end of struct NpdaNodeData

void GenerateNpda (Representation const &representation, Graph &npda_graph);

} // end of namespace Trison

#endif // !defined(_TRISON_NPDA_HPP_)
