// 2016.08.10 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include "barftest/sem/Base.hpp"

namespace barftest {
namespace sem {

struct Break : public Base
{
    Break (FiRange const &firange = FiRange::INVALID) : Base(firange) { }
    virtual ~Break () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::BREAK; }
    virtual bool equals (Base const &other) const override { return true; } // This is a singleton type.
    virtual Break *cloned () const override;
    virtual void print (Log &out) const override;
};

template <typename... Args_>
nnup<Break> make_break (Args_&&... args)
{
    return make_nnup<Break>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace barftest
