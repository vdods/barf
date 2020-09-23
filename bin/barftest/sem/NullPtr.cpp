// 2020.07.07 - Victor Dods

#include "sem/NullPtr.hpp"

#include "cbz/cgen/Context.hpp"

namespace cbz {
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

void NullPtr::resolve_symbols (cgen::Context &context)
{
    m_pointer_type->resolve_symbols(context);
}

Determinability NullPtr::generate_determinability (cgen::Context &context) const
{
    return m_pointer_type->generate_determinability(context);
}

llvm::PointerType *NullPtr::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    up<TypeBase> pointer_type_abstract;
    llvm::Type *pointer_type_concrete = m_pointer_type->generate_rvalue_type(context, &pointer_type_abstract);

    // m_pointer_type must be a PointerType
    if (pointer_type_abstract->type_enum__raw() != TypeEnum::POINTER_TYPE)
        throw TypeError(LVD_LOG_FMT("__nullptr__{} builtin expected a pointer type, but got " << pointer_type_abstract), firange());

    if (abstract_type != nullptr)
        *abstract_type = std::move(pointer_type_abstract);

    assert(llvm::isa<llvm::PointerType>(pointer_type_concrete));
    return llvm::cast<llvm::PointerType>(pointer_type_concrete);

//     if (abstract_type != nullptr)
//         *abstract_type = clone_of(m_pointer_type);
//
//
//     up<TypeBase> referent_abstract_type;
//     llvm::Type *referent_concrete_type = m_referent->generate_rvalue_type(context, &referent_abstract_type);
//
//     // Can't have a reference to a reference.
//     if (referent_abstract_type->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
//         throw TypeError(LVD_LOG_FMT("it is illegal to form a pointer to a reference type " << referent_abstract_type), firange());
//
//     return llvm::NullPtr::get(referent_concrete_type, 0); // 0 is default address space
}

llvm::Value *NullPtr::generate_rvalue (cgen::Context &context) const
{
    return llvm::Constant::getNullValue(generate_rvalue_type(context));
}

} // end namespace sem
} // end namespace cbz
