// 2017.08.15 - Copyright Victor Dods - Licensed under Apache 2.0

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
    ERROR_DUMMY,
    IDENTIFIER,
    INTEGER_LITERAL,
    NUMERIC_LITERAL,
    OPERATOR,
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

struct ErrorDummy : public Base
{
    std::string m_description;

    template <typename... Args_>
    ErrorDummy (Args_&&... args)
        :   m_description(std::forward<Args_>(args)...)
    { }
    virtual ~ErrorDummy () { }

    virtual Type type () const override { return Type::ERROR_DUMMY; }
    virtual int compare (Base const &other) const override { return false; } // There is only one distinct ErrorDummy
    virtual void print (std::ostream &out, std::uint32_t indent_level = 0) const override;
};

template <typename... Args_>
std::shared_ptr<ErrorDummy> error_dummy (Args_&&... args)
{
    return std::make_shared<ErrorDummy>(std::forward<Args_>(args)...);
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
    virtual int compare (Base const &other) const override { return Ast::compare(m_value, dynamic_cast<Value const &>(other).m_value); }
    virtual void print (std::ostream &out, std::uint32_t indent_level = 0) const override
    {
        out << std::string(4*indent_level, ' ');
        out << "Value<T_," << AsString(TYPE_) << "> = " << m_value << '\n';
    }
};

template <typename T_, Type TYPE_, typename... Args_>
std::shared_ptr<Value<T_,TYPE_>> value (Args_&&... args)
{
    return std::make_shared<Value<T_,TYPE_>>(std::forward<Args_>(args)...);
}

typedef Value<std::string,  Type::BAD_TOKEN>        BadToken;
typedef Value<std::uint8_t, Type::CHAR_LITERAL>     CharLiteral;
typedef Value<std::string,  Type::IDENTIFIER>       Identifier;
typedef Value<std::int64_t, Type::INTEGER_LITERAL>  IntegerLiteral;
typedef Value<double,       Type::NUMERIC_LITERAL>  NumericLiteral;
typedef Value<std::string,  Type::STRING_LITERAL>   StringLiteral;

template <typename... Args_>
inline std::shared_ptr<BadToken> bad_token (Args_&&... args)
{
    return std::make_shared<BadToken>(std::forward<Args_>(args)...);
}

template <typename... Args_>
inline std::shared_ptr<CharLiteral> char_literal (Args_&&... args)
{
    return std::make_shared<CharLiteral>(std::forward<Args_>(args)...);
}

template <typename... Args_>
inline std::shared_ptr<Identifier> identifier (Args_&&... args)
{
    return std::make_shared<Identifier>(std::forward<Args_>(args)...);
}

template <typename... Args_>
inline std::shared_ptr<IntegerLiteral> integer_literal (Args_&&... args)
{
    return std::make_shared<IntegerLiteral>(std::forward<Args_>(args)...);
}

template <typename... Args_>
inline std::shared_ptr<NumericLiteral> numeric_literal (Args_&&... args)
{
    return std::make_shared<NumericLiteral>(std::forward<Args_>(args)...);
}

template <typename... Args_>
inline std::shared_ptr<StringLiteral> string_literal (Args_&&... args)
{
    return std::make_shared<StringLiteral>(std::forward<Args_>(args)...);
}

template <typename ChildType_>
struct Operator : public Base
{
    typedef std::vector<std::shared_ptr<ChildType_>> ChildNodes;

    std::string m_operator_string;
    ChildNodes m_child_nodes;

    template <typename... Args_>
    Operator (std::string const &operator_string, Args_&&... args)
        :   m_operator_string(operator_string)
        ,   m_child_nodes(std::forward<Args_>(args)...)
    { }
    virtual ~Operator () { }

    virtual Type type () const override { return Type::OPERATOR; }
    virtual int compare (Base const &other_) const override
    {
        Operator const &other = dynamic_cast<Operator const &>(other_);

        int c;

        c = m_operator_string.compare(other.m_operator_string);
        if (c != 0)
            return c;

        if (m_child_nodes.size() != other.m_child_nodes.size())
        {
            if (m_child_nodes.size() < other.m_child_nodes.size())
                return -1;
            else
                return 1;
        }

        for (std::size_t i = 0; i < m_child_nodes.size(); ++i)
        {
            c = Ast::compare(*m_child_nodes[i], *other.m_child_nodes[i]);
            if (c != 0)
                return c;
        }

        return 0;
    }
    virtual void print (std::ostream &out, std::uint32_t indent_level = 0) const override
    {
        out << std::string(4*indent_level, ' ');
        out << "Operator<...> = " << m_operator_string << " with child nodes:\n";
        for (auto const &child : m_child_nodes)
            child->print(out, indent_level+1);
    }
};

template <typename ChildType_, typename... Args_>
std::shared_ptr<Operator<ChildType_>> operator_ (Args_&&... args)
{
    return std::make_shared<Operator<ChildType_>>(std::forward<Args_>(args)...);
}

} // end of namespace Ast
