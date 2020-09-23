// 2016.08.10 - Victor Dods

#include "barftest/sem/Break.hpp"

#include <exception>

namespace barftest {
namespace sem {

Break *Break::cloned () const
{
    return new Break(firange());
}

void Break::print (Log &out) const
{
    out << "Break(" << firange() << ')';
}

} // end namespace sem
} // end namespace barftest
