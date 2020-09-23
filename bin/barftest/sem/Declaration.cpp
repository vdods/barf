// 2016.08.09 - Victor Dods

#include "barftest/sem/Declaration.hpp"

#include "barftest/Exception.hpp"
#include "barftest/sem/FunctionPrototype.hpp"
#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/SymbolSpecifier.hpp"
#include "barftest/sem/Type.hpp"

namespace barftest {
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

} // end namespace sem
} // end namespace barftest
