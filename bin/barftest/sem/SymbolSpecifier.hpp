// 2019.11.20 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"
#include "sem/Identifier.hpp"
#include "sem/Specifier.hpp"
#include <string>

namespace cbz {
namespace sem {

struct SymbolSpecifier : public Base
{
    SymbolSpecifier (nnup<Identifier> &&id)
    :   SymbolSpecifier(
            firange_of(id),
            std::move(id),
            make_value_kind_specifier(ValueKindContextual::DETERMINE_FROM_CONTEXT),
            make_value_lifetime_specifier(ValueLifetimeContextual::DETERMINE_FROM_CONTEXT),
            make_global_value_linkage_specifier(GlobalValueLinkageContextual::DETERMINE_FROM_CONTEXT)
        )
    { }
    SymbolSpecifier (FiRange const &firange, nnup<Identifier> &&id)
    :   SymbolSpecifier(
            firange,
            std::move(id),
            make_value_kind_specifier(ValueKindContextual::DETERMINE_FROM_CONTEXT),
            make_value_lifetime_specifier(ValueLifetimeContextual::DETERMINE_FROM_CONTEXT),
            make_global_value_linkage_specifier(GlobalValueLinkageContextual::DETERMINE_FROM_CONTEXT)
        )
    { }
    SymbolSpecifier (
        nnup<Identifier> &&id,
        nnup<ValueKindSpecifier> &&value_kind_specifier,
        nnup<ValueLifetimeSpecifier> &&value_lifetime_specifier,
        nnup<GlobalValueLinkageSpecifier> &&global_value_linkage_specifier)
    :   SymbolSpecifier(
            firange_of(id)+firange_of(value_kind_specifier)+firange_of(value_lifetime_specifier)+firange_of(global_value_linkage_specifier),
            std::move(id),
            std::move(value_kind_specifier),
            std::move(value_lifetime_specifier),
            std::move(global_value_linkage_specifier)
        )
    { }
    SymbolSpecifier (
        FiRange const &firange,
        nnup<Identifier> &&id,
        nnup<ValueKindSpecifier> &&value_kind_specifier,
        nnup<ValueLifetimeSpecifier> &&value_lifetime_specifier,
        nnup<GlobalValueLinkageSpecifier> &&global_value_linkage_specifier)
    :   Base(firange)
    ,   m_id(std::move(id))
    ,   m_value_kind_specifier(std::move(value_kind_specifier))
    ,   m_value_lifetime_specifier(std::move(value_lifetime_specifier))
    ,   m_global_value_linkage_specifier(std::move(global_value_linkage_specifier))
    { }
    virtual ~SymbolSpecifier ();

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::SYMBOL_SPECIFIER; }
    virtual bool equals (Base const &other) const override;
    virtual SymbolSpecifier *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;
//     virtual llvm::Value *generate_lvalue (cgen::Context &context) const override;

    Identifier const &id () const { return *m_id; }
    ValueKindSpecifier const &value_kind_specifier () const { return *m_value_kind_specifier; }
    ValueLifetimeSpecifier const &value_lifetime_specifier () const { return *m_value_lifetime_specifier; }
    GlobalValueLinkageSpecifier const &global_value_linkage_specifier () const { return *m_global_value_linkage_specifier; }

    nnup<SymbolSpecifier> with_specified_value_kind (ValueKind value_kind, FiRange const &specifier_firange) const;
    nnup<SymbolSpecifier> with_specified_value_lifetime (ValueLifetime value_lifetime, FiRange const &specifier_firange) const;
    nnup<SymbolSpecifier> with_specified_global_value_linkage (GlobalValueLinkage global_value_linkage, FiRange const &specifier_firange) const;

private:

    nnup<Identifier> m_id; // TODO: This doesn't need to be const
    nnup<ValueKindSpecifier> m_value_kind_specifier;
    nnup<ValueLifetimeSpecifier> m_value_lifetime_specifier;
    nnup<GlobalValueLinkageSpecifier> m_global_value_linkage_specifier;
};

template <typename... Args_>
nnup<SymbolSpecifier> make_symbol_specifier (Args_&&... args)
{
    return make_nnup<SymbolSpecifier>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
