// 2016.09.18 - Victor Dods

#include "barftest/sem/TypeIdentifier.hpp"

#include "barftest/sem/Identifier.hpp"

namespace barftest {
namespace sem {

bool TypeIdentifier::equals (Base const &other_) const
{
    TypeIdentifier const &other = dynamic_cast<TypeIdentifier const &>(other_);
    return are_equal(this->m_id, other.m_id);
}

TypeIdentifier *TypeIdentifier::cloned () const
{
    return new TypeIdentifier(firange(), clone_of(m_id));
}

void TypeIdentifier::print (Log &out) const
{
    out << "TypeIdentifier(" << firange() << '\n';
    out << IndentGuard()
        << m_id << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace barftest
