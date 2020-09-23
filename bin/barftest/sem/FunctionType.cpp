// 2016.08.09 - Victor Dods

#include "sem/FunctionType.hpp"

#include "cbz/cgen/Context.hpp"
#include "Exception.hpp"
#include "sem/Identifier.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace cbz {
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

void FunctionType::resolve_symbols (cgen::Context &context)
{
    m_domain->resolve_symbols(context);
    m_codomain->resolve_symbols(context);
}

llvm::PointerType *FunctionType::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    if (abstract_type != nullptr)
        *abstract_type = clone_of(this);

    std::vector<llvm::Type*> param_types;
    for (auto const &t : m_domain->elements())
        param_types.emplace_back(t->generate_rvalue_type(context));
    // 0 is the default address space.
    return llvm::PointerType::get(llvm::FunctionType::get(m_codomain->generate_rvalue_type(context), param_types, false), 0);
}

llvm::Function *FunctionType::generate_function_prototype (cgen::Context &context) const
{
    // TODO: Maybe this should handle the retrieval of any existing llvm::Function from the module.
    // YES, that would simplify Function::generate_rvalue and Function::resolve_symbols

    std::string function_name;
    sem::GlobalValueLinkage global_value_linkage = sem::GlobalValueLinkage::EXTERNAL;
    if (context.symbol_carrier().has_identifier())
    {
        function_name = context.symbol_carrier().identifier().text();
        // Determine GlobalValueLinkage.  The default linkage for functions is EXTERNAL (so that they're linkable from outside the module).
        if (context.symbol_carrier().is_decl())
            global_value_linkage = sem::global_value_linkage_determined<sem::GlobalValueLinkage::EXTERNAL>(context.symbol_carrier().as_decl().symbol_specifier().global_value_linkage_specifier().value());
    }
    else
        function_name = context.generate_unique_anonymous_function_id();

    llvm::PointerType *function_pointer_type = generate_rvalue_type(context);
    assert(function_pointer_type->getElementType()->getTypeID() == llvm::Type::FunctionTyID);
    llvm::FunctionType *function_type = static_cast<llvm::FunctionType*>(function_pointer_type->getElementType());

    std::string fqsi = context.symbol_table().fully_qualified_symbol_id(function_name, firange());
//     g_log << Log::dbg() << "FunctionType::generate_function_prototype; creating function \"" << function_name << "\" with fully qualified symbol id \"" << fqsi << "\"\n";

    std::string llvm_symbol_name = fqsi;
    auto llvm_global_value_linkage = sem::as_llvm_linkage_type(global_value_linkage);

    // Here is where GlobalValueLinkage is handled.  In particular, if GlobalValueLinkage::EXTERNAL is used,
    // then the LLVM symbol name will be the raw identifier, instead of the fully-qualified ID
    // as used within cbz.  NOTE: This particular formulation of external might be temporary,
    // where the raw identifier may be separately specified later at some point, e.g. in the
    // initialization.  Though for now that's unnecessary complexity.
    if (context.symbol_carrier().has_identifier())
    {
        // TODO: Combine this with above logic dealing with GlobalValueLinkage
        if (context.symbol_carrier().is_decl())
        {
            // NOTE: This is probably a TEMP HACK situation, since this forces the LLVM symbol name to be
            // the raw function name, but should ideally be controllable somehow, so that the cbz identifier
            // and the LLVM symbol name are decoupled.
            // NOTE: GlobalValueLinkageContextual::DETERMINE_FROM_CONTEXT (which the symbol had no specifier given for linkage)
            // can still resolve to EXTERNAL (for now, but may change).
            if (context.symbol_carrier().as_decl().symbol_specifier().global_value_linkage_specifier().value() == GlobalValueLinkageContextual::EXTERNAL)
            {
//                 g_log << Log::trc() << "FunctionType::generate_function_prototype; using raw ID for external function\n";
                llvm_symbol_name = function_name; // go back to raw name; used for external declarations.
            }
        }
        else if (context.symbol_carrier().is_init())
            LVD_ABORT("Does this ever actually occur?");
        else
            LVD_ABORT("this should be impossible");
    }

    // This function prototype is inserted into the end of the function list for that module.
    llvm::Function *function = llvm::Function::Create(
        function_type,
        llvm_global_value_linkage,
        llvm_symbol_name,
        &context.module()
    );
    assert(m_domain->elements().size() == function->arg_size());
    // The argument names are not set here.  They have to be set later in the function definition.
    return function;
}

void FunctionType::validate_parameter_count (ParameterList const &parameter_list, FiRange const &function_call_firange) const
{
    if (parameter_list.elements().size() != m_domain->elements().size())
        throw ProgrammerError(LVD_LOG_FMT("function call of type " << *this << " expected " << m_domain->elements().size() << " parameter(s), but got " << parameter_list.elements().size()), function_call_firange);
}

void FunctionType::validate_parameter_count_and_types (cgen::Context &context, ParameterList const &parameter_list, FiRange const &function_call_firange) const
{
    validate_parameter_count(parameter_list, function_call_firange);
    assert(parameter_list.elements().size() == m_domain->elements().size());
    for (size_t i = 0; i < m_domain->elements().size(); ++i)
    {
        auto const &parameter = parameter_list.elements()[i];
        auto const &domain_element = m_domain->elements()[i];

        up<TypeBase> domain_element_abstract_type;
        llvm::Type *domain_element_concrete_type = domain_element->generate_rvalue_type(context, &domain_element_abstract_type);

        up<TypeBase> parameter_abstract_type;
        llvm::Type *parameter_concrete_type = nullptr;
        if (domain_element_abstract_type->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
            parameter_concrete_type = parameter->generate_lvalue_type(context, &parameter_abstract_type);
        else
            parameter_concrete_type = parameter->generate_rvalue_type(context, &parameter_abstract_type);

        // TEMP HACK: This is only checking LLVM types, not abstract types.
        if (parameter_concrete_type != domain_element_concrete_type)
            throw TypeError(LVD_LOG_FMT("function call of type " << *this << " expected type " << domain_element_concrete_type << " for parameter " << i << " (i.e. " << domain_element_abstract_type << ") but got " << parameter << " which has type " << parameter_concrete_type << " (i.e. " << parameter_abstract_type << ')'), function_call_firange);
    }
}

} // end namespace sem
} // end namespace cbz
