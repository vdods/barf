// ///////////////////////////////////////////////////////////////////////////
// barf_searchpath.hpp by Victor Dods, created 2006/11/09
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_SEARCHPATH_HPP_)
#define _BARF_SEARCHPATH_HPP_

#include "barf.hpp"

#include <vector>

namespace Barf {

class SearchPath
{
public:

    enum AddPathReturnCode { ADD_PATH_SUCCESS, ADD_PATH_FAILURE_EMPTY, ADD_PATH_FAILURE_INVALID };

    inline bool GetIsEmpty () const { return m_path_stack.empty(); }
    // returns a string containing the search path directories, from highest
    // search priority to lowest, each delimited by the given delimiter.
    inline string GetAsString (string const &delimiter = ", ") const { return GetAsStringPrivate(delimiter, NON_VERBOSE); }
    // returns a string containing the search path directories, from highest
    // search priority to lowest, each followed by their "set by" strings, 
    // each delimited by the given delimiter.
    string GetAsVerboseString (string const &delimiter = "\n") const { return GetAsStringPrivate(delimiter, VERBOSE); }
    // returns the full path to the matching file, or empty if none matched
    string GetFilePath (string const &filename) const;

    // add another path to the top of the path stack (with higher priority).
    AddPathReturnCode AddPath (string path, string const &set_by);

private:

    enum Verbosity { NON_VERBOSE, VERBOSE };

    string GetAsStringPrivate (string const &delimiter, Verbosity verbosity) const;

    class PathEntry
    {
    public:
    
        PathEntry (string const &path, string const &set_by)
            :
            m_path(path),
            m_set_by(set_by)
        {
            assert(!m_path.empty());
            assert(!m_set_by.empty());
        }
    
        inline string const &GetPath () const { return m_path; }
        inline string const &GetSetBy () const { return m_set_by; }
    
    private:
    
        string m_path;
        string m_set_by;
    }; // end of class SearchPath::PathEntry

    typedef vector<PathEntry> PathEntryStack;

    PathEntryStack m_path_stack;
}; // end of class SearchPath

} // end of namespace Barf

#endif // !defined(_BARF_SEARCHPATH_HPP_)
