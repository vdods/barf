// 2016.08.09 - Victor Dods

#include "barftest/sem/Definition.hpp"

#include "barftest/Exception.hpp"
#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/SymbolSpecifier.hpp"
#include "barftest/sem/Type.hpp"
#include "barftest/sem/Value.hpp"

namespace barftest {
namespace sem {

Definition::Definition (FiRange const &firange, nnup<SymbolSpecifier> &&symbol_specifier, nnup<Base> &&content)
    :   Base(firange)
    ,   m_symbol_specifier(std::move(symbol_specifier))
    ,   m_content(std::move(content))
{
    if (m_content->type_enum__raw() == TypeEnum::TYPE_KEYWORD)
        LVD_ABORT_WITH_FIRANGE("definition content must not be TypeKeyword", firange);
}

// This is declared here so that the definition of SymbolSpecifier is complete..
// See http://web.archive.org/web/20140903193346/http://home.roadrunner.com/~hinnant/incomplete.html
Definition::~Definition () = default;

bool Definition::equals (Base const &other_) const
{
    Definition const &other = dynamic_cast<Definition const &>(other_);
    return are_equal(m_symbol_specifier, other.m_symbol_specifier) && are_equal(m_content, other.m_content);
}

Definition *Definition::cloned () const
{
    return new Definition(firange(), clone_of(m_symbol_specifier), clone_of(m_content));
}

void Definition::print (Log &out) const
{
    out << "Definition(" << firange() << '\n';
    out << IndentGuard()
        << m_symbol_specifier << " ::= " << m_content << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace barftest
