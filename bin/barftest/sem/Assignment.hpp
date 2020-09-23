// 2016.08.09 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"

namespace cbz {
namespace sem {

struct Assignment : public Base
{
    Assignment (nnup<Base> &&target, nnup<Base> &&content) : Assignment(firange_of(target) + firange_of(content), std::move(target), std::move(content)) { }
    Assignment (FiRange const &firange, nnup<Base> &&target, nnup<Base> &&content)
    :   Base(firange)
    ,   m_target(std::move(target))
    ,   m_content(std::move(content))
    { }
    virtual ~Assignment () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::ASSIGNMENT; }
    virtual bool equals (Base const &other) const override;
    virtual void print (Log &out) const override;
    virtual Assignment *cloned () const override;
    virtual void resolve_symbols (cgen::Context &context) override;
    virtual void generate_code (cgen::Context &context) const override;

    Base const &target () const { return *m_target; }
    Base const &content () const { return *m_content; }

private:

    nnup<Base> m_target;
    nnup<Base> m_content;
};

template <typename... Args_>
nnup<Assignment> make_assignment (Args_&&... args)
{
    return make_nnup<Assignment>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
