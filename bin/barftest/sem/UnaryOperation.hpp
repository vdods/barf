// 2016.08.09 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"

namespace cbz {
namespace sem {

// TODO: Rename to UnaryOperationEnum
enum class UnaryOperationType : uint8_t
{
    LOGICAL_NOT = 0,
    NEGATE,
    AT_SYMBOL,
    HASH_SYMBOL,
    CT,
    RT,
    LOCAL,
    GLOBAL,
    EXTERNAL,
    INTERNAL,
    PRIVATE,

    __LOWEST__ = LOGICAL_NOT,
    __HIGHEST__ = PRIVATE
};

std::string const &as_string (UnaryOperationType un_op_type);

inline std::ostream &operator << (std::ostream &out, UnaryOperationType un_op_type)
{
    return out << as_string(un_op_type);
}

struct UnaryOperation : public Base
{
    // NOTE: This is commented out for now because really a UnaryOperation firange should
    // encompass the operator as well, not just the operand (otherwise that's confusing
    // in an error message).
//     UnaryOperation (UnaryOperationType un_op_type, nnup<Base> &&operand) : UnaryOperation(firange_of(operand), un_op_type, std::move(operand)) { }
    UnaryOperation (FiRange const &firange, UnaryOperationType un_op_type, nnup<Base> &&operand)
    :   Base(firange)
    ,   m_un_op_type(un_op_type)
    ,   m_operand(std::move(operand))
    { }
    virtual ~UnaryOperation () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::UNARY_OPERATION; }
    virtual bool equals (Base const &other) const override;
    virtual UnaryOperation *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;

    // For now, UnaryOperation is only defined on built-in types.
    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override;
    virtual Determinability generate_determinability (cgen::Context &context) const override;
    virtual llvm::Type *generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_lvalue (cgen::Context &context) const override;
    virtual llvm::Type *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_rvalue (cgen::Context &context) const override;
    virtual nnup<SymbolSpecifier> generate_svalue (cgen::Context &context) const override;

    UnaryOperationType un_op_type () const { return m_un_op_type; }
    Base const &operand () const { return *m_operand; }

private:

    void validate_operand_type (cgen::Context &context, llvm::Type *operand_type, TypeBase const &operand_abstract_type) const;

    UnaryOperationType m_un_op_type;
    nnup<Base> m_operand;
};

template <typename... Args_>
nnup<UnaryOperation> make_unary_operation (Args_&&... args)
{
    return make_nnup<UnaryOperation>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
