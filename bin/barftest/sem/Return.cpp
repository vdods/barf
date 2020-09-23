// 2016.08.10 - Victor Dods

#include "sem/Return.hpp"

#include "cbz/cgen/Context.hpp"
#include "cbz/cgen/TypeSymbol.hpp"

namespace cbz {
namespace sem {

Return::Return (FiRange const &firange, nnup<Base> &&return_expression)
:   Base(firange)
,   m_return_expression(std::move(return_expression))
{ }

bool Return::equals (Base const &other_) const
{
    Return const &other = dynamic_cast<Return const &>(other_);
    return are_equal(m_return_expression, other.m_return_expression);
}

Return *Return::cloned () const
{
    if (m_return_expression != nullptr)
        return new Return(firange(), clone_of(m_return_expression));
    else
        return new Return(firange());
}

void Return::print (Log &out) const
{
    out << "Return(" << firange();
    if (m_return_expression != nullptr)
        out << IndentGuard() << '\n' << m_return_expression << '\n';
    out << ')';
}

void Return::resolve_symbols (cgen::Context &context)
{
    if (m_return_expression != nullptr)
        m_return_expression->resolve_symbols(context);
}

void Return::generate_code (cgen::Context &context) const
{
    llvm::Value *return_value = nullptr;
    // TODO: Check return type
    if (m_return_expression != nullptr)
    {
        // TODO: Handle void return type (return nullptr as value)
        // Use initialization semantics here.  This means that:
        // -   if the return type of the function is a reference type, then evaluate the return expression as an lvalue,
        // -   otherwise evaluate the return expression as an rvalue.
        auto const &codomain_entry = context.symbol_table().entry_of_kind<cgen::TypeSymbol>("__codomain__", firange(), cgen::SymbolState::IS_DEFINED);
        if (codomain_entry.type().abstract().type_enum__raw() == TypeEnum::REFERENCE_TYPE)
            return_value = m_return_expression->generate_lvalue(context);
        else
            return_value = m_return_expression->generate_rvalue(context);
    }
    context.ir_builder().CreateRet(return_value);
}

} // end namespace sem
} // end namespace cbz
