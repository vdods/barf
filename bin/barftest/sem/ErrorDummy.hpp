// 2018.08.30 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"

namespace cbz {
namespace sem {

struct ErrorDummy : public Base
{
    ErrorDummy (FiRange const &firange = FiRange::INVALID) : Base(firange) { }
    virtual ~ErrorDummy () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::ERROR_DUMMY; }
    virtual bool equals (Base const &other) const override { return true; } // This is a singleton type.
    virtual ErrorDummy *cloned () const override;
    virtual void print (Log &out) const override;
};

template <typename... Args_>
nnup<ErrorDummy> make_error_dummy (Args_&&... args)
{
    return make_nnup<ErrorDummy>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
