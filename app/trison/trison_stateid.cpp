// ///////////////////////////////////////////////////////////////////////////
// trison_stateid.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_stateid.hpp"

namespace Trison {

ostream &operator << (ostream &stream, StateId const &state_id)
{
    for (StateId::const_iterator it = state_id.begin(),
                                      it_end = state_id.end();
         it != it_end;
         ++it)
    {
        RulePhase const &rule_phase = *it;
        stream << rule_phase;

        StateId::const_iterator test_it = it;
        ++test_it;
        if (test_it != it_end)
            stream << " ";
    }
    return stream;
}

} // end of namespace Trison
