// 2019.06.06 - Victor Dods

#include "sem/ElementAccess.hpp"

#include "cbz/cgen/Context.hpp"
#include "Exception.hpp"
#include "sem/ReferenceType.hpp"
#include "sem/SymbolSpecifier.hpp"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Casting.h"

namespace cbz {
namespace sem {

bool ElementAccess::equals (Base const &other_) const
{
    ElementAccess const &other = dynamic_cast<ElementAccess const &>(other_);
    return are_equal(m_referent, other.m_referent) && are_equal(m_element_key, other.m_element_key);
}

ElementAccess *ElementAccess::cloned () const
{
    return new ElementAccess(firange(), clone_of(m_referent), clone_of(m_element_key));
}

void ElementAccess::print (Log &out) const
{
    out << "ElementAccess(" << firange() << '\n';
    out << IndentGuard()
        << m_referent << '[' << m_element_key << "]\n";
    out << ')';
}

void ElementAccess::resolve_symbols (cgen::Context &context)
{
    m_referent->resolve_symbols(context);
    m_element_key->resolve_symbols(context);
}

Determinability ElementAccess::generate_determinability (cgen::Context &context) const
{
    auto r = m_referent->generate_determinability(context);
    auto k = m_element_key->generate_determinability(context);
    // All must be COMPILETIME in order for this expression to be COMPILETIME
    if (r == Determinability::COMPILETIME && k == Determinability::COMPILETIME)
        return Determinability::COMPILETIME;
    else
        return Determinability::RUNTIME;
}

llvm::Type *ElementAccess::generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    // Unfortunately this is problematic in the case of tuples because m_element_key may not be
    // known at compile time, and therefore the type of the element may not be known.  The way
    // around this would be to store a symbol table for each tuple, or use uniformly typed arrays
    // (same type for all elements).

    // TODO: Implement other cases where the type is known at compile time regardless of
    // the index (e.g. vectors and arrays)

    // The type is well-defined If and only if m_element_key is constant.
    llvm::Value *element_key_value = m_element_key->generate_rvalue(context);
    if (!llvm::isa<llvm::Constant>(element_key_value))
        throw ProgrammerError("in forming lvalue type, element access key must be a compile-time constant (and must be an Sint64)", m_element_key->firange());
    if (!llvm::isa<llvm::ConstantInt>(element_key_value))
        throw ProgrammerError("in forming lvalue type, element access key must be a compile-time constant Sint64", m_element_key->firange());

    llvm::ConstantInt *element_key_constant = static_cast<llvm::ConstantInt*>(element_key_value);
    if (element_key_constant->getType() != llvm::Type::getInt64Ty(context.llvm_context()))
        throw ProgrammerError("in forming lvalue type, element access key must be a compile-time constant Sint64 (not Boolean)", m_element_key->firange());

    up<TypeBase> referent_abstract_type;
    m_referent->generate_lvalue_type(context, &referent_abstract_type);
    assert(referent_abstract_type != nullptr);

    TypeBase const *referent_abstract_type_temp = referent_abstract_type.get();

    auto reference_type_abstract = dynamic_cast<ReferenceType const *>(referent_abstract_type_temp);
    if (reference_type_abstract != nullptr)
        referent_abstract_type_temp = &reference_type_abstract->referent();

    auto referent_type_aggregate = dynamic_cast<TypeAggregate const *>(referent_abstract_type_temp);
    if (referent_type_aggregate == nullptr)
        throw ProgrammerError(LVD_LOG_FMT("in forming lvalue type, referent has type " << *referent_abstract_type << ", which is not indexable"), m_referent->firange());

    int64_t key = element_key_constant->getSExtValue(); // SExtValue = sign extended value
    if (key < 0)
        throw ProgrammerError("in forming lvalue type, negative indexing not supported yet", m_element_key->firange());
    if (uint64_t(key) >= referent_type_aggregate->length())
        throw ProgrammerError(LVD_LOG_FMT("in forming lvalue type, index out of bounds (expected it to be less than " << referent_type_aggregate->length() << ')'), m_element_key->firange());
    return referent_type_aggregate->element(uint64_t(key)).generate_lvalue_type(context, abstract_type);
}

llvm::Value *ElementAccess::generate_lvalue (cgen::Context &context) const
{
    // NOTE TEMPORARY CONSTRAINT: The type is well-defined If and only if m_element_key is constant.
    llvm::Value *element_key_value = m_element_key->generate_rvalue(context);
    if (!llvm::isa<llvm::Constant>(element_key_value))
        throw ProgrammerError("in forming lvalue, element access key must be a compile-time constant (and must be an Sint64)", m_element_key->firange());
    if (!llvm::isa<llvm::ConstantInt>(element_key_value))
        throw ProgrammerError("in forming lvalue, element access key must be a compile-time constant Sint64", m_element_key->firange());

    llvm::ConstantInt *element_key_constant = static_cast<llvm::ConstantInt*>(element_key_value);
    if (element_key_constant->getType() != llvm::Type::getInt64Ty(context.llvm_context()))
        throw ProgrammerError("in forming lvalue, element access key must be a compile-time constant Sint64 (not Boolean)", m_element_key->firange());

    int64_t key = element_key_constant->getSExtValue(); // SExtValue = sign extended value
    // TODO: Handle the integer truncation 64 -> 32 bit
    if (key < 0)
        throw ProgrammerError("in forming lvalue, negative index access is not yet supported", firange());
    if (key >= (int64_t(1) << 32))
        throw ProgrammerError("in forming lvalue, element access beyond a 32 bit index is not yet supported", firange());

    llvm::Value *referent_value = m_referent->generate_lvalue(context);
    // An L-value has to correspond to a memory location.  The particulars here are that the L-value will be
    // be a pointer.  It must be a pointer to an aggregate type in order for the GEP to mean anything.
    assert(referent_value->getType()->isPointerTy());
    assert(static_cast<llvm::PointerType*>(referent_value->getType())->getElementType()->isAggregateType());
    // TODO: Have to handle VectorType
    auto llvm_int32 = llvm::Type::getInt32Ty(context.llvm_context());
    return context.ir_builder().CreateGEP(
        referent_value,
        llvm::ArrayRef<llvm::Value*>(
            std::vector<llvm::Value*>{
                llvm::ConstantInt::get(llvm_int32, 0),
                llvm::ConstantInt::get(llvm_int32, key)
            }
        ),
        "GEPtmp"
    );
}

llvm::Type *ElementAccess::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    // Unfortunately this is problematic in the case of tuples because m_element_key may not be
    // known at compile time, and therefore the type of the element may not be known.  The way
    // around this would be to store a symbol table for each tuple, or use uniformly typed arrays
    // (same type for all elements).

    // TODO: Implement other cases where the type is known at compile time regardless of
    // the index (e.g. vectors and arrays)

    // The type is well-defined If and only if m_element_key is constant.
    llvm::Value *element_key_value = m_element_key->generate_rvalue(context);
    if (!llvm::isa<llvm::Constant>(element_key_value))
        throw ProgrammerError("in forming rvalue type, element access key must be a compile-time constant (and must be an Sint64)", m_element_key->firange());
    if (!llvm::isa<llvm::ConstantInt>(element_key_value))
        throw ProgrammerError("in forming rvalue type, element access key must be a compile-time constant Sint64", m_element_key->firange());

    llvm::ConstantInt *element_key_constant = static_cast<llvm::ConstantInt*>(element_key_value);
    if (element_key_constant->getType() != llvm::Type::getInt64Ty(context.llvm_context()))
        throw ProgrammerError("in forming rvalue type, element access key must be a compile-time constant Sint64 (not Boolean)", m_element_key->firange());

    up<TypeBase> referent_abstract_type;
    m_referent->generate_rvalue_type(context, &referent_abstract_type);
    assert(referent_abstract_type != nullptr);

    TypeAggregate *referent_type_aggregate = dynamic_cast<TypeAggregate *>(referent_abstract_type.get());
    if (referent_type_aggregate == nullptr)
        throw ProgrammerError(LVD_LOG_FMT("in forming rvalue type, referent has type " << *referent_abstract_type << ", which is not indexable"), m_referent->firange());

    int64_t key = element_key_constant->getSExtValue(); // SExtValue = sign extended value
    if (key < 0)
        throw ProgrammerError("in forming rvalue type, negative indexing not supported yet", m_element_key->firange());
    if (uint64_t(key) >= referent_type_aggregate->length())
        throw ProgrammerError(LVD_LOG_FMT("in forming rvalue type, index out of bounds (expected it to be less than " << referent_type_aggregate->length() << ')'), m_element_key->firange());
    return referent_type_aggregate->element(uint64_t(key)).generate_rvalue_type(context, abstract_type);
}

llvm::Value *ElementAccess::generate_rvalue (cgen::Context &context) const
{
    // NOTE TEMPORARY CONSTRAINT: The type is well-defined If and only if m_element_key is constant.
    llvm::Value *element_key_value = m_element_key->generate_rvalue(context);
    if (!llvm::isa<llvm::Constant>(element_key_value))
        throw ProgrammerError("in forming rvalue, element access key must be a compile-time constant (and must be an Sint64)", m_element_key->firange());
    if (!llvm::isa<llvm::ConstantInt>(element_key_value))
        throw ProgrammerError("in forming rvalue, element access key must be a compile-time constant Sint64", m_element_key->firange());

    llvm::ConstantInt *element_key_constant = static_cast<llvm::ConstantInt*>(element_key_value);
    if (element_key_constant->getType() != llvm::Type::getInt64Ty(context.llvm_context()))
        throw ProgrammerError("in forming rvalue, element access key must be a compile-time constant Sint64 (not Boolean)", m_element_key->firange());

    int64_t key = element_key_constant->getSExtValue(); // SExtValue = sign extended value
    // TODO: Handle the integer truncation 64 -> 32 bit
    if (key < 0)
        throw ProgrammerError("in forming rvalue, negative index access is not yet supported", firange());

    llvm::Value *referent_value = m_referent->generate_rvalue(context);
    assert(referent_value->getType()->isAggregateType());
    // TODO: Have to handle VectorType
    return context.ir_builder().CreateExtractValue(referent_value, std::vector<unsigned>{unsigned(key)});
}

} // end namespace sem
} // end namespace cbz
