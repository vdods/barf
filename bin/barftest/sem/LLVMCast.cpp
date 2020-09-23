// 2020.07.07 - Victor Dods

#include "barftest/sem/LLVMCast.hpp"

#include "barftest/sem/Type.hpp"

namespace barftest {
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

} // end namespace sem
} // end namespace barftest
