// 2020.06.03 - Victor Dods

#include "sem/ReferenceType.hpp"

#include "cbz/cgen/Context.hpp"

namespace cbz {
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

void ReferenceType::resolve_symbols (cgen::Context &context)
{
    if (m_referent != nullptr)
        m_referent->resolve_symbols(context);
}

Determinability ReferenceType::generate_determinability (cgen::Context &context) const
{
    return m_referent->generate_determinability(context);
}

llvm::PointerType *ReferenceType::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(this);

    up<TypeBase> referent_abstract_type;
    llvm::Type *referent_concrete_type = m_referent->generate_rvalue_type(context, &referent_abstract_type);

    // Can't have a reference to a reference.
    if (referent_abstract_type->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
        throw TypeError(LVD_LOG_FMT("it is illegal to form a reference to a reference type " << referent_abstract_type), firange());

    // Concrete type of a reference type is really the corresponding pointer type.
    return llvm::PointerType::get(referent_concrete_type, 0); // 0 is default address space
}

} // end namespace sem
} // end namespace cbz
