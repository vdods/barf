// 2016.08.09 - Victor Dods

#include "sem/UnaryOperation.hpp"

#include "cbz/cgen/Context.hpp"
#include "Exception.hpp"
#include "sem/PointerType.hpp"
#include "sem/ReferenceType.hpp"
#include "sem/SymbolSpecifier.hpp"

namespace cbz {
namespace sem {

std::string const &as_string (UnaryOperationType un_op_type)
{
    static std::string const STRING_TABLE[uint32_t(UnaryOperationType::__HIGHEST__) - uint32_t(UnaryOperationType::__LOWEST__) + 1] = {
        "LOGICAL_NOT",
        "NEGATE",
        "AT_SYMBOL",
        "HASH_SYMBOL",
        "CT",
        "RT",
        "LOCAL",
        "GLOBAL",
        "EXTERNAL",
        "INTERNAL",
        "PRIVATE",
    };
    return STRING_TABLE[uint32_t(un_op_type)];
}

bool UnaryOperation::equals (Base const &other_) const
{
    UnaryOperation const &other = dynamic_cast<UnaryOperation const &>(other_);
    return m_un_op_type == other.m_un_op_type && are_equal(m_operand, other.m_operand);
}

UnaryOperation *UnaryOperation::cloned () const
{
    return new UnaryOperation(firange(), m_un_op_type, clone_of(m_operand));
}

void UnaryOperation::print (Log &out) const
{
    out << "UnaryOperation(" << firange() << '\n';
    out << IndentGuard()
        << m_un_op_type << ",\n"
        << m_operand << '\n';
    out << ')';
}

void UnaryOperation::resolve_symbols (cgen::Context &context)
{
    m_operand->resolve_symbols(context);
}

ExpressionKind UnaryOperation::generate_expression_kind (cgen::Context &context) const
{
    auto op_kind = m_operand->generate_expression_kind(context);
    if (op_kind == ExpressionKind::VALUE)
        return ExpressionKind::VALUE;

    LVD_ABORT_WITH_FIRANGE(LVD_FMT("UnaryOperation not yet supported; operand type: " << m_un_op_type << ", operation kind: " << op_kind), firange());
}

Determinability UnaryOperation::generate_determinability (cgen::Context &context) const
{
    // NOTE/TODO: For now, all operations are built-in, but if operator overloads become a thing,
    // then we would need to check if the overload's determinability.

    auto operand_determinability = m_operand->generate_determinability(context);
    switch (m_un_op_type)
    {
        case UnaryOperationType::LOGICAL_NOT:
        case UnaryOperationType::NEGATE:
            // No requirement
            break;

        case UnaryOperationType::AT_SYMBOL:
        case UnaryOperationType::HASH_SYMBOL:
            return Determinability::RUNTIME; // TEMP HACK -- this is the conservative definition, but eventually this will be refined to allow COMPILETIME in certain situations

        case UnaryOperationType::CT:
            if (operand_determinability != Determinability::COMPILETIME)
                throw ProgrammerError("expression marked ct (compiletime) does not satisfy requirement of compiletime determinability", m_operand->firange());

        case UnaryOperationType::RT:
        case UnaryOperationType::LOCAL:
        case UnaryOperationType::GLOBAL:
            // No requirement at this time, just forward it.
            break;

        default:
            LVD_ABORT_WITH_FIRANGE("invalid UnaryOperationType", firange());
    }
    return operand_determinability;
}

llvm::Type *UnaryOperation::generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    switch (m_un_op_type)
    {
        case UnaryOperationType::AT_SYMBOL:
        {
            // This is an l-value only if the operand rvalue type is PointerType.
            up<TypeBase> operand_abstract_type;
            llvm::Type *operand_concrete_type = m_operand->generate_rvalue_type(context, &operand_abstract_type);
            if (operand_abstract_type->type_enum__raw() != TypeEnum::POINTER_TYPE)
                throw ProgrammerError(LVD_LOG_FMT("operand for `*` operation was expected to be a pointer type, but actually was " << operand_abstract_type), firange());

            assert(llvm::isa<llvm::PointerType>(operand_concrete_type));
            if (abstract_type != nullptr)
                *abstract_type = clone_of(dynamic_cast<PointerType const &>(*operand_abstract_type).referent());
            // References and pointers have the same concrete type, so use that.
            return llvm::cast<llvm::PointerType>(operand_concrete_type)->getElementType();
        }

        case UnaryOperationType::HASH_SYMBOL:
        {
            LVD_ABORT_WITH_FIRANGE("UnaryOperation::generate_lvalue_type for UnaryOperationType::HASH_SYMBOL not yet implemented", firange());

//             // This is an l-value only if the operand rvalue type is ReferenceType.  In particular, it produces
//             // the memory location where the reference's pointer is stored.  An example of this usage is
//             // to re-seat a reference:
//             //
//             //     x ::= 3.0
//             //     y ::= 4.0
//             //     r : Float64&;
//             //     r := x // In this case, the RHS uses generate_lvalue -- though is this confusing?
//             //     r = 5.0 // Assign 5.0 to x (which is what r refers to).
//             //     r& = y& // Re-seat the reference -- change the underlying pointer value of r.
//             up<TypeBase> operand_abstract_type;
//             llvm::Type *operand_concrete_type = m_operand->generate_rvalue_type(context, &operand_abstract_type);
//             if (operand_abstract_type->type_enum__raw() != TypeEnum::REFERENCE_TYPE)
//                 throw ProgrammerError(LVD_FMT("operand for `*` operation was expected to be a pointer type, but actually was " << operand_abstract_type), firange());
//
//             if (abstract_type != nullptr)
//                 *abstract_type = make_reference_type(firange(), clone_of(dynamic_cast<PointerType const &>(*operand_abstract_type).referent()));
//             // References and pointers have the same concrete type, so use that.
//             return operand_concrete_type;
        }

        case UnaryOperationType::LOGICAL_NOT: throw ProgrammerError(LVD_FMT("unary operator `not` can not be used to produce an l-value"), firange());
        case UnaryOperationType::NEGATE:      throw ProgrammerError(LVD_FMT("unary operator `-` can not be used to produce an l-value"), firange());
        case UnaryOperationType::CT:          throw ProgrammerError(LVD_FMT("unary operator `ct` as lvalue not yet supported"), firange());
        case UnaryOperationType::RT:          throw ProgrammerError(LVD_FMT("unary operator `rt` as lvalue not yet supported"), firange());
        case UnaryOperationType::LOCAL:       throw ProgrammerError(LVD_FMT("unary operator `local` as lvalue not yet supported"), firange());
        case UnaryOperationType::GLOBAL:      throw ProgrammerError(LVD_FMT("unary operator `global` as lvalue not yet supported"), firange());
        case UnaryOperationType::EXTERNAL:    throw ProgrammerError(LVD_FMT("unary operator `external` as lvalue not yet supported"), firange());
        case UnaryOperationType::INTERNAL:    throw ProgrammerError(LVD_FMT("unary operator `internal` as lvalue not yet supported"), firange());
        case UnaryOperationType::PRIVATE:     throw ProgrammerError(LVD_FMT("unary operator `private` as lvalue not yet supported"), firange());

        default:
            LVD_ABORT_WITH_FIRANGE("invalid UnaryOperationType", firange());
    }
}

llvm::Value *UnaryOperation::generate_lvalue (cgen::Context &context) const
{
    switch (m_un_op_type)
    {
        case UnaryOperationType::LOGICAL_NOT: throw ProgrammerError("unary operator `not` can not be used directly to form an lvalue", firange());
        case UnaryOperationType::NEGATE:      throw ProgrammerError("unary operator `-` can not be used directly to form an lvalue", firange());
        case UnaryOperationType::AT_SYMBOL:
        {
            llvm::Value *operand_value = m_operand->generate_rvalue(context);
            llvm::Type *operand_concrete_type = m_operand->generate_rvalue_type(context);
            g_log << Log::trc() << "UnaryOperation::generate_lvalue; UnaryOperationType::AT_SYMBOL\n"
                  << IndentGuard()
                  << LVD_REFLECT(operand_value->getType()) << '\n'
                  << LVD_REFLECT(operand_concrete_type) << '\n';
            assert(operand_value->getType() == operand_concrete_type);
            assert(llvm::isa<llvm::PointerType>(operand_concrete_type));
            return operand_value;
        }

        case UnaryOperationType::HASH_SYMBOL:
        {
            LVD_ABORT_WITH_FIRANGE("UnaryOperation::generate_lvalue for UnaryOperationType::HASH_SYMBOL not yet implemented", firange());

//             llvm::Value *operand_value = m_operand->generate_rvalue(context);
//             up<TypeBase> abstract_operand_type;
//             llvm::Type *operand_concrete_type = m_operand->generate_rvalue_type(context, &abstract_operand_type);
//             g_log << Log::trc() << "UnaryOperation::generate_lvalue; UnaryOperationType::HASH_SYMBOL\n"
//                   << IndentGuard()
//                   << LVD_REFLECT(operand_value->getType()) << '\n'
//                   << LVD_REFLECT(operand_concrete_type) << '\n'
//                   << LVD_REFLECT(abstract_operand_type) << '\n';
//             if (abstract_operand_type->type_enum__raw() != TypeEnum::REFERENCE_TYPE)
//                 throw ProgrammerError("can't form an lvalue using `&` unless the underlying type is a reference type", firange());
//             assert(operand_value->getType() == operand_concrete_type);
//             // References are the same as pointers, from a concrete-type level.
//             assert(llvm::isa<llvm::PointerType>(operand_concrete_type));
//             return operand_value;
        }

        case UnaryOperationType::CT:
        case UnaryOperationType::RT:
        case UnaryOperationType::LOCAL:
        case UnaryOperationType::GLOBAL:
            throw ProgrammerError("ct, rt, local, and global requirement operators are not supported in lvalues yet", firange());

        default:
            LVD_ABORT_WITH_FIRANGE("invalid UnaryOperationType", firange());
    }
}

llvm::Type *UnaryOperation::generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type) const
{
    up<TypeBase> operand_abstract_type;
    llvm::Type *operand_concrete_type = m_operand->generate_rvalue_type(context, &operand_abstract_type);
    validate_operand_type(context, operand_concrete_type, *operand_abstract_type);

    // Some convenience vars
    llvm::LLVMContext &c = context.llvm_context();
    llvm::IntegerType *llvm_Boolean = llvm::Type::getInt1Ty(c);
    llvm::IntegerType *llvm_Integer64 = llvm::Type::getInt64Ty(c);
    llvm::Type *llvm_Float64 = llvm::Type::getDoubleTy(c);

    std::function<void(TypeBase const &)> set_abstract_type__copy = [abstract_type](TypeBase const &abstract){
        if (abstract_type != nullptr)
            *abstract_type = clone_of(abstract);
    };
    std::function<void(up<TypeBase> &&)> set_abstract_type__move = [abstract_type](up<TypeBase> &&abstract){
        if (abstract_type != nullptr)
            *abstract_type = std::move(abstract);
    };

    switch (m_un_op_type)
    {
        case UnaryOperationType::LOGICAL_NOT:
            set_abstract_type__copy(Boolean::SINGLETON);
            return llvm_Boolean;

        case UnaryOperationType::NEGATE:
            if (operand_concrete_type == llvm_Float64)
            {
                set_abstract_type__copy(Float64::SINGLETON);
                return llvm_Float64;
            }
            else if (operand_concrete_type == llvm_Integer64)
            {
                set_abstract_type__copy(Sint64::SINGLETON);
                return llvm_Integer64;
            }
            else
                LVD_ABORT_WITH_FIRANGE("unhandled operand type for UnaryOperationType::NEGATE", firange());

        case UnaryOperationType::AT_SYMBOL:
        {
            // If the operand is a value, then its type must be a PointerType, so produce the underlying type.
            if (operand_abstract_type->type_enum__raw() != TypeEnum::POINTER_TYPE)
                throw ProgrammerError(LVD_FMT("can't dereference non-pointer type " << m_un_op_type << " with unary operator '*'"), firange());
            assert(llvm::isa<llvm::PointerType>(operand_concrete_type));
            // TODO: Could use move semantics to make this set_abstract_type__copy more efficient, though that's
            // a bit tricky because it would be moving the referent out of the PointerType instance, and that's
            // not really supported yet.
            set_abstract_type__copy(dynamic_cast<PointerType const &>(*operand_abstract_type).referent());
            return llvm::cast<llvm::PointerType>(operand_concrete_type)->getElementType();
        }

        case UnaryOperationType::HASH_SYMBOL:
        {
            up<TypeBase> operand_abstract_lvalue_type;
            llvm::Type *operand_concrete_lvalue_type = m_operand->generate_lvalue_type(context, &operand_abstract_lvalue_type);
            if (operand_abstract_lvalue_type->type_enum__raw() == TypeEnum::REFERENCE_TYPE)
            {
                // If it's a reference, turn it into a pointer.
                set_abstract_type__move(make_pointer_type(firange(), clone_of(dynamic_cast<ReferenceType &>(*operand_abstract_lvalue_type).referent())));
                return operand_concrete_lvalue_type;
            }
            else
                LVD_ABORT_WITH_FIRANGE(LVD_LOG_FMT("Can't apply `&` to an expression of type " << operand_abstract_lvalue_type), firange());
        }

        case UnaryOperationType::CT:
        case UnaryOperationType::RT:
        case UnaryOperationType::LOCAL:
        case UnaryOperationType::GLOBAL:
            return m_operand->generate_rvalue_type(context, abstract_type);

        default:
            LVD_ABORT_WITH_FIRANGE("invalid UnaryOperationType", firange());
    }
}

llvm::Value *UnaryOperation::generate_rvalue (cgen::Context &context) const
{
    llvm::Type *operand_concrete_type = m_operand->generate_rvalue_type(context);

    // Some convenience vars
    llvm::LLVMContext &c = context.llvm_context();
    llvm::IRBuilder<> &b = context.ir_builder();
    llvm::IntegerType *llvm_Integer64 = llvm::Type::getInt64Ty(c);
    llvm::Type *llvm_Float64 = llvm::Type::getDoubleTy(c);

    switch (m_un_op_type)
    {
        case UnaryOperationType::LOGICAL_NOT:
        {
            llvm::Value *operand_value = m_operand->generate_rvalue(context);
            assert(operand_value->getType() == operand_concrete_type);
            return b.CreateNot(operand_value, "NOTtmp");
        }

        case UnaryOperationType::NEGATE:
        {
            llvm::Value *operand_value = m_operand->generate_rvalue(context);
            assert(operand_value->getType() == operand_concrete_type);
            if (operand_concrete_type == llvm_Float64)
                return b.CreateFNeg(operand_value, "NEGtmp");
            else if (operand_concrete_type == llvm_Integer64)
                return b.CreateNeg(operand_value, "NEGtmp");
            else
                LVD_ABORT_WITH_FIRANGE("unhandled operand type for UnaryOperationType::NEGATE", firange());
        }

        case UnaryOperationType::AT_SYMBOL:
        {
            llvm::Value *operand_value = m_operand->generate_rvalue(context);
//             g_log << Log::trc() << "UnaryOperation::generate_rvalue; " << LVD_REFLECT(operand_value->getType()) << ", " << LVD_REFLECT(operand_concrete_type) << '\n';
            assert(operand_value->getType() == operand_concrete_type);
            assert(llvm::isa<llvm::PointerType>(operand_concrete_type));
            return context.ir_builder().CreateLoad(operand_value, "DEREFtmp");
        }

        case UnaryOperationType::HASH_SYMBOL:
        {
            up<TypeBase> operand_abstract_lvalue_type;
            llvm::Type *operand_concrete_lvalue_type = m_operand->generate_lvalue_type(context, &operand_abstract_lvalue_type);
            llvm::Value *operand_lvalue = m_operand->generate_lvalue(context);
//             g_log << Log::trc() << "UnaryOperation::generate_rvalue; HASH_SYMBOL; " << LVD_REFLECT(operand_abstract_lvalue_type) << ", " << LVD_REFLECT(operand_concrete_lvalue_type) << '\n';
            assert(operand_lvalue->getType() == operand_concrete_lvalue_type);
//             assert(llvm::isa<llvm::PointerType>(operand_concrete_lvalue_type));
            return operand_lvalue;
        }

        case UnaryOperationType::CT: // Require everything below this to be determinable at compiletime.
        {
            if (m_operand->generate_determinability(context) != Determinability::COMPILETIME)
                throw ProgrammerError("expression marked ct (compiletime) does not satisfy requirement of compiletime determinability", m_operand->firange());
            llvm::Value *operand_value = m_operand->generate_rvalue(context);
            assert(operand_value->getType() == operand_concrete_type);
            return operand_value;
        }

        case UnaryOperationType::RT:
        case UnaryOperationType::LOCAL:
        case UnaryOperationType::GLOBAL:
            throw ProgrammerError("rt, local, and global requirement operators are not supported in rvalues yet", firange());

        default:
            LVD_ABORT_WITH_FIRANGE("invalid UnaryOperationType", firange());
    }
}

nnup<SymbolSpecifier> UnaryOperation::generate_svalue (cgen::Context &context) const
{
    switch (m_un_op_type)
    {
        case UnaryOperationType::LOGICAL_NOT:
        case UnaryOperationType::NEGATE:
        case UnaryOperationType::AT_SYMBOL:
        case UnaryOperationType::HASH_SYMBOL:
            throw ProgrammerError("unary operator " + as_string(m_un_op_type) + " not allowed in symbol specification", firange());

        case UnaryOperationType::CT:
            return m_operand->generate_svalue(context)->with_specified_value_kind(ValueKind::CONSTANT, firange());

        case UnaryOperationType::RT:
            return m_operand->generate_svalue(context)->with_specified_value_kind(ValueKind::VARIABLE, firange());

        case UnaryOperationType::LOCAL:
            return m_operand->generate_svalue(context)->with_specified_value_lifetime(ValueLifetime::LOCAL, firange());

        case UnaryOperationType::GLOBAL:
            return m_operand->generate_svalue(context)->with_specified_value_lifetime(ValueLifetime::GLOBAL, firange());

        case UnaryOperationType::EXTERNAL:
            return m_operand->generate_svalue(context)->with_specified_global_value_linkage(GlobalValueLinkage::EXTERNAL, firange());

        case UnaryOperationType::INTERNAL:
            return m_operand->generate_svalue(context)->with_specified_global_value_linkage(GlobalValueLinkage::INTERNAL, firange());

        case UnaryOperationType::PRIVATE:
            return m_operand->generate_svalue(context)->with_specified_global_value_linkage(GlobalValueLinkage::PRIVATE, firange());

        default:
            LVD_ABORT_WITH_FIRANGE("Invalid UnaryOperationType", firange());
    }
}

void UnaryOperation::validate_operand_type (cgen::Context &context, llvm::Type *operand_concrete_type, TypeBase const &operand_abstract_type) const
{
    assert(operand_concrete_type != nullptr);

    // Some convenience vars
    llvm::LLVMContext &c = context.llvm_context();
    llvm::IntegerType *llvm_Boolean = llvm::Type::getInt1Ty(c);
    llvm::IntegerType *llvm_Integer64 = llvm::Type::getInt64Ty(c);
    llvm::Type *llvm_Float64 = llvm::Type::getDoubleTy(c);

    // Operator-specific type checking.
    switch (m_un_op_type)
    {
        case UnaryOperationType::LOGICAL_NOT:
            if (operand_concrete_type != llvm_Boolean)
                throw TypeError(LVD_LOG_FMT("unary operator `not` expects Boolean value, but got " << operand_abstract_type), firange());
            break;

        case UnaryOperationType::NEGATE:
            if (operand_concrete_type != llvm_Integer64 && operand_concrete_type != llvm_Float64)
                throw TypeError(LVD_LOG_FMT("unary operator `-` expects a numeric value, but got " << operand_abstract_type), firange());
            break;

        case UnaryOperationType::AT_SYMBOL:
            if (!llvm::isa<llvm::PointerType>(operand_concrete_type))
                throw TypeError(LVD_LOG_FMT("unary operator `*` expects a value with pointer type, but got " << operand_abstract_type), firange());
            break;

        case UnaryOperationType::HASH_SYMBOL:
            // The type validation doesn't actually say anything about if the operand actually has an address to take,
            // so this check is a no-op.  The "has an address" check has to be done elsewhere.
            break;

        case UnaryOperationType::CT: // Require everything below this to be determinable at compiletime.
            if (m_operand->generate_determinability(context) != Determinability::COMPILETIME)
                throw ProgrammerError("expression marked ct (compiletime) does not satisfy requirement of compiletime determinability", m_operand->firange());
            break;

        case UnaryOperationType::RT:
        case UnaryOperationType::LOCAL:
        case UnaryOperationType::GLOBAL:
            throw ProgrammerError("rt, local, and global requirement operators are not supported yet", firange());
//             // No requirement at this time.
//             break;

        default:
            LVD_ABORT_WITH_FIRANGE("invalid UnaryOperationType", firange());
    }
}

} // end namespace sem
} // end namespace cbz
