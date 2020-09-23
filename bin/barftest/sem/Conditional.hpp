// 2016.08.09 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include "barftest/sem/Base.hpp"

namespace barftest {
namespace sem {

struct Conditional : public Base
{
    Conditional (nnup<Base> &&condition, nnup<Base> &&positive_element, up<Base> &&negative_element = nullptr);
    Conditional (FiRange const &firange, nnup<Base> &&condition, nnup<Base> &&positive_element, up<Base> &&negative_element = nullptr);
    virtual ~Conditional () { }

    virtual bool equals (Base const &other) const override;
    virtual Conditional *cloned () const override = 0;

    Base const &condition () const { return *m_condition; }
    Base const &positive_element () const { return *m_positive_element; }
    bool has_negative_element () const { return m_negative_element != nullptr; }
    Base const &negative_element () const
    {
        if (m_negative_element == nullptr)
            LVD_ABORT_WITH_FIRANGE("negative_element is null; you should have checked has_negative_element()", firange());
        return *m_negative_element;
    }
    Base const *negative_element_ptr () const { return m_negative_element.get(); }

private:

    nnup<Base> m_condition;
    nnup<Base> m_positive_element;
    up<Base> m_negative_element;
};

struct ConditionalExpression : public Conditional
{
    ConditionalExpression (nnup<Base> &&condition, nnup<Base> &&positive_element, nnup<Base> &&negative_element)
    :   Conditional(std::move(condition), std::move(positive_element), std::move(negative_element))
    { }
    ConditionalExpression (FiRange const &firange, nnup<Base> &&condition, nnup<Base> &&positive_element, nnup<Base> &&negative_element)
    :   Conditional(firange, std::move(condition), std::move(positive_element), std::move(negative_element))
    { }
    virtual ~ConditionalExpression () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::CONDITIONAL_EXPRESSION; }
    virtual ConditionalExpression *cloned () const override;
    virtual void print (Log &out) const override;
};

struct ConditionalStatement : public Conditional
{
    ConditionalStatement (nnup<Base> &&condition, nnup<Base> &&positive_element, up<Base> &&negative_element = nullptr)
    :   Conditional(std::move(condition), std::move(positive_element), std::move(negative_element))
    { }
    ConditionalStatement (FiRange const &firange, nnup<Base> &&condition, nnup<Base> &&positive_element, up<Base> &&negative_element = nullptr)
    :   Conditional(firange, std::move(condition), std::move(positive_element), std::move(negative_element))
    { }
    virtual ~ConditionalStatement () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::CONDITIONAL_STATEMENT; }
    virtual ConditionalStatement *cloned () const override;
    virtual void print (Log &out) const override;
};

template <typename... Args_>
nnup<ConditionalExpression> make_conditional_expression (Args_&&... args)
{
    return make_nnup<ConditionalExpression>(std::forward<Args_>(args)...);
}

template <typename... Args_>
nnup<ConditionalStatement> make_conditional_statement (Args_&&... args)
{
    return make_nnup<ConditionalStatement>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace barftest
