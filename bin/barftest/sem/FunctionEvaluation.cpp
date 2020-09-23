// 2016.08.09 - Victor Dods

#include "sem/FunctionEvaluation.hpp"

#include "Exception.hpp"
#include "sem/Function.hpp"
#include "sem/FunctionPrototype.hpp"
#include "sem/FunctionType.hpp"

namespace cbz {
namespace sem {

bool FunctionEvaluation::equals (Base const &other_) const
{
    FunctionEvaluation const &other = dynamic_cast<FunctionEvaluation const &>(other_);
    return are_equal(m_function_expression, other.m_function_expression) && are_equal(m_parameter_list, other.m_parameter_list);
}

FunctionEvaluation *FunctionEvaluation::cloned () const
{
    return new FunctionEvaluation(firange(), clone_of(m_function_expression), clone_of(m_parameter_list));
}

void FunctionEvaluation::print (Log &out) const
{
    out << "FunctionEvaluation(" << firange() << '\n';
    out << IndentGuard()
        << m_function_expression << ",\n"
        << m_parameter_list << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace cbz
