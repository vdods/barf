// 2016.08.09 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"
#include "sem/Specifier.hpp"

namespace cbz {
namespace sem {

struct SymbolSpecifier;
struct TypeBaseOrTypeKeyword;

struct Declaration : public Base
{
    Declaration (nnup<SymbolSpecifier> &&symbol_specifier, nnup<TypeBaseOrTypeKeyword> &&content);
    Declaration (FiRange const &firange, nnup<SymbolSpecifier> &&symbol_specifier, nnup<TypeBaseOrTypeKeyword> &&content);
    virtual ~Declaration ();

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::DECLARATION; }
    virtual bool equals (Base const &other) const override;
    virtual Declaration *cloned () const override;
    virtual void print (Log &out) const override;

    SymbolSpecifier const &symbol_specifier () const { return *m_symbol_specifier; }
    TypeBaseOrTypeKeyword const &content () const { return *m_content; }

private:

    nnup<SymbolSpecifier> m_symbol_specifier;
    nnup<TypeBaseOrTypeKeyword> m_content;
};

template <typename... Args_>
nnup<Declaration> make_declaration (Args_&&... args)
{
    return make_nnup<Declaration>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
