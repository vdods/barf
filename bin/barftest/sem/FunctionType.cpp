// 2016.08.09 - Victor Dods

#include "barftest/sem/FunctionType.hpp"

#include "barftest/Exception.hpp"
#include "barftest/sem/Identifier.hpp"

namespace barftest {
namespace sem {

bool FunctionType::equals (Base const &other_) const
{
    FunctionType const &other = dynamic_cast<FunctionType const &>(other_);
    return are_equal(m_domain, other.m_domain) && are_equal(m_codomain, other.m_codomain);
}

FunctionType *FunctionType::cloned () const
{
    return new FunctionType(firange(), clone_of(m_domain), clone_of(m_codomain));
}

void FunctionType::print (Log &out) const
{
    out << "FunctionType(" << firange() << '\n';
    out << IndentGuard()
        << m_domain << " -> " << m_codomain << '\n';
    out << ')';
}

void FunctionType::validate_parameter_count (ParameterList const &parameter_list, FiRange const &function_call_firange) const
{
    if (parameter_list.elements().size() != m_domain->elements().size())
        throw ProgrammerError(LVD_LOG_FMT("function call of type " << *this << " expected " << m_domain->elements().size() << " parameter(s), but got " << parameter_list.elements().size()), function_call_firange);
}

} // end namespace sem
} // end namespace barftest
