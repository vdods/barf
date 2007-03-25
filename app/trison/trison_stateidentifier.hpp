// ///////////////////////////////////////////////////////////////////////////
// trison_stateidentifier.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_STATEIDENTIFIER_HPP_)
#define _TRISON_STATEIDENTIFIER_HPP_

#include "trison.hpp"

#include <iostream>
#include <set>

#include "trison_rulephase.hpp"

namespace Trison {

typedef set<RulePhase> StateIdentifier;

ostream &operator << (ostream &stream, StateIdentifier const &state_identifier);

} // end of namespace Trison

#endif // !defined(_TRISON_STATEIDENTIFIER_HPP_)
