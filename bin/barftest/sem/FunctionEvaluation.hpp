// 2016.08.09 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include "barftest/sem/Base.hpp"
#include "barftest/sem/Vector.hpp"

namespace barftest {
namespace sem {

struct FunctionEvaluation : public Base
{
    FunctionEvaluation (nnup<Base> &&function_expression, nnup<ParameterList> &&parameter_list) : FunctionEvaluation(firange_of(function_expression)+firange_of(parameter_list), std::move(function_expression), std::move(parameter_list)) { }
    FunctionEvaluation (FiRange const &firange, nnup<Base> &&function_expression, nnup<ParameterList> &&parameter_list)
    :   Base(firange)
    ,   m_function_expression(std::move(function_expression))
    ,   m_parameter_list(std::move(parameter_list))
    { }
    virtual ~FunctionEvaluation () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::FUNCTION_EVALUATION; }
    virtual bool equals (Base const &other) const override;
    virtual FunctionEvaluation *cloned () const override;
    virtual void print (Log &out) const override;

    Base const &function_expression () const { return *m_function_expression; }
    ParameterList const &parameter_list () const { return *m_parameter_list; }

private:

    nnup<Base> m_function_expression;
    nnup<ParameterList> m_parameter_list;
};

template <typename... Args_>
nnup<FunctionEvaluation> make_function_evaluation (Args_&&... args)
{
    return make_nnup<FunctionEvaluation>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace barftest
