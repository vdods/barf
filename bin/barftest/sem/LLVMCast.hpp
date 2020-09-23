// 2020.07.07 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"

namespace cbz {
namespace sem {

enum class LLVMCastInstruction : uint8_t
{
    // Basic cast instructions
    TRUNC = 0,
    ZEXT,
    SEXT,
    FPTRUNC,
    FPEXT,
    FP_TO_UI,
    FP_TO_SI,
    UI_TO_FP,
    SI_TO_FP,
    PTR_TO_INT,
    INT_TO_PTR,
    BITCAST,
    // ADDRSPACECAST is not currently supported (this may or may not be easy)

    // Advanced cast instructions provided by llvm::IRBuilder C++ API
    ZEXT_OR_TRUNC,
    SEXT_OR_TRUNC,
    ZEXT_OR_BITCAST,
    SEXT_OR_BITCAST,
    TRUNC_OR_BITCAST,
    PTR_CAST,
    // PTR_BITCAST_OR_ADDRSPACECAST is not currently supported
    UINT_CAST,
    SINT_CAST,
    BIT_OR_PTR_CAST,
    FP_CAST,

    __LOWEST__ = TRUNC,
    __HIGHEST__ = FP_CAST,
};

std::string const &as_string (LLVMCastInstruction instruction);
LLVMCastInstruction llvm_cast_instruction_from_string (std::string const &s, FiRange const &firange = FiRange::INVALID);

inline std::ostream &operator << (std::ostream &out, LLVMCastInstruction instruction)
{
    return out << as_string(instruction);
}

// This really should only occur as a temporary between the Scanner and Parser.
struct LLVMCastKeyword : public Base
{
    LLVMCastKeyword (FiRange const &firange, LLVMCastInstruction instruction) : Base(firange), m_instruction(instruction) { }
    virtual ~LLVMCastKeyword () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::LLVM_CAST_KEYWORD; }
    virtual bool equals (Base const &other) const override;
    virtual LLVMCastKeyword *cloned () const override;
    virtual void print (Log &out) const override;
//     virtual void resolve_symbols (cgen::Context &context) override;

//     virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override { return ExpressionKind::VALUE; }
//     virtual Determinability generate_determinability (cgen::Context &context) const override;
//     // TODO: implement these
// //     virtual llvm::Type *generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
// //     virtual llvm::Value *generate_lvalue (cgen::Context &context) const override;
//     virtual llvm::Type *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
//     virtual llvm::Value *generate_rvalue (cgen::Context &context) const override;

    LLVMCastInstruction instruction () const { return m_instruction; }
//     Base const &source_value () const { return *m_source_value; }
//     TypeBase const &target_type () const { return *m_target_type; }

private:

    LLVMCastInstruction m_instruction;
//     nnup<Base> m_source_value;
//     nnup<TypeBase> m_target_type;
};

struct LLVMCast : public Base
{
    LLVMCast (FiRange const &firange, LLVMCastInstruction instruction, nnup<Base> &&source_value, nnup<TypeBase> &&target_type) : Base(firange), m_instruction(instruction), m_source_value(std::move(source_value)), m_target_type(std::move(target_type)) { }
    virtual ~LLVMCast () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::LLVM_CAST; }
    virtual bool equals (Base const &other) const override;
    virtual LLVMCast *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;

    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override { return ExpressionKind::VALUE; }
    virtual Determinability generate_determinability (cgen::Context &context) const override;
    // TODO: implement these
//     virtual llvm::Type *generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
//     virtual llvm::Value *generate_lvalue (cgen::Context &context) const override;
    virtual llvm::Type *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_rvalue (cgen::Context &context) const override;

    LLVMCastInstruction instruction () const { return m_instruction; }
    Base const &source_value () const { return *m_source_value; }
    TypeBase const &target_type () const { return *m_target_type; }

private:

    LLVMCastInstruction m_instruction;
    nnup<Base> m_source_value;
    nnup<TypeBase> m_target_type;
};

template <typename... Args_>
nnup<LLVMCastKeyword> make_llvm_cast_keyword (Args_&&... args)
{
    return make_nnup<LLVMCastKeyword>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<LLVMCast> make_llvm_cast (Args_&&... args)
{
    return make_nnup<LLVMCast>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
