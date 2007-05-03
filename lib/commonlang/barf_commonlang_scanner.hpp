// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY
// barf_commonlang_scanner.hpp generated by reflex
// from barf_commonlang_scanner.reflex using reflex.cpp.langspec and reflex.cpp.header.codespec
// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY

#include <cassert>
#include <deque>
#include <string>

#if !defined(ReflexCpp_namespace_)
#define ReflexCpp_namespace_
namespace ReflexCpp_ {

// /////////////////////////////////////////////////////////////////////////////
// a bunch of template metaprogramming to intelligently determine
// what type to use for an integer of the given bit width
// /////////////////////////////////////////////////////////////////////////////

template <bool condition_, typename Then_, typename Else_> struct If_;
template <typename Then_, typename Else_> struct If_<true,Then_,Else_> { typedef Then_ T_; };
template <typename Then_, typename Else_> struct If_<false,Then_,Else_> { typedef Else_ T_; };

template <bool condition_> struct Assert_;
template <> struct Assert_<true> { static bool const v_ = true; operator bool () { return v_; } };

template <typename Sint_, typename Uint_> struct IntPair_ { typedef Sint_ S_; typedef Uint_ U_; };

template <int bits_> struct Integer_ {
private:

    typedef
        typename If_<bits_ == 8*sizeof(char),      IntPair_<char,unsigned char>,
        typename If_<bits_ == 8*sizeof(short),     IntPair_<short,unsigned short>,
        typename If_<bits_ == 8*sizeof(int),       IntPair_<int,unsigned int>,
        typename If_<bits_ == 8*sizeof(long),      IntPair_<long,unsigned long>,
        typename If_<bits_ == 8*sizeof(long long), IntPair_<long long,unsigned long long>,
        Integer_<0> // if no match, cause a compile error
        >::T_ >::T_ >::T_ >::T_ >::T_ PrivateIntPair_;
    static bool const assert_size =
        Assert_<bits_ == 8*sizeof(typename PrivateIntPair_::S_) &&
                bits_ == 8*sizeof(typename PrivateIntPair_::U_)>::v_;

public:

    typedef typename PrivateIntPair_::S_ Signed_;
    typedef typename PrivateIntPair_::U_ Unsigned_;
};
template <> struct Integer_<0> { }; // empty for intentional compile errors

// /////////////////////////////////////////////////////////////////////////////
// typedefs for the integer types used in the "cpp" reflex target
// /////////////////////////////////////////////////////////////////////////////

typedef Integer_<8> ::Signed_   Sint8_;
typedef Integer_<8> ::Unsigned_ Uint8_;
typedef Integer_<16>::Signed_   Sint16_;
typedef Integer_<16>::Unsigned_ Uint16_;
typedef Integer_<32>::Signed_   Sint32_;
typedef Integer_<32>::Unsigned_ Uint32_;
typedef Integer_<8*sizeof(void*)>::Signed_   Diff_;
typedef Integer_<8*sizeof(void*)>::Unsigned_ Size_;

enum
{
    TYPE_SIZE_ASSERTIONS_ =
        Assert_<sizeof(Sint8_) == 1>::v_ &&
        Assert_<sizeof(Uint8_) == 1>::v_ &&
        Assert_<sizeof(Sint16_) == 2>::v_ &&
        Assert_<sizeof(Uint16_) == 2>::v_ &&
        Assert_<sizeof(Sint32_) == 4>::v_ &&
        Assert_<sizeof(Uint32_) == 4>::v_ &&
        Assert_<sizeof(Diff_) == sizeof(void*)>::v_ &&
        Assert_<sizeof(Size_) == sizeof(void*)>::v_
};

// /////////////////////////////////////////////////////////////////////////////
// implements the "input buffer" API as described in the documentation
// /////////////////////////////////////////////////////////////////////////////

class InputBuffer_
{
public:

    typedef bool (InputBuffer_::*IsInputAtEndMethod_)();
    typedef Uint8_ (InputBuffer_::*ReadNextAtomMethod_)();

    InputBuffer_ (IsInputAtEndMethod_ IsInputAtEnd_, ReadNextAtomMethod_ ReadNextAtom_)
        :
        m_IsInputAtEnd_(IsInputAtEnd_),
        m_ReadNextAtom_(ReadNextAtom_)
    {
        // subclasses must call InputBuffer_::ResetForNewInput_ in their constructors.
    }

    bool IsAtEndOfInput () { return IsConditionalMet_(CF_END_OF_INPUT, CF_END_OF_INPUT); }

protected:

    Uint8_ GetCurrentConditionalFlags_ ()
    {
        UpdateConditionalFlags_();
        return m_current_conditional_flags_;
    }
    bool IsConditionalMet_ (Uint8_ conditional_mask, Uint8_ conditional_flags)
    {
        UpdateConditionalFlags_();
        // return true iff no bits indicated by conditional_mask differ between
        // conditional_flags and m_current_conditional_flags_.
        return ((conditional_flags ^ m_current_conditional_flags_) & conditional_mask) == 0;
    }
    Uint8_ GetInputAtom_ ()
    {
        FillBuffer_();
        assert(m_read_cursor_ > 0);
        assert(m_read_cursor_ < m_buffer_.size());
        return m_buffer_[m_read_cursor_];
    }
    void AdvanceReadCursor_ ()
    {
        assert(m_read_cursor_ > 0);
        assert(m_read_cursor_ < m_buffer_.size());
        if (m_buffer_[m_read_cursor_] != '\0')
            ++m_read_cursor_;
    }
    void SetAcceptCursor_ ()
    {
        assert(m_read_cursor_ > 0);
        assert(m_read_cursor_ <= m_buffer_.size());
        m_accept_cursor_ = m_read_cursor_;
    }
    void Accept_ (std::string &s)
    {
        assert(s.empty());
        assert(m_accept_cursor_ > 0 && "can't Accept_ if the accept cursor was not set");
        assert(m_accept_cursor_ <= m_buffer_.size());
        assert(m_read_cursor_ > 0);
        assert(m_read_cursor_ <= m_buffer_.size());
        assert(m_accept_cursor_ <= m_read_cursor_);
        // calculate the iterators for the buffer string to erase.
        Buffer_::iterator delete_it = m_buffer_.begin();
        Buffer_::iterator delete_it_end = delete_it;
        for (Buffer_::size_type i = 0; i < m_accept_cursor_-1; ++i)
        {
            assert(delete_it_end != m_buffer_.end());
            ++delete_it_end;
        }
        // the accepted string iterators are one atom right from the
        // buffer's to-be-erased string.
        Buffer_::iterator accept_it = delete_it;
        ++accept_it;
        Buffer_::iterator accept_it_end = delete_it_end;
        assert(accept_it_end != m_buffer_.end());
        ++accept_it_end;
        // extract the accepted string and erase all but the new previous atom
        s.insert(s.begin(), accept_it, accept_it_end);
        m_buffer_.erase(delete_it, delete_it_end);
        assert(m_buffer_.size() >= 1);
        // reset the cursors
        m_read_cursor_ = 1;
        m_accept_cursor_ = 0;
    }
    void Reject_ (std::string &s)
    {
        assert(s.empty());
        assert(m_accept_cursor_ == 0 && "can't Reject_ if the accept cursor was set");
        assert(m_read_cursor_ > 0);
        assert(m_read_cursor_ < m_buffer_.size());
        assert(m_buffer_.size() >= 2);
        // only pop the front of the buffer if we're not at EOF.
        Uint8_ rejected_atom = m_buffer_[1];
        if (m_buffer_[1] != '\0')
            m_buffer_.pop_front();
        // reset the read cursor and return the rejected atom
        m_read_cursor_ = 1;
        s += rejected_atom;
    }
    void ResetForNewInput_ ()
    {
        m_buffer_.clear();
        m_buffer_.push_front('\0'); // special "previous" atom for beginning of input
        assert(m_buffer_.size() == 1);
        m_read_cursor_ = 1;
        m_accept_cursor_ = 0;
        m_current_conditional_flags_ = 0;
    }

private:

    enum ConditionalFlag_
    {
        CF_BEGINNING_OF_INPUT = (1 << 0),
        CF_END_OF_INPUT       = (1 << 1),
        CF_BEGINNING_OF_LINE  = (1 << 2),
        CF_END_OF_LINE        = (1 << 3),
        CF_WORD_BOUNDARY      = (1 << 4)
    }; // end of enum ReflexCpp_::InputBuffer_::ConditionalFlag_

    void FillBuffer_ ()
    {
        assert(m_read_cursor_ > 0);
        assert(m_read_cursor_ <= m_buffer_.size());
        // if we already have at least one atom ahead of the read cursor in
        // the input buffer, there is no need to suck another one out.
        if (m_read_cursor_ < m_buffer_.size())
            return;
        // if the last atom (in front of the read cursor) in the input buffer
        // is '\0' then we have reached EOF, so there is no need to suck
        // another atom out.
        if (m_buffer_.size() >= 2 && *m_buffer_.rbegin() == '\0')
        {
            assert(m_read_cursor_ < m_buffer_.size());
            return;
        }
        // if we're at the end of input, push a null char
        if ((this->*m_IsInputAtEnd_)())
            m_buffer_.push_back('\0');
        // otherwise retrieve and push the next input atom
        else
        {
            Uint8_ atom = (this->*m_ReadNextAtom_)();
            assert(atom != '\0' && "may not return '\\0' from return_next_input_char");
            m_buffer_.push_back(atom);
        }
        // ensure there is at least one atom on each side of the read cursor.
        assert(m_buffer_.size() >= 2);
        assert(m_read_cursor_ == m_buffer_.size()-1);
    }
    void UpdateConditionalFlags_ ()
    {
        FillBuffer_();
        assert(m_read_cursor_ > 0);
        assert(m_read_cursor_ < m_buffer_.size());
        // given the atoms surrounding the read cursor, calculate the
        // current conditional flags.
        m_current_conditional_flags_ = 0;
        if (m_buffer_[m_read_cursor_-1] == '\0')                                                m_current_conditional_flags_ |= CF_BEGINNING_OF_INPUT;
        if (m_buffer_[m_read_cursor_] == '\0')                                                  m_current_conditional_flags_ |= CF_END_OF_INPUT;
        if (m_buffer_[m_read_cursor_-1] == '\0' || m_buffer_[m_read_cursor_-1] == '\n')         m_current_conditional_flags_ |= CF_BEGINNING_OF_LINE;
        if (m_buffer_[m_read_cursor_] == '\0' || m_buffer_[m_read_cursor_] == '\n')             m_current_conditional_flags_ |= CF_END_OF_LINE;
        if (IsWordChar_(m_buffer_[m_read_cursor_-1]) != IsWordChar_(m_buffer_[m_read_cursor_])) m_current_conditional_flags_ |= CF_WORD_BOUNDARY;
    }
    static bool IsWordChar_ (Uint8_ c) { return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9' || c == '_'; }

    typedef std::deque<Uint8_> Buffer_;

    Uint8_ m_current_conditional_flags_;
    Buffer_ m_buffer_;
    Buffer_::size_type m_read_cursor_;
    Buffer_::size_type m_accept_cursor_;
    IsInputAtEndMethod_ m_IsInputAtEnd_;
    ReadNextAtomMethod_ m_ReadNextAtom_;
}; // end of class ReflexCpp_::InputBuffer_

// /////////////////////////////////////////////////////////////////////////////
// this baseclass contains all the state machinery for "cpp" reflex target
// /////////////////////////////////////////////////////////////////////////////

class Scanner_ : protected InputBuffer_
{
public:

    struct DfaTransition_;
    struct DfaState_
    {
        Uint32_ m_accept_handler_index_;
        Size_ m_transition_count_;
        DfaTransition_ const *m_transition_;
    }; // end of struct ReflexCpp_::Scanner_::DfaState_
    struct DfaTransition_
    {
        enum Type_
        {
            TT_INPUT_ATOM_ = 0, TT_INPUT_ATOM_RANGE_, TT_CONDITIONAL_, TT_EPSILON_
        }; // end of enum ReflexCpp_::Scanner_::DfaTransition_::Type_

        Uint8_ m_transition_type_;
        Uint8_ m_data_0_;
        Uint8_ m_data_1_;
        DfaState_ const *m_target_dfa_state_;

        bool AcceptsInputAtom_ (Uint8_ input_atom) const
        {
            assert(m_transition_type_ == TT_INPUT_ATOM_ || m_transition_type_ == TT_INPUT_ATOM_RANGE_);
            // returns true iff this transition is TT_INPUT_ATOM_ and input_atom
            // matches m_data_0_, or this transition is TT_INPUT_ATOM_RANGE_ and
            // input_atom is within the range [m_data_0_, m_data_1_] inclusive.
            return m_transition_type_ == TT_INPUT_ATOM_ &&
                   m_data_0_ == input_atom
                   ||
                   m_transition_type_ == TT_INPUT_ATOM_RANGE_ &&
                   m_data_0_ <= input_atom && input_atom <= m_data_1_;
        }
        bool AcceptsConditionalFlags_ (Uint8_ conditional_flags) const
        {
            assert(m_transition_type_ == TT_CONDITIONAL_);
            // returns true iff this transition is TT_CONDITIONAL_ and no relevant bits
            // in conditional_flags conflict with this transition's conditional mask
            // (m_data_0_) and flags (m_data_1_).
            return ((conditional_flags ^ m_data_1_) & m_data_0_) == 0;
        }
    }; // end of struct ReflexCpp_::Scanner_::DfaTransition_

    Scanner_ (
        DfaState_ const *state_table,
        Size_ state_count,
        DfaTransition_ const *transition_table,
        Size_ transition_count,
        Uint32_ accept_handler_count,
        IsInputAtEndMethod_ IsInputAtEnd_,
        ReadNextAtomMethod_ ReadNextAtom_)
        :
        InputBuffer_(IsInputAtEnd_, ReadNextAtom_),
        m_accept_handler_count_(accept_handler_count)
    {
        CheckDfa_(state_table, state_count, transition_table, transition_count);
        // subclasses must call ReflexCpp_::InputBuffer_::ResetForNewInput_ in their constructors.
    }

protected:

    DfaState_ const *InitialState_ () const
    {
        return m_initial_state_;
    }
    void InitialState_ (DfaState_ const *initial_state)
    {
        assert(initial_state != NULL);
        m_initial_state_ = initial_state;
    }
    void ResetForNewInput_ (DfaState_ const *initial_state)
    {
        InputBuffer_::ResetForNewInput_();
        InitialState_(initial_state);
        m_current_state_ = NULL;
        m_accept_state_ = NULL;
    }
    Uint32_ RunDfa_ (std::string &s)
    {
        // clear the destination string
        s.clear();
        // reset the current state to the initial state.
        assert(m_initial_state_ != NULL);
        m_current_state_ = m_initial_state_;
        assert(m_accept_state_ == NULL);
        // loop until there are no valid transitions from the current state.
        while (m_current_state_ != NULL)
        {
            // if the current state is an accept state, save it
            if (IsAcceptState_(m_current_state_))
            {
                m_accept_state_ = m_current_state_;
                SetAcceptCursor_();
            }
            // turn the crank on the state machine, exercising the appropriate
            // conditional (using m_current_conditional_flags_) or atomic
            // transition (using the atom at the read cursor in the buffer).
            // m_current_state_ will be set to the transitioned-to state, or
            // NULL if no transition was possible.
            m_current_state_ = ProcessInputAtom_();
        }
        // if we have a most recent accept state, accept the accumulated input
        // using the accept handler indicated by the most recent accept state.
        if (m_accept_state_ != NULL)
        {
            // extract the accepted string from the buffer
            Accept_(s);
            // save off the accept handler index
            Uint32_ accept_handler_index = m_accept_state_->m_accept_handler_index_;
            // clear the accept state for next time
            m_accept_state_ = NULL;
            // return accept_handler_index to indicate which handler to call
            return accept_handler_index;
        }
        // otherwise the input atom went unhandled, so call the special
        // unmatched character handler on the rejected atom.
        else
        {
            // put the rejected atom in the return string and indicate
            // that no accept handler should be called.
            Reject_(s);
            return m_accept_handler_count_;
        }
    }

private:

    DfaState_ const *ProcessInputAtom_ ()
    {
        assert(m_current_state_ != NULL);
        // get the current conditional flags and input atom once before looping
        Uint8_ current_conditional_flags = GetCurrentConditionalFlags_();
        Uint8_ input_atom = GetInputAtom_();
        // iterate through the current state's transitions, exercising the first
        // acceptable one and returning the target state
        for (DfaTransition_ const *transition = m_current_state_->m_transition_,
                                  *transition_end = transition + m_current_state_->m_transition_count_;
             transition != transition_end;
             ++transition)
        {
            assert(transition->m_transition_type_ == DfaTransition_::TT_INPUT_ATOM_ ||
                   transition->m_transition_type_ == DfaTransition_::TT_INPUT_ATOM_RANGE_ ||
                   transition->m_transition_type_ == DfaTransition_::TT_CONDITIONAL_);
            // if it's an atomic transition, check if it accepts input_atom.
            if (transition->m_transition_type_ == DfaTransition_::TT_INPUT_ATOM_ ||
                transition->m_transition_type_ == DfaTransition_::TT_INPUT_ATOM_RANGE_)
            {
                if (transition->AcceptsInputAtom_(input_atom))
                {
                    AdvanceReadCursor_();
                    return transition->m_target_dfa_state_;
                }
            }
            // otherwise it must be a conditional transition, so check the flags.
            else if (transition->AcceptsConditionalFlags_(current_conditional_flags))
                return transition->m_target_dfa_state_;
        }
        // if we reached here, no transition was possible, so return NULL.
        return NULL;
    }
    bool IsAcceptState_ (DfaState_ const *state) const
    {
        assert(state != NULL);
        return state->m_accept_handler_index_ < m_accept_handler_count_;
    }
    void CheckDfa_ (
        DfaState_ const *state_table,
        Size_ state_count,
        DfaTransition_ const *transition_table,
        Size_ transition_count)
    {
        // if any assertions in this method fail, the state and/or
        // transition tables were created incorrectly.
        assert(state_table != NULL && "must have a state table");
        assert(state_count > 0 && "must have at least one state");
        assert(transition_table != NULL && "must have a transition table");
        {
            DfaTransition_ const *t = transition_table;
            for (DfaState_ const *s = state_table, *s_end = state_table + state_count;
                 s != s_end;
                 ++s)
            {
                assert(s->m_transition_ == t &&
                       "states' transitions must be contiguous and in ascending order");
                t += s->m_transition_count_;
            }
            assert(t == transition_table + transition_count &&
                   "there are too many or too few referenced "
                   "transitions in the state table");
        }
        for (DfaTransition_ const *t = transition_table,
                                  *t_end = transition_table + transition_count;
             t != t_end;
             ++t)
        {
            assert((t->m_transition_type_ == DfaTransition_::TT_INPUT_ATOM_ ||
                    t->m_transition_type_ == DfaTransition_::TT_INPUT_ATOM_RANGE_ ||
                    t->m_transition_type_ == DfaTransition_::TT_CONDITIONAL_)
                   &&
                   "invalid DfaTransition_::Type_");
            assert(t->m_target_dfa_state_ >= state_table &&
                   t->m_target_dfa_state_ < state_table + state_count &&
                   "transition target state out of range "
                   "(does not point to a valid state)");
            if (t->m_transition_type_ == DfaTransition_::TT_INPUT_ATOM_RANGE_)
            {
                assert(t->m_data_0_ < t->m_data_1_ &&
                       "can't specify a single-element range of atoms");
            }
            else if (t->m_transition_type_ == DfaTransition_::TT_CONDITIONAL_)
            {
                assert(t->m_data_0_ != 0 &&
                       "can't have a conditional with a mask of zero");
                assert((t->m_data_1_ & ~t->m_data_0_) == 0 &&
                       "there are bits set in the conditional flags "
                       "which are outside of the conditional mask");
            }
        }
    }

    Uint32_ const m_accept_handler_count_;
    DfaState_ const *m_initial_state_;
    DfaState_ const *m_current_state_;
    DfaState_ const *m_accept_state_;
}; // end of class ReflexCpp_::Scanner_

} // end of namespace ReflexCpp_
#endif // !defined(ReflexCpp_namespace_)


#line 19 "barf_commonlang_scanner.reflex"

#if !defined(_BARF_COMMONLANG_SCANNER_HPP_)
#define _BARF_COMMONLANG_SCANNER_HPP_

#include "barf_commonlang.hpp"

#include <ostream>

#include "barf_inputbase.hpp"

namespace Barf {
namespace Ast {

class Base;

} // end of namespace Ast

namespace CommonLang {

#line 511 "barf_commonlang_scanner.hpp"

class Scanner : private ReflexCpp_::Scanner_, 
#line 39 "barf_commonlang_scanner.reflex"
 protected InputBase 
#line 516 "barf_commonlang_scanner.hpp"

{
public:

    using Scanner_::IsAtEndOfInput;

    struct Mode
    {
        enum Name
        {
            BLOCK_COMMENT = 0,
            CHAR_LITERAL_END = 6,
            CHAR_LITERAL_GUTS = 13,
            CHAR_LITERAL_INSIDE_STRICT_CODE_BLOCK = 26,
            DUMB_CODE_BLOCK = 33,
            MAIN = 39,
            REGULAR_EXPRESSION = 61,
            REGULAR_EXPRESSION_BRACKET_EXPRESSION = 72,
            STRICT_CODE_BLOCK = 82,
            STRING_LITERAL_GUTS = 93,
            STRING_LITERAL_INSIDE_STRICT_CODE_BLOCK = 108,
            // default starting scanner mode
            START_ = MAIN
        }; // end of enum Scanner::Mode::Name
    }; // end of struct Scanner::Mode


#line 40 "barf_commonlang_scanner.reflex"

    struct Token
    {
        enum Type
        {
            BAD_END_OF_FILE = 0x100,
            BAD_TOKEN,
            CHAR_LITERAL,
            DIRECTIVE_ADD_CODESPEC,
            DIRECTIVE_ADD_OPTIONAL_DIRECTIVE,
            DIRECTIVE_ADD_REQUIRED_DIRECTIVE,
            DIRECTIVE_DEFAULT,
            DIRECTIVE_DEFAULT_PARSE_NONTERMINAL,
            DIRECTIVE_DUMB_CODE_BLOCK,
            DIRECTIVE_ERROR,
            DIRECTIVE_ID,
            DIRECTIVE_LEFT,
            DIRECTIVE_MACRO,
            DIRECTIVE_NONASSOC,
            DIRECTIVE_PREC,
            DIRECTIVE_RIGHT,
            DIRECTIVE_START_IN_SCANNER_MODE,
            DIRECTIVE_STATE,
            DIRECTIVE_STRICT_CODE_BLOCK,
            DIRECTIVE_STRING,
            DIRECTIVE_TARGET,
            DIRECTIVE_TARGETS,
            DIRECTIVE_TOKEN,
            DIRECTIVE_TYPE,
            DUMB_CODE_BLOCK,
            END_OF_FILE,
            END_PREAMBLE,
            ID,
            NEWLINE,
            REGEX,
            STRICT_CODE_BLOCK,
            STRING_LITERAL,

            COUNT_PLUS_0x100
        }; // end of enum Scanner::Token::Type
    }; // end of struct Scanner::Token

#line 587 "barf_commonlang_scanner.hpp"

public:

    Scanner ();
    ~Scanner ();

    bool DebugSpew () const { return m_debug_spew_; }
    void DebugSpew (bool debug_spew) { m_debug_spew_ = debug_spew; }

    Mode::Name ScannerMode () const;
    void ScannerMode (Mode::Name mode);

    Scanner::Token::Type Scan (
#line 82 "barf_commonlang_scanner.reflex"
 Ast::Base **token 
#line 603 "barf_commonlang_scanner.hpp"
);

private:

    bool IsInputAtEnd_ ();
    ReflexCpp_::Uint8_ ReadNextAtom_ ();
    void ResetForNewInput_ ();

    static void DebugPrintAtom_ (ReflexCpp_::Uint8_ atom);
    static void DebugPrintString_ (std::string const &s);

    bool m_debug_spew_;

    static Scanner_::DfaState_ const ms_state_table_[];
    static ReflexCpp_::Size_ const ms_state_count_;
    static Scanner_::DfaTransition_ const ms_transition_table_[];
    static ReflexCpp_::Size_ const ms_transition_count_;
    static ReflexCpp_::Uint32_ const ms_accept_handler_count_;

public:

#line 83 "barf_commonlang_scanner.reflex"

    using InputBase::GetIsOpen;
    using InputBase::GetFiLoc;
    using InputBase::GetInputName;

    bool OpenFile (string const &input_filename);
    void OpenString (string const &input_string, string const &input_name, bool use_line_numbers = false);
    void OpenUsingStream (istream *input_stream, string const &input_name, bool use_line_numbers);

    using InputBase::Close;

private:

    Token::Type ParseDirective (string const &accepted_string, Ast::Base **token);

    bool m_is_in_preamble;
    Uint32 m_regex_paren_level;
    Uint32 m_regex_bracket_level;
    Uint32 m_code_block_bracket_level;
    Mode::Name m_return_state;

#line 647 "barf_commonlang_scanner.hpp"

}; // end of class Scanner


#line 104 "barf_commonlang_scanner.reflex"

ostream &operator << (ostream &stream, Scanner::Token::Type scanner_token_type);

} // end of namespace CommonLang
} // end of namespace Barf

#endif // !defined(_BARF_COMMONLANG_SCANNER_HPP_)

#line 661 "barf_commonlang_scanner.hpp"
