// 2016.08.16 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include <exception>
#include "barftest/FiRange.hpp"
#include <lvd/abort.hpp>

namespace barftest {

// The baseclass for all exceptions in barftest
struct Error : public std::runtime_error
{
    Error (std::string const &what_arg, FiRange const &firange = FiRange::INVALID)
        :   std::runtime_error((firange.is_valid() ? firange.as_string()+": error: " : std::string("error: ")) + what_arg)
        ,   m_firange(firange)
    { }
    virtual ~Error () = 0;

    FiRange const &firange () const { return m_firange; }

private:

    FiRange m_firange;
};

// This exception is used to indicate an error in the use of barftest, and
// is part of the correct operation of barftest.  For example, compile-time
// and run-time errors will be indicated via subclasses of this class.
struct ProgrammerError : public Error
{
    ProgrammerError (std::string const &what_arg, FiRange const &firange = FiRange::INVALID)
        :   Error(what_arg, firange)
    { }
    virtual ~ProgrammerError ();
};

struct TypeError : public Error
{
    TypeError (std::string const &what_arg, FiRange const &firange = FiRange::INVALID)
        :   Error(what_arg, firange)
    { }
    virtual ~TypeError ();
};

struct WellFormednessError : public Error
{
    WellFormednessError (std::string const &what_arg, FiRange const &firange = FiRange::INVALID)
        :   Error(what_arg, firange)
    { }
    virtual ~WellFormednessError ();
};

} // end namespace barftest
