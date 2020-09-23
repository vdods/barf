// 2016.08.09 - Victor Dods

#include "sem/Declaration.hpp"

#include "cbz/cgen/Context.hpp"
#include "cbz/cgen/Generation.hpp"
#include "Exception.hpp"
#include "sem/FunctionPrototype.hpp"
#include "sem/Identifier.hpp"
#include "sem/SymbolSpecifier.hpp"
#include "sem/Type.hpp"
#include "llvm/IR/Instructions.h"

namespace cbz {
namespace sem {

Declaration::Declaration (nnup<SymbolSpecifier> &&symbol_specifier, nnup<TypeBaseOrTypeKeyword> &&content)
:   Base(firange_of(symbol_specifier) + firange_of(content))
,   m_symbol_specifier(std::move(symbol_specifier))
,   m_content(std::move(content))
{ }

Declaration::Declaration (FiRange const &firange, nnup<SymbolSpecifier> &&symbol_specifier, nnup<TypeBaseOrTypeKeyword> &&content)
:   Base(firange)
,   m_symbol_specifier(std::move(symbol_specifier))
,   m_content(std::move(content))
{ }

// This is declared here so that the definition of SymbolSpecifier is complete..
// See http://web.archive.org/web/20140903193346/http://home.roadrunner.com/~hinnant/incomplete.html
Declaration::~Declaration () = default;

bool Declaration::equals (Base const &other_) const
{
    Declaration const &other = dynamic_cast<Declaration const &>(other_);
    return are_equal(m_symbol_specifier, other.m_symbol_specifier) && are_equal(m_content, other.m_content);
}

Declaration *Declaration::cloned () const
{
    return new Declaration(firange(), clone_of(m_symbol_specifier), clone_of(m_content));
}

void Declaration::print (Log &out) const
{
    out << "Declaration(" << firange() << '\n';
    out << IndentGuard()
        << m_symbol_specifier << " : " << m_content << '\n';
    out << ')';
}

void Declaration::resolve_symbols (cgen::Context &context)
{
    m_symbol_specifier->resolve_symbols(context);
    auto scope_guard = context.push_symbol_carrier(make_nnup<cgen::SymbolCarrierDecl>(clone_of(m_symbol_specifier)));
    m_content->resolve_symbols(context);
}

void Declaration::generate_code (cgen::Context &context) const
{
    cgen::generate_declaration(context, *m_symbol_specifier, *m_content, firange());
}

} // end namespace sem
} // end namespace cbz
