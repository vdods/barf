// 2019.11.06 - Victor Dods

#include "barftest/sem/TypeArray.hpp"

namespace barftest {
namespace sem {

TypeArray::TypeArray (FiRange const &firange, nnup<TypeBase> &&element_type, uint64_t element_count)
:   TypeAggregate(firange)
,   m_element_type(std::move(element_type))
,   m_element_count(element_count)
{ }

bool TypeArray::equals (Base const &other_) const
{
    TypeArray const &other = dynamic_cast<TypeArray const &>(other_);
    return are_equal(m_element_type, other.m_element_type) && m_element_count == other.m_element_count;
}

TypeArray *TypeArray::cloned () const
{
    return new TypeArray(firange(), clone_of(m_element_type), m_element_count);
}

void TypeArray::print (Log &out) const
{
    out << "TypeArray(" << firange() << '\n';
    out << IndentGuard()
        << m_element_type << " x " << m_element_count << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace barftest
