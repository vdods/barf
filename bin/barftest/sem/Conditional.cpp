// 2016.08.08 - Victor Dods

#include "sem/Conditional.hpp"

#include "cbz/cgen/Context.hpp"
#include "Exception.hpp"
#include "cbz/LLVMUtil.hpp"
#include "llvm/IR/IRBuilder.h"

namespace cbz {
namespace sem {

Conditional::Conditional (nnup<Base> &&condition, nnup<Base> &&positive_element, up<Base> &&negative_element)
    : Base(firange_of(condition) + firange_of(positive_element) + firange_of(negative_element))
    , m_condition(std::move(condition))
    , m_positive_element(std::move(positive_element))
    , m_negative_element(std::move(negative_element))
{ }

Conditional::Conditional (FiRange const &firange, nnup<Base> &&condition, nnup<Base> &&positive_element, up<Base> &&negative_element)
    : Base(firange)
    , m_condition(std::move(condition))
    , m_positive_element(std::move(positive_element))
    , m_negative_element(std::move(negative_element))
{ }

bool Conditional::equals (Base const &other_) const
{
    Conditional const &other = dynamic_cast<Conditional const &>(other_);
    return type_enum__raw() == other.type_enum__raw() &&
           are_equal(m_condition, other.m_condition) &&
           are_equal(m_positive_element, other.m_positive_element) &&
           are_equal(m_negative_element, other.m_negative_element);
}

void Conditional::resolve_symbols (cgen::Context &context)
{
    m_condition->resolve_symbols(context);
    m_positive_element->resolve_symbols(context);
    if (m_negative_element != nullptr)
        m_negative_element->resolve_symbols(context);
}

//
// ConditionalExpression
//

ConditionalExpression *ConditionalExpression::cloned () const
{
    return new ConditionalExpression(firange(), clone_of(condition()), clone_of(positive_element()), clone_of(negative_element()));
}

void ConditionalExpression::print (Log &out) const
{
    assert(has_negative_element());
    out << "ConditionalExpression(" << firange() << '\n';
    out << IndentGuard()
        << condition() << ",\n"
        << positive_element() << ",\n"
        << negative_element() << '\n';
    out << ')';
}

ExpressionKind ConditionalExpression::generate_expression_kind (cgen::Context &context) const
{
    auto p = positive_element().generate_expression_kind(context);
    auto n = negative_element().generate_expression_kind(context);
    if (p != n)
        throw ProgrammerError("positive and negative elements of conditional expression do not have the same expression kind (e.g. type, value)", firange());
    return p;
}

Determinability ConditionalExpression::generate_determinability (cgen::Context &context) const
{
    auto c = condition().generate_determinability(context);
    auto p = positive_element().generate_determinability(context);
    auto n = negative_element().generate_determinability(context);
    // All must be COMPILETIME in order for this expression to be COMPILETIME
    if (c == Determinability::COMPILETIME && p == Determinability::COMPILETIME && n == Determinability::COMPILETIME)
        return Determinability::COMPILETIME;
    else
        return Determinability::RUNTIME;
}

llvm::Type *ConditionalExpression::generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    up<TypeBase> positive_abstract_type;
    up<TypeBase> negative_abstract_type;
    llvm::Type *positive_element_type = positive_element().generate_lvalue_type(context, &positive_abstract_type);
    assert(positive_element_type != nullptr);
    assert(positive_abstract_type != nullptr);
    llvm::Type *negative_element_type = negative_element().generate_lvalue_type(context, &negative_abstract_type);
    assert(negative_element_type != nullptr);
    assert(negative_abstract_type != nullptr);
    validate_element_types(context, *positive_element_type, *negative_element_type);
    validate_element_types(context, *positive_abstract_type, *negative_abstract_type);
    assert(positive_element_type == negative_element_type && "this should be true due to validate_element_types");
    assert(are_equal(positive_abstract_type, negative_abstract_type) && "this should be true due to validate_element_types");
    if (abstract_type != nullptr)
        *abstract_type = std::move(positive_abstract_type);
    return positive_element_type;
}

llvm::Value *ConditionalExpression::generate_lvalue (cgen::Context &context) const
{
    assert(has_negative_element());

    // Check the types of the positive and negative elements.
    llvm::Type *positive_element_type = positive_element().generate_lvalue_type(context);
    llvm::Type *negative_element_type = negative_element().generate_lvalue_type(context);
    assert(positive_element_type != nullptr);
    assert(negative_element_type != nullptr);
    validate_element_types(context, *positive_element_type, *negative_element_type);

    // For convenience
    llvm::IRBuilder<> &b = context.ir_builder();

    llvm::Value *condition_value = condition().generate_rvalue(context);
    if (condition_value->getType() != b.getInt1Ty())
        throw TypeError("condition must evaluate to Boolean", condition().firange());

    llvm::Function *function = b.GetInsertBlock()->getParent();

    llvm::BasicBlock *then_bb = llvm::BasicBlock::Create(context.llvm_context(), "then", function);
    llvm::BasicBlock *else_bb = llvm::BasicBlock::Create(context.llvm_context(), "else");
    llvm::BasicBlock *merge_bb = llvm::BasicBlock::Create(context.llvm_context(), "ifcont");

    b.CreateCondBr(condition_value, then_bb, else_bb);
    b.SetInsertPoint(then_bb);
    llvm::Value *positive_element_value = positive_element().generate_lvalue(context);
    assert(positive_element_value != nullptr);
//     // This should be true because an lvalue has to be a reference.
//     assert(llvm::isa<llvm::PointerType>(positive_element_value->getType()));
//     // Consistency check.
//     assert(llvm::cast<llvm::PointerType>(positive_element_value->getType()) == positive_element_type);
    b.CreateBr(merge_bb);
    then_bb = b.GetInsertBlock();

    function->getBasicBlockList().push_back(else_bb);
    b.SetInsertPoint(else_bb);
    llvm::Value *negative_element_value = negative_element().generate_lvalue(context);
    assert(negative_element_value != nullptr);
//     // This should be true because an lvalue has to be a reference.
//     assert(llvm::isa<llvm::PointerType>(negative_element_value->getType()));
//     // Consistency check.
//     assert(llvm::cast<llvm::PointerType>(negative_element_value->getType()) == negative_element_type);
    if (positive_element_value->getType() != negative_element_value->getType())
        throw TypeError("type mismatch in conditional expression", firange());
    b.CreateBr(merge_bb);
    else_bb = b.GetInsertBlock();

    function->getBasicBlockList().push_back(merge_bb);
    b.SetInsertPoint(merge_bb);
    llvm::PHINode *phi = b.CreatePHI(positive_element_value->getType(), 2, "iftmp");
    phi->addIncoming(positive_element_value, then_bb);
    phi->addIncoming(negative_element_value, else_bb);
    return phi;
}

llvm::Type *ConditionalExpression::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    up<TypeBase> positive_abstract_type;
    up<TypeBase> negative_abstract_type;
    llvm::Type *positive_element_type = positive_element().generate_rvalue_type(context, &positive_abstract_type);
    assert(positive_element_type != nullptr);
    assert(positive_abstract_type != nullptr);
    llvm::Type *negative_element_type = negative_element().generate_rvalue_type(context, &negative_abstract_type);
    assert(negative_element_type != nullptr);
    assert(negative_abstract_type != nullptr);
    validate_element_types(context, *positive_element_type, *negative_element_type);
    validate_element_types(context, *positive_abstract_type, *negative_abstract_type);
    assert(positive_element_type == negative_element_type && "this should be true due to validate_element_types");
    assert(are_equal(positive_abstract_type, negative_abstract_type) && "this should be true due to validate_element_types");
    if (abstract_type != nullptr)
        *abstract_type = std::move(positive_abstract_type);
    return positive_element_type;
}

llvm::Value *ConditionalExpression::generate_rvalue (cgen::Context &context) const
{
    assert(has_negative_element());

    // Check the types of the positive and negative elements.
    {
        llvm::Type *positive_element_type = positive_element().generate_rvalue_type(context);
        llvm::Type *negative_element_type = negative_element().generate_rvalue_type(context);
        assert(positive_element_type != nullptr);
        assert(negative_element_type != nullptr);
        validate_element_types(context, *positive_element_type, *negative_element_type);
    }

    llvm::IRBuilder<> &b = context.ir_builder(); // For convenience

    llvm::Value *condition_value = condition().generate_rvalue(context);
    if (condition_value->getType() != b.getInt1Ty())
        throw TypeError("condition must evaluate to Boolean", condition().firange());

    llvm::Function *function = b.GetInsertBlock()->getParent();

    llvm::BasicBlock *then_bb = llvm::BasicBlock::Create(context.llvm_context(), "then", function);
    llvm::BasicBlock *else_bb = llvm::BasicBlock::Create(context.llvm_context(), "else");
    llvm::BasicBlock *merge_bb = llvm::BasicBlock::Create(context.llvm_context(), "ifcont");

    b.CreateCondBr(condition_value, then_bb, else_bb);
    b.SetInsertPoint(then_bb);
    llvm::Value *positive_element_value = positive_element().generate_rvalue(context);
    assert(positive_element_value != nullptr);
    b.CreateBr(merge_bb);
    then_bb = b.GetInsertBlock();

    function->getBasicBlockList().push_back(else_bb);
    b.SetInsertPoint(else_bb);
    llvm::Value *negative_element_value = negative_element().generate_rvalue(context);
    assert(negative_element_value != nullptr);
    if (positive_element_value->getType() != negative_element_value->getType())
        throw TypeError("type mismatch in conditional expression", firange());
    b.CreateBr(merge_bb);
    else_bb = b.GetInsertBlock();

    function->getBasicBlockList().push_back(merge_bb);
    b.SetInsertPoint(merge_bb);
    llvm::PHINode *phi = b.CreatePHI(positive_element_value->getType(), 2, "iftmp");
    phi->addIncoming(positive_element_value, then_bb);
    phi->addIncoming(negative_element_value, else_bb);
    return phi;
}

void ConditionalExpression::validate_element_types (cgen::Context &context, llvm::Type const &positive_element_type, llvm::Type const &negative_element_type) const
{
    if (&positive_element_type != &negative_element_type)
        throw TypeError(LVD_FMT("type mismatch in conditional expression; types were " << &positive_element_type << " and " << &negative_element_type), firange()); // TEMP HACK: TODO: Figure out why printing the reference doesn't compile
}

void ConditionalExpression::validate_element_types (cgen::Context &context, TypeBase const &positive_element_type, TypeBase const &negative_element_type) const
{
    if (!are_equal(positive_element_type, negative_element_type))
        throw TypeError(LVD_FMT("abstract type mismatch in conditional expression; types were " << &positive_element_type << " and " << &negative_element_type), firange()); // TEMP HACK: TODO: Figure out why printing the reference doesn't compile
}

//
// ConditionalStatement
//

ConditionalStatement *ConditionalStatement::cloned () const
{
    return new ConditionalStatement(firange(), clone_of(condition()), clone_of(positive_element()), clone_of(negative_element_ptr()));
}

void ConditionalStatement::print (Log &out) const
{
    out << "ConditionalStatement(" << firange() << '\n';
    out << IndentGuard()
        << condition() << ",\n"
        << positive_element() << ",\n"
        << negative_element_ptr() << '\n';
    out << ')';
}

void ConditionalStatement::generate_code (cgen::Context &context) const
{
    llvm::IRBuilder<> &b = context.ir_builder(); // For convenience

    llvm::Value *condition_value = condition().generate_rvalue(context);
    if (condition_value->getType() != b.getInt1Ty())
        throw TypeError("condition must evaluate to Boolean", condition().firange());

    llvm::Function *function = b.GetInsertBlock()->getParent();

    llvm::BasicBlock *then_bb = llvm::BasicBlock::Create(context.llvm_context(), "then", function);
    llvm::BasicBlock *else_bb = nullptr;
    if (has_negative_element())
        else_bb = llvm::BasicBlock::Create(context.llvm_context(), "else");
    llvm::BasicBlock *merge_bb = llvm::BasicBlock::Create(context.llvm_context(), "ifcont");

    if (has_negative_element())
        b.CreateCondBr(condition_value, then_bb, else_bb);
    else
        b.CreateCondBr(condition_value, then_bb, merge_bb);

    b.SetInsertPoint(then_bb);
    positive_element().generate_code(context);
    b.CreateBr(merge_bb);

    if (has_negative_element())
    {
        function->getBasicBlockList().push_back(else_bb);
        b.SetInsertPoint(else_bb);
        negative_element().generate_code(context);
        b.CreateBr(merge_bb);
    }

    function->getBasicBlockList().push_back(merge_bb);
    b.SetInsertPoint(merge_bb);
}

} // end namespace sem
} // end namespace cbz
