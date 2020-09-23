// 2016.08.09 - Victor Dods

#include "sem/Initialization.hpp"

#include "Exception.hpp"
#include "sem/Identifier.hpp"
#include "sem/Type.hpp"

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

} // end namespace sem
} // end namespace cbz
