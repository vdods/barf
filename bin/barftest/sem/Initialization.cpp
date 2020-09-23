// 2016.08.09 - Victor Dods

#include "sem/Initialization.hpp"

#include "cbz/cgen/Context.hpp"
#include "cbz/cgen/Generation.hpp"
#include "Exception.hpp"
#include "sem/Identifier.hpp"
#include "sem/Type.hpp"
#include "llvm/IR/Instructions.h"

namespace cbz {
namespace sem {

Initialization::Initialization (FiRange const &firange, nnup<Identifier> &&id, nnup<Base> &&content)
:   Base(firange)
,   m_id(std::move(id))
,   m_content(std::move(content))
{ }

bool Initialization::equals (Base const &other_) const
{
    Initialization const &other = dynamic_cast<Initialization const &>(other_);
    return are_equal(m_id, other.m_id) && are_equal(m_content, other.m_content);
}

Initialization *Initialization::cloned () const
{
    return new Initialization(firange(), clone_of(m_id), clone_of(m_content));
}

void Initialization::print (Log &out) const
{
    out << "Initialization(" << firange() << '\n';
    out << IndentGuard()
        << m_id << " := " << m_content << '\n';
    out << ')';
}

void Initialization::resolve_symbols (cgen::Context &context)
{
    m_id->resolve_symbols(context);
    auto scope_guard = context.push_symbol_carrier(make_nnup<cgen::SymbolCarrierInit>(clone_of(m_id)));
    m_content->resolve_symbols(context);
}

void Initialization::generate_code (cgen::Context &context) const
{
    cgen::generate_initialization(context, *m_id, *m_content, firange());
}

} // end namespace sem
} // end namespace cbz
