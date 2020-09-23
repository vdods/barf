// 2016.08.09 - Victor Dods

#pragma once

#include "barftest/core.hpp"
#include "barftest/sem/FunctionType.hpp"
#include "barftest/sem/Value.hpp"
#include "barftest/sem/Vector.hpp"

namespace llvm {

class Function;
class PointerType;

} // end namespace llvm

namespace barftest {
namespace sem {

struct FunctionPrototype;

struct Function : public ValueBase
{
    Function (nnup<FunctionPrototype> &&function_prototype, up<StatementList> &&body = nullptr);
    Function (FiRange const &firange, nnup<FunctionPrototype> &&function_prototype, up<StatementList> &&body = nullptr)
    :   ValueBase(firange)
    ,   m_function_prototype(std::move(function_prototype))
    ,   m_body(std::move(body))
    { }
    virtual ~Function ();

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::FUNCTION; }
    virtual bool equals (Base const &other) const override;
    virtual Function *cloned () const override;
    virtual void print (Log &out) const override;

    void set_body (nnup<StatementList> &&body) { m_body = std::move(body); }

    FunctionPrototype const &function_prototype () const { return *m_function_prototype; }
    bool has_body () const { return m_body != nullptr; }
    StatementList const &body () const { return *m_body; }

private:

    nnup<FunctionPrototype> m_function_prototype;
    up<StatementList> m_body;
};

template <typename... Args_>
nnup<Function> make_function (Args_&&... args)
{
    return make_nnup<Function>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace barftest
