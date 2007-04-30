// ///////////////////////////////////////////////////////////////////////////
// barf_filoc.hpp by Victor Dods, created 2006/02/11
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_filoc.hpp"

#include <sstream>

#include "barf_util.hpp"

namespace Barf {

FiLoc const FiLoc::ms_invalid;

string FiLoc::GetAsString () const
{
    assert(this != &ms_invalid && "can't use FiLoc::ms_invalid in this manner");
    assert(GetIsValid());

    ostringstream out;
    out << m_filename;
    if (m_line_number > 0)
        out << ":" << m_line_number;
    return out.str();
}

string FiLoc::GetLineDirectiveString () const
{
    assert(this != &ms_invalid && "do not use this on a FiLoc without a line number");
    assert(GetIsValid());

    ostringstream out;
    out << "#line " << m_line_number << " \"" << GetFilenamePortion(m_filename) << "\"";
    return out.str();
}

void FiLoc::IncrementLineNumber (Uint32 by_value)
{
    assert(m_line_number > 0 && "don't use this on non-line-number-using FiLocs");
    m_line_number += by_value;
}

} // end of namespace Barf
