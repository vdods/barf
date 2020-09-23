// 2014.12.03 - Victor Dods

#pragma once

#include "Log.hpp"
#include <lvd/aliases.hpp>
#include <lvd/move_cast.hpp>
#include <lvd/not_null.hpp>
#include <ostream>
#include <sstream>

namespace cbz {

// Aliases for common pointer types and function.
using lvd::nn;          // gsl::not_null<...>
using lvd::up;          // std::unique_ptr<...>
using lvd::nnup;        // gsl::not_null<std::unique_ptr<...>>
using lvd::make_nnup;

} // end namespace cbz

namespace std {

// Overload for printing std::unique_ptr to std::ostream -- this overload HAS to be inside
// namespace std, otherwise the overload resolution goes totally nuts in a hard-to-debug way.
// Note that actually has to be within a `namespace std { ... }` block, and just qualifying
// `operator<<` with `std::` doesn't work.
template <typename T_, typename Deleter_>
ostream &operator << (ostream &out, unique_ptr<T_,Deleter_> const &p)
{
    if (p != nullptr)
        out << *p;
    else
        out << "nullptr";
    return out;
}

} // end namespace std
