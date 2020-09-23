// 2016.08.08 - Victor Dods

#include "barftest/sem/Conditional.hpp"

#include "barftest/Exception.hpp"

namespace barftest {
namespace sem {

Conditional::Conditional (nnup<Base> &&condition, nnup<Base> &&positive_element, up<Base> &&negative_element)
    : Base(firange_of(condition) + firange_of(positive_element) + firange_of(negative_element))
    , m_condition(std::move(condition))
    , m_positive_element(std::move(positive_element))
    , m_negative_element(std::move(negative_element))
{ }

Conditional::Conditional (FiRange const &firange, nnup<Base> &&condition, nnup<Base> &&positive_element, up<Base> &&negative_element)
    : Base(firange)
    , m_condition(std::move(condition))
    , m_positive_element(std::move(positive_element))
    , m_negative_element(std::move(negative_element))
{ }

bool Conditional::equals (Base const &other_) const
{
    Conditional const &other = dynamic_cast<Conditional const &>(other_);
    return type_enum__raw() == other.type_enum__raw() &&
           are_equal(m_condition, other.m_condition) &&
           are_equal(m_positive_element, other.m_positive_element) &&
           are_equal(m_negative_element, other.m_negative_element);
}

//
// ConditionalExpression
//

ConditionalExpression *ConditionalExpression::cloned () const
{
    return new ConditionalExpression(firange(), clone_of(condition()), clone_of(positive_element()), clone_of(negative_element()));
}

void ConditionalExpression::print (Log &out) const
{
    assert(has_negative_element());
    out << "ConditionalExpression(" << firange() << '\n';
    out << IndentGuard()
        << condition() << ",\n"
        << positive_element() << ",\n"
        << negative_element() << '\n';
    out << ')';
}

//
// ConditionalStatement
//

ConditionalStatement *ConditionalStatement::cloned () const
{
    return new ConditionalStatement(firange(), clone_of(condition()), clone_of(positive_element()), clone_of(negative_element_ptr()));
}

void ConditionalStatement::print (Log &out) const
{
    out << "ConditionalStatement(" << firange() << '\n';
    out << IndentGuard()
        << condition() << ",\n"
        << positive_element() << ",\n"
        << negative_element_ptr() << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace barftest
