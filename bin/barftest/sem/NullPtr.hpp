// 2020.07.07 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace cbz {
namespace sem {

struct NullPtr : public Base
{
    NullPtr (FiRange const &firange, nnup<TypeBase> &&pointer_type) : Base(firange), m_pointer_type(std::move(pointer_type)) { }
    virtual ~NullPtr () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::NULL_PTR; }
    virtual bool equals (Base const &other) const override;
    virtual NullPtr *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;

    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override { return ExpressionKind::VALUE; }
    virtual Determinability generate_determinability (cgen::Context &context) const override;
    virtual llvm::PointerType *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_rvalue (cgen::Context &context) const override;

    TypeBase const &pointer_type () const { return *m_pointer_type; }

private:

    nnup<TypeBase> m_pointer_type;
};

template <typename... Args_>
nnup<NullPtr> make_nullptr (Args_&&... args)
{
    return make_nnup<NullPtr>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
