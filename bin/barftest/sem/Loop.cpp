// 2016.08.10 - Victor Dods

#include "sem/Loop.hpp"

#include "cbz/cgen/Context.hpp"
#include "Exception.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"

namespace cbz {
namespace sem {

std::string const &as_string (LoopType loop_type)
{
    static std::string const STRING_TABLE[uint32_t(LoopType::COUNT)] = {
        "WHILE_DO",
        "DO_WHILE",
    };
    return STRING_TABLE[uint32_t(loop_type)];
}

bool Loop::equals (Base const &other_) const
{
    Loop const &other = dynamic_cast<Loop const &>(other_);
    return m_loop_type == other.m_loop_type &&
           are_equal(m_condition, other.m_condition) &&
           are_equal(m_body, other.m_body);
}

Loop *Loop::cloned () const
{
    return new Loop(firange(), m_loop_type, clone_of(m_condition), clone_of(m_body));
}

void Loop::print (Log &out) const
{
    out << "Loop(" << firange() << '\n';
    {
        IndentGuard ig(out);
        out << m_loop_type << ",\n";
        switch (m_loop_type)
        {
            case LoopType::WHILE_DO:
                out << m_condition << ",\n"
                    << m_body << '\n';
                break;

            case LoopType::DO_WHILE:
                out << m_body << ",\n"
                    << m_condition << '\n';
                break;

            default:
                assert(false); // this should never happen.
        }
    }
    out << ')';
}

void Loop::resolve_symbols (cgen::Context &context)
{
    m_condition->resolve_symbols(context);
    m_body->resolve_symbols(context);
}

void Loop::generate_code (cgen::Context &context) const
{
    if (!has_body())
        throw ProgrammerError("Loop::generate_code -- no loop body");

    // For convenience
    llvm::LLVMContext &c = context.llvm_context();
    llvm::IRBuilder<> &b = context.ir_builder();

    llvm::Function *function = b.GetInsertBlock()->getParent();

    // Generate the conditional and loop body blocks.
    llvm::BasicBlock *conditional_bb = llvm::BasicBlock::Create(c, "cond", function);
    llvm::BasicBlock *loop_body_bb = llvm::BasicBlock::Create(c, "loopbody", function);
    llvm::BasicBlock *after_bb = llvm::BasicBlock::Create(c, "after", function);

    switch (loop_type())
    {
        case LoopType::WHILE_DO: b.CreateBr(conditional_bb); break;
        case LoopType::DO_WHILE: b.CreateBr(loop_body_bb);   break;
        default: LVD_ABORT("unsupported LoopType");
    }

    // Generate code for conditional block
    {
        b.SetInsertPoint(conditional_bb);
        llvm::Value *condition_value = condition().generate_rvalue(context);
        b.CreateCondBr(condition_value, loop_body_bb, after_bb);
    }

    // Generate code for loop body block
    {
        b.SetInsertPoint(loop_body_bb);
        body().generate_code(context);
        b.CreateBr(conditional_bb);
    }

    // Make sure the insertion point is in the "after" block for later code.
    b.SetInsertPoint(after_bb);
}

} // end namespace sem
} // end namespace cbz
