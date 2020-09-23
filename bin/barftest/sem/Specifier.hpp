// 2019.09.23 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include "barftest/sem/Base.hpp"
#include "barftest/sem/Determinability.hpp"
#include "barftest/sem/GlobalValueLinkage.hpp"
#include "barftest/sem/ValueKind.hpp"
#include "barftest/sem/ValueLifetime.hpp"

namespace barftest {
namespace sem {

struct SpecifierBase : public Base
{
    SpecifierBase (FiRange const &firange) : Base(firange) { }
    virtual ~SpecifierBase () { }
};

template <TypeEnum TYPE_ENUM_, typename T_>
struct Specifier : public SpecifierBase
{
    Specifier (T_ value) : Specifier(FiRange::INVALID, value) { }
    Specifier (FiRange const &firange, T_ value) : SpecifierBase(firange), m_value(value) { }
    virtual ~Specifier () { }

    virtual TypeEnum type_enum__raw () const override { return TYPE_ENUM_; }
    virtual bool equals (Base const &other_) const override
    {
        Specifier const &other = dynamic_cast<Specifier const &>(other_);
        return m_value == other.m_value;
    }
    virtual Specifier *cloned () const override
    {
        return new Specifier(firange(), m_value);
    }
    virtual void print (Log &out) const override
    {
        out << "Specifier<" << TYPE_ENUM_ << ">(" << m_value << ')';
    }

    T_ value () const { return m_value; }

private:

    T_ m_value;
};

typedef Specifier<TypeEnum::VALUE_KIND_SPECIFIER,ValueKindContextual> ValueKindSpecifier;
typedef Specifier<TypeEnum::VALUE_LIFETIME_SPECIFIER,ValueLifetimeContextual> ValueLifetimeSpecifier;
typedef Specifier<TypeEnum::GLOBAL_VALUE_LIFETIME,GlobalValueLinkageContextual> GlobalValueLinkageSpecifier;

template <typename... Args_>
nnup<ValueKindSpecifier> make_value_kind_specifier (Args_&&... args)
{
    return make_nnup<ValueKindSpecifier>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<ValueLifetimeSpecifier> make_value_lifetime_specifier (Args_&&... args)
{
    return make_nnup<ValueLifetimeSpecifier>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<GlobalValueLinkageSpecifier> make_global_value_linkage_specifier (Args_&&... args)
{
    return make_nnup<GlobalValueLinkageSpecifier>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace barftest
