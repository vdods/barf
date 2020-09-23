// 2019.11.12 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"
#include <string>

namespace cbz {
namespace sem {

// TODO: This should inherit from ValueLiteralBase once that exists
struct StringLiteral : public Base
{
    StringLiteral (std::string const &text) : StringLiteral(FiRange::INVALID, text) { }
    StringLiteral (FiRange const &firange, std::string const &text) : Base(firange), m_text(text) { }
    StringLiteral (FiRange const &firange) : Base(firange) { }
    virtual ~StringLiteral () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::STRING_LITERAL; }
    virtual bool equals (Base const &other) const override;
    virtual StringLiteral *cloned () const override;
    virtual void print (Log &out) const override;

    std::string const &text () const { return m_text; }

    void accumulate_text (std::string const &s) { m_text += s; }

private:

    std::string m_text;
};

template <typename... Args_>
nnup<StringLiteral> make_string_literal (Args_&&... args)
{
    return make_nnup<StringLiteral>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
