// 2019.04.26 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"

namespace cbz {
namespace sem {

struct Dummy : public Base
{
    Dummy (FiRange const &firange = FiRange::INVALID) : Base(firange) { }
    virtual ~Dummy () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::DUMMY; }
    virtual bool equals (Base const &other) const override { return true; } // This is a singleton type.
    virtual Dummy *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override { } // Nothing needed.
};

template <typename... Args_>
nnup<Dummy> make_dummy (Args_&&... args)
{
    return make_nnup<Dummy>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
