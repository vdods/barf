// ///////////////////////////////////////////////////////////////////////////
// barf_path.hpp by Victor Dods, created 2019/06/01
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_path.hpp"

// Reference: https://stackoverflow.com/questions/143174/how-do-i-get-the-directory-that-a-program-is-running-from
#include <cstdio>
#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
#endif

namespace Barf {

Path::Path (std::string const &s)
    :   m_is_absolute(false)
    ,   m_is_normalized(false)
{
    if (s.empty())
        return;

    m_is_absolute = s[0] == slash;

    std::string::size_type current = m_is_absolute ? 1 : 0;
    while (true)
    {
        std::string::size_type next = s.find_first_of(slash, current);
        std::string::size_type length = next == std::string::npos ? std::string::npos : next - current;
        {
            auto segment = s.substr(current, length);
            if (!segment.empty() && segment != std::string(1, slash))
                m_segments.emplace_back(std::move(segment));
        }
        if (next == std::string::npos)
            break;
        assert(next < s.size());
        current = next+1;
    }
}

std::string Path::as_string () const
{
    std::string retval;
    if (m_is_absolute)
        retval += slash;
    else if (m_segments.empty())
        return std::string(1, '.');
    for (std::size_t i = 0; i < m_segments.size(); ++i)
    {
        retval += m_segments[i];
        if (i != m_segments.size()-1)
            retval += slash;
    }
    return retval;
}

Path &Path::normalize ()
{
    if (m_is_normalized)
        return *this;

    // First collapse all "." (and ".." if absolute) at the head into nothing.
    {
        std::size_t i;
        for (i = 0; i < m_segments.size() && (m_segments[i] == "." || (m_is_absolute && m_segments[i] == "..")); ++i)
        {
            // Nothing to do
        }
        std::size_t j;
        for (j = 0; j < m_segments.size()-i; ++j)
            m_segments[j] = m_segments[j+i];
        m_segments.resize(j);
    }

    // Now cancel all ".." that follow segments (which aren't "." or "..")
    std::vector<std::string> new_segments;
    for (auto &&s : m_segments)
    {
        if (s == ".")
        {
            // Just ignore
        }
        else if (s == "..")
        {
            if (new_segments.empty())
            {
                if (m_is_absolute)
                {
                    // This is fine, swallow the ".."
                }
                else
                {
                    // Must keep the ".."
                    new_segments.emplace_back(std::move(s));
                    assert(new_segments.size() == 1);
                    assert(new_segments[0] == "..");
                }
            }
            else
            {
                if (m_is_absolute)
                {
                    assert(new_segments.back() != ".");
                    assert(new_segments.back() != "..");
                    // If there's something to cancel with, cancel it.
                    new_segments.pop_back();
                }
                else
                {
                    assert(new_segments.back() != ".");
                    if (new_segments.back() != "..")
                        // Can only cancel if the last one is not "..".
                        new_segments.pop_back();
                    else
                        // Otherwise keep it.
                        new_segments.emplace_back(std::move(s));
                }
            }
        }
        else
            // Keep it.
            new_segments.emplace_back(std::move(s));
    }
    std::swap(m_segments, new_segments);

    // Now can set the m_is_normalized flag.
    m_is_normalized = true;

    return *this;
}

Path &Path::make_absolute ()
{
    if (!is_absolute())
    {
        // Only need to do stuff if the path isn't already absolute.
        Path cwd(get_cwd());
        assert(cwd.is_absolute());
        assert(cwd.is_normalized());
        *this = cwd.joined_with(*this);
        assert(this->is_absolute());
        assert(this->is_normalized());
    }

    return *this;
}

Path &Path::truncate_to_common_path_with (Path other)
{
    if (this->is_absolute() != other.is_absolute())
        throw std::domain_error("Can only truncate_to_common_path_with for paths with matching is_absolute() values");

    this->normalize();
    other.normalize();

    std::size_t i;
    for (i = 0; i < std::min(this->m_segments.size(), other.m_segments.size()) && this->m_segments[i] == other.m_segments[i]; ++i)
    {
        // Do nothing
    }

    // Truncate this Path's segments down.
    m_segments.resize(i);

    return *this;
}

Path &Path::set_relative_to (Path base)
{
    if (this->is_absolute() != base.is_absolute())
        throw std::domain_error("Can only set_relative_to for paths with matching is_absolute() values; paths were \"" + this->as_string() + "\" and \"" + base.as_string() + "\"");

    this->normalize();
    base.normalize();

    Path common(common_path_with(base));

    std::vector<std::string> new_segments;
    // Add a ".." for each non-common segment in the base.
    for (std::size_t i = common.m_segments.size(); i < base.m_segments.size(); ++i)
    {
        if (base.m_segments[i] == "..")
        {
            assert(!base.is_absolute());
            throw std::domain_error("Can only set_relative_to for relative paths whose common path contains all leading \"..\" segments");
        }
        new_segments.emplace_back("..");
    }
    // Add each non-common segment in this path.
    for (std::size_t i = common.m_segments.size(); i < this->m_segments.size(); ++i)
    {
        if (this->m_segments[i] == "..")
        {
            assert(!this->is_absolute());
            throw std::domain_error("Can only set_relative_to for relative paths whose common path contains all leading \"..\" segments");
        }
        new_segments.emplace_back(std::move(this->m_segments[i]));
    }
    // Swap out with new_segments.
    std::swap(m_segments, new_segments);

    // The path is no longer absolute, but it is normalized.
    m_is_absolute = false;
    m_is_normalized = true;

    return *this;
}

Path &Path::join_with (Path const &other)
{
    if (other.is_absolute())
    {
        m_segments.clear();
        m_is_absolute = true;
    }

    for (auto &&s : other.m_segments)
        m_segments.emplace_back(s);

    // Because m_segments has changed, it is no longer normalized.
    m_is_normalized = false;
    normalize();

    return *this;
}

Path Path::get_cwd ()
{
    char cwd[FILENAME_MAX];
    if (!GetCurrentDir(cwd, sizeof(cwd)))
        throw std::runtime_error("Error while trying to retrieve the current working directory");
    cwd[sizeof(cwd) - 1] = '\0';

    Path retval = std::string(cwd);
    if (!retval.is_absolute())
        throw std::runtime_error("System did not produce an absolute path for current working directory");
    retval.normalize();
    return retval;
}

std::ostream &operator << (std::ostream &out, Path const &path)
{
    out << "Path(m_is_absolute = " << std::boolalpha << path.is_absolute() << ", m_is_normalized = " << std::boolalpha << path.is_normalized() << ", m_segments = ";
    {
        out << '[';
        for (std::size_t i = 0; i < path.segments().size(); ++i)
        {
            out << " \"" << path.segments()[i] << '"';
            if (i+1 < path.segments().size())
                out << ',';
        }
        out << " ]";
    }
    out <<')';
    return out;
}

} // end of namespace Barf
