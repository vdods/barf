// 2020.06.03 - Victor Dods

#include "sem/PointerType.hpp"

namespace cbz {
namespace sem {

bool PointerType::equals (Base const &other_) const
{
    auto const &other = dynamic_cast<PointerType const &>(other_);
    return are_equal(m_referent, other.m_referent);
}

PointerType *PointerType::cloned () const
{
    return new PointerType(firange(), clone_of(m_referent));
}

void PointerType::print (Log &out) const
{
    out << "PointerType(" << firange();
    if (m_referent != nullptr)
        out << IndentGuard() << '\n' << m_referent << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace cbz
