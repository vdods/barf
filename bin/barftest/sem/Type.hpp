// 2016.08.08 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include "barftest/sem/Base.hpp"

namespace llvm {

class Type;

} // end namespace llvm

namespace barftest {
namespace sem {

// Used for declarations.
struct TypeBaseOrTypeKeyword : public Base
{
    TypeBaseOrTypeKeyword (FiRange const &firange) : Base(firange) { }
    virtual ~TypeBaseOrTypeKeyword () { }

    virtual TypeBaseOrTypeKeyword *cloned () const override = 0;
};

struct TypeKeyword : public TypeBaseOrTypeKeyword
{
    TypeKeyword (FiRange const &firange = FiRange::INVALID) : TypeBaseOrTypeKeyword(firange) { }
    virtual ~TypeKeyword () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::TYPE_KEYWORD; }
    virtual bool equals (Base const &other) const override { return true; } // Because each template instance of TypeKeyword is a singleton.
    virtual TypeKeyword *cloned () const override;
    virtual void print (Log &out) const override;

    // Note that is_type is the default `return false` -- the `type` keyword is a metatype.
};

template <typename... Args_>
nnup<TypeKeyword> make_type_keyword (Args_&&... args)
{
    return make_nnup<TypeKeyword>(std::forward<Args_>(args)...);
}

// Base class for types (e.g. Boolean, Sint64, Float64, etc)
struct TypeBase : public TypeBaseOrTypeKeyword
{
    TypeBase (FiRange const &firange) : TypeBaseOrTypeKeyword(firange) { }
    virtual ~TypeBase () { }

    virtual bool is_type () const override { return true; }
    virtual TypeBase *cloned () const override = 0; // TESTING -- does this work?
};

template <TypeEnum TYPE_ENUM_>
struct Type : public TypeBase
{
    static TypeEnum constexpr TYPE_ENUM = TYPE_ENUM_;
    static Type const SINGLETON;

    Type (FiRange const &firange = FiRange::INVALID) : TypeBase(firange) { }
    virtual ~Type () { }

    virtual TypeEnum type_enum__raw () const override { return TYPE_ENUM_; }
    virtual bool equals (Base const &other) const override { return true; } // Because each template instance of Type is a singleton.
    virtual Type *cloned () const override;
    virtual void print (Log &out) const override;
};

typedef Type<TypeEnum::VOID_TYPE> VoidType;
typedef Type<TypeEnum::NULL_TYPE> NullType;
typedef Type<TypeEnum::BOOLEAN> Boolean;
typedef Type<TypeEnum::SINT8> Sint8;
typedef Type<TypeEnum::SINT16> Sint16;
typedef Type<TypeEnum::SINT32> Sint32;
typedef Type<TypeEnum::SINT64> Sint64;
typedef Type<TypeEnum::UINT8> Uint8;
typedef Type<TypeEnum::UINT16> Uint16;
typedef Type<TypeEnum::UINT32> Uint32;
typedef Type<TypeEnum::UINT64> Uint64;
typedef Type<TypeEnum::FLOAT32> Float32;
typedef Type<TypeEnum::FLOAT64> Float64;
typedef Type<TypeEnum::TYPE_DUMMY> TypeDummy;

template <TypeEnum TYPE_ENUM_> Type<TYPE_ENUM_> const Type<TYPE_ENUM_>::SINGLETON;

template <typename... Args_>
nnup<VoidType> make_void_type (Args_&&... args)
{
    return make_nnup<VoidType>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<NullType> make_null_type (Args_&&... args)
{
    return make_nnup<NullType>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Boolean> make_boolean (Args_&&... args)
{
    return make_nnup<Boolean>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Sint8> make_sint8 (Args_&&... args)
{
    return make_nnup<Sint8>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Sint16> make_sint16 (Args_&&... args)
{
    return make_nnup<Sint16>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Sint32> make_sint32 (Args_&&... args)
{
    return make_nnup<Sint32>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Sint64> make_sint64 (Args_&&... args)
{
    return make_nnup<Sint64>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Uint8> make_uint8 (Args_&&... args)
{
    return make_nnup<Uint8>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Uint16> make_uint16 (Args_&&... args)
{
    return make_nnup<Uint16>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Uint32> make_uint32 (Args_&&... args)
{
    return make_nnup<Uint32>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Uint64> make_uint64 (Args_&&... args)
{
    return make_nnup<Uint64>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Float32> make_float32 (Args_&&... args)
{
    return make_nnup<Float32>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Float64> make_float64 (Args_&&... args)
{
    return make_nnup<Float64>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<TypeDummy> make_type_dummy (Args_&&... args)
{
    return make_nnup<TypeDummy>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace barftest
