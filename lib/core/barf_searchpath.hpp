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

    SearchPath ();

    inline bool GetIsEmpty () const { return m_path_stack.empty(); }
    // returns a string containing the search path directories, from highest
    // search priority to lowest.
    string GetAsString () const;
    // returns the full path to the matching file, or empty if none matched
    string GetFilePath (string const &filename) const;

    // add another path to the top of the path stack (with higher priority).
    void AddPath (string path, string const &set_by);

private:

    typedef vector<string> PathStack;

    PathStack m_path_stack;
}; // end of class SearchPath

} // end of namespace Barf

#endif // !defined(_BARF_SEARCHPATH_HPP_)
