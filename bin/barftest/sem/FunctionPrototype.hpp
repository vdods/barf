// 2019.04.25 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Type.hpp"
#include "sem/Vector.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"

namespace cbz {
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
    virtual void resolve_symbols (cgen::Context &context) override;

    virtual llvm::PointerType *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Function *generate_function_prototype (cgen::Context &context) const override;

    DeclarationTuple const &domain_variable_declaration_tuple () const { return *m_domain_variable_declaration_tuple; }
    FunctionType const &function_type () const { return *m_function_type; }

    // This will throw if the size of the parameter list doesn't match the number of elements in the domain.
    void validate_parameter_count (ParameterList const &parameter_list, FiRange const &function_call_firange) const;
    // This will throw if the types of the parameter list elements don't match the corresponding domain elements.
    void validate_parameter_count_and_types (cgen::Context &context, ParameterList const &parameter_list, FiRange const &function_call_firange) const;

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
} // end namespace cbz
