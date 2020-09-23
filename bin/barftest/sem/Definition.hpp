// 2016.08.09 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"

namespace cbz {
namespace sem {

struct SymbolSpecifier;

struct Definition : public Base
{
    Definition (nnup<SymbolSpecifier> &&symbol_specifier, nnup<Base> &&content) : Definition(firange_of(symbol_specifier) + firange_of(content), std::move(symbol_specifier), std::move(content)) { }
    Definition (FiRange const &firange, nnup<SymbolSpecifier> &&symbol_specifier, nnup<Base> &&content);
    virtual ~Definition ();

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::DEFINITION; }
    virtual bool equals (Base const &other) const override;
    virtual Definition *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;
    virtual void generate_code (cgen::Context &context) const override;

    SymbolSpecifier const &symbol_specifier () const { return *m_symbol_specifier; }
    Base const &content () const { return *m_content; }

private:

    nnup<SymbolSpecifier> m_symbol_specifier;
    nnup<Base> m_content;
};

template <typename... Args_>
nnup<Definition> make_definition (Args_&&... args)
{
    return make_nnup<Definition>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
