// 2016.09.18 - Victor Dods

#pragma once

#include "barftest/sem/Type.hpp"

namespace barftest {
namespace sem {

struct Identifier;

// This is going away soon.
struct TypeIdentifier : public TypeBase
{
    TypeIdentifier (nnup<Identifier> &&id) : TypeIdentifier(firange_of(id), std::move(id)) { }
    TypeIdentifier (FiRange const &firange, nnup<Identifier> &&id)
    :   TypeBase(firange)
    ,   m_id(std::move(id))
    { }
    virtual ~TypeIdentifier () { } // NOTE: Not sure why this doesn't complain of Identifier being incomplete.

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::TYPE_IDENTIFIER; }
    virtual bool equals (Base const &other) const override;
    virtual TypeIdentifier *cloned () const override;
    virtual void print (Log &out) const override;

    Identifier const &id () const { return *m_id; }

private:

    nnup<Identifier> m_id;
};

template <typename... Args_>
nnup<TypeIdentifier> make_type_identifier (Args_&&... args)
{
    return make_nnup<TypeIdentifier>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace barftest
