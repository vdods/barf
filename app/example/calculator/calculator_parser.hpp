#include <ostream>
#include <string>
#include <vector>


#line 13 "calculator_parser.trison"

#if !defined(_CALCULATOR_PARSER_HPP_)
#define _CALCULATOR_PARSER_HPP_

#include "calculator.hpp"

namespace Calculator {

class Scanner;

#line 18 "calculator_parser.hpp"

class Parser

{
public:

    struct Token
    {
        enum Type
        {
            // user-defined terminal, non-single-character tokens
            BAD_TOKEN = 0x100,
            FLOAT,
            HELP,
            MOD,
            NEWLINE,
            NUMBER,
            RESULT,

            // special end-of-input terminal
            END_,

            // user-defined nonterminal tokens
            at_least_zero_newlines__,
            command__,
            expression__,
            root__,

            // special start nonterminal
            START_,

            // parser's sentinel and special tokens
            ERROR_,
            DEFAULT_,
            INVALID_
        }; // end of enum Parser::Token::Type
    }; // end of struct Parser::Token

    Parser ();
    ~Parser ();

    inline cl_N const &GetAcceptedToken () const { return m_reduction_token; }
    inline void ClearAcceptedToken () { m_reduction_token = 0; }

    inline unsigned int GetDebugSpewLevel () const { return m_debug_spew_level; }
    inline void SetDebugSpewLevel (unsigned int debug_spew_level) { m_debug_spew_level = debug_spew_level; }

    static void CheckStateConsistency ();

    enum ParserReturnCode
    {
        PRC_SUCCESS = 0,
        PRC_UNHANDLED_PARSE_ERROR = 1
    }; // end of enum ParserReturnCode

public:

    ParserReturnCode Parse ();

public:


#line 24 "calculator_parser.trison"

public:

    bool ShouldPrintResult () const { return m_should_print_result; }

    void SetInputString (string const &input_string);

private:

    Token::Type Scan ();

    Scanner *m_scanner;
    cl_N m_last_result;
    cl_R m_modulus;
    bool m_should_print_result;

#line 98 "calculator_parser.hpp"

private:

    typedef unsigned int StateNumber;

    enum TransitionAction
    {
        TA_SHIFT_AND_PUSH_STATE = 0,
        TA_PUSH_STATE,
        TA_REDUCE_USING_RULE,
        TA_REDUCE_AND_ACCEPT_USING_RULE,
        TA_THROW_AWAY_LOOKAHEAD_TOKEN,

        TA_COUNT
    };

    enum ActionReturnCode
    {
        ARC_CONTINUE_PARSING = 0,
        ARC_ACCEPT_AND_RETURN
    };

    struct ReductionRule
    {
        typedef cl_N (Parser::*ReductionRuleHandler)();

        Token::Type m_non_terminal_to_reduce_to;
        unsigned int m_number_of_tokens_to_reduce_by;
        ReductionRuleHandler m_handler;
        std::string m_description;
    };

    struct Action
    {
        TransitionAction m_transition_action;
        unsigned int m_data;
    };

    struct StateTransition
    {
        Token::Type m_token_type;
        Action m_action;
    };

    struct State
    {
        unsigned int m_lookahead_transition_offset;
        unsigned int m_lookahead_transition_count;
        unsigned int m_default_action_offset;
        unsigned int m_non_terminal_transition_offset;
        unsigned int m_non_terminal_transition_count;
    };

    inline void GetNewLookaheadToken ()
    {
        if (m_is_new_lookahead_token_required)
        {
            m_is_new_lookahead_token_required = false;
            if (m_get_new_lookahead_token_type_from_saved)
            {
                m_get_new_lookahead_token_type_from_saved = false;
                m_lookahead_token_type = m_saved_lookahead_token_type;
            }
            else
                ScanANewLookaheadToken();
        }
    }
    inline Token::Type GetLookaheadTokenType ()
    {
        GetNewLookaheadToken();
        return m_lookahead_token_type;
    }
    inline cl_N const &GetLookaheadToken ()
    {
        GetNewLookaheadToken();
        return m_lookahead_token;
    }
    bool GetDoesStateAcceptErrorToken (StateNumber state_number) const;

    ParserReturnCode PrivateParse ();

    ActionReturnCode ProcessAction (Action const &action);
    void ShiftLookaheadToken ();
    void PushState (StateNumber state_number);
    void ReduceUsingRule (ReductionRule const &reduction_rule, bool and_accept);
    void PopStates (unsigned int number_of_states_to_pop, bool print_state_stack = true);
    void PrintStateStack () const;
    void PrintTokenStack () const;
    void PrintStateTransition (unsigned int state_transition_number) const;
    void ScanANewLookaheadToken ();
    void ThrowAwayToken (cl_N token);
    void ThrowAwayTokenStack ();

    typedef std::vector<StateNumber> StateStack;
    typedef std::vector< cl_N > TokenStack;

    unsigned int m_debug_spew_level;

    StateStack m_state_stack;
    TokenStack m_token_stack;

    Token::Type m_lookahead_token_type;
    cl_N m_lookahead_token;
    bool m_is_new_lookahead_token_required;

    Token::Type m_saved_lookahead_token_type;
    bool m_get_new_lookahead_token_type_from_saved;
    bool m_previous_transition_accepted_error_token;

    bool m_is_returning_with_non_terminal;
    Token::Type m_returning_with_this_non_terminal;

    cl_N m_reduction_token;
    unsigned int m_reduction_rule_token_count;

    static State const ms_state[];
    static unsigned int const ms_state_count;
    static ReductionRule const ms_reduction_rule[];
    static unsigned int const ms_reduction_rule_count;
    static StateTransition const ms_state_transition[];
    static unsigned int const ms_state_transition_count;

    cl_N ReductionRuleHandler0000 ();
    cl_N ReductionRuleHandler0001 ();
    cl_N ReductionRuleHandler0002 ();
    cl_N ReductionRuleHandler0003 ();
    cl_N ReductionRuleHandler0004 ();
    cl_N ReductionRuleHandler0005 ();
    cl_N ReductionRuleHandler0006 ();
    cl_N ReductionRuleHandler0007 ();
    cl_N ReductionRuleHandler0008 ();
    cl_N ReductionRuleHandler0009 ();
    cl_N ReductionRuleHandler0010 ();
    cl_N ReductionRuleHandler0011 ();
    cl_N ReductionRuleHandler0012 ();
    cl_N ReductionRuleHandler0013 ();
    cl_N ReductionRuleHandler0014 ();
    cl_N ReductionRuleHandler0015 ();
    cl_N ReductionRuleHandler0016 ();
    cl_N ReductionRuleHandler0017 ();
    cl_N ReductionRuleHandler0018 ();
    cl_N ReductionRuleHandler0019 ();

}; // end of class Parser

std::ostream &operator << (std::ostream &stream, Parser::Token::Type token_type);


#line 41 "calculator_parser.trison"

} // end of namespace Calculator

#endif // !defined(_CALCULATOR_PARSER_HPP_)

#line 253 "calculator_parser.hpp"
