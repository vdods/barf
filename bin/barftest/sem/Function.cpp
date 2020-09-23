// 2016.08.09 - Victor Dods

#include "sem/Function.hpp"

#include "cbz/cgen/Context.hpp"
#include "cbz/cgen/FunctionConstantSymbol.hpp"
#include "cbz/cgen/SymbolTable.hpp"
#include "cbz/cgen/TypeSymbol.hpp"
#include "cbz/cgen/VariableSymbol.hpp"
#include "Exception.hpp"
#include "cbz/LLVMUtil.hpp"
#include "sem/FunctionPrototype.hpp"
#include "sem/SymbolSpecifier.hpp"
#include "sem/ValueLifetime.hpp"
#include <iostream> // TEMP
#include "llvm/IR/Argument.h"
#include "llvm/IR/Verifier.h"

namespace cbz {
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

void Function::resolve_symbols (cgen::Context &context)
{
    // Do the resolve_symbols first on m_function_prototype (since it exists outside of its own scope)
    m_function_prototype->resolve_symbols(context);

    // NOTE: This has a lot in common with generate_rvalue; try to factor out the common code.

    // Most of the code below is just to set up the correct scope in the context object.

    std::string function_name;
    if (context.symbol_carrier().has_identifier())
        function_name = context.symbol_carrier().identifier().text();
    else
        LVD_ABORT_WITH_FIRANGE(LVD_FMT("in Function::resolve_symbols: this (which is probably 'completely anonymous functions') is not yet supported; SymbolCarrier firange: " << context.symbol_carrier().firange()), firange());

    // Push SymbolCarrierEmpty so that any identified SymbolCarrier on the top of
    // the stack isn't used in nested code generation.
    auto symbol_carrier_guard = context.push_symbol_carrier(make_nnup<cgen::SymbolCarrierEmpty>(m_body->firange()));

    auto &function_constant_symbol = context.symbol_table().entry_of_kind<cgen::FunctionConstantSymbol>(function_name, firange());
//     g_log << Log::dbg() << "Function::resolve_symbols -- function_constant_symbol.fully_qualified_id() = " << function_constant_symbol.fully_qualified_id() << ", pushing scope.\n";
    // Push the scope -- the IRBuilder and SymbolTable have already been created.  The IRBuilder already has a
    // llvm::BasicBlock associated with it which is the "function_constant_symbol block" for the function.
    // NOTE: It's possible that m_function_prototype needs to be generate_rvalue_type'd before this call.
    auto scope_guard = context.push_scope(function_constant_symbol.ir_builder(), function_constant_symbol.symbol_table());

    // Now that the scope is context object's scope is correctly set up, do the resolve_symbols.
    m_body->resolve_symbols(context);
}

Determinability Function::generate_determinability (cgen::Context &context) const
{
    // NOTE: The real implementation is basically a check for side-effects.  Only a pure function
    // can be COMPILETIME (it could use global vars, but those vars have to be COMPILETIME).  A
    // function that accepts input or produces output can't be COMPILETIME in this sense.
//     g_log << Log::wrn() << "WARNING: Function::generate_determinability not fully implemented; to be safe, it assumes that every function has side-effects (this is false in general) so that it doesn't make invalid optimizations or compile-time determinations.\n";
    return Determinability::RUNTIME;
}

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of the function.
/// This is used for mutable variables etc.
// This is probably not really necessary -- it forces all stack vars to be alloca'ed in the
// entry block of the function, but that's not really appropriate for when there are scopes
// that limit the lifetime of the stack vars.
llvm::AllocaInst *CreateEntryBlockAlloca (llvm::Function *function, std::string const &var_name, llvm::Type *type) {
    llvm::IRBuilder<> builder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return builder.CreateAlloca(type, nullptr, var_name);
}

llvm::PointerType *Function::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    return m_function_prototype->generate_rvalue_type(context, abstract_type);
}

llvm::Function *Function::generate_rvalue (cgen::Context &context) const
{
    std::string function_name;
    llvm::Function *function = nullptr;

    if (context.symbol_carrier().has_identifier())
    {
        auto const &id = context.symbol_carrier().identifier();
        function_name = id.text();

        if (context.symbol_carrier().is_decl())
        {
            assert(!context.symbol_carrier().is_init() && "TEMP HACK for now, assume SymbolCarrierFlags::IS_DECL and IS_INIT are mutually exclusive");

            std::string fqsn = context.symbol_table().fully_qualified_symbol_id(function_name, id.firange());
//             g_log << Log::trc() << "Function::generate_rvalue; id = " << id << ", fully qualified symbol name = " << fqsn << '\n';
            function = m_function_prototype->generate_function_prototype(context);
            assert(function != nullptr);
        }

        if (context.symbol_carrier().is_init())
        {
            assert(!context.symbol_carrier().is_decl() && "TEMP HACK for now, assume SymbolCarrierFlags::IS_DECL and IS_INIT are mutually exclusive");

            auto &entry = context.symbol_table().entry_of_kind<cgen::FunctionConstantSymbol>(function_name, id.firange());
//             g_log << Log::trc() << "Function::generate_rvalue; id = " << id << ", fully qualified symbol id = " << entry.fully_qualified_id() << '\n';
            function = entry.value().concrete_as<llvm::Function>();
            assert(function != nullptr);
        }
    }
    else
        LVD_ABORT_WITH_FIRANGE("in Function::generate_rvalue: this (which is probably 'completely anonymous functions') is not yet supported", firange());

    assert(function != nullptr);
    assert(!function_name.empty());

    // Check that the function type for the body matches the declared type.
    llvm::PointerType *function_pointer_type = m_function_prototype->generate_rvalue_type(context);
    // TODO: Improve the error message by including the conflicting declaration's location.
    if (function->getType() != function_pointer_type)
        throw ProgrammerError(LVD_LOG_FMT("return type or parameter type(s) in function definition did not match that of function declaration"), firange());

    // Push SymbolCarrierEmpty so that any identified SymbolCarrier on the top of
    // the stack isn't used in nested code generation.
    auto symbol_carrier_guard = context.push_symbol_carrier(make_nnup<cgen::SymbolCarrierEmpty>(m_body->firange()));

    auto &function_constant_symbol = context.symbol_table().entry_of_kind<cgen::FunctionConstantSymbol>(function_name, firange());

    assert(function_constant_symbol.has_ir_builder() == function_constant_symbol.has_symbol_table());
    assert(!function_constant_symbol.has_ir_builder());
    // Create the IRBuilder and SymbolTable.
    {
//         g_log << Log::trc() << "Creating IRBuilder and SymbolTable for " << function_constant_symbol.fully_qualified_id() << '\n';
        // The llvm::Function* value is the concrete value of the symbol.
        llvm::Function *function = function_constant_symbol.value().concrete_as<llvm::Function>();
        assert(function != nullptr);
        // Create an entry block for use in the llvm::IRBuilder<> that will be associated with entry.
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(context.llvm_context(), "entry", function);
        function_constant_symbol.set_ir_builder_and_symbol_table(
            std::make_unique<llvm::IRBuilder<>>(bb, bb->end()),
            std::make_unique<cgen::SymbolTable>(cgen::SymbolTableKind::FUNCTION, function_constant_symbol.id(), &context.symbol_table())
        );
    }

    // Create the __codomain__ typedef BEFORE push_scope, so that the resolution (if we need to use it) will work correctly.
    llvm::Type *codomain_concrete_type = m_function_prototype->function_type().codomain().generate_rvalue_type(context);
    cgen::TypeSymbol &codomain_type_symbol = function_constant_symbol.symbol_table().define_type(
        "__codomain__",
        m_function_prototype->function_type().codomain().firange(),
        cgen::SymbolType(
            clone_of(m_function_prototype->function_type().codomain()),
            codomain_concrete_type
        )
    );

//     g_log << Log::dbg() << "Function::generate_rvalue -- function_constant_symbol.fully_qualified_id() = " << function_constant_symbol.fully_qualified_id() << ", pushing scope.\n";
    // Push the scope -- the IRBuilder and SymbolTable have already been created.  The IRBuilder already has a
    // llvm::BasicBlock associated with it which is the "function_constant_symbol block" for the function.
    // NOTE: It's possible that m_function_prototype needs to be generate_rvalue_type'd before this call.
    auto scope_guard = context.push_scope(function_constant_symbol.ir_builder(), function_constant_symbol.symbol_table());

    std::size_t i = 0;
    for (llvm::Argument &arg : function->args())
    {
        auto const &param_id = m_function_prototype->domain_variable_declaration_tuple().elements()[i]->symbol_specifier().id();
        // TODO: Maybe warn if the names don't match up.  But only do this for each named argument.
        // Unnamed arguments appear to have an empty name.
        arg.setName(param_id.text());
        // Create an alloca for this param, store the initial value into the alloca, and store in the symbol table.
        // NOTE: This causes the alloca instructions to appear in reverse order as the args.
        // TODO: Fix this probably.
//         llvm::AllocaInst *alloca = CreateEntryBlockAlloca(function, arg.getName(), arg.getType());
        llvm::AllocaInst *alloca = context.ir_builder().CreateAlloca(arg.getType(), nullptr, arg.getName());
        context.ir_builder().CreateStore(&arg, alloca);

        up<TypeBase> arg_abstract_type;
        llvm::Type *arg_type = m_function_prototype->domain_variable_declaration_tuple().elements()[i]->content().generate_rvalue_type(context, &arg_abstract_type);
        assert(arg_type == arg.getType());
        assert(arg_abstract_type != nullptr);

        // Technically the value passed as definition_firange is the declaration_firange, but there's
        // no way to get the definition_firange, since that's not known except at the (many) function
        // call site(s).
        auto &variable_symbol = context.symbol_table().define_variable(
            param_id.text(),
            sem::GlobalValueLinkage::INTERNAL, // GlobalValueLinkage doesn't really apply to local vars.
            m_function_prototype->domain_variable_declaration_tuple().elements()[i]->firange(),
            sem::ValueLifetime::LOCAL, // Function args are always LOCAL
            cgen::SymbolType(std::move(arg_abstract_type), arg.getType()),
            cgen::SymbolValue<Base,llvm::Value>(nullptr, alloca) // Leave abstract value unset for now.
        );
        assert(variable_symbol.is_defined());
        ++i;
    }

    // Generate code for the body of the function.
    m_body->generate_code(context);

    // If the current insert point BasicBlock doesn't have a terminator (i.e. return statement)...
    assert(context.ir_builder().GetInsertBlock() != nullptr);
    if (context.ir_builder().GetInsertBlock()->getTerminator() == nullptr)
    {
        // If the return type is Void, then this is ok -- just add a return instruction.
        if (codomain_type_symbol.type().abstract().type_enum__raw() == TypeEnum::VOID_TYPE)
            context.ir_builder().CreateRet(nullptr);
        // Otherwise this is an error.
        else
            throw ProgrammerError(LVD_FMT("function body for " << function_constant_symbol.fully_qualified_id() << " is missing a return statement"), m_body->firange().end_as_firange());
    }

    // Print out the SymbolTable for debugging purposes
    g_log << Log::trc() << "Function::generate_rvalue; finished generating code for " << function_constant_symbol.fully_qualified_id() << '\n'
          << IndentGuard() << context.symbol_table() << '\n';

    // Validate the generated code.  TODO: Maybe this should be done as a second pass?
    if (true)
    {
        std::string s;
        llvm::raw_string_ostream rso(s); // Note: Have to call flush on rso to populate s.  Otherwise use rso.str().
        if (llvm::verifyFunction(*function, &rso))
        {
            g_log << Log::err() << "Function verification failed: " << rso.str() << '\n'
                  << function << '\n';
            throw ProgrammerError("Function verification failed: " + rso.str(), firange());
        }
    }

    {
        IndentGuard ig(g_log);

        g_log << Log::trc() << "Function " << function << " BEFORE optimization:"
              << IndentGuard() << function;

        // Run optimizer on function
        context.fpm().run(*function);

        g_log << Log::trc() << "Function " << function << " AFTER optimization:"
              << IndentGuard() << function;
    }

    return function;
}

} // end namespace sem
} // end namespace cbz
