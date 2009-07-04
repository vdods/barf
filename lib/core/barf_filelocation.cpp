// ///////////////////////////////////////////////////////////////////////////
// barf_filelocation.hpp by Victor Dods, created 2006/02/11
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_filelocation.hpp"

#include <sstream>

#include "barf_util.hpp"

namespace Barf {

FileLocation const FileLocation::ms_invalid;

string FileLocation::AsString () const
{
    assert(this != &ms_invalid && "can't use FileLocation::ms_invalid in this manner");
    assert(IsValid());

    ostringstream out;
    out << m_filename;
    if (m_line_number > 0)
        out << ":" << m_line_number;
    return out.str();
}

string FileLocation::LineDirectiveString () const
{
    assert(this != &ms_invalid && "do not use this on a FileLocation without a line number");
    assert(IsValid());

    ostringstream out;
    out << "#line " << m_line_number << " \"" << FilenamePortion(m_filename) << "\"";
    return out.str();
}

void FileLocation::IncrementLineNumber (Uint32 by_value)
{
    assert(m_line_number > 0 && "don't use this on non-line-number-using FiLocs");
    m_line_number += by_value;
}

ostream &operator << (ostream &stream, FileLocation const &filoc)
{
    return stream << filoc.AsString();
}

} // end of namespace Barf
