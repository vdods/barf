// 2016.08.08 - Victor Dods

#pragma once

#include "core.hpp"
#include "Exception.hpp"
#include "FiRange.hpp"
#include "sem/Determinability.hpp"
#include "sem/ExpressionKind.hpp"
#include "sem/TypeEnum.hpp"
#include <ostream>
#include <string>

namespace llvm {

class Function;
class PointerType;
class Type;
class Value;

} // end namespace llvm

namespace cbz {
namespace cgen {

struct Context;

} // end namespace cgen

namespace sem {

struct SymbolSpecifier;
struct TypeBase;

// Base class for Abstract Syntax Tree class hierarchy
struct Base
{
    explicit Base (FiRange const &firange) : m_firange(firange) { }
    virtual ~Base () { }

    // TODO: Ideally FiRange would be separated from the AST because AST doesn't intrinsically
    // have anything to do with a text-based parser.  But this is the most direct way to
    // do this for now.
    FiRange const &firange () const { return m_firange; }

    void set_firange (FiRange const &firange) { m_firange = firange; }
    // Adds the specified FiRange into this one using FiRange::operator+= .
    void grow_firange (FiRange const &firange) { m_firange += firange; }

    virtual TypeEnum type_enum__raw () const = 0;
    virtual bool is_type () const { return false; }
    virtual bool is_value () const { return false; }
    // This must only be called if other is an instance of a subclass of the type of this
    // (non-strict subclass, meaning the type of this is also acceptable).  E.g. it's
    // acceptable to call Boolean::equals(other) when other has type Boolean const &.
    virtual bool equals (Base const &other) const = 0;
    // This should produce a copy of this instance (of whatever the actual type is),
    // such that the new and old instances `are_equal` (see definition of that function
    // below).  Note that the return value is a raw pointer to a newly allocated object,
    // and the caller is meant to own this pointer, so it should immediately go into
    // a std::unique_ptr -- this can be done automatically with the global convenience
    // function `clone_of` (see below).
    virtual Base *cloned () const = 0;
    virtual void print (Log &out) const = 0;

    // This should recursively resolve all symbol lookups (i.e. Identifiers) into absolute
    // fully qualified symbol lookups -- it should modify this object in-place.
    // TODO: Maybe make context be a constant ref, which would mean that it doesn't generate any code.
    virtual void resolve_symbols (cgen::Context &context) { LVD_ABORT_WITH_FIRANGE("resolve_symbols for " + as_string(type_enum__raw()) + " not implemented", firange()); }
    // If this is a type AST (or symbolically refers to one), then return the TypeEnum for it.
    // TODO: Maybe make context be a constant ref, which would mean that it doesn't generate any code.
    virtual TypeEnum type_enum__resolved (cgen::Context &context) const { return type_enum__raw(); }

    //
    // LLVM code generation methods
    //

    // Determine the kind of expression this is.  In particular, this is ExpressionKind::METATYPE (which
    // currently only corresponds to the `Type` keyword), ExpressionKind::TYPE, and ExpressionKind::VALUE.
    // TODO: Maybe make context be a constant ref, which would mean that it doesn't generate any code.
    virtual ExpressionKind generate_expression_kind (cgen::Context &context) const { LVD_ABORT_WITH_FIRANGE("generate_expression_kind for " + as_string(type_enum__raw()) + " not implemented", firange()); }
    // Determine the mutability of this value if this->generate_expression_kind(context) returns
    // ExpressionKind::VALUE (otherwise should be left undefined).  In particular, this is
    // Determinability::CONSTANT and Determinability::VARIABLE.
    // TODO: Maybe make context be a constant ref, which would mean that it doesn't generate any code.
    virtual Determinability generate_determinability (cgen::Context &context) const { LVD_ABORT_WITH_FIRANGE("generate_determinability for " + as_string(type_enum__raw()) + " not implemented", firange()); }
    // See generate_lvalue for more details on L-values.
    virtual llvm::Type *generate_lvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const { LVD_ABORT_WITH_FIRANGE("generate_lvalue_type for " + as_string(type_enum__raw()) + " not implemented", firange()); }
    // Generate the L-value for this AST.  An L-value is defined to be any of the following:
    // -   The left-hand side of an assignment
    // -   The right-hand side of an initialization of a ReferenceType'd symbol (this is different
    //     than the right-hand side of an assignment to a ReferenceType'd symbol; that is an R-value).
    // -   An expression passed as a parameter to a function evaluation, when that parameter (as declared
    //     in the function type) is a ReferenceType -- this is a sub-case of initialization; the function's
    //     parameters are being initialized.
    // The returned value must be the value that can be stored to (via LLVM's IRBuilder::CreateStore method),
    // TODO: Change the return type to nn<llvm::Value*> (though this prevents covariant return typing).
    virtual llvm::Value *generate_lvalue (cgen::Context &context) const { LVD_ABORT_WITH_FIRANGE("generate_lvalue for " + as_string(type_enum__raw()) + " not implemented", firange()); }
    // If this is a value AST (or symbolically refers to one), then generate the LLVM
    // type for it (including indirection via symbol table).  Call this the "concrete"
    // type.  E.g. the type of `1.0 + 2.0` (BinaryOperation add) would be LLVM's double
    // type (whatever that is).  If abstract_type is not null, then the "abstract" type
    // (i.e. AST type) of this AST node will be assigned to it.
    // TODO: Change the return type to nn<llvm::Type*> (or nn<llvm::Type const *> ?)
    //       or maybe even to cgen::TypePair (which consists of a concrete and an abstract type)
    //       and get rid of the optional abstract_type out-param
    // TODO: Maybe make context be a constant ref, which would mean that it doesn't generate any code.
    virtual llvm::Type *generate_rvalue_type (cgen::Context &context, up<TypeBase> *abstract_type = nullptr) const { LVD_ABORT_WITH_FIRANGE("generate_rvalue_type for " + as_string(type_enum__raw()) + " not implemented", firange()); }
    // Generate the R-value for this AST (this is the kind of value that can only appear on the right hand side of an assignment).
    // The default implementation is to attempt to generate an L-value
    // TODO: Change the return type to nn<llvm::Value*> (though this prevents covariant return typing).
    virtual llvm::Value *generate_rvalue (cgen::Context &context) const { LVD_ABORT_WITH_FIRANGE("generate_rvalue for " + as_string(type_enum__raw()) + " not implemented", firange()); }
    // Generate the SymbolSpecifier for this AST (this is the kind of expression that can appear on the left hand side
    // of a Declaration or Definition).
    virtual nnup<SymbolSpecifier> generate_svalue (cgen::Context &context) const { LVD_ABORT_WITH_FIRANGE("generate_svalue for " + as_string(type_enum__raw()) + " not implemented", firange()); }
    // Generate a function prototype (only applies if this is a function type AST)
    // TODO: Change the return type to nn<llvm::Function*> (though this prevents covariant return typing).
    virtual llvm::Function *generate_function_prototype (cgen::Context &context) const { LVD_ABORT_WITH_FIRANGE("generate_function_prototype for " + as_string(type_enum__raw()) + " not implemented", firange()); }
    // Generate code for statement ASTs and function bodies.
    virtual void generate_code (cgen::Context &context) const { LVD_ABORT_WITH_FIRANGE("generate_code for " + as_string(type_enum__raw()) + " not implemented", firange()); }

private:

    FiRange m_firange;
};

} // end namespace sem

//
// firange_of
//

inline FiRange const &firange_of (sem::Base const &b)
{
    return b.firange();
}

inline FiRange const &firange_of (sem::Base const *p)
{
    return p == nullptr ? FiRange::INVALID : firange_of(*p);
}

template <typename T_, typename Deleter_>
FiRange const &firange_of (std::unique_ptr<T_,Deleter_> const &p)
{
    return p == nullptr ? FiRange::INVALID : firange_of(*p);
}

template <typename T_>
FiRange const &firange_of (gsl::not_null<T_> const &p)
{
    return firange_of(*p);
}

namespace sem {

//
// are_equal
//

// This indirection was necessary to overcome Clang warning -Wpotentially-evaluated-expression
template <typename Lhs_, typename Rhs_>
bool are_equal_typeid (Lhs_ const &lhs, Rhs_ const &rhs)
{
    return typeid(lhs) == typeid(rhs);
}

inline bool are_equal (Base const &lhs, Base const &rhs)
{
    return &lhs == &rhs                                             // pointer equality
           ||                                                       // or
           (are_equal_typeid(lhs, rhs) &&                           // class equality and
            lhs.equals(rhs));                                       // value equality.
}

template <typename Lhs_, typename LhsDeleter_, typename Rhs_, typename RhsDeleter_>
bool are_equal (std::unique_ptr<Lhs_,LhsDeleter_> const &lhs, std::unique_ptr<Rhs_,RhsDeleter_> const &rhs)
{
    // TODO: Add static asserts to make sure that Lhs_ and Rhs_ are pointer or pointer-like types
    // (including unique_ptr, etc).
    return lhs.get() == rhs.get()                                   // pointer equality
           ||                                                       // or
           (lhs != nullptr &&
            rhs != nullptr &&                                       // pointer validity and
            are_equal_typeid(*lhs, *rhs) &&                         // class equality and
            lhs->equals(*rhs));                                     // value equality.
}

template <typename Lhs_, typename Rhs_>
bool are_equal (gsl::not_null<Lhs_> const &lhs, gsl::not_null<Rhs_> const &rhs)
{
    return are_equal(*lhs, *rhs);
}

//
// clone_of
//

template <typename T_>
nnup<T_> clone_of (T_ const &t)
{
    T_ *t_cloned = t.cloned();
    assert(t_cloned != nullptr);
//     assert(are_equal(t, *t_cloned)); // NOTE: This is potentially very slow
    return nnup<T_>(up<T_>(t_cloned));
}

template <typename T_>
up<T_> clone_of (T_ const *t)
{
    if (t == nullptr)
        return nullptr;
    else
    {
        T_ *t_cloned = t->cloned();
        assert(t_cloned != nullptr);
//         assert(are_equal(*t, *t_cloned)); // NOTE: This is potentially very slow
        return up<T_>(t_cloned);
    }
}

template <typename T_>
nnup<std::remove_const_t<T_>> clone_of (nn<T_*> const &t)
{
    std::remove_const_t<T_> *t_cloned = t->cloned();
    assert(t_cloned != nullptr);
//     assert(are_equal(*t, *t_cloned)); // NOTE: This is potentially very slow
    return nnup<std::remove_const_t<T_>>(up<std::remove_const_t<T_>>(t_cloned));
}

template <typename T_>
up<std::remove_const_t<T_>> clone_of (up<T_> const &t)
{
    if (t == nullptr)
        return nullptr;
    else
    {
        std::remove_const_t<T_> *t_cloned = t->cloned();
        assert(t_cloned != nullptr);
//         assert(are_equal(*t, *t_cloned)); // NOTE: This is potentially very slow
        return up<std::remove_const_t<T_>>(t_cloned);
    }
}

template <typename T_>
nnup<std::remove_const_t<T_>> clone_of (nnup<T_> const &t)
{
    std::remove_const_t<T_> *t_cloned = t->cloned();
    assert(t_cloned != nullptr);
//     assert(are_equal(*t, *t_cloned)); // NOTE: This is potentially very slow
    return nnup<std::remove_const_t<T_>>(up<std::remove_const_t<T_>>(t_cloned));
}

// TODO: Does it makes sense to override clone_of(nn<T_> &&) to effectively forward the && to the inside?

//
// overloads for printing stuff
//

inline std::ostream &operator << (std::ostream &out, Base const &b)
{
    Log log_out(out);
    b.print(log_out);
    return out;
}

// Note that there must be a corresponding specialization of lvd::HasCustomLogOutputOverload for this to work.
inline Log &operator << (Log &out, Base const &b)
{
    b.print(out);
    return out;
}

} // end namespace sem
} // end namespace cbz

// For the above operator<< function to work, the following needs a definition with value = true.
template <> struct lvd::HasCustomLogOutputOverload<cbz::sem::Base> : std::true_type { };
