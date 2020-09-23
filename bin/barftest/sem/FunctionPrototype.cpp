// 2019.04.25 - Victor Dods

#include "sem/FunctionPrototype.hpp"

#include "cbz/cgen/Context.hpp"
#include "Exception.hpp"
#include "cbz/LLVMUtil.hpp"
#include "sem/FunctionType.hpp"
#include "sem/Identifier.hpp"
#include "sem/SymbolSpecifier.hpp"
#include "sem/ValueLifetime.hpp"

namespace cbz {
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

void FunctionPrototype::resolve_symbols (cgen::Context &context)
{
    m_domain_variable_declaration_tuple->resolve_symbols(context);
    m_function_type->resolve_symbols(context);
}

llvm::PointerType *FunctionPrototype::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    return m_function_type->generate_rvalue_type(context, abstract_type);
}

llvm::Function *FunctionPrototype::generate_function_prototype (cgen::Context &context) const
{
    // Check that the declaration tuple's elements all have DETERMINE_FROM_CONTEXT scope specifier.
    for (auto const &arg : m_domain_variable_declaration_tuple->elements())
        if (arg->symbol_specifier().value_lifetime_specifier().value() != sem::ValueLifetimeContextual::DETERMINE_FROM_CONTEXT)
            throw ProgrammerError("function parameter declarations must not have a scope specifier (e.g. `local`, `global`)", arg->firange());

    llvm::Function *function = m_function_type->generate_function_prototype(context);
    std::size_t i = 0;
    for (auto &arg : function->args())
        arg.setName(m_domain_variable_declaration_tuple->elements()[i++]->symbol_specifier().id().text());
    return function;
}

void FunctionPrototype::validate_parameter_count (ParameterList const &parameter_list, FiRange const &function_call_firange) const
{
    if (parameter_list.elements().size() != m_domain_variable_declaration_tuple->elements().size())
        throw ProgrammerError(LVD_LOG_FMT(*this << " expected " << m_domain_variable_declaration_tuple->elements().size() << " parameter(s), but got " << parameter_list.elements().size()));
}

void FunctionPrototype::validate_parameter_count_and_types (cgen::Context &context, ParameterList const &parameter_list, FiRange const &function_call_firange) const
{
    validate_parameter_count(parameter_list, function_call_firange);
    assert(parameter_list.elements().size() == m_domain_variable_declaration_tuple->elements().size());
    for (size_t i = 0; i < m_domain_variable_declaration_tuple->elements().size(); ++i)
    {
        auto const &parameter = parameter_list.elements()[i];
        auto const &declaration = m_domain_variable_declaration_tuple->elements()[i];
        // NOTE: This won't work in general, because the current context may be different than the on in which the domain element was declared.

        up<TypeBase> declaration_abstract_type;
        llvm::Type *declaration_concrete_type = declaration->generate_rvalue_type(context, &declaration_abstract_type);

        up<TypeBase> parameter_abstract_type;
        llvm::Type *parameter_concrete_type = nullptr;
        if (declaration_abstract_type->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
            parameter_concrete_type = parameter->generate_lvalue_type(context, &parameter_abstract_type);
        else
            parameter_concrete_type = parameter->generate_rvalue_type(context, &parameter_abstract_type);

        // TEMP HACK: This is only checking LLVM types, not abstract types.
        if (parameter_concrete_type != declaration_concrete_type)
            throw TypeError(LVD_LOG_FMT("function call of type " << *this << " expected type " << declaration_concrete_type << " for parameter " << i << " (i.e. " << *declaration << ") but got " << parameter << " which has type " << parameter_concrete_type << " (i.e. " << parameter_abstract_type << ')'), function_call_firange);
    }
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
} // end namespace cbz
