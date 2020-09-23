// 2016.08.09 - Victor Dods

#include "sem/FunctionEvaluation.hpp"

#include "cbz/cgen/Context.hpp"
#include "cbz/cgen/FunctionConstantSymbol.hpp"
#include "Exception.hpp"
#include "sem/Function.hpp"
#include "sem/FunctionPrototype.hpp"
#include "sem/FunctionType.hpp"
#include "llvm/IR/DerivedTypes.h"

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

void FunctionEvaluation::resolve_symbols (cgen::Context &context)
{
    m_function_expression->resolve_symbols(context);
    m_parameter_list->resolve_symbols(context);
}

Determinability FunctionEvaluation::generate_determinability (cgen::Context &context) const
{
    auto f = m_function_expression->generate_determinability(context);
    auto p = m_parameter_list->generate_determinability(context);
    // All must be COMPILETIME in order for this expression to be COMPILETIME
    if (f == Determinability::COMPILETIME && p == Determinability::COMPILETIME)
        return Determinability::COMPILETIME;
    else
        return Determinability::RUNTIME;
}

llvm::Type *FunctionEvaluation::generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    // NOTE: This particular formulation assumes that resolve_symbols have already been called
    // so that the typedefs resolve properly, and not to some local typedefs that have nothing
    // to do with e.g. a global function.
    up<TypeBase> function_expression_abstract_type_;
    llvm::Type *function_expression_concrete_type = m_function_expression->generate_rvalue_type(context, &function_expression_abstract_type_);
    assert(function_expression_abstract_type_ != nullptr);

    FunctionType *function_expression_abstract_type = dynamic_cast<FunctionType *>(function_expression_abstract_type_.get());
    if (function_expression_abstract_type == nullptr)
        LVD_ABORT_WITH_FIRANGE("expected function expression abstract type to be FunctionType", m_function_expression->firange());

    llvm::Type *return_type = function_expression_abstract_type->codomain().generate_rvalue_type(context, abstract_type);

    // Do a bunch of sanity checking.
    if (function_expression_concrete_type->getTypeID() != llvm::Type::PointerTyID)
        LVD_ABORT_WITH_FIRANGE("expected function expression to have [function] pointer type", m_function_expression->firange());
    llvm::PointerType *function_expression_type_ = static_cast<llvm::PointerType*>(function_expression_concrete_type);
    if (function_expression_type_->getElementType()->getTypeID() != llvm::Type::FunctionTyID)
        LVD_ABORT_WITH_FIRANGE("expected function expression to have function pointer type", m_function_expression->firange());
    llvm::FunctionType *function_type = static_cast<llvm::FunctionType*>(function_expression_type_->getElementType());
    assert(function_type->getReturnType() == return_type);

    return return_type;
}

llvm::Value *FunctionEvaluation::generate_lvalue (cgen::Context &context) const
{
    // TEMP HACK: require that m_function_expression be an sem::Identifier
    if (m_function_expression->type_enum__raw() != TypeEnum::IDENTIFIER)
        LVD_ABORT_WITH_FIRANGE("Function expression must be a single identifier", m_function_expression->firange());

    auto function_id = dynamic_cast<Identifier const &>(*m_function_expression);

    // NOTE: This used to be a call `function = context.module().getFunction(function_id.text());`
    auto &entry = context.symbol_table().entry_of_kind<cgen::FunctionConstantSymbol>(function_id.text(), function_id.firange(), cgen::SymbolState::IS_DECLARED);
    llvm::Function *function = entry.value().concrete_as<llvm::Function>();

    up<TypeBase> function_expression_abstract_type_;
    m_function_expression->generate_rvalue_type(context, &function_expression_abstract_type_);
    assert(function_expression_abstract_type_ != nullptr);

    FunctionType *function_expression_abstract_type = dynamic_cast<FunctionType *>(function_expression_abstract_type_.get());
    if (function_expression_abstract_type == nullptr)
        LVD_ABORT_WITH_FIRANGE("expected function expression abstract type to be FunctionType", m_function_expression->firange());

    function_expression_abstract_type->validate_parameter_count_and_types(context, *m_parameter_list, firange());

    std::vector<llvm::Value*> parameters;
    for (size_t i = 0; i < m_parameter_list->elements().size(); ++i)
    {
        auto const &parameter = m_parameter_list->elements()[i];
        auto const &domain_element = function_expression_abstract_type->domain().elements()[i];
        // ReferenceType'd parameters get special treatment, where generate_lvalue is called
        // on the expression given for that parameter.  Otherwise, use generate_rvalue.
        llvm::Value *parameter_value = nullptr;
//         g_log << Log::trc() << "parameter " << i << ":\n";
//         g_log << Log::trc() << IndentGuard() << LVD_REFLECT(domain_element) << '\n';
//         g_log << Log::trc() << IndentGuard() << LVD_REFLECT(parameter) << '\n';

        up<TypeBase> parameter_abstract_type;
//         llvm::Type *parameter_concrete_type = nullptr;
        if (domain_element->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
        {
//             parameter_concrete_type =
            parameter->generate_lvalue_type(context, &parameter_abstract_type);
            parameter_value = parameter->generate_lvalue(context);
        }
        else
        {
//             parameter_concrete_type =
            parameter->generate_rvalue_type(context, &parameter_abstract_type);
            parameter_value = parameter->generate_rvalue(context);
        }

//         g_log << Log::trc() << IndentGuard() << LVD_REFLECT(parameter_abstract_type) << '\n';
//         g_log << Log::trc() << IndentGuard() << LVD_REFLECT(parameter_concrete_type) << '\n';
//         g_log << Log::trc() << IndentGuard() << LVD_REFLECT(parameter_value->getType()) << '\n';

        parameters.emplace_back(parameter_value);
    }

    if (function_expression_abstract_type->codomain().type_enum__raw() == TypeEnum::VOID_TYPE)
        return context.ir_builder().CreateCall(function, parameters);
    else
        return context.ir_builder().CreateCall(function, parameters, "calltmp");
}

llvm::Type *FunctionEvaluation::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    up<TypeBase> function_expression_abstract_type_;
    llvm::Type *function_expression_concrete_type = m_function_expression->generate_rvalue_type(context, &function_expression_abstract_type_);
    assert(function_expression_abstract_type_ != nullptr);

    FunctionType *function_expression_abstract_type = dynamic_cast<FunctionType *>(function_expression_abstract_type_.get());
    if (function_expression_abstract_type == nullptr)
        LVD_ABORT_WITH_FIRANGE("expected function expression abstract type to be FunctionType", m_function_expression->firange());

    if (abstract_type != nullptr)
        // TODO: This clone_of is technically unnecessary -- ideally this would steal/move
        // function_expression_abstract_type->codomain(), since function_expression_abstract_type_
        // is garbage anyway, but for now we have no access to the underlying codomain nnup.
        *abstract_type = clone_of(function_expression_abstract_type->codomain());

    if (function_expression_concrete_type->getTypeID() != llvm::Type::PointerTyID)
        LVD_ABORT_WITH_FIRANGE("expected function expression to have [function] pointer type", m_function_expression->firange());
    llvm::PointerType *function_expression_type_ = static_cast<llvm::PointerType*>(function_expression_concrete_type);
    if (function_expression_type_->getElementType()->getTypeID() != llvm::Type::FunctionTyID)
        LVD_ABORT_WITH_FIRANGE("expected function expression to have function pointer type", m_function_expression->firange());
    llvm::FunctionType *function_type = static_cast<llvm::FunctionType*>(function_expression_type_->getElementType());
    return function_type->getReturnType();
}

llvm::Value *FunctionEvaluation::generate_rvalue (cgen::Context &context) const
{
//     g_log << Log::dbg() << "FunctionEvaluation::generate_rvalue; m_function_expression = " << m_function_expression << '\n';

    up<TypeBase> function_expression_abstract_type;
    llvm::Type *function_expression_concrete_type = m_function_expression->generate_rvalue_type(context, &function_expression_abstract_type);
    llvm::Value *function_expression_value = m_function_expression->generate_rvalue(context);
    assert(function_expression_value->getType() == function_expression_concrete_type);

    // Check that the call is well-formed.
    if (function_expression_abstract_type->type_enum__raw() != TypeEnum::FUNCTION_TYPE)
        throw ProgrammerError("can't call an expression which does not evaluate to a function", m_function_expression->firange());
    auto function_type_abstract = dynamic_cast<FunctionType const *>(function_expression_abstract_type.get());
    assert(function_type_abstract != nullptr);
    function_type_abstract->validate_parameter_count_and_types(context, *m_parameter_list, firange());

//     g_log << Log::trc() << "FunctionEvaluation::generate_rvalue\n";
//     IndentGuard ig(g_log);
//     g_log << Log::trc()
//           << LVD_REFLECT(m_function_expression) << '\n'
//           << LVD_REFLECT(function_expression_concrete_type) << '\n';

    std::vector<llvm::Value*> parameters;
    for (size_t i = 0; i < m_parameter_list->elements().size(); ++i)
    {
        auto const &parameter = m_parameter_list->elements()[i];
        auto const &domain_element = function_type_abstract->domain().elements()[i];
        // ReferenceType'd parameters get special treatment, where generate_lvalue is called
        // on the expression given for that parameter.  Otherwise, use generate_rvalue.
        llvm::Value *parameter_value = nullptr;
//         g_log << Log::trc() << "parameter " << i << ":\n";
//         g_log << Log::trc() << IndentGuard() << LVD_REFLECT(domain_element) << '\n';
//         g_log << Log::trc() << IndentGuard() << LVD_REFLECT(parameter) << '\n';

        up<TypeBase> parameter_abstract_type;
//         llvm::Type *parameter_concrete_type = nullptr;
        if (domain_element->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
        {
//             parameter_concrete_type =
            parameter->generate_lvalue_type(context, &parameter_abstract_type);
            parameter_value = parameter->generate_lvalue(context);
        }
        else
        {
//             parameter_concrete_type =
            parameter->generate_rvalue_type(context, &parameter_abstract_type);
            parameter_value = parameter->generate_rvalue(context);
        }

//         g_log << Log::trc() << IndentGuard() << LVD_REFLECT(parameter_abstract_type) << '\n';
//         g_log << Log::trc() << IndentGuard() << LVD_REFLECT(parameter_concrete_type) << '\n';
//         g_log << Log::trc() << IndentGuard() << LVD_REFLECT(parameter_value->getType()) << '\n';

        parameters.emplace_back(parameter_value);
    }

    if (function_type_abstract->codomain().type_enum__raw() == TypeEnum::VOID_TYPE)
        return context.ir_builder().CreateCall(function_expression_value, parameters);
    else
        return context.ir_builder().CreateCall(function_expression_value, parameters, "calltmp");
}

void FunctionEvaluation::generate_code (cgen::Context &context) const
{
    // The value is ignored; this is just for the side effects.
    generate_rvalue(context);
}

} // end namespace sem
} // end namespace cbz
