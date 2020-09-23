// 2019.11.20 - Victor Dods

#include "sem/SymbolSpecifier.hpp"

#include "cbz/cgen/Context.hpp"
#include "cbz/cgen/TypeSymbol.hpp"
#include "cbz/cgen/VariableSymbol.hpp"
#include "cbz/literal.hpp"
#include "Exception.hpp"
#include "llvm/IR/Instructions.h"

namespace cbz {
namespace sem {

// This is declared here so that the definition of Identifier is complete..
// See http://web.archive.org/web/20140903193346/http://home.roadrunner.com/~hinnant/incomplete.html
SymbolSpecifier::~SymbolSpecifier () = default;

bool SymbolSpecifier::equals (Base const &other_) const
{
    SymbolSpecifier const &other = dynamic_cast<SymbolSpecifier const &>(other_);
    return are_equal(m_id, other.m_id) && are_equal(m_value_kind_specifier, other.m_value_kind_specifier) && are_equal(m_value_lifetime_specifier, other.m_value_lifetime_specifier);
}

SymbolSpecifier *SymbolSpecifier::cloned () const
{
    return new SymbolSpecifier(firange(), clone_of(m_id), clone_of(m_value_kind_specifier), clone_of(m_value_lifetime_specifier), clone_of(m_global_value_linkage_specifier));
}

void SymbolSpecifier::print (Log &out) const
{
    out << "SymbolSpecifier(" << firange() << '\n';
    {
        IndentGuard ig(out);
        out << string_literal_of(m_id->text());
        if (m_value_kind_specifier->value() != ValueKindContextual::DETERMINE_FROM_CONTEXT)
            out << ' ' << as_string_lowercase(m_value_kind_specifier->value());
        if (m_value_lifetime_specifier->value() != ValueLifetimeContextual::DETERMINE_FROM_CONTEXT)
            out << ' ' << as_string_lowercase(m_value_lifetime_specifier->value());
        out << '\n';
    }
    out << ')';
}

void SymbolSpecifier::resolve_symbols (cgen::Context &context)
{
    m_id->resolve_symbols(context);
    m_value_kind_specifier->resolve_symbols(context);
    m_value_lifetime_specifier->resolve_symbols(context);
}

// llvm::Value *SymbolSpecifier::generate_lvalue (cgen::Context &context) const
// {
//     // Just forward to m_id.
//     return m_id->generate_lvalue(context);
// }

nnup<SymbolSpecifier> SymbolSpecifier::with_specified_value_kind (ValueKind value_kind, FiRange const &specifier_firange) const
{
    // Can "promote" from DETERMINE_FROM_CONTEXT to CONSTANT or VARIABLE
    if (m_value_kind_specifier->value() == ValueKindContextual::DETERMINE_FROM_CONTEXT)
        return make_symbol_specifier(
            clone_of(m_id),
            make_value_kind_specifier(specifier_firange, ValueKindContextual(value_kind)),
            clone_of(m_value_lifetime_specifier),
            clone_of(m_global_value_linkage_specifier)
        );

    if (m_value_kind_specifier->value() == ValueKindContextual(value_kind))
        return make_symbol_specifier(
            clone_of(m_id),
            clone_of(m_value_kind_specifier),
            clone_of(m_value_lifetime_specifier),
            clone_of(m_global_value_linkage_specifier)
        );

    throw ProgrammerError(LVD_FMT("conflicting rt/ct specifiers (original was at " << m_value_kind_specifier->firange() << ')'), specifier_firange);
}

nnup<SymbolSpecifier> SymbolSpecifier::with_specified_value_lifetime (ValueLifetime value_lifetime, FiRange const &specifier_firange) const
{
    // Can "promote" from DETERMINE_FROM_CONTEXT to LOCAL or GLOBAL
    if (m_value_lifetime_specifier->value() == ValueLifetimeContextual::DETERMINE_FROM_CONTEXT)
        return make_symbol_specifier(
            clone_of(m_id),
            clone_of(m_value_kind_specifier),
            make_value_lifetime_specifier(specifier_firange, ValueLifetimeContextual(value_lifetime)),
            clone_of(m_global_value_linkage_specifier)
        );

    if (m_value_lifetime_specifier->value() == ValueLifetimeContextual(value_lifetime))
        return make_symbol_specifier(
            clone_of(m_id),
            clone_of(m_value_kind_specifier),
            clone_of(m_value_lifetime_specifier),
            clone_of(m_global_value_linkage_specifier)
        );

    throw ProgrammerError(LVD_FMT("conflicting local/global specifiers (original was at " << m_value_lifetime_specifier->firange() << ')'), specifier_firange);
}

nnup<SymbolSpecifier> SymbolSpecifier::with_specified_global_value_linkage (GlobalValueLinkage global_value_linkage, FiRange const &specifier_firange) const
{
    // Can "promote" from DETERMINE_FROM_CONTEXT to a GlobalValueLinkage
    if (m_value_lifetime_specifier->value() == ValueLifetimeContextual::DETERMINE_FROM_CONTEXT)
        return make_symbol_specifier(
            clone_of(m_id),
            clone_of(m_value_kind_specifier),
            clone_of(m_value_lifetime_specifier),
            make_global_value_linkage_specifier(specifier_firange, GlobalValueLinkageContextual(global_value_linkage))
        );

    if (m_global_value_linkage_specifier->value() == GlobalValueLinkageContextual(global_value_linkage))
        return make_symbol_specifier(
            clone_of(m_id),
            clone_of(m_value_kind_specifier),
            clone_of(m_value_lifetime_specifier),
            clone_of(m_global_value_linkage_specifier)
        );

    throw ProgrammerError(LVD_FMT("conflicting GlobalValueLinkage specifiers (original was at " << m_global_value_linkage_specifier->firange() << ')'), specifier_firange);
}

} // end namespace sem
} // end namespace cbz
