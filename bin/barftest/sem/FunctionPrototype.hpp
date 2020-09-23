// 2019.04.25 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include "barftest/sem/Type.hpp"
#include "barftest/sem/Vector.hpp"

namespace barftest {
namespace sem {

struct FunctionType;
struct Identifier;

struct FunctionPrototype : public TypeBase
{
    FunctionPrototype (nnup<DeclarationTuple> &&domain_variable_declaration_tuple, nnup<TypeBase> &&codomain) : FunctionPrototype(firange_of(domain_variable_declaration_tuple) + firange_of(codomain), std::move(domain_variable_declaration_tuple), std::move(codomain)) { }
    FunctionPrototype (FiRange const &firange, nnup<DeclarationTuple> &&domain_variable_declaration_tuple, nnup<TypeBase> &&codomain);
    virtual ~FunctionPrototype ();

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::FUNCTION_PROTOTYPE; }
    virtual bool equals (Base const &other) const override;
    virtual FunctionPrototype *cloned () const override;
    virtual void print (Log &out) const override;

    DeclarationTuple const &domain_variable_declaration_tuple () const { return *m_domain_variable_declaration_tuple; }
    FunctionType const &function_type () const { return *m_function_type; }

    // This will throw if the size of the parameter list doesn't match the number of elements in the domain.
    void validate_parameter_count (ParameterList const &parameter_list, FiRange const &function_call_firange) const;

private:

    static nnup<FunctionType> extract_function_type (FiRange const &firange, DeclarationTuple const &domain_variable_declaration_tuple, nnup<TypeBase> &&codomain);

    nnup<DeclarationTuple> m_domain_variable_declaration_tuple;
    nnup<FunctionType> m_function_type;
};

template <typename... Args_>
nnup<FunctionPrototype> make_function_prototype (Args_&&... args)
{
    return make_nnup<FunctionPrototype>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace barftest
