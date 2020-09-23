// 2016.08.09 - Victor Dods

#include "barftest/sem/Assignment.hpp"

#include "barftest/Exception.hpp"
#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/Type.hpp"

namespace barftest {
namespace sem {

bool Assignment::equals (Base const &other_) const
{
    Assignment const &other = dynamic_cast<Assignment const &>(other_);
    return are_equal(m_target, other.m_target) && are_equal(m_content, other.m_content);
}

void Assignment::print (Log &out) const
{
    out << "Assignment(" << firange() << '\n';
    out << IndentGuard()
        << m_target << " = " << m_content << '\n';
    out << ')';
}

Assignment *Assignment::cloned () const
{
    return new Assignment(firange(), clone_of(m_target), clone_of(m_content));
}

} // end namespace sem
} // end namespace barftest
