// 2020.06.03 - Victor Dods

#include "barftest/sem/ReferenceType.hpp"

namespace barftest {
namespace sem {

bool ReferenceType::equals (Base const &other_) const
{
    auto const &other = dynamic_cast<ReferenceType const &>(other_);
    return are_equal(m_referent, other.m_referent);
}

ReferenceType *ReferenceType::cloned () const
{
    return new ReferenceType(firange(), clone_of(m_referent));
}

void ReferenceType::print (Log &out) const
{
    out << "ReferenceType(" << firange();
    if (m_referent != nullptr)
        out << IndentGuard() << '\n' << m_referent << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace barftest
