// 2016.08.08 - Victor Dods

#include "sem/Identifier.hpp"

#include "cbz/cgen/Context.hpp"
#include "cbz/cgen/TypeSymbol.hpp"
#include "cbz/cgen/VariableSymbol.hpp"
#include "Exception.hpp"
#include "cbz/literal.hpp"
#include "sem/ReferenceType.hpp"
#include "sem/SymbolSpecifier.hpp"
#include "llvm/IR/Instructions.h"

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

void Identifier::resolve_symbols (cgen::Context &context)
{
    // TODO: catch exception, print error message

    // TODO: This will be used after the refactor of declaration/assigment identifier to use stack
    // is implemented and the corresponding change is made to generate_initialization_for_value_function_constant
//     auto const &entry = context.symbol_table().entry(m_text, firange());
//     m_text = entry.fully_qualified_id();

    // TEMP HACK
    if (is_compound_string(m_text))
    {
        auto const &entry = context.symbol_table().entry(m_text, firange());
        m_text = entry.fully_qualified_id();
    }
    else
    {
        // The reason fully_qualified_symbol_id is used is because that doesn't expect the symbol
        // to be declared yet.
        m_text = context.symbol_table().fully_qualified_symbol_id(m_text, firange());
    }
}

TypeEnum Identifier::type_enum__resolved (cgen::Context &context) const
{
    // TODO: catch exception, print error message
    auto &entry = context.symbol_table().entry_of_kind<cgen::TypeSymbol>(m_text, firange(), cgen::SymbolState::IS_DEFINED);
    return entry.type().abstract().type_enum__raw();
}

ExpressionKind Identifier::generate_expression_kind (cgen::Context &context) const
{
    // It's just whatever the expression_kind of the symbol is.
    return context.symbol_table().entry(m_text, firange()).expression_kind();
}

Determinability Identifier::generate_determinability (cgen::Context &context) const
{
    // It's just whatever the determinability of the symbol is.
    return context.symbol_table().entry(m_text, firange()).determinability();
}

llvm::Type *Identifier::generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    // TODO: catch exception, print error message

    // TODO: retrieve the entry without any requirement, then
    // if entry.symbol_enum() is variable, require that the entry IS_DECLARED
    // if entry.symbol_enum() is type, require that the entry IS_DEFINED

    auto &entry = context.symbol_table().entry_of_kind<cgen::TypedSymbol>(m_text, firange(), cgen::SymbolState::IS_DECLARED);
    assert(entry.type().is_set());
    assert(entry.type().abstract_as_unique_ptr_ref() != nullptr);
    assert(entry.type().concrete() != nullptr);

    // TODO: If it could be guaranteed that a declared symbol would necessarily have
    // a non-null concrete value (e.g. AllocaInst, Function, Constant, etc), then this
    // code could be much simplified.

    up<TypeBase> symbol_abstract_type;
    llvm::Type *symbol_concrete_type = entry.type().abstract().generate_rvalue_type(context, &symbol_abstract_type);
    // If the symbol is a reference or function, then return it directly.
    if (symbol_abstract_type->type_enum__raw() == TypeEnum::REFERENCE_TYPE || symbol_abstract_type->type_enum__raw() == TypeEnum::FUNCTION_TYPE)
    {
        if (abstract_type != nullptr)
            *abstract_type = std::move(symbol_abstract_type);
        return symbol_concrete_type;
    }
    // Otherwise the symbol is a variable, so create a reference to the symbol.
    else
    {
        if (abstract_type != nullptr)
            *abstract_type = make_reference_type(firange(), clone_of(entry.type().abstract()));
        assert(entry.type().concrete() == symbol_concrete_type);
        // For e.g. an AllocaInst, this would give a pointer to the allocated type.
        return llvm::PointerType::get(symbol_concrete_type, 0);
    }
}

llvm::Value *Identifier::generate_lvalue (cgen::Context &context) const
{
    // TODO: catch exception, print error message

    // TEMP HACK: Because in-line global var initialization within functions isn't yet implemented (and
    // it's possible that some global vars will never be initialized that way), don't require IS_DEFINED
    // at first.  Handle GLOBAL vars separately.

    // TODO: Produce a more sensible error message if the identifier isn't a variable.

//     auto &entry = context.symbol_table().entry_of_kind<cgen::VariableSymbol>(m_text, firange(), cgen::SymbolState::IS_DEFINED);
    auto &entry = context.symbol_table().entry_of_kind<cgen::VariableSymbol>(m_text, firange());
    assert(entry.value().concrete() != nullptr);

    llvm::Value *value = entry.value().concrete();

    // References are syntactic sugar that operate at the level of Identifier as a special case.
    // If x is a reference, then using x as an rvalue really means to dereference its underlying
    // pointer value.
    // NOTE: This should be valid for the local var and the global/const case, but I'm not
    // sure about the function case -- though this conditional should not be triggered by functions.
    {
        up<TypeBase> abstract_type;
        llvm::Type *concrete_type = entry.type().abstract().generate_rvalue_type(context, &abstract_type);
        if (abstract_type->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
        {
            assert(llvm::isa<llvm::PointerType>(concrete_type));
            // Apply another Load instruction in order to dereference the underlying pointer.
            value = context.ir_builder().CreateLoad(value, "DEREFtmp");
        }
    }

    return value;
}

llvm::Type *Identifier::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    // TODO: catch exception, print error message

    // TODO: retrieve the entry without any requirement, then
    // if entry.symbol_enum() is variable, require that the entry IS_DECLARED
    // if entry.symbol_enum() is type, require that the entry IS_DEFINED

    auto &entry = context.symbol_table().entry_of_kind<cgen::TypedSymbol>(m_text, firange(), cgen::SymbolState::IS_DECLARED);
    assert(entry.type().is_set());
    assert(entry.type().abstract_as_unique_ptr_ref() != nullptr);
    assert(entry.type().concrete() != nullptr);

    up<TypeBase> entry_abstract_type;
    // This is meant to handle typedefs, etc.
    llvm::Type *entry_concrete_type = entry.type().abstract().generate_rvalue_type(context, &entry_abstract_type);
    assert(entry_concrete_type == entry.type().concrete()); // Consistency check
    if (entry_abstract_type->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
    {
        ReferenceType &entry_reference_type = dynamic_cast<ReferenceType &>(*entry_abstract_type);
        if (abstract_type != nullptr)
            *abstract_type = clone_of(entry_reference_type.referent());

        // A reference as an rvalue is transparent and produces the thing that the reference refers to.
        assert(llvm::isa<llvm::PointerType>(entry.type().concrete()));
        return llvm::cast<llvm::PointerType>(entry.type().concrete())->getElementType();
    }
    else
    {
        if (abstract_type != nullptr)
            *abstract_type = std::move(entry_abstract_type);

        // Just return the symbol type directly.
        return entry.type().concrete();
    }
}

llvm::Value *Identifier::generate_rvalue (cgen::Context &context) const
{
    // TODO: catch exception, print error message

    // TEMP HACK: Because in-line global var initialization within functions isn't yet implemented (and
    // it's possible that some global vars will never be initialized that way), don't require IS_DEFINED
    // at first.  Handle GLOBAL vars separately.

    auto &entry = context.symbol_table().entry_of_kind<cgen::ValueSymbol>(m_text, firange());

//     g_log << Log::trc() << "Identifier::generate_rvalue (" << firange() << ");\n";
//     IndentGuard ig(g_log);
//     g_log << Log::trc() << LVD_REFLECT(entry) << '\n';

    // NOTE/TODO: The below cases seem very piecemeal.  See if there's a way to consolidate these
    // discrepancies and simplify this.

    assert(entry.value().concrete() != nullptr);
    llvm::Value *value = nullptr;
    if (llvm::isa<llvm::AllocaInst>(entry.value().concrete()))
    {
//         g_log << Log::trc() << "entry " << entry.fully_qualified_id() << " is an llvm::AllocaInst; " << LVD_REFLECT(entry.value().concrete()->getType()) << '\n';
        // AllocaInst means that this is a local var.
        value = context.ir_builder().CreateLoad(entry.value().concrete(), m_text.c_str());
        assert(value->getType() == llvm::cast<llvm::AllocaInst>(entry.value().concrete())->getAllocatedType());
    }
    else if (llvm::isa<llvm::Function>(entry.value().concrete()))
    {
//         g_log << Log::trc() << "entry " << entry.fully_qualified_id() << " is an llvm::Function; " << LVD_REFLECT(entry.value().concrete()->getType()) << '\n';
        value = entry.value().concrete();
    }
    else
    {
        // This is the case for global vars and constants.
//         g_log << Log::trc() << "entry " << entry.fully_qualified_id() << " is an llvm constant or global; " << LVD_REFLECT(entry.value().concrete()->getType()) << '\n';
        value = context.ir_builder().CreateLoad(entry.value().concrete(), m_text.c_str());
    }

    // References are syntactic sugar that operate at the level of Identifier as a special case.
    // If x is a reference, then using x as an rvalue really means to dereference its underlying
    // pointer value.
    // NOTE: This should be valid for the local var and the global/const case, but I'm not
    // sure about the function case -- though this conditional should not be triggered by functions.
    {
        up<TypeBase> abstract_type;
        llvm::Type *concrete_type = entry.type().abstract().generate_rvalue_type(context, &abstract_type);
//         g_log << Log::trc() << "checking if it's a reference...\n" << LVD_REFLECT(abstract_type) << '\n' << LVD_REFLECT(concrete_type) << '\n' << LVD_REFLECT(value->getType()) << '\n';
        if (abstract_type->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
        {
            assert(llvm::isa<llvm::PointerType>(concrete_type));
            // Apply another Load instruction in order to dereference the underlying pointer.
            value = context.ir_builder().CreateLoad(value, "DEREFtmp");
        }
    }

//     assert(value->getType() == entry.type().concrete()); // TODO: Get this working some day (probably some cases are involved)

    return value;
}

nnup<SymbolSpecifier> Identifier::generate_svalue (cgen::Context &context) const
{
    return make_symbol_specifier(
        clone_of(this),
        make_value_kind_specifier(firange(), ValueKindContextual::DETERMINE_FROM_CONTEXT),
        make_value_lifetime_specifier(firange(), ValueLifetimeContextual::DETERMINE_FROM_CONTEXT),
        make_global_value_linkage_specifier(firange(), GlobalValueLinkageContextual::DETERMINE_FROM_CONTEXT)
    );
}

void Identifier::generate_code (cgen::Context &context) const
{
    context.symbol_table().entry(m_text, firange());
    throw ProgrammerError("this statement has no effect", firange());
}

} // end namespace sem
} // end namespace cbz
