// ///////////////////////////////////////////////////////////////////////////
// trison_stateidentifier.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_stateidentifier.hpp"

namespace Trison {

ostream &operator << (ostream &stream, StateIdentifier const &state_identifier)
{
    for (StateIdentifier::const_iterator it = state_identifier.begin(),
                                      it_end = state_identifier.end();
         it != it_end;
         ++it)
    {
        RulePhase const &rule_phase = *it;
        stream << rule_phase;

        StateIdentifier::const_iterator test_it = it;
        ++test_it;
        if (test_it != it_end)
            stream << " ";
    }
    return stream;
}

} // end of namespace Trison
