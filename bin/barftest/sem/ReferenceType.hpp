// 2020.06.03 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Type.hpp"

namespace cbz {
namespace sem {

struct ReferenceType : public TypeBase
{
    ReferenceType (FiRange const &firange, nnup<TypeBase> &&referent) : TypeBase(firange), m_referent(std::move(referent)) { }
    virtual ~ReferenceType () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::REFERENCE_TYPE; }
    virtual bool equals (Base const &other) const override;
    virtual ReferenceType *cloned () const override;
    virtual void print (Log &out) const override;

    TypeBase const &referent () const { return *m_referent; }

private:

    nnup<TypeBase> m_referent;
};

template <typename... Args_>
nnup<ReferenceType> make_reference_type (Args_&&... args)
{
    return make_nnup<ReferenceType>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
