// 2016.08.14 - Victor Dods

#include "sem/Value.hpp"

#include "cbz/cgen/Context.hpp"
#include "llvm/IR/Constants.h"

namespace cbz {
namespace sem {

// nnup<BooleanValue> const TRUE = sem::make_boolean_value(FiRange::INVALID, true);
// nnup<BooleanValue> const FALSE = sem::make_boolean_value(FiRange::INVALID, false);

template <>
llvm::Type *BooleanValue::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Boolean::SINGLETON);
    return llvm::Type::getInt1Ty(context.llvm_context());
}

template <>
llvm::Value *BooleanValue::generate_rvalue (cgen::Context &context) const
{
    return m_value ? llvm::ConstantInt::getTrue(context.llvm_context()) : llvm::ConstantInt::getFalse(context.llvm_context());
}

template <>
llvm::Type *Sint8Value::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Sint8::SINGLETON);
    return llvm::Type::getInt8Ty(context.llvm_context());
}

template <>
llvm::Value *Sint8Value::generate_rvalue (cgen::Context &context) const
{
    return llvm::ConstantInt::getSigned(generate_rvalue_type(context), m_value);
}

template <>
llvm::Type *Sint16Value::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Sint16::SINGLETON);
    return llvm::Type::getInt16Ty(context.llvm_context());
}

template <>
llvm::Value *Sint16Value::generate_rvalue (cgen::Context &context) const
{
    return llvm::ConstantInt::getSigned(generate_rvalue_type(context), m_value);
}

template <>
llvm::Type *Sint32Value::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Sint32::SINGLETON);
    return llvm::Type::getInt32Ty(context.llvm_context());
}

template <>
llvm::Value *Sint32Value::generate_rvalue (cgen::Context &context) const
{
    return llvm::ConstantInt::getSigned(generate_rvalue_type(context), m_value);
}

template <>
llvm::Type *Sint64Value::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Sint64::SINGLETON);
    return llvm::Type::getInt64Ty(context.llvm_context());
}

template <>
llvm::Value *Sint64Value::generate_rvalue (cgen::Context &context) const
{
    return llvm::ConstantInt::getSigned(generate_rvalue_type(context), m_value);
}

template <>
llvm::Type *Uint8Value::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Uint8::SINGLETON);
    return llvm::Type::getInt8Ty(context.llvm_context());
}

template <>
llvm::Value *Uint8Value::generate_rvalue (cgen::Context &context) const
{
    return llvm::ConstantInt::getSigned(generate_rvalue_type(context), m_value);
}

template <>
llvm::Type *Uint16Value::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Uint16::SINGLETON);
    return llvm::Type::getInt16Ty(context.llvm_context());
}

template <>
llvm::Value *Uint16Value::generate_rvalue (cgen::Context &context) const
{
    return llvm::ConstantInt::getSigned(generate_rvalue_type(context), m_value);
}

template <>
llvm::Type *Uint32Value::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Uint32::SINGLETON);
    return llvm::Type::getInt32Ty(context.llvm_context());
}

template <>
llvm::Value *Uint32Value::generate_rvalue (cgen::Context &context) const
{
    return llvm::ConstantInt::getSigned(generate_rvalue_type(context), m_value);
}

template <>
llvm::Type *Uint64Value::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Uint64::SINGLETON);
    return llvm::Type::getInt64Ty(context.llvm_context());
}

template <>
llvm::Value *Uint64Value::generate_rvalue (cgen::Context &context) const
{
    return llvm::ConstantInt::getSigned(generate_rvalue_type(context), m_value);
}

template <>
llvm::Type *Float32Value::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Float32::SINGLETON);
    return llvm::Type::getFloatTy(context.llvm_context());
}

template <>
llvm::Value *Float32Value::generate_rvalue (cgen::Context &context) const
{
    return llvm::ConstantFP::get(generate_rvalue_type(context), m_value);
}

template <>
llvm::Type *Float64Value::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(Float64::SINGLETON);
    return llvm::Type::getDoubleTy(context.llvm_context());
}

template <>
llvm::Value *Float64Value::generate_rvalue (cgen::Context &context) const
{
    return llvm::ConstantFP::get(generate_rvalue_type(context), m_value);
}

llvm::Type *VoidValue::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(VoidType::SINGLETON);
    return context.ir_builder().getVoidTy();
}

llvm::Value *VoidValue::generate_rvalue (cgen::Context &context) const
{
    // nullptr indicates void.
    return nullptr;
}

llvm::Type *NullValue::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    return NullType::SINGLETON.generate_rvalue_type(context, abstract_type);
}

llvm::Value *NullValue::generate_rvalue (cgen::Context &context) const
{
    llvm::Type *concrete_type = NullType::SINGLETON.generate_rvalue_type(context);
    return llvm::Constant::getNullValue(concrete_type);
}

} // end namespace sem
} // end namespace cbz
