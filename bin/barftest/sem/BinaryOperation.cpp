// 2016.08.09 - Victor Dods

#include "sem/BinaryOperation.hpp"

#include "Exception.hpp"

namespace cbz {
namespace sem {

std::string const &as_string (BinaryOperationType bin_op_type)
{
    static std::string const STRING_TABLE[uint32_t(BinaryOperationType::__HIGHEST__) - uint32_t(BinaryOperationType::__LOWEST__) + 1] = {
        "LOGICAL_OR",
        "LOGICAL_XOR",
        "LOGICAL_AND",
        "EQUALS",
        "NOT_EQUALS",
        "LESS_THAN",
        "GREATER_THAN",
        "LESS_OR_EQUAL",
        "GREATER_OR_EQUAL",
        "ADD",
        "SUB",
        "MUL",
        "DIV",
        "MOD",
        "POW",
        "TIN",
        "SPLUNGE",
        "WOOD",
        "DINSDALE",
    };
    return STRING_TABLE[uint32_t(bin_op_type)];
}

std::string const &as_symbol (BinaryOperationType bin_op_type)
{
    static std::string const STRING_TABLE[uint32_t(BinaryOperationType::__HIGHEST__) - uint32_t(BinaryOperationType::__LOWEST__) + 1] = {
        "or",
        "xor",
        "and",
        "==",
        "!=",
        "<",
        ">",
        "<=",
        ">=",
        "+",
        "-",
        "*",
        "/",
        "%",
        "^",
        "@@",
        "@",
        "@@@",
    };
    return STRING_TABLE[uint32_t(bin_op_type)];
}

BinaryOperation::BinaryOperation (BinaryOperationType bin_op_type, nnup<Base> &&lhs, nnup<Base> &&rhs)
    : Base(firange_of(lhs) + firange_of(rhs))
    , m_bin_op_type(bin_op_type)
    , m_lhs(std::move(lhs))
    , m_rhs(std::move(rhs))
{ }

BinaryOperation::BinaryOperation (FiRange const &firange, BinaryOperationType bin_op_type, nnup<Base> &&lhs, nnup<Base> &&rhs)
    : Base(firange)
    , m_bin_op_type(bin_op_type)
    , m_lhs(std::move(lhs))
    , m_rhs(std::move(rhs))
{ }

bool BinaryOperation::equals (Base const &other_) const
{
    BinaryOperation const &other = dynamic_cast<BinaryOperation const &>(other_);
    return m_bin_op_type == other.m_bin_op_type &&
           are_equal(m_lhs, other.m_lhs) &&
           are_equal(m_rhs, other.m_rhs);
}

BinaryOperation *BinaryOperation::cloned () const
{
    return new BinaryOperation(firange(), m_bin_op_type, clone_of(m_lhs), clone_of(m_rhs));
}

void BinaryOperation::print (Log &out) const
{
    out << "BinaryOperation(" << firange() << '\n';
    out << IndentGuard()
        << as_string(m_bin_op_type) << ",\n"
        << m_lhs << ",\n"
        << m_rhs << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace cbz
