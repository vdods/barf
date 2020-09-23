// 2016.08.09 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"

namespace cbz {
namespace sem {

struct Identifier;

struct Initialization : public Base
{
    Initialization (nnup<Identifier> &&id, nnup<Base> &&content) : Initialization(firange_of(id) + firange_of(content), std::move(id), std::move(content)) { }
    Initialization (FiRange const &firange, nnup<Identifier> &&id, nnup<Base> &&content);
    virtual ~Initialization () { } // NOTE: Not sure how this doesn't complain about Identifier being incomplete

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::INITIALIZATION; }
    virtual bool equals (Base const &other) const override;
    virtual Initialization *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;
    virtual void generate_code (cgen::Context &context) const override;

    Identifier const &id () const { return *m_id; }
    Base const &content () const { return *m_content; }

private:

    nnup<Identifier> m_id;
    nnup<Base> m_content;
};

template <typename... Args_>
nnup<Initialization> make_initialization (Args_&&... args)
{
    return make_nnup<Initialization>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
