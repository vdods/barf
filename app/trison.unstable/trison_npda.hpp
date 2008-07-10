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
struct PrimarySource;
struct Rule;

extern bool const g_minimal_npda_graphing;

struct NpdaNodeData : public Graph::Node::Data
{
    virtual Nonterminal const *GetAssociatedNonterminal () const { return NULL; }
    virtual Rule const *GetAssociatedRule () const { return NULL; }
    // returns the index of the "stage" of the rule (e.g. "exp <- . exp '+' exp"
    // gives rule stage of 0, while "exp <- exp '+' . exp" gives rule stage of 2)
    virtual Uint32 GetRuleStage () const { return UINT32_UPPER_BOUND; }
    // return a one-line description of this node, without terminating newline.
    virtual string GetOneLineDescription () const = 0;
    // return a full description of this node, with terminating newline.
    // min_width gives the minimum width of the output (it should be left-
    // justified, padded with whitespace ' ').
    virtual string GetFullDescription (Uint32 min_width) const;
    virtual bool IsStartState () const { return false; }
    virtual bool IsReturnState () const { return false; }
}; // end of struct NpdaNodeData

void GenerateNpda (PrimarySource const &primary_source, Graph &npda_graph);

} // end of namespace Trison

#endif // !defined(_TRISON_NPDA_HPP_)
