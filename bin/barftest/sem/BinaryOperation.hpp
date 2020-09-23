// 2016.08.09 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"

namespace cbz {
namespace sem {

enum class BinaryOperationType : uint8_t
{
    LOGICAL_OR = 0,
    LOGICAL_XOR,
    LOGICAL_AND,
    EQUALS,
    NOT_EQUALS,
    LESS_THAN,
    GREATER_THAN,
    LESS_OR_EQUAL,
    GREATER_OR_EQUAL,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    POW,
    TIN,
    SPLUNGE,
    WOOD,
    DINSDALE,

    __LOWEST__ = LOGICAL_OR,
    __HIGHEST__ = DINSDALE
};

std::string const &as_string (BinaryOperationType bin_op_type);
std::string const &as_symbol (BinaryOperationType bin_op_type);

struct BinaryOperation : public Base
{
    BinaryOperation (BinaryOperationType bin_op_type, nnup<Base> &&lhs, nnup<Base> &&rhs);
    BinaryOperation (FiRange const &firange, BinaryOperationType bin_op_type, nnup<Base> &&lhs, nnup<Base> &&rhs);
    virtual ~BinaryOperation () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::BINARY_OPERATION; }
    virtual bool equals (Base const &other) const override;
    virtual BinaryOperation *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;

    // For now, BinaryOperation is only defined on built-in types.
    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override;
    virtual Determinability generate_determinability (cgen::Context &context) const override;
    virtual llvm::Type *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_rvalue (cgen::Context &context) const override;

    BinaryOperationType const &bin_op_type () const { return m_bin_op_type; }
    Base const &lhs () const { return *m_lhs; }
    Base const &rhs () const { return *m_rhs; }

private:

    void validate_operand_types (cgen::Context &context, llvm::Type *lhs_type_concrete, llvm::Type *rhs_type_concrete) const;

    BinaryOperationType m_bin_op_type;
    nnup<Base> m_lhs;
    nnup<Base> m_rhs;
};

template <typename... Args_>
nnup<BinaryOperation> make_binary_operation (Args_&&... args)
{
    return make_nnup<BinaryOperation>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
