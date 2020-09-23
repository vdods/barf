// 2016.08.09 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include "barftest/sem/Base.hpp"
#include "barftest/sem/Type.hpp"
#include <cstdint>

namespace barftest {
namespace sem {

// TODO: Rename this to ValueLiteralBase
struct ValueBase : public Base
{
    ValueBase (FiRange const &firange) : Base(firange) { }
    virtual ~ValueBase () { }

    virtual bool is_value () const override { return true; }
    virtual ValueBase *cloned () const override = 0;
};

// TODO: Rename this to ValueLiteral
template <TypeEnum TYPE_ENUM_, typename T_>
struct Value : public ValueBase
{
    static TypeEnum constexpr TYPE_ENUM = TYPE_ENUM_;
    typedef T_ T;

    Value (T_ value) : Value(FiRange::INVALID, value) { }
    Value (FiRange const &firange, T_ value) : ValueBase(firange), m_value(value) { }
    virtual ~Value () { }

    virtual TypeEnum type_enum__raw () const override { return TYPE_ENUM_; }
    virtual bool equals (Base const &other_) const override
    {
        Value const &other = dynamic_cast<Value const &>(other_);
        return m_value == other.m_value;
    }
    virtual Value *cloned () const override
    {
        return new Value(firange(), m_value);
    }
    virtual void print (Log &out) const override;

    T_ value () const { return m_value; }

    void set_value (T_ new_value) { m_value = new_value; }

private:

    T_ m_value;
};

typedef Value<TypeEnum::BOOLEAN_VALUE,bool> BooleanValue;
typedef Value<TypeEnum::SINT8_VALUE,int8_t> Sint8Value;
typedef Value<TypeEnum::SINT16_VALUE,int16_t> Sint16Value;
typedef Value<TypeEnum::SINT32_VALUE,int32_t> Sint32Value;
typedef Value<TypeEnum::SINT64_VALUE,int64_t> Sint64Value;
typedef Value<TypeEnum::UINT8_VALUE,int8_t> Uint8Value;
typedef Value<TypeEnum::UINT16_VALUE,int16_t> Uint16Value;
typedef Value<TypeEnum::UINT32_VALUE,int32_t> Uint32Value;
typedef Value<TypeEnum::UINT64_VALUE,int64_t> Uint64Value;
// typedef Value<TypeEnum::FLOAT16_VALUE,float> Float16Value;
typedef Value<TypeEnum::FLOAT32_VALUE,float> Float32Value;
typedef Value<TypeEnum::FLOAT64_VALUE,double> Float64Value;

template <> inline void BooleanValue::print (Log &out) const { out << (m_value ? "TRUE" : "FALSE"); }
template <> inline void Sint8Value::print (Log &out) const { out << m_value; }
template <> inline void Sint16Value::print (Log &out) const { out << m_value; }
template <> inline void Sint32Value::print (Log &out) const { out << m_value; }
template <> inline void Sint64Value::print (Log &out) const { out << m_value; }
template <> inline void Uint8Value::print (Log &out) const { out << m_value; }
template <> inline void Uint16Value::print (Log &out) const { out << m_value; }
template <> inline void Uint32Value::print (Log &out) const { out << m_value; }
template <> inline void Uint64Value::print (Log &out) const { out << m_value; }
// template <> inline void Float16Value::print (Log &out) const { out << m_value; }
template <> inline void Float32Value::print (Log &out) const { out << m_value; }
template <> inline void Float64Value::print (Log &out) const { out << m_value; }

// // Singletons for TRUE and FALSE BooleanValue-s.
// extern nnup<BooleanValue> const TRUE;
// extern nnup<BooleanValue> const FALSE;

template <typename... Args_>
nnup<BooleanValue> make_boolean_value (Args_&&... args)
{
    return make_nnup<BooleanValue>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Sint8Value> make_sint8_value (Args_&&... args)
{
    return make_nnup<Sint8Value>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Sint16Value> make_sint16_value (Args_&&... args)
{
    return make_nnup<Sint16Value>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Sint32Value> make_sint32_value (Args_&&... args)
{
    return make_nnup<Sint32Value>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Sint64Value> make_sint64_value (Args_&&... args)
{
    return make_nnup<Sint64Value>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Uint8Value> make_uint8_value (Args_&&... args)
{
    return make_nnup<Uint8Value>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Uint16Value> make_uint16_value (Args_&&... args)
{
    return make_nnup<Uint16Value>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Uint32Value> make_uint32_value (Args_&&... args)
{
    return make_nnup<Uint32Value>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Uint64Value> make_uint64_value (Args_&&... args)
{
    return make_nnup<Uint64Value>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Float32Value> make_float32_value (Args_&&... args)
{
    return make_nnup<Float32Value>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<Float64Value> make_float64_value (Args_&&... args)
{
    return make_nnup<Float64Value>(std::forward<Args_>(args)...);
}

struct VoidValue : public ValueBase
{
    VoidValue () : VoidValue(FiRange::INVALID) { }
    VoidValue (FiRange const &firange) : ValueBase(firange) { }
    virtual ~VoidValue () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::VOID_VALUE; }
    virtual bool equals (Base const &other) const override
    {
        assert(dynamic_cast<VoidValue const *>(&other) != nullptr);
        return true;
    }
    virtual VoidValue *cloned () const override
    {
        return new VoidValue(firange());
    }
    virtual void print (Log &out) const override { out << "void"; }
};

template <typename... Args_>
nnup<VoidValue> make_void_value (Args_&&... args)
{
    return make_nnup<VoidValue>(std::forward<Args_>(args)...);
}

struct NullValue : public ValueBase
{
    NullValue () : NullValue(FiRange::INVALID) { }
    NullValue (FiRange const &firange) : ValueBase(firange) { }
    virtual ~NullValue () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::NULL_VALUE; }
    virtual bool equals (Base const &other) const override
    {
        assert(dynamic_cast<NullValue const *>(&other) != nullptr);
        return true;
    }
    virtual NullValue *cloned () const override
    {
        return new NullValue(firange());
    }
    virtual void print (Log &out) const override { out << "null"; }
};

template <typename... Args_>
nnup<NullValue> make_null_value (Args_&&... args)
{
    return make_nnup<NullValue>(std::forward<Args_>(args)...);
}

} // end namespace sem

inline uint64_t string_to_uint64_t (std::string const &s)
{
    static_assert(sizeof(long) == sizeof(uint64_t), "expected long to be 64 bit type");
    return std::stoull(s);
}

inline int64_t string_to_int64_t (std::string const &s)
{
    static_assert(sizeof(long) == sizeof(int64_t), "expected long to be 64 bit type");
    return std::stoll(s);
}

inline double string_to_double (std::string const &s)
{
    return std::stod(s);
}

} // end namespace barftest
