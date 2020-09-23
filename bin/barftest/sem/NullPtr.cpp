// 2020.07.07 - Victor Dods

#include "barftest/sem/NullPtr.hpp"

#include "barftest/sem/PointerType.hpp"

namespace barftest {
namespace sem {

bool NullPtr::equals (Base const &other_) const
{
    auto const &other = dynamic_cast<NullPtr const &>(other_);
    return are_equal(m_pointer_type, other.m_pointer_type);
}

NullPtr *NullPtr::cloned () const
{
    return new NullPtr(firange(), clone_of(m_pointer_type));
}

void NullPtr::print (Log &out) const
{
    out << "NullPtr(" << firange();
    out << IndentGuard() << '\n' << m_pointer_type << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace barftest
