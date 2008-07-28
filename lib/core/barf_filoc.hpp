// ///////////////////////////////////////////////////////////////////////////
// barf_filoc.hpp by Victor Dods, created 2006/02/11
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_FILOC_HPP_)
#define BARF_FILOC_HPP_

#include "barf.hpp"

#include <ostream>

namespace Barf {

class FiLoc
{
public:

    static FiLoc const ms_invalid;

    FiLoc (string const &filename)
        :
        m_filename(filename),
        m_line_number(0)
    {
        assert(!m_filename.empty());
    }
    FiLoc (string const &filename, Uint32 line)
        :
        m_filename(filename),
        m_line_number(line)
    {
        assert(!m_filename.empty());
        assert(m_line_number > 0);
    }

    inline bool GetIsValid () const
    {
        return !m_filename.empty();
    }
    inline bool GetHasLineNumber () const
    {
        return !m_filename.empty() && m_line_number > 0;
    }
    inline string const &GetFilename () const
    {
        assert(this != &ms_invalid && "can't use FiLoc::ms_invalid in this manner");
        return m_filename;
    }
    inline Uint32 GetLineNumber () const
    {
        assert(this != &ms_invalid && "can't use FiLoc::ms_invalid in this manner");
        return m_line_number;
    }
    string GetAsString () const;
    string GetLineDirectiveString () const;

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
    FiLoc () : m_filename(), m_line_number(0) { }

    string m_filename;
    Uint32 m_line_number;
}; // end of class FiLoc

ostream &operator << (ostream &stream, FiLoc const &filoc);

} // end of namespace Barf

#endif // !defined(BARF_FILOC_HPP_)
