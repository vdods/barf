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
