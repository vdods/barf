// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY
// calculator_scanner.hpp generated by reflex
// from calculator_scanner.reflex using reflex.cpp.targetspec and reflex.cpp.header.codespec
// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY

#pragma once

#include <cassert>
#include <cstdint>
#include <deque>
#include <string>

#if !defined(ReflexCpp_namespace_)
#define ReflexCpp_namespace_
namespace ReflexCpp_ {

// /////////////////////////////////////////////////////////////////////////////
// implements the InputApparatus interface as described in the documentation
// /////////////////////////////////////////////////////////////////////////////

class InputApparatus_Interactive_
{
protected:

    typedef bool (InputApparatus_Interactive_::*IsInputAtEndMethod_)();
    typedef std::uint8_t (InputApparatus_Interactive_::*ReadNextAtomMethod_)();

    InputApparatus_Interactive_ (IsInputAtEndMethod_ IsInputAtEnd, ReadNextAtomMethod_ ReadNextAtom)
        :
        m_IsInputAtEnd(IsInputAtEnd),
        m_ReadNextAtom(ReadNextAtom)
    {
        // subclasses must call InputApparatus_Interactive_::ResetForNewInput_ in their constructors.
    }

    bool IsAtEndOfInput () { return IsConditionalMet(CF_END_OF_INPUT, CF_END_OF_INPUT); }

    void KeepString ()
    {
        assert(m_accept_cursor > m_start_cursor && "may only KeepString within accept or reject handler code");
        // reset the start cursor, so that the entire accepted string is
        // "put back in the already-read buffer", but we'll keep reading
        // from the same read cursor.
        m_start_cursor = 0;
        // save the accepted string's end position at the time of KeepString
        m_kept_string_cursor = m_accept_cursor;
        // leave the accept cursor to be reset in the next loop
    }
    void Unaccept (std::uint32_t unaccept_char_count)
    {
        assert(m_accept_cursor > m_start_cursor && "may only Unaccept within accept handler code");
        UnacceptUnrejectCommon(unaccept_char_count);
    }
    void Unreject (std::uint32_t unreject_char_count)
    {
        assert(m_accept_cursor == m_start_cursor+1 && "may only Unreject within accept handler code");
        UnacceptUnrejectCommon(unreject_char_count);
    }

    void PrepareToScan_ ()
    {
        assert(m_start_cursor < m_read_cursor);
        // update the read and keep_string cursors.  if KeepString was called,
        // this should do nothing, since in that case, m_start_cursor will be
        // zero;  if Unaccept was called, it should have postcondition
        // m_read_cursor == m_start_cursor + 1
        m_read_cursor -= m_start_cursor;
        m_kept_string_cursor -= m_start_cursor;
        // dump the first m_start_cursor chars from the buffer
        while (m_start_cursor > 0)
        {
            --m_start_cursor;
            m_buffer.pop_front();
        }
        // reset the accept cursor
        m_accept_cursor = m_start_cursor;
        m_keep_string_has_been_called = false;
        assert(m_kept_string_cursor > m_start_cursor);
        assert(m_accept_cursor == 0);
    }
    void ResetForNewInput_ ()
    {
        m_current_conditional_flags = 0;
        m_buffer.clear();
        m_buffer.push_front('\0'); // special "previous" atom for beginning of input
        assert(m_buffer.size() == 1);
        m_start_cursor = 0;
        m_read_cursor = 1;
        m_kept_string_cursor = 1;
        m_accept_cursor = 0;
        m_keep_string_has_been_called = false;
    }

    // for use in AutomatonApparatus_FastAndBig_Interactive_ only
    std::uint8_t CurrentConditionalFlags_ ()
    {
        UpdateConditionalFlags();
        return m_current_conditional_flags;
    }
    std::uint8_t InputAtom_ ()
    {
        FillBuffer();
        assert(m_read_cursor > 0);
        assert(m_read_cursor < m_buffer.size());
        return m_buffer[m_read_cursor];
    }
    void AdvanceReadCursor_ ()
    {
        assert(m_start_cursor == 0);
        assert(m_read_cursor > 0);
        assert(m_read_cursor < m_buffer.size());
        if (m_buffer[m_read_cursor] != '\0')
            ++m_read_cursor;
    }
    void SetAcceptCursor_ ()
    {
        assert(m_start_cursor == 0);
        assert(m_read_cursor > 0);
        assert(m_read_cursor <= m_buffer.size());
        m_accept_cursor = m_read_cursor;
    }
    void Accept_ (std::string &s)
    {
        assert(m_accept_cursor > 0 && "can't Accept_ if the accept cursor is not set");
        assert(m_accept_cursor <= m_read_cursor);
        AcceptRejectCommon(s);
    }
    void Reject_ (std::string &s)
    {
        assert(m_accept_cursor == 0 && "can't Reject_ if the accept cursor was set");
        // must set the accept cursor to indicate the rejected string
        // (which is the kept string plus the rejected atom)
        m_accept_cursor = m_kept_string_cursor;
        assert(m_accept_cursor < m_buffer.size());
        if (m_buffer[m_accept_cursor] != '\0')
            ++m_accept_cursor;
        AcceptRejectCommon(s);
    }

private:

    void UnacceptUnrejectCommon (std::uint32_t char_count)
    {
        assert(!m_keep_string_has_been_called && "may only Unaccept/Unreject before KeepString");
        assert(char_count <= m_start_cursor && "can't Unaccept/Unreject more characters than were rejected");
        if (char_count == 0)
            return; // nothing to do
        // update the cursors
        m_start_cursor -= char_count;
        m_read_cursor = m_start_cursor + 1;
        m_kept_string_cursor = m_start_cursor + 1;
        m_accept_cursor -= char_count;
    }
    void AcceptRejectCommon (std::string &s)
    {
        assert(s.empty());
        assert(m_buffer.size() >= 2);
        assert(m_start_cursor == 0);
        // the accept cursor indicates the end of the string to accept/reject
        assert(m_accept_cursor > 0 && m_accept_cursor <= m_buffer.size());
        // there should not be an EOF-indicating '\0' at the end of the string
        assert(m_accept_cursor == 1 || m_buffer[m_accept_cursor-1] != '\0');
        // extract the accepted/rejected string: range [1,m_accept_cursor).
        s.insert(s.begin(), m_buffer.begin()+1, m_buffer.begin()+m_accept_cursor);
        assert(s.size() == m_accept_cursor-1);
        // set the start cursor to one before the end of the string
        // (the last char in the string becomes the previous atom)
        m_start_cursor = m_accept_cursor - 1;
        // reset the other cursors
        m_read_cursor = m_accept_cursor;
        m_kept_string_cursor = m_accept_cursor;
    }

    enum ConditionalFlag
    {
        CF_BEGINNING_OF_INPUT = (1 << 0),
        CF_END_OF_INPUT       = (1 << 1),
        CF_BEGINNING_OF_LINE  = (1 << 2),
        CF_END_OF_LINE        = (1 << 3),
        CF_WORD_BOUNDARY      = (1 << 4)
    }; // end of enum ReflexCpp_::InputApparatus_Interactive_::ConditionalFlag

    bool IsConditionalMet (std::uint8_t conditional_mask, std::uint8_t conditional_flags)
    {
        UpdateConditionalFlags();
        // return true iff no bits indicated by conditional_mask differ between
        // conditional_flags and m_current_conditional_flags.
        return ((conditional_flags ^ m_current_conditional_flags) & conditional_mask) == 0;
    }
    void FillBuffer ()
    {
        assert(m_read_cursor > 0);
        assert(m_read_cursor <= m_buffer.size());
        // if we already have at least one atom ahead of the read cursor in
        // the input buffer, there is no need to suck another one out.
        if (m_read_cursor < m_buffer.size())
            return;
        // if the last atom (in front of the read cursor) in the input buffer
        // is '\0' then we have reached EOF, so there is no need to suck
        // another atom out.
        if (m_buffer.size() >= 2 && *m_buffer.rbegin() == '\0')
        {
            assert(m_read_cursor < m_buffer.size());
            return;
        }

        // if we're at the end of input, push a null char
        if ((this->*m_IsInputAtEnd)())
            m_buffer.push_back('\0');
        // otherwise retrieve and push the next input atom
        else
        {
            std::uint8_t atom = (this->*m_ReadNextAtom)();
            assert(atom != '\0' && "may not return '\\0' from return_next_input_char");
            m_buffer.push_back(atom);
        }

        // ensure there is at least one atom on each side of the read cursor.
        assert(m_buffer.size() >= 2);
        assert(m_read_cursor < m_buffer.size());
    }
    void UpdateConditionalFlags ()
    {
        FillBuffer();
        assert(m_read_cursor > 0);
        assert(m_read_cursor < m_buffer.size());
        // given the atoms surrounding the read cursor, calculate the
        // current conditional flags.
        m_current_conditional_flags = 0;
        if (m_buffer[m_read_cursor-1] == '\0')                                            m_current_conditional_flags |= CF_BEGINNING_OF_INPUT;
        if (m_buffer[m_read_cursor] == '\0')                                              m_current_conditional_flags |= CF_END_OF_INPUT;
        if (m_buffer[m_read_cursor-1] == '\0' || m_buffer[m_read_cursor-1] == '\n')       m_current_conditional_flags |= CF_BEGINNING_OF_LINE;
        if (m_buffer[m_read_cursor] == '\0' || m_buffer[m_read_cursor] == '\n')           m_current_conditional_flags |= CF_END_OF_LINE;
        if (IsWordChar(m_buffer[m_read_cursor-1]) != IsWordChar(m_buffer[m_read_cursor])) m_current_conditional_flags |= CF_WORD_BOUNDARY;
    }
    static bool IsWordChar (std::uint8_t c)
    {
        // the return value should be
        // (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'
        static std::uint8_t const s_is_word_char_table[256] =
        {
            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 0, 0, 0, 0, 0, 0, // '0' through '9'
            0, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1, // 'A' through 'O'
            1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 0, 0, 0, 0, 1, // 'P' through 'Z', then '_'
            0, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1, // 'a' through 'o'
            1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 0, 0, 0, 0, 0, // 'p' through 'z'

            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
        };
        return s_is_word_char_table[c] != 0;
    }

    typedef std::deque<std::uint8_t> Buffer;

    std::uint8_t m_current_conditional_flags;
    Buffer m_buffer;
    // indicates the "previous" atom (of the kept string)
    Buffer::size_type m_start_cursor;
    // indicates how far the scanner has read
    Buffer::size_type m_read_cursor;
    // indicates the end of the kept string
    Buffer::size_type m_kept_string_cursor;
    // if != m_start_cursor, indicates the most recent, longest accepted string
    Buffer::size_type m_accept_cursor;
    // indicates if KeepString has been called during the accept/reject handler
    bool m_keep_string_has_been_called;
    // corresponds to the directive return_true_iff_input_is_at_end
    IsInputAtEndMethod_ m_IsInputAtEnd;
    // corresponds to the directive return_next_input_char
    ReadNextAtomMethod_ m_ReadNextAtom;
}; // end of class ReflexCpp_::InputApparatus_Interactive_

// /////////////////////////////////////////////////////////////////////////////
// implements the AutomatonApparatus interface as described in the documentation
// -- it contains all the generalized state machinery for running a reflex DFA.
// /////////////////////////////////////////////////////////////////////////////

class AutomatonApparatus_FastAndBig_Interactive_ : protected InputApparatus_Interactive_
{
protected:

    // state machine mode flags
    enum
    {
        MF_CASE_INSENSITIVE_ = (1 << 0),
        MF_UNGREEDY_         = (1 << 1)
    };

    struct DfaState_
    {
        std::uint32_t m_accept_handler_index;
        std::uint32_t m_transition_count;
        std::uint32_t m_transition_offset;
        std::uint8_t m_transition_type;
        std::uint8_t m_transition_first_index;
    }; // end of struct ReflexCpp_::AutomatonApparatus_FastAndBig_Interactive_::DfaState_
    struct DfaTransition_
    {
        enum Type
        {
            INPUT_ATOM = 0, INPUT_ATOM_RANGE, CONDITIONAL
        }; // end of enum ReflexCpp_::AutomatonApparatus_FastAndBig_Interactive_::DfaTransition_::Type

        std::uint32_t m_target_dfa_state_offset;
    }; // end of struct ReflexCpp_::AutomatonApparatus_FastAndBig_Interactive_::DfaTransition_

    AutomatonApparatus_FastAndBig_Interactive_ (
        DfaState_ const *state_table,
        std::size_t state_count,
        DfaTransition_ const *transition_table,
        std::size_t transition_count,
        std::uint32_t accept_handler_count,
        IsInputAtEndMethod_ IsInputAtEnd,
        ReadNextAtomMethod_ ReadNextAtom)
        :
        InputApparatus_Interactive_(IsInputAtEnd, ReadNextAtom),
        m_accept_handler_count(accept_handler_count),
        m_state_table(state_table),
        m_state_count(state_count),
        m_transition_table(transition_table)
    {
        CheckDfa(state_table, state_count, transition_table, transition_count);
        // subclasses must call ReflexCpp_::InputApparatus_Interactive_::ResetForNewInput_ in their constructors.
    }

    DfaState_ const *InitialState_ () const
    {
        return m_initial_state;
    }
    void InitialState_ (DfaState_ const *initial_state)
    {
        assert(initial_state != NULL);
        m_initial_state = initial_state;
    }
    void ModeFlags_ (std::uint8_t mode_flags)
    {
        m_mode_flags = mode_flags;
    }
    void ResetForNewInput_ (DfaState_ const *initial_state, std::uint8_t mode_flags)
    {
        InputApparatus_Interactive_::ResetForNewInput_();
        if (initial_state != NULL)
            InitialState_(initial_state);
        m_current_state = NULL;
        m_accept_state = NULL;
        m_mode_flags = mode_flags;
    }
    std::uint32_t RunDfa_ (std::string &s)
    {
        assert(s.empty());
        // reset the current state to the initial state.
        assert(m_initial_state != NULL);
        m_current_state = m_initial_state;
        assert(m_accept_state == NULL);
        // loop until there are no valid transitions from the current state.
        while (m_current_state != NULL)
        {
            // if the current state is an accept state, save it
            if (IsAcceptState(m_current_state))
            {
                m_accept_state = m_current_state;
                SetAcceptCursor_();
                // if we're in ungreedy mode, accept the shortest string
                // possible; don't process any more input.
                if ((m_mode_flags & MF_UNGREEDY_) != 0)
                {
                    assert(m_accept_state != NULL);
                    m_current_state = NULL;
                    break;
                }
            }
            // turn the crank on the state machine, exercising the appropriate
            // conditional (using m_current_conditional_flags) or atomic
            // transition (using the atom at the read cursor in the buffer).
            // m_current_state will be set to the transitioned-to state, or
            // NULL if no transition was possible.
            m_current_state = ProcessInputAtom();
        }
        // if we have a most recent accept state, accept the accumulated input
        // using the accept handler indicated by the most recent accept state.
        if (m_accept_state != NULL)
        {
            // extract the accepted string from the buffer
            Accept_(s);
            // save off the accept handler index
            std::uint32_t accept_handler_index = m_accept_state->m_accept_handler_index;
            // clear the accept state for next time
            m_accept_state = NULL;
            // return accept_handler_index to indicate which handler to call
            return accept_handler_index;
        }
        // otherwise the input atom went unhandled; extract the rejected string.
        else
        {
            // put the rejected string in the return string and indicate
            // that no accept handler should be called.
            Reject_(s);
            return m_accept_handler_count;
        }
    }

private:

    // these InputApparatus_Interactive_ methods should not be accessable to Scanner
    using InputApparatus_Interactive_::CurrentConditionalFlags_;
    using InputApparatus_Interactive_::InputAtom_;
    using InputApparatus_Interactive_::AdvanceReadCursor_;
    using InputApparatus_Interactive_::SetAcceptCursor_;
    using InputApparatus_Interactive_::Accept_;
    using InputApparatus_Interactive_::Reject_;

    DfaState_ const *ProcessInputAtom ()
    {
        assert(m_current_state != NULL);
        std::uint32_t target_dfa_state_offset = m_state_count;
        if (m_current_state->m_transition_type == DfaTransition_::CONDITIONAL)
        {
            target_dfa_state_offset = m_transition_table[m_current_state->m_transition_offset + CurrentConditionalFlags_()].m_target_dfa_state_offset;
            // don't advance the read cursor, because no input was actualy eaten
        }
        else // m_current_state->m_transition_type == DfaTransition_::INPUT_ATOM
        {
            std::uint8_t input_atom = InputAtom_();

            // only do the lookup if the input atom is in range of the table
            if (input_atom >= m_current_state->m_transition_first_index &&
                input_atom < m_current_state->m_transition_first_index + m_current_state->m_transition_count)
            {
                target_dfa_state_offset = m_transition_table[m_current_state->m_transition_offset + input_atom - m_current_state->m_transition_first_index].m_target_dfa_state_offset;
            }

            // if we're case-insensitive and the above check didn't match, try
            // the switched case input atom
            if ((m_mode_flags & MF_CASE_INSENSITIVE_) != 0 && target_dfa_state_offset == m_state_count)
            {
                input_atom = SwitchCase(input_atom);
                // only do the lookup if the input atom is in range of the table
                if (input_atom >= m_current_state->m_transition_first_index &&
                    input_atom < m_current_state->m_transition_first_index + m_current_state->m_transition_count)
                {
                    target_dfa_state_offset = m_transition_table[m_current_state->m_transition_offset + input_atom - m_current_state->m_transition_first_index].m_target_dfa_state_offset;
                }
            }

            // only advance the read cursor if the transition was valid
            if (target_dfa_state_offset < m_state_count)
                AdvanceReadCursor_();
        }
        return target_dfa_state_offset < m_state_count ? m_state_table + target_dfa_state_offset : NULL;
    }
    bool IsAcceptState (DfaState_ const *state) const
    {
        assert(state != NULL);
        return state->m_accept_handler_index < m_accept_handler_count;
    }
    static std::uint8_t SwitchCase (std::uint8_t c)
    {
        if (c >= 'a' && c <= 'z')
            return c - 'a' + 'A';
        if (c >= 'A' && c <= 'Z')
            return c - 'A' + 'a';
        return c;
    }
    static void CheckDfa (
        DfaState_ const *state_table,
        std::size_t state_count,
        DfaTransition_ const *transition_table,
        std::size_t transition_count)
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
                assert(s->m_transition_type == DfaTransition_::INPUT_ATOM ||
                       s->m_transition_type == DfaTransition_::CONDITIONAL);
                assert(transition_table + s->m_transition_offset == t &&
                       "states' transitions must be contiguous and in ascending order");
                t += s->m_transition_count;
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
            assert(t->m_target_dfa_state_offset <= state_count &&
                   "transition target state out of range (highest acceptable value is state count)");
        }
    }

    std::uint32_t const m_accept_handler_count;
    DfaState_ const *const m_state_table;
    std::size_t const m_state_count;
    DfaTransition_ const *const m_transition_table;
    DfaState_ const *m_initial_state;
    DfaState_ const *m_current_state;
    DfaState_ const *m_accept_state;
    std::uint8_t m_mode_flags;
}; // end of class ReflexCpp_::AutomatonApparatus_FastAndBig_Interactive_

} // end of namespace ReflexCpp_
#endif // !defined(ReflexCpp_namespace_)


#line 19 "../calculator_scanner.reflex"

#if !defined(CALCULATOR_SCANNER_HPP_)
#define CALCULATOR_SCANNER_HPP_

#include "calculator.hpp"

#include <sstream>
#include <stdlib.h>

#include "calculator_parser.hpp"

namespace Calculator {

#line 540 "../calculator_scanner.hpp"

class Scanner : private ReflexCpp_::AutomatonApparatus_FastAndBig_Interactive_
{
public:

    struct StateMachine
    {
        enum Name
        {
            MAIN,
            // default starting state machine
            START_ = MAIN
        }; // end of enum Scanner::StateMachine::Name
    }; // end of struct Scanner::StateMachine

    Scanner (
#line 32 "../calculator_scanner.reflex"
 string const &input_string 
#line 559 "../calculator_scanner.hpp"
);
    ~Scanner ();

    /// Returns true if and only if "debug spew" is enabled (which prints, to the
    /// debug spew stream, what actions the scanner is taking).  This method, along
    /// with all other debug spew code can be removed by removing the
    /// %target.cpp.generate_debug_spew_code directive from the primary source.
    bool DebugSpewIsEnabled () const { return m_debug_spew_stream_ != NULL; }
    /// Returns the debug spew stream (see DebugSpewIsEnabled()).  This method,
    /// along with all other debug spew code can be removed by removing the
    /// %target.cpp.generate_debug_spew_code directive from the primary source.
    std::ostream *DebugSpewStream () { return m_debug_spew_stream_; }
    /// Sets the debug spew stream (see DebugSpewIsEnabled()).  If NULL is passed
    /// in, then debug spew printing will be disabled.  The default value is NULL.
    /// This method, along with all other debug spew code can be removed by removing
    /// the %target.cpp.generate_debug_spew_code directive from the primary source.
    void SetDebugSpewStream (std::ostream *debug_spew_stream) { m_debug_spew_stream_ = debug_spew_stream; }

    /// Returns the currently active state machine.
    StateMachine::Name CurrentStateMachine () const;
    /// Switches the current state machine to the specified one.
    void SwitchToStateMachine (StateMachine::Name state_machine);

    using AutomatonApparatus_FastAndBig_Interactive_::IsAtEndOfInput;
    void ResetForNewInput ();

    Parser::Token Scan () throw();

public:


#line 34 "../calculator_scanner.reflex"

    istringstream m_input;

#line 595 "../calculator_scanner.hpp"


private:

    void KeepString ();
    void Unaccept (std::uint32_t unaccept_char_count);
    void Unreject (std::uint32_t unreject_char_count);
    // ///////////////////////////////////////////////////////////////////////
    // begin internal reflex-generated parser guts -- don't use
    // ///////////////////////////////////////////////////////////////////////

    using InputApparatus_Interactive_::PrepareToScan_;
    using InputApparatus_Interactive_::ResetForNewInput_;

    using AutomatonApparatus_FastAndBig_Interactive_::InitialState_;
    using AutomatonApparatus_FastAndBig_Interactive_::ResetForNewInput_;
    using AutomatonApparatus_FastAndBig_Interactive_::RunDfa_;

    bool IsInputAtEnd_ () throw();
    std::uint8_t ReadNextAtom_ () throw();

    // debug spew methods
    static void PrintAtom_ (std::ostream &out, std::uint8_t atom);
    static void PrintString_ (std::ostream &out, std::string const &s);

    std::ostream *m_debug_spew_stream_;

    // state machine and automaton data
    static std::uint32_t const ms_state_machine_start_state_index_[];
    static std::uint8_t const ms_state_machine_mode_flags_[];
    static char const *const ms_state_machine_name_[];
    static std::uint32_t const ms_state_machine_count_;
    static AutomatonApparatus_FastAndBig_Interactive_::DfaState_ const ms_state_table_[];
    static std::size_t const ms_state_count_;
    static AutomatonApparatus_FastAndBig_Interactive_::DfaTransition_ const ms_transition_table_[];
    static std::size_t const ms_transition_count_;
    static char const *const ms_accept_handler_regex_[];
    static std::uint32_t const ms_accept_handler_count_;

    // ///////////////////////////////////////////////////////////////////////
    // end of internal reflex-generated parser guts
    // ///////////////////////////////////////////////////////////////////////
}; // end of class Scanner


#line 37 "../calculator_scanner.reflex"

} // end of namespace Calculator

#endif // !defined(CALCULATOR_SCANNER_HPP_)

#line 647 "../calculator_scanner.hpp"
