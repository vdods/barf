// ///////////////////////////////////////////////////////////////////////////
// barf_filelocation.hpp by Victor Dods, created 2006/02/11
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_FILELOCATION_HPP_)
#define BARF_FILELOCATION_HPP_

#include "barf.hpp"

#include <ostream>

namespace Barf {

class FileLocation
{
public:

    static FileLocation const ms_invalid;

    FileLocation (string const &filename)
        :
        m_filename(filename),
        m_line_number(0)
    {
        assert(!m_filename.empty());
    }
    FileLocation (string const &filename, Uint32 line)
        :
        m_filename(filename),
        m_line_number(line)
    {
        assert(!m_filename.empty());
        assert(m_line_number > 0);
    }

    inline bool IsValid () const
    {
        return !m_filename.empty();
    }
    inline bool HasLineNumber () const
    {
        return !m_filename.empty() && m_line_number > 0;
    }
    inline string const &Filename () const
    {
        assert(this != &ms_invalid && "can't use FileLocation::ms_invalid in this manner");
        return m_filename;
    }
    inline Uint32 LineNumber () const
    {
        assert(this != &ms_invalid && "can't use FileLocation::ms_invalid in this manner");
        return m_line_number;
    }
    string AsString () const;
    string LineDirectiveString () const;

    inline void SetFilename (string const &filename)
    {
        m_filename = filename;
    }
    inline void SetLineNumber (Uint32 line_number)
    {
        m_line_number = line_number;
    }

    void IncrementLineNumber (Uint32 by_value = 1);

private:

    // for use only by the constructor of ms_invalid
    FileLocation () : m_filename(), m_line_number(0) { }

    string m_filename;
    Uint32 m_line_number;
}; // end of class FileLocation

ostream &operator << (ostream &stream, FileLocation const &filoc);

} // end of namespace Barf

#endif // !defined(BARF_FILELOCATION_HPP_)
