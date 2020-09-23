// 2016.08.08 - Victor Dods

#include "sem/Identifier.hpp"

#include "Exception.hpp"
#include "literal.hpp"
#include "sem/ReferenceType.hpp"
#include "sem/SymbolSpecifier.hpp"

namespace cbz {
namespace sem {

Identifier::Identifier (FiRange const &firange, std::string const &text)
    : Base(firange)
    , m_text(text)
{
    if (m_text.empty())
        LVD_ABORT_WITH_FIRANGE("Identifier must have nonempty text", this->firange());
}

bool Identifier::equals (Base const &other_) const
{
    Identifier const &other = dynamic_cast<Identifier const &>(other_);
    return m_text == other.m_text;
}

Identifier *Identifier::cloned () const
{
    return new Identifier(firange(), m_text);
}

void Identifier::print (Log &out) const
{
    out << "Identifier(" << firange() << ", " << string_literal_of(m_text) << ')';
}

} // end namespace sem
} // end namespace cbz
