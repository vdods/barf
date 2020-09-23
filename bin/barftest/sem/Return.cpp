// 2016.08.10 - Victor Dods

#include "barftest/sem/Return.hpp"

namespace barftest {
namespace sem {

Return::Return (FiRange const &firange, nnup<Base> &&return_expression)
:   Base(firange)
,   m_return_expression(std::move(return_expression))
{ }

bool Return::equals (Base const &other_) const
{
    Return const &other = dynamic_cast<Return const &>(other_);
    return are_equal(m_return_expression, other.m_return_expression);
}

Return *Return::cloned () const
{
    if (m_return_expression != nullptr)
        return new Return(firange(), clone_of(m_return_expression));
    else
        return new Return(firange());
}

void Return::print (Log &out) const
{
    out << "Return(" << firange();
    if (m_return_expression != nullptr)
        out << IndentGuard() << '\n' << m_return_expression << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace barftest
