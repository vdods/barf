// 2016.08.10 - Victor Dods

#pragma once

#include "core.hpp"
#include "sem/Base.hpp"
#include "sem/Vector.hpp"

namespace cbz {
namespace sem {

enum class LoopType : uint8_t
{
    WHILE_DO = 0,
    DO_WHILE,
    // TODO: make until-do and do-until, for, repeat-forever (or something)

    COUNT
};

std::string const &as_string (LoopType loop_type);

inline std::ostream &operator << (std::ostream &out, LoopType loop_type)
{
    return out << as_string(loop_type);
}

struct Loop : public Base
{
    Loop (LoopType loop_type, nnup<Base> &&condition, up<StatementList> &&body = nullptr) : Loop(firange_of(condition)+firange_of(body), loop_type, std::move(condition), std::move(body)) { }
    Loop (FiRange const &firange, LoopType loop_type, nnup<Base> &&condition, up<StatementList> &&body = nullptr)
    :   Base(firange)
    ,   m_loop_type(loop_type)
    ,   m_condition(std::move(condition))
    ,   m_body(std::move(body))
    { }
    virtual ~Loop () { }

    virtual TypeEnum type_enum__raw () const override { return TypeEnum::LOOP; }
    virtual bool equals (Base const &other) const override;
    virtual Loop *cloned () const override;
    virtual void print (Log &out) const override;
    virtual void resolve_symbols (cgen::Context &context) override;

    virtual void generate_code (cgen::Context &context) const override;

    void set_body (nnup<StatementList> &&body)
    {
        m_body = std::move(body);
        grow_firange(m_body->firange());
    }

    LoopType loop_type () const { return m_loop_type; }
    Base const &condition () const { return *m_condition; }
    bool has_body () const { return m_body != nullptr; }
    StatementList const &body () const { return *m_body; }
    StatementList &body () { return *m_body; }

private:

    // TODO: initial_statement and end_of_loop_statement, so FOR loops can be done.
    LoopType m_loop_type;
    nnup<Base> m_condition;
    up<StatementList> m_body;
};

template <typename... Args_>
nnup<Loop> make_loop (Args_&&... args)
{
    return make_nnup<Loop>(std::forward<Args_>(args)...);
}

} // end namespace sem
} // end namespace cbz
