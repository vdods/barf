// 2019.06.06 - Victor Dods

#include "sem/ElementAccess.hpp"

#include "Exception.hpp"
#include "sem/ReferenceType.hpp"
#include "sem/SymbolSpecifier.hpp"

namespace cbz {
namespace sem {

bool ElementAccess::equals (Base const &other_) const
{
    ElementAccess const &other = dynamic_cast<ElementAccess const &>(other_);
    return are_equal(m_referent, other.m_referent) && are_equal(m_element_key, other.m_element_key);
}

ElementAccess *ElementAccess::cloned () const
{
    return new ElementAccess(firange(), clone_of(m_referent), clone_of(m_element_key));
}

void ElementAccess::print (Log &out) const
{
    out << "ElementAccess(" << firange() << '\n';
    out << IndentGuard()
        << m_referent << '[' << m_element_key << "]\n";
    out << ')';
}

} // end namespace sem
} // end namespace cbz
