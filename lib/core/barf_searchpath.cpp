// ///////////////////////////////////////////////////////////////////////////
// barf_searchpath.cpp by Victor Dods, created 2006/11/09
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_searchpath.hpp"

#include <iostream>

#include "barf_util.hpp"

namespace Barf {

string SearchPath::GetFilePath (string const &filename) const
{
    assert(!m_path_stack.empty() && "there must be at least one valid path");
    for (PathEntryStack::const_reverse_iterator it = m_path_stack.rbegin(),
                                                it_end = m_path_stack.rend();
         it != it_end;
         ++it)
    {
        PathEntry const &path_entry = *it;
        string file_path(path_entry.GetPath() + filename);
        if (GetIsValidFile(file_path))
            return file_path;
    }
    return g_empty_string;
}

SearchPath::AddPathReturnCode SearchPath::AddPath (string path, string const &set_by)
{
    // don't add empty paths.
    if (path.empty())
        return ADD_PATH_FAILURE_EMPTY;
    
    // make sure there's a slash at the end of the path.
    if (*path.rbegin() != DIRECTORY_SLASH_CHAR)
        path += DIRECTORY_SLASH_CHAR;
        
    // only add the path if it actually exists.
    if (!GetIsValidDirectory(path))
        return ADD_PATH_FAILURE_INVALID;
    
    // add the path to the top of the stack and return success
    m_path_stack.push_back(PathEntry(path, set_by));
    return ADD_PATH_SUCCESS;
}

string SearchPath::GetAsStringPrivate (string const &delimiter, SearchPath::Verbosity verbosity) const
{
    string path_string;
    for (PathEntryStack::const_reverse_iterator it = m_path_stack.rbegin(),
                                                it_end = m_path_stack.rend();
         it != it_end;
         ++it)
    {
        PathEntry const &path_entry = *it;
        path_string += GetStringLiteral(path_entry.GetPath());
        if (verbosity == VERBOSE)
            path_string += " (" + path_entry.GetSetBy() + ')';
        // only add the delimiter if there's another path to iterate over.
        PathEntryStack::const_reverse_iterator next_it = it;
        ++next_it;
        if (next_it != it_end)
            path_string += delimiter;
    }
    return path_string;
}

} // end of namespace Barf
