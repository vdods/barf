// 2016.08.09 - Victor Dods

#include "barftest/sem/Function.hpp"

#include "barftest/Exception.hpp"
#include <iostream> // TEMP
#include "barftest/sem/FunctionPrototype.hpp"
#include "barftest/sem/SymbolSpecifier.hpp"
#include "barftest/sem/ValueLifetime.hpp"

namespace barftest {
namespace sem {

Function::Function (nnup<FunctionPrototype> &&function_prototype, up<StatementList> &&body)
    :   Function(firange_of(function_prototype)+firange_of(body)
    ,   std::move(function_prototype)
    ,   std::move(body))
{ }

// This is declared here so that the definition of FunctionPrototype is complete.
// See http://web.archive.org/web/20140903193346/http://home.roadrunner.com/~hinnant/incomplete.html
Function::~Function () = default;

bool Function::equals (Base const &other_) const
{
    Function const &other = dynamic_cast<Function const &>(other_);
    return are_equal(m_function_prototype, other.m_function_prototype) && are_equal(m_body, other.m_body);
}

Function *Function::cloned () const
{
    return new Function(firange(), clone_of(m_function_prototype), clone_of(m_body));
}

void Function::print (Log &out) const
{
    out << "Function(" << firange() << '\n';
    out << IndentGuard()
        << m_function_prototype << ",\n"
        << m_body << '\n';
    out << ')';
}

} // end namespace sem
} // end namespace barftest
