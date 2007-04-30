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

SearchPath::SearchPath ()
{
    // TODO: config.h-specified path

    char const *search_path = getenv("BARF_SEARCH_PATH");
    if (search_path != NULL)
        AddPath(search_path, "set by BARF_SEARCH_PATH environment variable");
}

string SearchPath::GetAsString () const
{
    assert(!m_path_stack.empty() && "there must be at least one valid path");
    string path_string;
    for (PathStack::const_reverse_iterator it = m_path_stack.rbegin(),
                                           it_end = m_path_stack.rend();
         it != it_end;
         ++it)
    {
        path_string += '\"';
        path_string += *it;
        path_string += '\"';
        PathStack::const_reverse_iterator next_it = it;
        ++next_it;
        if (next_it != it_end)
            path_string += ", ";
    }
    return path_string;
}

string SearchPath::GetFilePath (string const &filename) const
{
    assert(!m_path_stack.empty() && "there must be at least one valid path");
    for (PathStack::const_reverse_iterator it = m_path_stack.rbegin(),
                                           it_end = m_path_stack.rend();
         it != it_end;
         ++it)
    {
        string file_path(*it + filename);
        if (GetIsValidFile(file_path))
            return file_path;
    }
    return gs_empty_string;
}

void SearchPath::AddPath (string path, string const &set_by)
{
    assert(!path.empty());
    if (*path.rbegin() != DIRECTORY_SLASH_CHAR)
        path += DIRECTORY_SLASH_CHAR;
    if (GetIsValidDirectory(path))
        m_path_stack.push_back(path);
    else
        cerr << "error: invalid data path \"" << path << "\" " << set_by << endl;
}

} // end of namespace Barf
