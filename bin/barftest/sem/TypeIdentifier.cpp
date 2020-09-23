// 2016.09.18 - Victor Dods

#include "sem/TypeIdentifier.hpp"

#include "cbz/cgen/Context.hpp"
#include "sem/Identifier.hpp"

namespace cbz {
namespace sem {

bool TypeIdentifier::equals (Base const &other_) const
{
    TypeIdentifier const &other = dynamic_cast<TypeIdentifier const &>(other_);
    return are_equal(this->m_id, other.m_id);
}

TypeIdentifier *TypeIdentifier::cloned () const
{
    return new TypeIdentifier(firange(), clone_of(m_id));
}

void TypeIdentifier::print (Log &out) const
{
    out << "TypeIdentifier(" << firange() << '\n';
    out << IndentGuard()
        << m_id << '\n';
    out << ')';
}

void TypeIdentifier::resolve_symbols (cgen::Context &context)
{
    m_id->resolve_symbols(context);
}

TypeEnum TypeIdentifier::type_enum__resolved (cgen::Context &context) const
{
    return m_id->type_enum__resolved(context);
}

Determinability TypeIdentifier::generate_determinability (cgen::Context &context) const
{
    return m_id->generate_determinability(context);
}

llvm::Type *TypeIdentifier::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    return m_id->generate_rvalue_type(context, abstract_type);
}

} // end namespace sem
} // end namespace cbz
