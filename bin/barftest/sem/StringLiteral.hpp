// 2019.11.12 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"
#include <string>

namespace cbz {
namespace sem {

// TODO: This should inherit from ValueLiteralBase once that exists
struct StringLiteral : public Base
{
    StringLiteral (std::string const &text) : StringLiteral(FiRange::INVALID, text) { }
    StringLiteral (FiRange const &firange, std::string const &text) : Base(firange), m_text(text) { }
    StringLiteral (FiRange const &firange) : Base(firange) { }
    virtual ~StringLiteral () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::STRING_LITERAL; }
    virtual bool equals (Base const &other) const override;
    virtual StringLiteral *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override { } // Nothing needed.
    virtual TypeEnum type_enum__resolved (cgen::Context &context) const override { return TypeEnum::STRING_LITERAL; }
    // NOTE: Even though TypeIdentifier uses StringLiteral in its implementation, this method is not used by it.
    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const override { return ExpressionKind::VALUE; }
    virtual Determinability generate_determinability (cgen::Context &context) const override { return Determinability::COMPILETIME; }
    virtual llvm::Type *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const override;
    virtual llvm::Value *generate_rvalue (cgen::Context &context) const override;

    std::string const &text () const { return m_text; }

    void accumulate_text (std::string const &s) { m_text += s; }

private:

    std::string m_text;
};

template <typename... Args_>
nnup<StringLiteral> make_string_literal (Args_&&... args)
{
    return make_nnup<StringLiteral>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
