// ///////////////////////////////////////////////////////////////////////////
// trison_npda.hpp by Victor Dods, created 2006/11/30
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(TRISON_NPDA_HPP_)
#define TRISON_NPDA_HPP_

#include "trison.hpp"

#include "barf_graph.hpp"

namespace Trison {

struct Nonterminal;
struct PrimarySource;
struct Rule;

struct NpdaNodeData : public Graph::Node::Data
{
    virtual Nonterminal const *AssociatedNonterminal () const { return NULL; }
    virtual Rule const *AssociatedRule () const { return NULL; }
    // returns the index of the "stage" of the rule (e.g. "exp <- . exp '+' exp"
    // gives rule stage of 0, while "exp <- exp '+' . exp" gives rule stage of 2)
    virtual Uint32 RuleStage () const { return UINT32_UPPER_BOUND; }
    // return a one-line description of this node, without terminating newline.
    virtual string OneLineDescription () const = 0;
    // return a full description of this node, with terminating newline.
    // min_width gives the minimum width of the output (it should be left-
    // justified, padded with whitespace ' '), and it must not end with a newline.
    virtual string FullDescription (Uint32 min_width = 0) const;
    virtual bool IsStartState () const { return false; }
    virtual bool IsReturnState () const { return false; }
}; // end of struct NpdaNodeData

void GenerateNpda (PrimarySource const &primary_source, Graph &npda_graph);

} // end of namespace Trison

#endif // !defined(TRISON_NPDA_HPP_)
