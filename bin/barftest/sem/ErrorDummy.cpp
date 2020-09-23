// 2018.08.30 - Victor Dods

#include "barftest/sem/ErrorDummy.hpp"

namespace barftest {
namespace sem {

ErrorDummy *ErrorDummy::cloned () const
{
    return new ErrorDummy(firange());
}

void ErrorDummy::print (Log &out) const
{
    out << "ErrorDummy(" << firange() << ')';
}

} // end namespace sem
} // end namespace barftest
