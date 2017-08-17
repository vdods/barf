// ///////////////////////////////////////////////////////////////////////////
// ast.hpp by Victor Dods, created 2017/08/15
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Ast {

enum Type : std::uint32_t
{
    BAD_TOKEN,
    CHAR_LITERAL,
    IDENTIFIER,
    INTEGER_LITERAL,
    NUMERIC_LITERAL,
    STRING_LITERAL,

    LOW_ = BAD_TOKEN,
    HIGH_ = STRING_LITERAL,
    COUNT_ = HIGH_+1-LOW_
};

std::string const &AsString (Type type);

template <typename T_>
inline int compare (T_ const &lhs, T_ const &rhs)
{
    if (lhs < rhs)
        return -1;
    else if (lhs == rhs)
        return 0;
    else
        return 1;
}

inline int compare (std::string const &lhs, std::string const &rhs)
{
    return lhs.compare(rhs);
}

struct Base
{
    Base () { }
    virtual ~Base () { }

    template <typename T_>
    T_ const &as () const { return dynamic_cast<T_ const &>(*this); }
    template <typename T_>
    T_ &as () { return dynamic_cast<T_ &>(*this); }

    virtual Type type () const = 0;
    virtual int compare (Base const &other) const = 0;
    virtual void print (std::ostream &out, std::uint32_t indent_level = 0) const = 0;
};

inline std::ostream &operator << (std::ostream &out, Base const &b)
{
    b.print(out);
    return out;
}

inline int compare (Base const &lhs, Base const &rhs)
{
    int c = compare(lhs.type(), rhs.type());
    if (c != 0)
        return c;
    else
        return lhs.compare(rhs);
}

inline bool operator == (Base const &lhs, Base const &rhs)
{
    return compare(lhs, rhs) == 0;
}

inline bool operator != (Base const &lhs, Base const &rhs)
{
    return compare(lhs, rhs) != 0;
}

inline bool operator < (Base const &lhs, Base const &rhs)
{
    return compare(lhs, rhs) < 0;
}

template <typename T_, Type TYPE_>
struct Value : public Base
{
    T_ m_value;

    Value ()                : Base()                               { }
    Value (T_ const &value) : Base(), m_value(value)               { }
    Value (T_ &&value)      : Base(), m_value(std::move(value))    { }
    virtual ~Value () { }

    virtual Type type () const override { return TYPE_; }
    virtual int compare (Base const &other) const { return Ast::compare(m_value, dynamic_cast<Value const &>(other).m_value); }
    virtual void print (std::ostream &out, std::uint32_t indent_level = 0) const
    {
        out << std::string(4*indent_level, ' ');
        out << "Value<T_," << AsString(TYPE_) << "> = " << m_value;
    }
};

typedef Value<std::string,  Type::BAD_TOKEN>        BadToken;
typedef Value<std::uint8_t, Type::CHAR_LITERAL>     CharLiteral;
typedef Value<std::string,  Type::IDENTIFIER>       Identifier;
typedef Value<std::int64_t, Type::INTEGER_LITERAL>  IntegerLiteral;
typedef Value<double,       Type::NUMERIC_LITERAL>  NumericLiteral;
typedef Value<std::string,  Type::STRING_LITERAL>   StringLiteral;

// struct Ast : public Base
// {
//     std::string m_text;
//     std::vector<std::shared_ptr<Ast>> m_child_nodes;
//
//     Ast (std::string const &text)
//         :   m_text(text)
//     { }
//     // TODO: Maybe use perfect template parameter forwarding instead of child_nodes param.
//     Ast (std::string const &text, std::vector<std::shared_ptr<Ast>> &&child_nodes)
//         :   m_text(text)
//         ,   m_child_nodes(std::move(child_nodes))
//     { }
//     Ast (Ast &&ast)
//         :   m_text(std::move(ast.m_text))
//         ,   m_child_nodes(std::move(ast.m_child_nodes))
//     { }
// };
//
// inline int Compare (Ast const &lhs, Ast const &rhs)
// {
//     int c;
//
//     c = lhs.m_text.compare(rhs.m_text);
//     if (c != 0)
//         return c;
//
//     if (lhs.m_child_nodes.size() != rhs.m_child_nodes.size())
//     {
//         if (lhs.m_child_nodes.size() < rhs.m_child_nodes.size())
//             return -1;
//         else
//             return 1;
//     }
//
//     for (std::size_t i = 0; i < lhs.m_child_nodes.size(); ++i)
//     {
//         c = Compare(*lhs.m_child_nodes[i], *rhs.m_child_nodes[i]);
//         if (c != 0)
//             return c;
//     }
//
//     return 0;
// }
//
// inline bool operator == (Ast const &lhs, Ast const &rhs)
// {
//     return Compare(lhs, rhs) == 0;
// }
//
// inline bool operator != (Ast const &lhs, Ast const &rhs)
// {
//     return Compare(lhs, rhs) != 0;
// }
//
// inline bool operator < (Ast const &lhs, Ast const &rhs)
// {
//     return Compare(lhs, rhs) < 0;
// }

} // end of namespace Ast
