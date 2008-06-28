// ///////////////////////////////////////////////////////////////////////////
// trison_dpda.hpp by Victor Dods, created 2006/11/30
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_DPDA_HPP_)
#define _TRISON_DPDA_HPP_

#include "trison.hpp"

namespace Trison {

void GenerateDpda (PrimarySource const &primary_source, Graph const &npda_graph, Graph &dpda_graph);

} // end of namespace Trison

#endif // !defined(_TRISON_DPDA_HPP_)
