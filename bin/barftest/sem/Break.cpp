// 2016.08.10 - Victor Dods

#include "sem/Break.hpp"

#include <exception>

namespace cbz {
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
} // end namespace cbz
