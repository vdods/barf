// 2019.06.06 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"
#include "sem/Vector.hpp"

namespace cbz {
namespace sem {

struct ElementAccess : public Base
{
    ElementAccess (nnup<Base> &&referent, nnup<Base> &&element_key) : ElementAccess(firange_of(referent)+firange_of(element_key), std::move(referent), std::move(element_key)) { }
    ElementAccess (FiRange const &firange, nnup<Base> &&referent, nnup<Base> &&element_key)
    :   Base(firange)
    ,   m_referent(std::move(referent))
    ,   m_element_key(std::move(element_key))
    { }
    virtual ~ElementAccess () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::ELEMENT_ACCESS; }
    virtual bool equals (Base const &other) const override;
    virtual ElementAccess *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;

    // NOTE: This assumes you can't have mixed tuples where there are variables and types within the same tuple.
    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override { return m_referent->generate_expression_kind(context); }
    virtual Determinability generate_determinability (cgen::Context &context) const override;
    virtual llvm::Type *generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_lvalue (cgen::Context &context) const override;
    virtual llvm::Type *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_rvalue (cgen::Context &context) const override;

    Base const &referent () const { return *m_referent; }
    Base const &element_key () const { return *m_element_key; }

private:

    nnup<Base> m_referent;
    nnup<Base> m_element_key;
};

template <typename... Args_>
nnup<ElementAccess> make_element_access (Args_&&... args)
{
    return make_nnup<ElementAccess>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
