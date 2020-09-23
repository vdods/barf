// 2020.07.07 - Victor Dods

#include "sem/LLVMCast.hpp"

#include "cbz/cgen/Context.hpp"

namespace cbz {
namespace sem {

static std::string const LLVM_CAST_INSTRUCTION_STRING_TABLE[uint32_t(LLVMCastInstruction::__HIGHEST__) - uint32_t(LLVMCastInstruction::__LOWEST__) + 1] = {
    "trunc",
    "zext",
    "sext",
    "fptrunc",
    "fpext",
    "fptoui",
    "fptosi",
    "uitofp",
    "sitofp",
    "ptrtoint",
    "inttoptr",
    "bitcast",
    // Advanced cast instructions provided by llvm::IRBuilder C++ API
    "zextortrunc",
    "sextortrunc",
    "zextorbitcast",
    "sextorbitcast",
    "truncorbitcast",
    "ptrcast",
    "uintcast",
    "sintcast",
    "bitorptrcast",
    "fpcast",
};

std::string const &as_string (LLVMCastInstruction instruction)
{
    return LLVM_CAST_INSTRUCTION_STRING_TABLE[uint32_t(instruction)-uint32_t(LLVMCastInstruction::__LOWEST__)];
}

LLVMCastInstruction llvm_cast_instruction_from_string (std::string const &s, FiRange const &firange)
{
    for (uint32_t i = 0; i < uint32_t(LLVMCastInstruction::__HIGHEST__)-uint32_t(LLVMCastInstruction::__LOWEST__)+1; ++i)
        if (LLVM_CAST_INSTRUCTION_STRING_TABLE[i] == s)
            return LLVMCastInstruction(i+uint32_t(LLVMCastInstruction::__LOWEST__));
    throw ProgrammerError(LVD_FMT("\"" << s << "\" is not a recognized llvm cast instruction"), firange);
}

//
// LLVMCastKeyword
//

bool LLVMCastKeyword::equals (Base const &other_) const
{
    auto const &other = dynamic_cast<LLVMCastKeyword const &>(other_);
    return m_instruction == other.m_instruction;
}

LLVMCastKeyword *LLVMCastKeyword::cloned () const
{
    return new LLVMCastKeyword(firange(), m_instruction);
}

void LLVMCastKeyword::print (Log &out) const
{
    out << "LLVMCastKeyword(" << firange();
    out << IndentGuard() << '\n'
        << "instruction : " << m_instruction << '\n';
    out << ')';
}

//
// LLVMCast
//

bool LLVMCast::equals (Base const &other_) const
{
    auto const &other = dynamic_cast<LLVMCast const &>(other_);
    return m_instruction == other.m_instruction && are_equal(m_source_value, other.m_source_value) && are_equal(m_target_type, other.m_target_type);
}

LLVMCast *LLVMCast::cloned () const
{
    return new LLVMCast(firange(), m_instruction, clone_of(m_source_value), clone_of(m_target_type));
}

void LLVMCast::print (Log &out) const
{
    out << "LLVMCast(" << firange();
    out << IndentGuard() << '\n'
        << "instruction : " << m_instruction << '\n'
        << "source_value: " << m_source_value << '\n'
        << "target_type : " << m_target_type << '\n';
    out << ')';
}

void LLVMCast::resolve_symbols (cgen::Context &context)
{
    m_source_value->resolve_symbols(context);
    m_target_type->resolve_symbols(context);
}

Determinability LLVMCast::generate_determinability (cgen::Context &context) const
{
    auto s = m_source_value->generate_determinability(context);
    auto t = m_target_type->generate_determinability(context);
    // All must be COMPILETIME in order for this expression to be COMPILETIME
    if (s == Determinability::COMPILETIME && t == Determinability::COMPILETIME)
        return Determinability::COMPILETIME;
    else
        return Determinability::RUNTIME;
}

// llvm::Type *LLVMCast::generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
// {
// }
//
// llvm::Value *LLVMCast::generate_lvalue (cgen::Context &context) const
// {
// }

llvm::Type *LLVMCast::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    // TODO: type-check the arguments. (LLVM will check this presumably,
    // but if the check fails, it will crash, which is not rad for the user).
    return m_target_type->generate_rvalue_type(context, abstract_type);
}

llvm::Value *LLVMCast::generate_rvalue (cgen::Context &context) const
{
    // TODO: type-check the arguments.

    llvm::Value *source_value_concrete = m_source_value->generate_rvalue(context);
    llvm::Type *target_type_concrete = m_target_type->generate_rvalue_type(context);
    switch (m_instruction)
    {
        case LLVMCastInstruction::TRUNC: return context.ir_builder().CreateTrunc(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::ZEXT: return context.ir_builder().CreateZExt(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::SEXT: return context.ir_builder().CreateSExt(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::FPTRUNC: return context.ir_builder().CreateFPTrunc(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::FPEXT: return context.ir_builder().CreateFPExt(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::FP_TO_UI: return context.ir_builder().CreateFPToUI(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::FP_TO_SI: return context.ir_builder().CreateFPToSI(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::UI_TO_FP: return context.ir_builder().CreateUIToFP(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::SI_TO_FP: return context.ir_builder().CreateSIToFP(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::PTR_TO_INT: return context.ir_builder().CreatePtrToInt(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::INT_TO_PTR: return context.ir_builder().CreateIntToPtr(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::BITCAST: return context.ir_builder().CreateBitCast(source_value_concrete, target_type_concrete, "casttmp");

        // Advanced cast instructions provided by llvm::IRBuilder C++ API
        case LLVMCastInstruction::ZEXT_OR_TRUNC: return context.ir_builder().CreateZExtOrTrunc(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::SEXT_OR_TRUNC: return context.ir_builder().CreateSExtOrTrunc(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::ZEXT_OR_BITCAST: return context.ir_builder().CreateZExtOrBitCast(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::SEXT_OR_BITCAST: return context.ir_builder().CreateSExtOrBitCast(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::TRUNC_OR_BITCAST: return context.ir_builder().CreateTruncOrBitCast(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::PTR_CAST: return context.ir_builder().CreatePointerCast(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::UINT_CAST: return context.ir_builder().CreateIntCast(source_value_concrete, target_type_concrete, false, "casttmp"); // boolean value indicates "is signed"
        case LLVMCastInstruction::SINT_CAST: return context.ir_builder().CreateIntCast(source_value_concrete, target_type_concrete, true, "casttmp"); // boolean value indicates "is signed"
        case LLVMCastInstruction::BIT_OR_PTR_CAST: return context.ir_builder().CreateBitOrPointerCast(source_value_concrete, target_type_concrete, "casttmp");
        case LLVMCastInstruction::FP_CAST: return context.ir_builder().CreateFPCast(source_value_concrete, target_type_concrete, "casttmp");

        default: LVD_ABORT_WITH_FIRANGE("invalid LLVMCastInstruction", firange());
    }
}

} // end namespace sem
} // end namespace cbz
