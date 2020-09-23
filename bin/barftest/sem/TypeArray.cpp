// 2019.11.06 - Victor Dods

#include "sem/TypeArray.hpp"

#include "llvm/IR/DerivedTypes.h"

namespace cbz {
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

void TypeArray::resolve_symbols (cgen::Context &context)
{
    m_element_type->resolve_symbols(context);
}

llvm::Type *TypeArray::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(this);
    return llvm::ArrayType::get(m_element_type->generate_rvalue_type(context), m_element_count);
}

} // end namespace sem
} // end namespace cbz
