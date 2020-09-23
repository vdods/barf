// 2016.08.09 - Victor Dods

#include "sem/BinaryOperation.hpp"

#include "cbz/cgen/Context.hpp"
#include "Exception.hpp"
#include "cbz/LLVMUtil.hpp"

namespace cbz {
namespace sem {

std::string const &as_string (BinaryOperationType bin_op_type)
{
    static std::string const STRING_TABLE[uint32_t(BinaryOperationType::__HIGHEST__) - uint32_t(BinaryOperationType::__LOWEST__) + 1] = {
        "LOGICAL_OR",
        "LOGICAL_XOR",
        "LOGICAL_AND",
        "EQUALS",
        "NOT_EQUALS",
        "LESS_THAN",
        "GREATER_THAN",
        "LESS_OR_EQUAL",
        "GREATER_OR_EQUAL",
        "ADD",
        "SUB",
        "MUL",
        "DIV",
        "MOD",
        "POW",
        "TIN",
        "SPLUNGE",
        "WOOD",
        "DINSDALE",
    };
    return STRING_TABLE[uint32_t(bin_op_type)];
}

std::string const &as_symbol (BinaryOperationType bin_op_type)
{
    static std::string const STRING_TABLE[uint32_t(BinaryOperationType::__HIGHEST__) - uint32_t(BinaryOperationType::__LOWEST__) + 1] = {
        "or",
        "xor",
        "and",
        "==",
        "!=",
        "<",
        ">",
        "<=",
        ">=",
        "+",
        "-",
        "*",
        "/",
        "%",
        "^",
        "@@",
        "@",
        "@@@",
    };
    return STRING_TABLE[uint32_t(bin_op_type)];
}

BinaryOperation::BinaryOperation (BinaryOperationType bin_op_type, nnup<Base> &&lhs, nnup<Base> &&rhs)
    : Base(firange_of(lhs) + firange_of(rhs))
    , m_bin_op_type(bin_op_type)
    , m_lhs(std::move(lhs))
    , m_rhs(std::move(rhs))
{ }

BinaryOperation::BinaryOperation (FiRange const &firange, BinaryOperationType bin_op_type, nnup<Base> &&lhs, nnup<Base> &&rhs)
    : Base(firange)
    , m_bin_op_type(bin_op_type)
    , m_lhs(std::move(lhs))
    , m_rhs(std::move(rhs))
{ }

bool BinaryOperation::equals (Base const &other_) const
{
    BinaryOperation const &other = dynamic_cast<BinaryOperation const &>(other_);
    return m_bin_op_type == other.m_bin_op_type &&
           are_equal(m_lhs, other.m_lhs) &&
           are_equal(m_rhs, other.m_rhs);
}

BinaryOperation *BinaryOperation::cloned () const
{
    return new BinaryOperation(firange(), m_bin_op_type, clone_of(m_lhs), clone_of(m_rhs));
}

void BinaryOperation::print (Log &out) const
{
    out << "BinaryOperation(" << firange() << '\n';
    out << IndentGuard()
        << as_string(m_bin_op_type) << ",\n"
        << m_lhs << ",\n"
        << m_rhs << '\n';
    out << ')';
}

void BinaryOperation::resolve_symbols (cgen::Context &context)
{
    m_lhs->resolve_symbols(context);
    m_rhs->resolve_symbols(context);
}

ExpressionKind BinaryOperation::generate_expression_kind (cgen::Context &context) const
{
    // NOTE/TODO: For now, all operations are built-in, but operator overloads become a thing
    // and types become first-class values, then we would need to check the overload's return kind.

    auto l = m_lhs->generate_expression_kind(context);
    auto r = m_rhs->generate_expression_kind(context);
    if (l == ExpressionKind::VALUE && r == ExpressionKind::VALUE)
        return ExpressionKind::VALUE;

    // TODO: Support stuff like [Sint8]*22
    LVD_ABORT_WITH_FIRANGE("BinaryOperation not yet supported on " + as_string(l) + " and " + as_string(r), firange());
}

Determinability BinaryOperation::generate_determinability (cgen::Context &context) const
{
    // NOTE/TODO: For now, all operations are built-in, but if operator overloads become a thing,
    // then we would need to check if the overload's determinability.

    auto l = m_lhs->generate_determinability(context);
    auto r = m_rhs->generate_determinability(context);
    // All must be COMPILETIME in order for this expression to be COMPILETIME
    if (l == Determinability::COMPILETIME && r == Determinability::COMPILETIME)
        return Determinability::COMPILETIME;
    else
        return Determinability::RUNTIME;
}

llvm::Type *BinaryOperation::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    up<TypeBase> lhs_type_abstract;
    up<TypeBase> rhs_type_abstract;
    llvm::Type *lhs_type_concrete = m_lhs->generate_rvalue_type(context, &lhs_type_abstract);
    llvm::Type *rhs_type_concrete = m_rhs->generate_rvalue_type(context, &rhs_type_abstract);
    validate_operand_types(context, lhs_type_concrete, rhs_type_concrete);

    assert(lhs_type_concrete == rhs_type_concrete && "this should be true following validate_operand_types");
    assert(are_equal(lhs_type_abstract, rhs_type_abstract) && "this should be true following validate_operand_types");
    // Since the types are equal, there is a well defined single operand type, call it operand_type_concrete.
    llvm::Type *operand_type_concrete = lhs_type_concrete;
    up<TypeBase> operand_type_abstract = std::move(lhs_type_abstract);

    // Some convenience vars
    llvm::LLVMContext &c = context.llvm_context();
    llvm::IntegerType *llvm_Boolean = llvm::Type::getInt1Ty(c);

    std::function<void(TypeBase const &)> set_abstract_type = [abstract_type](TypeBase const &abstract){
        if (abstract_type != nullptr)
            *abstract_type = clone_of(abstract);
    };

    if (operand_type_concrete == llvm_Boolean)
        switch (m_bin_op_type)
        {
            case BinaryOperationType::LOGICAL_OR:        set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::LOGICAL_XOR:       set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::LOGICAL_AND:       set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::EQUALS:            set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::NOT_EQUALS:        set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::LESS_THAN:         set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::GREATER_THAN:      set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::LESS_OR_EQUAL:     set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::GREATER_OR_EQUAL:  set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::ADD:               throw ProgrammerError("addition not defined for Boolean values", firange());
            case BinaryOperationType::SUB:               throw ProgrammerError("subtraction not defined for Boolean values", firange());
            case BinaryOperationType::MUL:               throw ProgrammerError("multiplication not defined for Boolean values", firange());
            case BinaryOperationType::DIV:               throw ProgrammerError("division not defined for Boolean values", firange());
            case BinaryOperationType::MOD:               throw ProgrammerError("modulus not defined for Boolean values", firange());
            case BinaryOperationType::POW:               throw ProgrammerError("exponentiation not defined for Boolean values", firange());
            case BinaryOperationType::TIN:               LVD_ABORT_WITH_FIRANGE("TIN not implemented yet", firange());
            case BinaryOperationType::SPLUNGE:           LVD_ABORT_WITH_FIRANGE("SPLUNGE not implemented yet", firange());
            case BinaryOperationType::WOOD:              LVD_ABORT_WITH_FIRANGE("WOOD not implemented yet", firange());
            case BinaryOperationType::DINSDALE:          LVD_ABORT_WITH_FIRANGE("DINSDALE not implemented yet", firange());
            default: LVD_ABORT_WITH_FIRANGE("unhandled BinaryOperationType", firange());
        }
    else if (operand_type_concrete->isIntegerTy())
        switch (m_bin_op_type)
        {
            case BinaryOperationType::LOGICAL_OR:        set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::LOGICAL_XOR:       set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::LOGICAL_AND:       set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::EQUALS:            set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::NOT_EQUALS:        set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::LESS_THAN:         set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::GREATER_THAN:      set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::LESS_OR_EQUAL:     set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::GREATER_OR_EQUAL:  set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::ADD:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::SUB:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::MUL:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::DIV:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::MOD:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::POW:               LVD_ABORT_WITH_FIRANGE("POW not implemented yet", firange());
            case BinaryOperationType::TIN:               LVD_ABORT_WITH_FIRANGE("TIN not implemented yet", firange());
            case BinaryOperationType::SPLUNGE:           LVD_ABORT_WITH_FIRANGE("SPLUNGE not implemented yet", firange());
            case BinaryOperationType::WOOD:              LVD_ABORT_WITH_FIRANGE("WOOD not implemented yet", firange());
            case BinaryOperationType::DINSDALE:          LVD_ABORT_WITH_FIRANGE("DINSDALE not implemented yet", firange());
            default: LVD_ABORT_WITH_FIRANGE("unhandled BinaryOperationType", firange());
        }
    else if (operand_type_concrete->isFloatingPointTy())
        switch (m_bin_op_type)
        {
            // The O in all the FCmp instructions stands for "ordered", meaning no nans
            case BinaryOperationType::EQUALS:            set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::NOT_EQUALS:        set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::LESS_THAN:         set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::GREATER_THAN:      set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::LESS_OR_EQUAL:     set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::GREATER_OR_EQUAL:  set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::ADD:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::SUB:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::MUL:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::DIV:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::MOD:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::POW:               set_abstract_type(*operand_type_abstract); return operand_type_concrete;
            case BinaryOperationType::TIN:               LVD_ABORT_WITH_FIRANGE("TIN not implemented yet", firange());
            case BinaryOperationType::SPLUNGE:           LVD_ABORT_WITH_FIRANGE("SPLUNGE not implemented yet", firange());
            case BinaryOperationType::WOOD:              LVD_ABORT_WITH_FIRANGE("WOOD not implemented yet", firange());
            case BinaryOperationType::DINSDALE:          LVD_ABORT_WITH_FIRANGE("DINSDALE not implemented yet", firange());
            default: LVD_ABORT_WITH_FIRANGE("unhandled BinaryOperationType", firange());
        }
    else if (llvm::isa<llvm::PointerType>(operand_type_concrete))
        switch (m_bin_op_type)
        {
            case BinaryOperationType::EQUALS:            set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::NOT_EQUALS:        set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::LESS_THAN:         set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::GREATER_THAN:      set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::LESS_OR_EQUAL:     set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            case BinaryOperationType::GREATER_OR_EQUAL:  set_abstract_type(Boolean::SINGLETON); return llvm_Boolean;
            default: LVD_ABORT_WITH_FIRANGE("unhandled BinaryOperationType", firange());
        }
    else
        // TODO: Composite types (e.g. tuples) and other types (e.g. function[pointer]s)
        throw TypeError(LVD_LOG_FMT("BinaryOperation::generate_rvalue_type; unsupported operation at this time; this: " << *this), firange());
}

llvm::Value *BinaryOperation::generate_rvalue (cgen::Context &context) const
{
    llvm::Type *lhs_type_concrete = m_lhs->generate_rvalue_type(context);
    llvm::Type *rhs_type_concrete = m_rhs->generate_rvalue_type(context);
    validate_operand_types(context, lhs_type_concrete, rhs_type_concrete);

    llvm::Value *lhs_value = m_lhs->generate_rvalue(context);
    llvm::Value *rhs_value = m_rhs->generate_rvalue(context);
//     g_log << Log::dbg() << "BinaryOperation (" << m_lhs->firange().as_string() << "); lhs_value->getType() = " << ::as_string(lhs_value->getType()) << ", lhs_type_concrete = " << ::as_string(lhs_type_concrete) << '\n';
//     g_log << Log::dbg() << "BinaryOperation (" << m_lhs->firange().as_string() << "); rhs_value->getType() = " << ::as_string(rhs_value->getType()) << ", rhs_type_concrete = " << ::as_string(rhs_type_concrete) << '\n';
    assert(lhs_value->getType() == lhs_type_concrete);
    assert(rhs_value->getType() == rhs_type_concrete);

    // Some convenience vars
    llvm::LLVMContext &c = context.llvm_context();
    llvm::IRBuilder<> &b = context.ir_builder();
    llvm::IntegerType *llvm_Boolean = llvm::Type::getInt1Ty(c);

    assert(lhs_type_concrete == rhs_type_concrete && "this should be true following validate_operand_types");
    // Since the types are equal, there is a well defined single operand type, call it operand_type_concrete.
    llvm::Type *operand_type_concrete = lhs_type_concrete;

    if (operand_type_concrete == llvm_Boolean)
        switch (m_bin_op_type)
        {
            case BinaryOperationType::LOGICAL_OR:        return b.CreateOr(lhs_value, rhs_value, "ORtmp");
            case BinaryOperationType::LOGICAL_XOR:       return b.CreateXor(lhs_value, rhs_value, "XORtmp");
            case BinaryOperationType::LOGICAL_AND:       return b.CreateAnd(lhs_value, rhs_value, "ANDtmp");
            case BinaryOperationType::EQUALS:            return b.CreateICmpEQ(lhs_value, rhs_value, "EQtmp");
            case BinaryOperationType::NOT_EQUALS:        return b.CreateICmpNE(lhs_value, rhs_value, "NEtmp");
            case BinaryOperationType::LESS_THAN:         return b.CreateICmpSLT(lhs_value, rhs_value, "SLTtmp");
            case BinaryOperationType::GREATER_THAN:      return b.CreateICmpSGT(lhs_value, rhs_value, "SGTtmp");
            case BinaryOperationType::LESS_OR_EQUAL:     return b.CreateICmpSLE(lhs_value, rhs_value, "SLEtmp");
            case BinaryOperationType::GREATER_OR_EQUAL:  return b.CreateICmpSGE(lhs_value, rhs_value, "SGEtmp");
            case BinaryOperationType::ADD:               throw ProgrammerError("addition not defined for Boolean values", firange());
            case BinaryOperationType::SUB:               throw ProgrammerError("subtraction not defined for Boolean values", firange());
            case BinaryOperationType::MUL:               throw ProgrammerError("multiplication not defined for Boolean values", firange());
            case BinaryOperationType::DIV:               throw ProgrammerError("division not defined for Boolean values", firange());
            case BinaryOperationType::MOD:               throw ProgrammerError("modulus not defined for Boolean values", firange());
            case BinaryOperationType::POW:               throw ProgrammerError("exponentiation not defined for Boolean values", firange());
            case BinaryOperationType::TIN:               LVD_ABORT_WITH_FIRANGE("TIN not implemented yet", firange());
            case BinaryOperationType::SPLUNGE:           LVD_ABORT_WITH_FIRANGE("SPLUNGE not implemented yet", firange());
            case BinaryOperationType::WOOD:              LVD_ABORT_WITH_FIRANGE("WOOD not implemented yet", firange());
            case BinaryOperationType::DINSDALE:          LVD_ABORT_WITH_FIRANGE("DINSDALE not implemented yet", firange());
            default: LVD_ABORT_WITH_FIRANGE("unhandled BinaryOperationType", firange());
        }
    else if (operand_type_concrete->isIntegerTy())
        switch (m_bin_op_type)
        {
            case BinaryOperationType::LOGICAL_OR:        return b.CreateOr(lhs_value, rhs_value, "ORtmp");
            case BinaryOperationType::LOGICAL_XOR:       return b.CreateXor(lhs_value, rhs_value, "XORtmp");
            case BinaryOperationType::LOGICAL_AND:       return b.CreateAnd(lhs_value, rhs_value, "ANDtmp");
            case BinaryOperationType::EQUALS:            return b.CreateICmpEQ(lhs_value, rhs_value, "EQtmp");
            case BinaryOperationType::NOT_EQUALS:        return b.CreateICmpNE(lhs_value, rhs_value, "NEtmp");
            case BinaryOperationType::LESS_THAN:         return b.CreateICmpSLT(lhs_value, rhs_value, "SLTtmp");
            case BinaryOperationType::GREATER_THAN:      return b.CreateICmpSGT(lhs_value, rhs_value, "SGTtmp");
            case BinaryOperationType::LESS_OR_EQUAL:     return b.CreateICmpSLE(lhs_value, rhs_value, "SLEtmp");
            case BinaryOperationType::GREATER_OR_EQUAL:  return b.CreateICmpSGE(lhs_value, rhs_value, "SGEtmp");
            case BinaryOperationType::ADD:               return b.CreateAdd(lhs_value, rhs_value, "ADDtmp");
            case BinaryOperationType::SUB:               return b.CreateSub(lhs_value, rhs_value, "SUBtmp");
            case BinaryOperationType::MUL:               return b.CreateMul(lhs_value, rhs_value, "MULtmp");
            case BinaryOperationType::DIV:               return b.CreateSDiv(lhs_value, rhs_value, "SDIVtmp");
            case BinaryOperationType::MOD:               return b.CreateSRem(lhs_value, rhs_value, "SREMtmp");
            case BinaryOperationType::POW:               LVD_ABORT_WITH_FIRANGE("POW not implemented yet", firange());
            case BinaryOperationType::TIN:               LVD_ABORT_WITH_FIRANGE("TIN not implemented yet", firange());
            case BinaryOperationType::SPLUNGE:           LVD_ABORT_WITH_FIRANGE("SPLUNGE not implemented yet", firange());
            case BinaryOperationType::WOOD:              LVD_ABORT_WITH_FIRANGE("WOOD not implemented yet", firange());
            case BinaryOperationType::DINSDALE:          LVD_ABORT_WITH_FIRANGE("DINSDALE not implemented yet", firange());
            default: LVD_ABORT_WITH_FIRANGE("unhandled BinaryOperationType", firange());
        }
    else if (operand_type_concrete->isFloatingPointTy())
        switch (m_bin_op_type)
        {
            // The O in all the FCmp instructions stands for "ordered", meaning no nans
            case BinaryOperationType::EQUALS:            return b.CreateFCmpOEQ(lhs_value, rhs_value, "OEQtmp");
            case BinaryOperationType::NOT_EQUALS:        return b.CreateFCmpONE(lhs_value, rhs_value, "ONEtmp");
            case BinaryOperationType::LESS_THAN:         return b.CreateFCmpOLT(lhs_value, rhs_value, "OLTtmp");
            case BinaryOperationType::GREATER_THAN:      return b.CreateFCmpOGT(lhs_value, rhs_value, "OGTtmp");
            case BinaryOperationType::LESS_OR_EQUAL:     return b.CreateFCmpOLE(lhs_value, rhs_value, "OLEtmp");
            case BinaryOperationType::GREATER_OR_EQUAL:  return b.CreateFCmpOGE(lhs_value, rhs_value, "OGEtmp");
            case BinaryOperationType::ADD:               return b.CreateFAdd(lhs_value, rhs_value, "ADDtmp");
            case BinaryOperationType::SUB:               return b.CreateFSub(lhs_value, rhs_value, "SUBtmp");
            case BinaryOperationType::MUL:               return b.CreateFMul(lhs_value, rhs_value, "MULtmp");
            case BinaryOperationType::DIV:               return b.CreateFDiv(lhs_value, rhs_value, "DIVtmp");
            case BinaryOperationType::MOD:               return b.CreateFRem(lhs_value, rhs_value, "REMtmp");
            case BinaryOperationType::POW:               return b.CreateBinaryIntrinsic(llvm::Intrinsic::pow, lhs_value, rhs_value, nullptr, "POWtmp");
            case BinaryOperationType::TIN:               LVD_ABORT_WITH_FIRANGE("TIN not implemented yet", firange());
            case BinaryOperationType::SPLUNGE:           LVD_ABORT_WITH_FIRANGE("SPLUNGE not implemented yet", firange());
            case BinaryOperationType::WOOD:              LVD_ABORT_WITH_FIRANGE("WOOD not implemented yet", firange());
            case BinaryOperationType::DINSDALE:          LVD_ABORT_WITH_FIRANGE("DINSDALE not implemented yet", firange());
            default: LVD_ABORT_WITH_FIRANGE("unhandled BinaryOperationType", firange());
        }
    else if (llvm::isa<llvm::PointerType>(operand_type_concrete))
        switch (m_bin_op_type)
        {
            case BinaryOperationType::EQUALS:            return b.CreateICmpEQ(lhs_value, rhs_value, "EQtmp");
            case BinaryOperationType::NOT_EQUALS:        return b.CreateICmpNE(lhs_value, rhs_value, "NEtmp");
            case BinaryOperationType::LESS_THAN:         return b.CreateICmpSLT(lhs_value, rhs_value, "SLTtmp");
            case BinaryOperationType::GREATER_THAN:      return b.CreateICmpSGT(lhs_value, rhs_value, "SGTtmp");
            case BinaryOperationType::LESS_OR_EQUAL:     return b.CreateICmpSLE(lhs_value, rhs_value, "SLEtmp");
            case BinaryOperationType::GREATER_OR_EQUAL:  return b.CreateICmpSGE(lhs_value, rhs_value, "SGEtmp");
            default: LVD_ABORT_WITH_FIRANGE("unhandled BinaryOperationType", firange());
        }
    else
        // TODO: Composite types (e.g. tuples) and other types (e.g. function[pointer]s)
        throw TypeError(LVD_LOG_FMT("BinaryOperation::generate_rvalue; unsupported operation at this time; this: " << *this), firange());
}

void BinaryOperation::validate_operand_types (cgen::Context &context, llvm::Type *lhs_type_concrete, llvm::Type *rhs_type_concrete) const
{
    // Some convenience vars
    llvm::LLVMContext &c = context.llvm_context();
    llvm::IntegerType *llvm_Boolean = llvm::Type::getInt1Ty(c);

    // Types in llvm are unique'd, so direct pointer comparison works for equality check.
    // Do a basic type equality check (all operands (currently) must have the same type).
    if (lhs_type_concrete != rhs_type_concrete)
        throw TypeError("operands to binary operator must have the same type", firange());

    // Since the types are equal, there is a well defined single operand type, call it operand_type_concrete.
    llvm::Type *operand_type_concrete = lhs_type_concrete;

    // TODO: See if using BinaryOps from llvm/IR/Instruction.h here (and maybe in this AST class
    // itself) makes sense.  It could greatly ease this implementation.

    // Operator-specific type checking
    switch (m_bin_op_type)
    {
        case BinaryOperationType::LOGICAL_OR:
        case BinaryOperationType::LOGICAL_XOR:
        case BinaryOperationType::LOGICAL_AND:
            if (operand_type_concrete != llvm_Boolean)
                throw TypeError("operands to logical or/xor/and must both be Boolean", firange());
            break;

        case BinaryOperationType::EQUALS:
        case BinaryOperationType::NOT_EQUALS:
            // No constraint.
            break;

        case BinaryOperationType::LESS_THAN:
        case BinaryOperationType::GREATER_THAN:
        case BinaryOperationType::LESS_OR_EQUAL:
        case BinaryOperationType::GREATER_OR_EQUAL:
        case BinaryOperationType::ADD:
        case BinaryOperationType::SUB:
        case BinaryOperationType::MUL:
        case BinaryOperationType::DIV:
        case BinaryOperationType::MOD:
        case BinaryOperationType::POW:
        case BinaryOperationType::TIN:
        case BinaryOperationType::SPLUNGE:
        case BinaryOperationType::WOOD:
        case BinaryOperationType::DINSDALE:
            if (!operand_type_concrete->isIntegerTy() && !operand_type_concrete->isFloatingPointTy())
                throw TypeError("operands must be numeric type (IntegerX or FloatX)", firange());
            break;

        default:
            LVD_ABORT("Invalid BinaryOperationType");
    }
}

} // end namespace sem
} // end namespace cbz
