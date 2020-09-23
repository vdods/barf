// 2016.08.08 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"
#include <string>

namespace cbz {
namespace sem {

// TODO: This should really be named SymbolicRef (though this would imply a separation between
// the AST (which is only about syntax) and some abstract representation of the code itself).
struct Identifier : public Base
{
    Identifier (std::string const &text) : Identifier(FiRange::INVALID, text) { }
    Identifier (FiRange const &firange, std::string const &text);
    virtual ~Identifier () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::IDENTIFIER; }
    virtual bool equals (Base const &other) const override;
    virtual Identifier *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;
    virtual TypeEnum type_enum__resolved (cgen::Context &context) const override;

    // NOTE: Even though TypeIdentifier uses Identifier in its implementation, this method is not used by it.
    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override;
    virtual Determinability generate_determinability (cgen::Context &context) const override;
    virtual llvm::Type *generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_lvalue (cgen::Context &context) const override;
    virtual llvm::Type *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_rvalue (cgen::Context &context) const override;
    virtual nnup<SymbolSpecifier> generate_svalue (cgen::Context &context) const override;
    virtual void generate_code (cgen::Context &context) const override;

    std::string const &text () const { return m_text; }

private:

    std::string m_text;
};

template <typename... Args_>
nnup<Identifier> make_identifier (Args_&&... args)
{
    return make_nnup<Identifier>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
