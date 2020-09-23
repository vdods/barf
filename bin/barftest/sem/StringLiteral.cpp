// 2019.11.12 - Victor Dods

#include "barftest/sem/StringLiteral.hpp"

#include "barftest/literal.hpp"
#include "barftest/sem/TypeArray.hpp"

namespace barftest {
namespace sem {

bool StringLiteral::equals (Base const &other_) const
{
    StringLiteral const &other = dynamic_cast<StringLiteral const &>(other_);
    return m_text == other.m_text;
}

StringLiteral *StringLiteral::cloned () const
{
    return new StringLiteral(firange(), m_text);
}

void StringLiteral::print (Log &out) const
{
    out << "StringLiteral(" << firange() << '\n';
    out << IndentGuard()
        << string_literal_of(m_text) << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace barftest
