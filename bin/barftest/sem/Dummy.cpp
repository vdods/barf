// 2019.04.26 - Victor Dods

#include "sem/Dummy.hpp"

namespace cbz {
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
} // end namespace cbz
