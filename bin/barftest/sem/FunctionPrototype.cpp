// 2019.04.25 - Victor Dods

#include "barftest/sem/FunctionPrototype.hpp"

#include "barftest/Exception.hpp"
#include "barftest/sem/FunctionType.hpp"
#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/SymbolSpecifier.hpp"
#include "barftest/sem/ValueLifetime.hpp"

namespace barftest {
namespace sem {

FunctionPrototype::FunctionPrototype (FiRange const &firange, nnup<DeclarationTuple> &&domain_variable_declaration_tuple, nnup<TypeBase> &&codomain)
:   TypeBase(firange)
,   m_domain_variable_declaration_tuple(std::move(domain_variable_declaration_tuple))
,   m_function_type(extract_function_type(firange, *m_domain_variable_declaration_tuple, std::move(codomain)))
{ }

// This is declared here so that the definition of FunctionType is complete.
// See http://web.archive.org/web/20140903193346/http://home.roadrunner.com/~hinnant/incomplete.html
FunctionPrototype::~FunctionPrototype () = default;

bool FunctionPrototype::equals (Base const &other_) const
{
    FunctionPrototype const &other = dynamic_cast<FunctionPrototype const &>(other_);
    return are_equal(m_domain_variable_declaration_tuple, other.m_domain_variable_declaration_tuple) &&
           are_equal(m_function_type, other.m_function_type);
}

FunctionPrototype *FunctionPrototype::cloned () const
{
    return new FunctionPrototype(firange(), clone_of(m_domain_variable_declaration_tuple), clone_of(m_function_type->codomain()));
}

void FunctionPrototype::print (Log &out) const
{
    out << "FunctionPrototype(" << firange() << '\n';
    out << IndentGuard()
        << m_domain_variable_declaration_tuple << " -> " << m_function_type->codomain() << '\n';
    out << ')';
}

void FunctionPrototype::validate_parameter_count (ParameterList const &parameter_list, FiRange const &function_call_firange) const
{
    if (parameter_list.elements().size() != m_domain_variable_declaration_tuple->elements().size())
        throw ProgrammerError(LVD_LOG_FMT(*this << " expected " << m_domain_variable_declaration_tuple->elements().size() << " parameter(s), but got " << parameter_list.elements().size()));
}

// Create a FunctionType from the parameters to FunctionPrototype constructor.
nnup<FunctionType> FunctionPrototype::extract_function_type (FiRange const &firange, DeclarationTuple const &domain_variable_declaration_tuple, nnup<TypeBase> &&codomain)
{
    // Extract the function domain tuple from domain_variable_declaration_tuple.
    auto domain = sem::make_type_tuple();
    for (auto const &domain_variable_declaration : domain_variable_declaration_tuple.elements())
    {
        assert(dynamic_cast<TypeBase const *>(&domain_variable_declaration->content()) != nullptr);
        domain->push_back(clone_of(static_cast<TypeBase const &>(domain_variable_declaration->content())));
    }

    // Construct the function type.
    return sem::make_function_type(firange, std::move(domain), std::move(codomain));
}

} // end namespace sem
} // end namespace barftest
