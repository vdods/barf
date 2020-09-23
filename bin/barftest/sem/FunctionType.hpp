// 2016.08.09 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include "barftest/sem/Type.hpp"
#include "barftest/sem/Vector.hpp"

namespace barftest {
namespace sem {

struct FunctionType : public TypeBase
{
    FunctionType (nnup<TypeTuple> &&domain, nnup<TypeBase> &&codomain) : FunctionType(firange_of(domain) + firange_of(codomain), std::move(domain), std::move(codomain)) { }
    FunctionType (FiRange const &firange, nnup<TypeTuple> &&domain, nnup<TypeBase> &&codomain)
    :   TypeBase(firange)
    ,   m_domain(std::move(domain))
    ,   m_codomain(std::move(codomain))
    { }
    virtual ~FunctionType () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::FUNCTION_TYPE; }
    virtual bool equals (Base const &other) const override;
    virtual FunctionType *cloned () const override;
    virtual void print (Log &out) const override;

    TypeTuple const &domain () const { return *m_domain; }
    TypeBase const &codomain () const { return *m_codomain; }

    // This will throw if the size of the parameter list doesn't match the number of elements in the domain.
    void validate_parameter_count (ParameterList const &parameter_list, FiRange const &function_call_firange) const;

private:

    nnup<TypeTuple> m_domain;
    nnup<TypeBase> m_codomain;
};

template <typename... Args_>
nnup<FunctionType> make_function_type (Args_&&... args)
{
    return make_nnup<FunctionType>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace barftest
