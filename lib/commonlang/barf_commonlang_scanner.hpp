// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY
// barf_commonlang_scanner.hpp generated by reflex
// from barf_commonlang_scanner.reflex using reflex.cpp.targetspec and reflex.cpp.header.codespec
// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY

#include <cassert>
#include <deque>
#include <string>

#if !defined(BarfCpp_namespace_)
#define BarfCpp_namespace_
namespace BarfCpp_ {

// /////////////////////////////////////////////////////////////////////////////
// a bunch of template metaprogramming to intelligently determine what type to
// use for an integer of the given bit width or value range.
// /////////////////////////////////////////////////////////////////////////////

template <bool condition, typename Then, typename Else> struct If;
template <typename Then, typename Else> struct If<true,Then,Else> { typedef Then T; };
template <typename Then, typename Else> struct If<false,Then,Else> { typedef Else T; };

template <bool condition> struct Assert;
template <> struct Assert<true> { static bool const v = true; operator bool () { return v; } };

template <typename Sint, typename Uint> struct IntPair { typedef Sint S; typedef Uint U; };

template <int bits> struct Integer
{
private:

    typedef
        typename If<bits == 8*sizeof(char),      IntPair<char,unsigned char>,
        typename If<bits == 8*sizeof(short),     IntPair<short,unsigned short>,
        typename If<bits == 8*sizeof(int),       IntPair<int,unsigned int>,
        typename If<bits == 8*sizeof(long),      IntPair<long,unsigned long>,
        typename If<bits == 8*sizeof(long long), IntPair<long long,unsigned long long>,
        Integer<0> // if no match, cause a compile error
        >::T >::T >::T >::T >::T PrivateIntPair;
    static bool const assert_size =
        Assert<bits == 8*sizeof(typename PrivateIntPair::S) &&
               bits == 8*sizeof(typename PrivateIntPair::U)>::v;

public:

    typedef typename PrivateIntPair::S Signed;
    typedef typename PrivateIntPair::U Unsigned;
};
template <> struct Integer<0> { }; // empty for intentional compile errors

// /////////////////////////////////////////////////////////////////////////////
// use the above to define specific integer types.  you could trivially add
// 64-bit integers here, but for the purposes of this file, there is no reason
// to use them (though on a 64-bit machine, Diff and Size WILL be 64-bit).
// /////////////////////////////////////////////////////////////////////////////

typedef Integer<8> ::Signed                Sint8;
typedef Integer<8> ::Unsigned              Uint8;
typedef Integer<16>::Signed                Sint16;
typedef Integer<16>::Unsigned              Uint16;
typedef Integer<32>::Signed                Sint32;
typedef Integer<32>::Unsigned              Uint32;
typedef Integer<8*sizeof(void*)>::Signed   Diff; // difference between pointers
typedef Integer<8*sizeof(void*)>::Unsigned Size; // size of blocks of memory

// /////////////////////////////////////////////////////////////////////////////
// here are a few compile-time assertions to check that the integers actually
// turned out to be the right sizes.
// /////////////////////////////////////////////////////////////////////////////

enum
{
    TYPE_SIZE_ASSERTIONS =
        Assert<sizeof(Sint8)  == 1>::v &&
        Assert<sizeof(Uint8)  == 1>::v &&
        Assert<sizeof(Sint16) == 2>::v &&
        Assert<sizeof(Uint16) == 2>::v &&
        Assert<sizeof(Sint32) == 4>::v &&
        Assert<sizeof(Uint32) == 4>::v &&
        Assert<sizeof(Diff)   == sizeof(void*)>::v &&
        Assert<sizeof(Size)   == sizeof(void*)>::v
};

} // end of namespace BarfCpp_
#endif // !defined(BarfCpp_namespace_)

#if !defined(ReflexCpp_namespace_)
#define ReflexCpp_namespace_
namespace ReflexCpp_ {

// /////////////////////////////////////////////////////////////////////////////
// implements the InputApparatus interface as described in the documentation
// /////////////////////////////////////////////////////////////////////////////

class InputApparatus
{
public:

    typedef bool (InputApparatus::*IsInputAtEndMethod)();
    typedef BarfCpp_::Uint8 (InputApparatus::*ReadNextAtomMethod)();

    InputApparatus (IsInputAtEndMethod IsInputAtEnd, ReadNextAtomMethod ReadNextAtom)
        :
        m_IsInputAtEnd(IsInputAtEnd),
        m_ReadNextAtom(ReadNextAtom)
    {
        // subclasses must call InputApparatus::ResetForNewInput_ in their constructors.
    }

    bool IsAtEndOfInput () { return IsConditionalMet(CF_END_OF_INPUT, CF_END_OF_INPUT); }

protected:

    BarfCpp_::Uint8 GetCurrentConditionalFlags ()
    {
        UpdateConditionalFlags();
        return m_current_conditional_flags;
    }
    BarfCpp_::Uint8 GetInputAtom ()
    {
        FillBuffer();
        assert(m_read_cursor > 0);
        assert(m_read_cursor < m_buffer.size());
        return m_buffer[m_read_cursor];
    }
    void AdvanceReadCursor ()
    {
        assert(m_read_cursor > 0);
        assert(m_read_cursor < m_buffer.size());
        if (m_buffer[m_read_cursor] != '\0')
            ++m_read_cursor;
    }
    void SetAcceptCursor ()
    {
        assert(m_read_cursor > 0);
        assert(m_read_cursor <= m_buffer.size());
        m_accept_cursor = m_read_cursor;
    }
    void Accept (std::string &s)
    {
        assert(s.empty());
        assert(m_accept_cursor > 0 && "can't Accept if the accept cursor was not set");
        assert(m_accept_cursor <= m_buffer.size());
        assert(m_read_cursor > 0);
        assert(m_read_cursor <= m_buffer.size());
        assert(m_accept_cursor <= m_read_cursor);
        // calculate the iterators for the buffer string to erase.
        Buffer::iterator delete_it = m_buffer.begin();
        Buffer::iterator delete_it_end = delete_it;
        for (Buffer::size_type i = 0; i < m_accept_cursor-1; ++i)
        {
            assert(delete_it_end != m_buffer.end());
            ++delete_it_end;
        }
        // the accepted string iterators are one atom right from the
        // buffer's to-be-erased string.
        Buffer::iterator accept_it = delete_it;
        ++accept_it;
        Buffer::iterator accept_it_end = delete_it_end;
        assert(accept_it_end != m_buffer.end());
        ++accept_it_end;
        // extract the accepted string and erase all but the new previous atom
        s.insert(s.begin(), accept_it, accept_it_end);
        m_buffer.erase(delete_it, delete_it_end);
        assert(m_buffer.size() >= 1);
        // reset the cursors
        m_read_cursor = 1;
        m_accept_cursor = 0;
    }
    void Reject (std::string &s)
    {
        assert(s.empty());
        assert(m_accept_cursor == 0 && "can't Reject if the accept cursor was set");
        assert(m_read_cursor > 0);
        assert(m_read_cursor < m_buffer.size());
        assert(m_buffer.size() >= 2);
        // only pop the front of the buffer if we're not at EOF.
        BarfCpp_::Uint8 rejected_atom = m_buffer[1];
        if (m_buffer[1] != '\0')
            m_buffer.pop_front();
        // reset the read cursor and return the rejected atom
        m_read_cursor = 1;
        s += rejected_atom;
    }
    void ResetForNewInput_ ()
    {
        m_buffer.clear();
        m_buffer.push_front('\0'); // special "previous" atom for beginning of input
        assert(m_buffer.size() == 1);
        m_read_cursor = 1;
        m_accept_cursor = 0;
        m_current_conditional_flags = 0;
    }

private:

    enum ConditionalFlag
    {
        CF_BEGINNING_OF_INPUT = (1 << 0),
        CF_END_OF_INPUT       = (1 << 1),
        CF_BEGINNING_OF_LINE  = (1 << 2),
        CF_END_OF_LINE        = (1 << 3),
        CF_WORD_BOUNDARY      = (1 << 4)
    }; // end of enum ReflexCpp_::InputApparatus::ConditionalFlag

    bool IsConditionalMet (BarfCpp_::Uint8 conditional_mask, BarfCpp_::Uint8 conditional_flags)
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
            BarfCpp_::Uint8 atom = (this->*m_ReadNextAtom)();
            assert(atom != '\0' && "may not return '\\0' from return_next_input_char");
            m_buffer.push_back(atom);
        }
        // ensure there is at least one atom on each side of the read cursor.
        assert(m_buffer.size() >= 2);
        assert(m_read_cursor == m_buffer.size()-1);
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
    static bool IsWordChar (BarfCpp_::Uint8 c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; }

    typedef std::deque<BarfCpp_::Uint8> Buffer;

    BarfCpp_::Uint8 m_current_conditional_flags;
    Buffer m_buffer;
    Buffer::size_type m_read_cursor;
    Buffer::size_type m_accept_cursor;
    IsInputAtEndMethod m_IsInputAtEnd;
    ReadNextAtomMethod m_ReadNextAtom;
}; // end of class ReflexCpp_::InputApparatus

// /////////////////////////////////////////////////////////////////////////////
// implements the AutomatonApparatus interface as described in the documentation
// -- it contains all the generalized state machinery for running a reflex DFA.
// /////////////////////////////////////////////////////////////////////////////

class AutomatonApparatus : protected InputApparatus
{
public:

    struct DfaTransition_;
    struct DfaState_
    {
        BarfCpp_::Uint32 m_accept_handler_index;
        BarfCpp_::Size m_transition_count;
        DfaTransition_ const *m_transition;
    }; // end of struct ReflexCpp_::AutomatonApparatus::DfaState_
    struct DfaTransition_
    {
        enum Type
        {
            INPUT_ATOM = 0, INPUT_ATOM_RANGE, CONDITIONAL
        }; // end of enum ReflexCpp_::AutomatonApparatus::DfaTransition_::Type

        BarfCpp_::Uint8 m_transition_type;
        BarfCpp_::Uint8 m_data_0;
        BarfCpp_::Uint8 m_data_1;
        DfaState_ const *m_target_dfa_state;

        bool AcceptsInputAtom (BarfCpp_::Uint8 input_atom) const
        {
            assert(m_transition_type == INPUT_ATOM || m_transition_type == INPUT_ATOM_RANGE);
            // returns true iff this transition is INPUT_ATOM and input_atom
            // matches m_data_0, or this transition is INPUT_ATOM_RANGE and
            // input_atom is within the range [m_data_0, m_data_1] inclusive.
            return (m_transition_type == INPUT_ATOM &&
                    m_data_0 == input_atom)
                   ||
                   (m_transition_type == INPUT_ATOM_RANGE &&
                    m_data_0 <= input_atom && input_atom <= m_data_1);
        }
        bool AcceptsConditionalFlags (BarfCpp_::Uint8 conditional_flags) const
        {
            assert(m_transition_type == CONDITIONAL);
            // returns true iff this transition is CONDITIONAL and no relevant bits
            // in conditional_flags conflict with this transition's conditional mask
            // (m_data_0) and flags (m_data_1).
            return ((conditional_flags ^ m_data_1) & m_data_0) == 0;
        }
    }; // end of struct ReflexCpp_::AutomatonApparatus::DfaTransition_

    AutomatonApparatus (
        DfaState_ const *state_table,
        BarfCpp_::Size state_count,
        DfaTransition_ const *transition_table,
        BarfCpp_::Size transition_count,
        BarfCpp_::Uint32 accept_handler_count,
        IsInputAtEndMethod IsInputAtEnd,
        ReadNextAtomMethod ReadNextAtom)
        :
        InputApparatus(IsInputAtEnd, ReadNextAtom),
        m_accept_handler_count(accept_handler_count)
    {
        CheckDfa(state_table, state_count, transition_table, transition_count);
        // subclasses must call ReflexCpp_::InputApparatus::ResetForNewInput_ in their constructors.
    }

protected:

    DfaState_ const *InitialState_ () const
    {
        return m_initial_state;
    }
    void InitialState_ (DfaState_ const *initial_state)
    {
        assert(initial_state != NULL);
        m_initial_state = initial_state;
    }
    void ResetForNewInput_ (DfaState_ const *initial_state)
    {
        InputApparatus::ResetForNewInput_();
        if (initial_state != NULL)
            InitialState_(initial_state);
        m_current_state = NULL;
        m_accept_state = NULL;
    }
    BarfCpp_::Uint32 RunDfa_ (std::string &s)
    {
        // clear the destination string
        s.clear();
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
                SetAcceptCursor();
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
            Accept(s);
            // save off the accept handler index
            BarfCpp_::Uint32 accept_handler_index = m_accept_state->m_accept_handler_index;
            // clear the accept state for next time
            m_accept_state = NULL;
            // return accept_handler_index to indicate which handler to call
            return accept_handler_index;
        }
        // otherwise the input atom went unhandled, so call the special
        // unmatched character handler on the rejected atom.
        else
        {
            // put the rejected atom in the return string and indicate
            // that no accept handler should be called.
            Reject(s);
            return m_accept_handler_count;
        }
    }

private:

    DfaState_ const *ProcessInputAtom ()
    {
        assert(m_current_state != NULL);
        // get the current conditional flags and input atom once before looping
        BarfCpp_::Uint8 current_conditional_flags = GetCurrentConditionalFlags();
        BarfCpp_::Uint8 input_atom = GetInputAtom();
        // iterate through the current state's transitions, exercising the first
        // acceptable one and returning the target state
        for (DfaTransition_ const *transition = m_current_state->m_transition,
                                  *transition_end = transition + m_current_state->m_transition_count;
             transition != transition_end;
             ++transition)
        {
            assert(transition->m_transition_type == DfaTransition_::INPUT_ATOM ||
                   transition->m_transition_type == DfaTransition_::INPUT_ATOM_RANGE ||
                   transition->m_transition_type == DfaTransition_::CONDITIONAL);
            // if it's an atomic transition, check if it accepts input_atom.
            if (transition->m_transition_type == DfaTransition_::INPUT_ATOM ||
                transition->m_transition_type == DfaTransition_::INPUT_ATOM_RANGE)
            {
                if (transition->AcceptsInputAtom(input_atom))
                {
                    AdvanceReadCursor();
                    return transition->m_target_dfa_state;
                }
            }
            // otherwise it must be a conditional transition, so check the flags.
            else if (transition->AcceptsConditionalFlags(current_conditional_flags))
                return transition->m_target_dfa_state;
        }
        // if we reached here, no transition was possible, so return NULL.
        return NULL;
    }
    bool IsAcceptState (DfaState_ const *state) const
    {
        assert(state != NULL);
        return state->m_accept_handler_index < m_accept_handler_count;
    }
    void CheckDfa (
        DfaState_ const *state_table,
        BarfCpp_::Size state_count,
        DfaTransition_ const *transition_table,
        BarfCpp_::Size transition_count)
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
                assert(s->m_transition == t &&
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
            assert((t->m_transition_type == DfaTransition_::INPUT_ATOM ||
                    t->m_transition_type == DfaTransition_::INPUT_ATOM_RANGE ||
                    t->m_transition_type == DfaTransition_::CONDITIONAL)
                   &&
                   "invalid DfaTransition_::Type");
            assert(t->m_target_dfa_state >= state_table &&
                   t->m_target_dfa_state < state_table + state_count &&
                   "transition target state out of range "
                   "(does not point to a valid state)");
            if (t->m_transition_type == DfaTransition_::INPUT_ATOM_RANGE)
            {
                assert(t->m_data_0 < t->m_data_1 &&
                       "can't specify a single-element range of atoms");
            }
            else if (t->m_transition_type == DfaTransition_::CONDITIONAL)
            {
                assert(t->m_data_0 != 0 &&
                       "can't have a conditional with a mask of zero");
                assert((t->m_data_1 & ~t->m_data_0) == 0 &&
                       "there are bits set in the conditional flags "
                       "which are outside of the conditional mask");
            }
        }
    }

    BarfCpp_::Uint32 const m_accept_handler_count;
    DfaState_ const *m_initial_state;
    DfaState_ const *m_current_state;
    DfaState_ const *m_accept_state;
}; // end of class ReflexCpp_::AutomatonApparatus

} // end of namespace ReflexCpp_
#endif // !defined(ReflexCpp_namespace_)


#line 19 "barf_commonlang_scanner.reflex"

#if !defined(BARF_COMMONLANG_SCANNER_HPP_)
#define BARF_COMMONLANG_SCANNER_HPP_

#include "barf_commonlang.hpp"

#include <ostream>

#include "barf_inputbase.hpp"

namespace Barf {
namespace Ast {

class Base;

} // end of namespace Ast

namespace CommonLang {

#line 528 "barf_commonlang_scanner.hpp"

class Scanner : private ReflexCpp_::AutomatonApparatus, 
#line 39 "barf_commonlang_scanner.reflex"
 protected InputBase 
#line 533 "barf_commonlang_scanner.hpp"

{
public:

    using AutomatonApparatus::IsAtEndOfInput;

    struct Mode
    {
        enum Name
        {
            BLOCK_COMMENT = 0,
            CHAR_LITERAL_END = 6,
            CHAR_LITERAL_GUTS = 13,
            CHAR_LITERAL_INSIDE_STRICT_CODE_BLOCK = 26,
            DUMB_CODE_BLOCK = 34,
            MAIN = 40,
            REGULAR_EXPRESSION = 62,
            REGULAR_EXPRESSION_BRACKET_EXPRESSION = 73,
            STRICT_CODE_BLOCK = 83,
            STRING_LITERAL_GUTS = 94,
            STRING_LITERAL_INSIDE_STRICT_CODE_BLOCK = 110,
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
            DIRECTIVE_END,
            DIRECTIVE_ERROR,
            DIRECTIVE_ID,
            DIRECTIVE_MACRO,
            DIRECTIVE_NONTERMINAL,
            DIRECTIVE_PREC,
            DIRECTIVE_START_IN_SCANNER_MODE,
            DIRECTIVE_STATE,
            DIRECTIVE_STRICT_CODE_BLOCK,
            DIRECTIVE_STRING,
            DIRECTIVE_TARGET,
            DIRECTIVE_TARGETS,
            DIRECTIVE_TERMINAL,
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

#line 603 "barf_commonlang_scanner.hpp"

public:

    Scanner ();
    ~Scanner ();

    bool DebugSpew () const { return m_debug_spew_; }
    void DebugSpew (bool debug_spew) { m_debug_spew_ = debug_spew; }

    Mode::Name ScannerMode () const;
    void ScannerMode (Mode::Name mode);

    void ResetForNewInput ();

    Scanner::Token::Type Scan (
#line 81 "barf_commonlang_scanner.reflex"
 Ast::Base **token 
#line 621 "barf_commonlang_scanner.hpp"
) throw();

public:


#line 82 "barf_commonlang_scanner.reflex"

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

#line 649 "barf_commonlang_scanner.hpp"


private:

    // ///////////////////////////////////////////////////////////////////////
    // begin internal reflex-generated parser guts -- don't use
    // ///////////////////////////////////////////////////////////////////////

    bool IsInputAtEnd_ () throw();
    BarfCpp_::Uint8 ReadNextAtom_ () throw();

    // debug spew methods
    static void PrintAtom_ (BarfCpp_::Uint8 atom);
    static void PrintString_ (std::string const &s);
    static void PrintScannerMode_ (Mode::Name mode);

    bool m_debug_spew_;

    static AutomatonApparatus::DfaState_ const ms_state_table_[];
    static BarfCpp_::Size const ms_state_count_;
    static AutomatonApparatus::DfaTransition_ const ms_transition_table_[];
    static BarfCpp_::Size const ms_transition_count_;
    static char const *const ms_accept_handler_regex_[];
    static BarfCpp_::Uint32 const ms_accept_handler_count_;

    // ///////////////////////////////////////////////////////////////////////
    // end of internal reflex-generated parser guts
    // ///////////////////////////////////////////////////////////////////////
}; // end of class Scanner


#line 103 "barf_commonlang_scanner.reflex"

ostream &operator << (ostream &stream, Scanner::Token::Type scanner_token_type);

} // end of namespace CommonLang
} // end of namespace Barf

#endif // !defined(BARF_COMMONLANG_SCANNER_HPP_)

#line 690 "barf_commonlang_scanner.hpp"
