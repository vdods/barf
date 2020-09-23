// 2019.04.26 - Victor Dods

#include "barftest/sem/Dummy.hpp"

namespace barftest {
namespace sem {

Dummy *Dummy::cloned () const
{
    return new Dummy(firange());
}

void Dummy::print (Log &out) const
{
    out << "Dummy(" << firange() << ')';
}

} // end namespace sem
} // end namespace barftest
