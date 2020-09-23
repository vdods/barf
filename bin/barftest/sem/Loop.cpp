// 2016.08.10 - Victor Dods

#include "barftest/sem/Loop.hpp"

#include "barftest/Exception.hpp"

namespace barftest {
namespace sem {

std::string const &as_string (LoopType loop_type)
{
    static std::string const STRING_TABLE[uint32_t(LoopType::COUNT)] = {
        "WHILE_DO",
        "DO_WHILE",
    };
    return STRING_TABLE[uint32_t(loop_type)];
}

bool Loop::equals (Base const &other_) const
{
    Loop const &other = dynamic_cast<Loop const &>(other_);
    return m_loop_type == other.m_loop_type &&
           are_equal(m_condition, other.m_condition) &&
           are_equal(m_body, other.m_body);
}

Loop *Loop::cloned () const
{
    return new Loop(firange(), m_loop_type, clone_of(m_condition), clone_of(m_body));
}

void Loop::print (Log &out) const
{
    out << "Loop(" << firange() << '\n';
    {
        IndentGuard ig(out);
        out << m_loop_type << ",\n";
        switch (m_loop_type)
        {
            case LoopType::WHILE_DO:
                out << m_condition << ",\n"
                    << m_body << '\n';
                break;

            case LoopType::DO_WHILE:
                out << m_body << ",\n"
                    << m_condition << '\n';
                break;

            default:
                assert(false); // this should never happen.
        }
    }
    out << ')';
}

} // end namespace sem
} // end namespace barftest
