// 2016.08.09 - Victor Dods

#include "sem/Assignment.hpp"

#include "cbz/cgen/Context.hpp"
#include "cbz/cgen/Generation.hpp"
#include "Exception.hpp"
#include "cbz/LLVMUtil.hpp"
#include "sem/Identifier.hpp"
#include "sem/Type.hpp"
#include "llvm/IR/Instructions.h"

namespace cbz {
namespace sem {

bool Assignment::equals (Base const &other_) const
{
    Assignment const &other = dynamic_cast<Assignment const &>(other_);
    return are_equal(m_target, other.m_target) && are_equal(m_content, other.m_content);
}

void Assignment::print (Log &out) const
{
    out << "Assignment(" << firange() << '\n';
    out << IndentGuard()
        << m_target << " = " << m_content << '\n';
    out << ')';
}

Assignment *Assignment::cloned () const
{
    return new Assignment(firange(), clone_of(m_target), clone_of(m_content));
}

void Assignment::resolve_symbols (cgen::Context &context)
{
    m_target->resolve_symbols(context);
    m_content->resolve_symbols(context);
}

void Assignment::generate_code (cgen::Context &context) const
{
//     g_log << Log::trc() << "Assignment::generate_code\n";
//     IndentGuard ig(g_log);
//
//     g_log << Log::trc()
//           << LVD_REFLECT(m_target) << '\n'
//           << LVD_REFLECT(m_content) << '\n';

    up<TypeBase> target_lvalue_abstract_type;
//     llvm::Type *target_lvalue_concrete_type =
    m_target->generate_lvalue_type(context, &target_lvalue_abstract_type);
//     g_log << Log::trc()
//           << LVD_REFLECT(target_lvalue_abstract_type) << '\n'
//           << LVD_REFLECT(target_lvalue_concrete_type) << '\n';

    llvm::IRBuilder<> *ir_builder = &context.ir_builder();

    llvm::Value *target_lvalue = m_target->generate_lvalue(context);
//     g_log << Log::trc() << LVD_REFLECT(target_lvalue->getType()) << '\n';

    llvm::Value *content_rvalue = m_content->generate_rvalue(context);
    assert(llvm::isa<llvm::PointerType>(target_lvalue->getType()));
//     g_log << Log::trc()
//           << LVD_REFLECT(target_lvalue->getType()) << '\n'
//           << LVD_REFLECT(content_rvalue->getType()) << '\n';
    assert(content_rvalue->getType() == llvm::cast<llvm::PointerType>(target_lvalue->getType())->getElementType());
    ir_builder->CreateStore(content_rvalue, target_lvalue);
}

} // end namespace sem
} // end namespace cbz
