// 2017.08.17 - Copyright Victor Dods - Licensed under Apache 2.0

#include "ast.hpp"
#include "util.hpp"

namespace Ast {

std::string const &AsString (Type type)
{
    static std::string const s_lookup_table[Type::COUNT_] = {
        "BAD_TOKEN",
        "CHAR_LITERAL",
        "ERROR_DUMMY",
        "IDENTIFIER",
        "INTEGER_LITERAL",
        "NUMERIC_LITERAL",
        "OPERATOR",
        "STRING_LITERAL",
    };
    return s_lookup_table[std::uint32_t(type) - std::uint32_t(Type::LOW_)];
}

void ErrorDummy::print (std::ostream &out, std::uint32_t indent_level) const
{
    out << std::string(4*indent_level, ' ');
    out << "ErrorDummy = " << ::StringLiteral(m_description) << '\n';
}

template <>
void BadToken::print (std::ostream &out, std::uint32_t indent_level) const
{
    out << std::string(4*indent_level, ' ');
    out << "BadToken = " << m_value << '\n';
}

template <>
void CharLiteral::print (std::ostream &out, std::uint32_t indent_level) const
{
    out << std::string(4*indent_level, ' ');
    out << "CharLiteral = " << ::CharLiteral(m_value) << '\n';
}

template <>
void Identifier::print (std::ostream &out, std::uint32_t indent_level) const
{
    out << std::string(4*indent_level, ' ');
    out << "Identifier = " << m_value << '\n';
}

template <>
void IntegerLiteral::print (std::ostream &out, std::uint32_t indent_level) const
{
    out << std::string(4*indent_level, ' ');
    out << "IntegerLiteral = " << m_value << '\n';
}

template <>
void NumericLiteral::print (std::ostream &out, std::uint32_t indent_level) const
{
    out << std::string(4*indent_level, ' ');
    out << "NumericLiteral = " << m_value << '\n';
}

template <>
void StringLiteral::print (std::ostream &out, std::uint32_t indent_level) const
{
    out << std::string(4*indent_level, ' ');
    out << "StringLiteral = " << m_value << '\n';
}

} // end of namespace Ast
