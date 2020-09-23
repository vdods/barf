// 2016.08.10 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"

namespace cbz {
namespace sem {

struct Return : public Base
{
    Return () : Base(FiRange::INVALID) { }
    Return (FiRange const &firange) : Base(firange) { }
    Return (nnup<Base> &&return_expression) : Return(firange_of(return_expression), std::move(return_expression)) { }
    Return (FiRange const &firange, nnup<Base> &&return_expression);
    virtual ~Return () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::RETURN; }
    virtual bool equals (Base const &other) const override;
    virtual Return *cloned () const override;
    virtual void print (Log &out) const override;

    bool has_return_expression () const { return m_return_expression != nullptr; }
    Base const &return_expression () const { return *m_return_expression; }

    void set_return_expression (nnup<Base> &&return_expression)
    {
        m_return_expression = std::move(return_expression);
        grow_firange(m_return_expression->firange());
    }

private:

    up<Base> m_return_expression;
};

template <typename... Args_>
nnup<Return> make_return (Args_&&... args)
{
    return make_nnup<Return>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
