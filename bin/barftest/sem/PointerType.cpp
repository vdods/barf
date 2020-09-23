// 2020.06.03 - Victor Dods

#include "sem/PointerType.hpp"

#include "cbz/cgen/Context.hpp"

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

void PointerType::resolve_symbols (cgen::Context &context)
{
    if (m_referent != nullptr)
        m_referent->resolve_symbols(context);
}

Determinability PointerType::generate_determinability (cgen::Context &context) const
{
    return m_referent->generate_determinability(context);
}

llvm::PointerType *PointerType::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(this);

    up<TypeBase> referent_abstract_type;
    llvm::Type *referent_concrete_type = m_referent->generate_rvalue_type(context, &referent_abstract_type);

    // Can't have a pointer to a reference.
    if (referent_abstract_type->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
        throw TypeError(LVD_LOG_FMT("it is illegal to form a pointer to a reference type " << referent_abstract_type), firange());

    return llvm::PointerType::get(referent_concrete_type, 0); // 0 is default address space
}

} // end namespace sem
} // end namespace cbz
