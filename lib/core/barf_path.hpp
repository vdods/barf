// 2019.06.01 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_PATH_HPP_)
#define BARF_PATH_HPP_

#include "barf.hpp"

#include <exception>
#include <ostream>
#include <string>
#include <vector>

namespace Barf {

struct Path
{
    // Here is where the system-specific slash would be determined.
    static char constexpr slash = '/';

    Path () : m_is_absolute(false), m_is_normalized(false) { }
    Path (std::string const &s);
    Path (Path const &path)
        :   m_is_absolute(path.m_is_absolute)
        ,   m_is_normalized(path.m_is_normalized)
        ,   m_segments(path.m_segments)
    { }
    Path (Path &&path)
        :   m_is_absolute(path.m_is_absolute)
        ,   m_is_normalized(path.m_is_normalized)
        ,   m_segments(std::move(path.m_segments))
    { }

    Path &operator = (Path const &path)
    {
        m_is_absolute = path.m_is_absolute;
        m_is_normalized = path.m_is_normalized;
        m_segments = path.m_segments;
        return *this;
    }
    Path &operator = (Path &&path)
    {
        m_is_absolute = path.m_is_absolute;
        m_is_normalized = path.m_is_normalized;
        m_segments = std::move(path.m_segments);
        return *this;
    }

    bool is_absolute () const { return m_is_absolute; }
    bool is_normalized () const { return m_is_normalized; }
    std::vector<std::string> const &segments () const { return m_segments; }

    std::string as_string () const;
    Path as_absolute () const
    {
        Path retval(*this);
        retval.make_absolute();
        return retval;
    }
    Path normalized () const
    {
        Path retval(*this);
        retval.normalize();
        return retval;
    }
    // Compute the "greatest common divisor" of this path with other.
    // This and the other path must be absolute (or an exception will be thrown).
    Path common_path_with (Path const &other) const
    {
        Path retval(*this);
        retval.truncate_to_common_path_with(other);
        return retval;
    }
    Path relative_to (Path const &base) const
    {
        Path retval(*this);
        retval.set_relative_to(base);
        return retval;
    }
    Path joined_with (Path const &other) const
    {
        Path retval(*this);
        retval.join_with(other);
        return retval;
    }

    // Collapse "." and "..".  The return value is *this, so this method can be used to string together calls.
    Path &normalize ();
    // If this is a relative path, then this uses the current working directory to
    // make this Path absolute.
    Path &make_absolute ();
    // The return value is *this, so this method can be used to string together calls.
    Path &truncate_to_common_path_with (Path other);
    // This and other must either
    // - both be absolute paths, or
    // - both be relative paths whose normalized forms have in common all leading ".." segments,
    //   e.g. if base is "../../abc" and target is "../../pqr", then this is valid and renders "../pqr"
    //   e.g. if base is "../xyz" and target is "../../lmn", then this is invalid, because the common path
    //        is "..", and the non-common paths are "xyz" and "../lmn" respectively (so there is still a
    //        leading ".." segment).
    // The return value is *this, so this method can be used to string together calls.
    Path &set_relative_to (Path base);
    // Joins the given path to this one.  If other is an absolute path, then it entirely
    // replaces this one.
    Path &join_with (Path const &other);

    // Returns the current working directory as an absolute, normalized path.
    static Path get_cwd ();

private:

    bool                        m_is_absolute;
    mutable bool                m_is_normalized;
    std::vector<std::string>    m_segments;
};

std::ostream &operator << (std::ostream &out, Path const &path);

} // end of namespace Barf

#endif // !defined(BARF_PATH_HPP_)
