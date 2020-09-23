// 2016.08.09 - Victor Dods

#include "barftest/sem/UnaryOperation.hpp"

#include "barftest/Exception.hpp"
#include "barftest/sem/PointerType.hpp"
#include "barftest/sem/ReferenceType.hpp"
#include "barftest/sem/SymbolSpecifier.hpp"

namespace barftest {
namespace sem {

std::string const &as_string (UnaryOperationType un_op_type)
{
    static std::string const STRING_TABLE[uint32_t(UnaryOperationType::__HIGHEST__) - uint32_t(UnaryOperationType::__LOWEST__) + 1] = {
        "LOGICAL_NOT",
        "NEGATE",
        "AT_SYMBOL",
        "HASH_SYMBOL",
        "CT",
        "RT",
        "LOCAL",
        "GLOBAL",
        "EXTERNAL",
        "INTERNAL",
        "PRIVATE",
    };
    return STRING_TABLE[uint32_t(un_op_type)];
}

bool UnaryOperation::equals (Base const &other_) const
{
    UnaryOperation const &other = dynamic_cast<UnaryOperation const &>(other_);
    return m_un_op_type == other.m_un_op_type && are_equal(m_operand, other.m_operand);
}

UnaryOperation *UnaryOperation::cloned () const
{
    return new UnaryOperation(firange(), m_un_op_type, clone_of(m_operand));
}

void UnaryOperation::print (Log &out) const
{
    out << "UnaryOperation(" << firange() << '\n';
    out << IndentGuard()
        << m_un_op_type << ",\n"
        << m_operand << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace barftest
