// 2020.06.03 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"
#include "sem/Type.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace cbz {
namespace sem {

struct PointerType : public TypeBase
{
    PointerType (FiRange const &firange, nnup<TypeBase> &&referent) : TypeBase(firange), m_referent(std::move(referent)) { }
    virtual ~PointerType () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::POINTER_TYPE; }
    virtual bool equals (Base const &other) const override;
    virtual PointerType *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;

    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override { return ExpressionKind::TYPE; }
    virtual Determinability generate_determinability (cgen::Context &context) const override;
    virtual llvm::PointerType *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;

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
} // end namespace cbz
