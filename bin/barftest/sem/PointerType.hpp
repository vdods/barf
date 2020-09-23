// 2020.06.03 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include "barftest/sem/Base.hpp"
#include "barftest/sem/Type.hpp"

namespace barftest {
namespace sem {

struct PointerType : public TypeBase
{
    PointerType (FiRange const &firange, nnup<TypeBase> &&referent) : TypeBase(firange), m_referent(std::move(referent)) { }
    virtual ~PointerType () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::POINTER_TYPE; }
    virtual bool equals (Base const &other) const override;
    virtual PointerType *cloned () const override;
    virtual void print (Log &out) const override;

    TypeBase const &referent () const { return *m_referent; }

private:

    nnup<TypeBase> m_referent;
};

template <typename... Args_>
nnup<PointerType> make_pointer_type (Args_&&... args)
{
    return make_nnup<PointerType>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace barftest
