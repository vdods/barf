// 2019.11.06 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"
#include "sem/Vector.hpp"

namespace cbz {
namespace sem {

struct TypeArray : public TypeAggregate
{
    // TODO: Change this to an unsigned int
    TypeArray (FiRange const &firange, nnup<TypeBase> &&element_type, uint64_t element_count);
    virtual ~TypeArray () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::TYPE_ARRAY; }
    virtual bool equals (Base const &other) const override;
    virtual TypeArray *cloned () const override;
    virtual void print (Log &out) const override;

    virtual uint64_t length () const override { return m_element_count; }
    virtual TypeBase const &element (uint64_t i) const override { return *m_element_type; }
//     virtual bool is_uniform () const override { return true; }
    virtual bool has_uniform_element_type () const override { return true; }
    virtual TypeBase const &uniform_element_type () const override { return *m_element_type; }

    TypeBase const &element_type () const { return *m_element_type; }
    uint64_t element_count () const { return m_element_count; }

private:

    nnup<TypeBase> m_element_type;
    uint64_t m_element_count;
};

template <typename... Args_>
nnup<TypeArray> make_type_array (Args_&&... args)
{
    return make_nnup<TypeArray>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
