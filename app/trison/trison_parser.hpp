#include <ostream>
#include <string>
#include <vector>


#line 13 "trison_parser.trison"

#if !defined(_TRISON_PARSER_HPP_)
#define _TRISON_PARSER_HPP_

#include "trison.hpp"

#include "trison_message.hpp"

namespace Barf {
namespace Ast {

class Base;

} // end of namespace Ast
} // end of namespace Barf

namespace Trison {

class Scanner;

#line 28 "trison_parser.hpp"

class Parser

{
public:

    struct Token
    {
        enum Type
        {
            // user-defined terminal, non-single-character tokens
            BAD_TOKEN = 0x100,
            DIRECTIVE_ERROR,
            DIRECTIVE_PARSER_WITHOUT_PARAMETER,
            DIRECTIVE_PARSER_WITH_PARAMETER,
            DIRECTIVE_TOKEN,
            DUMB_CODE_BLOCK,
            END_PREAMBLE,
            ID,
            LEFT,
            NEWLINE,
            NONASSOC,
            PREC,
            RIGHT,
            START,
            STRICT_CODE_BLOCK,
            STRING,
            TOKEN_ID_CHAR,
            TYPE,

            // special end-of-input terminal
            END_,

            // user-defined nonterminal tokens
            at_least_one_newline__,
            at_least_zero_newlines__,
            directive__,
            directive_list__,
            nonterminal__,
            nonterminal_list__,
            nonterminal_specification__,
            root__,
            rule__,
            rule_list__,
            rule_precedence_directive__,
            rule_specification__,
            rule_token__,
            rule_token_list__,
            token_id__,
            token_id_list__,

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

    inline unsigned int GetDebugSpewLevel () const { return m_debug_spew_level; }
    inline void SetDebugSpewLevel (unsigned int debug_spew_level) { m_debug_spew_level = debug_spew_level; }

    static void CheckStateConsistency ();

    enum ParserReturnCode
    {
        PRC_SUCCESS = 0,
        PRC_UNHANDLED_PARSE_ERROR = 1
    }; // end of enum ParserReturnCode

public:

    ParserReturnCode Parse (Ast::Base * *return_token);

public:


#line 34 "trison_parser.trison"

    bool SetInputFilename (string const &input_filename);

private:

    FiLoc GetFiLoc () const;

    Token::Type Scan ();

    Scanner *m_scanner;

#line 123 "trison_parser.hpp"

private:

    typedef unsigned int StateNumber;

    enum TransitionAction
    {
        TA_SHIFT_AND_PUSH_STATE = 0,
        TA_PUSH_STATE,
        TA_REDUCE_USING_RULE,
        TA_REDUCE_AND_ACCEPT_USING_RULE,

        TA_COUNT
    };

    enum ActionReturnCode
    {
        ARC_CONTINUE_PARSING = 0,
        ARC_ACCEPT_AND_RETURN
    };

    struct ReductionRule
    {
        typedef Ast::Base * (Parser::*ReductionRuleHandler)();

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
            ScanANewLookaheadToken();
        }
    }
    inline Token::Type GetLookaheadTokenType ()
    {
        GetNewLookaheadToken();
        return m_lookahead_token_type;
    }
    inline Ast::Base * const &GetLookaheadToken ()
    {
        GetNewLookaheadToken();
        return m_lookahead_token;
    }
    bool GetDoesStateAcceptErrorToken (StateNumber state_number) const;

    ParserReturnCode PrivateParse (Ast::Base * *return_token);

    ActionReturnCode ProcessAction (Action const &action);
    void ShiftLookaheadToken ();
    void PushState (StateNumber state_number);
    void ReduceUsingRule (ReductionRule const &reduction_rule, bool and_accept);
    void PopStates (unsigned int number_of_states_to_pop, bool print_state_stack = true);
    void PrintStateStack () const;
    void PrintTokenStack () const;
    void PrintStateTransition (unsigned int state_transition_number) const;
    void ScanANewLookaheadToken ();
    void ThrowAwayToken (Ast::Base * token);
    void ThrowAwayTokenStack ();

    typedef std::vector<StateNumber> StateStack;
    typedef std::vector< Ast::Base * > TokenStack;

    unsigned int m_debug_spew_level;

    StateStack m_state_stack;
    TokenStack m_token_stack;

    Token::Type m_lookahead_token_type;
    Ast::Base * m_lookahead_token;
    bool m_is_new_lookahead_token_required;
    bool m_in_error_handling_mode;

    bool m_is_returning_with_non_terminal;
    Token::Type m_returning_with_this_non_terminal;

    Ast::Base * m_reduction_token;
    unsigned int m_reduction_rule_token_count;

    static State const ms_state[];
    static unsigned int const ms_state_count;
    static ReductionRule const ms_reduction_rule[];
    static unsigned int const ms_reduction_rule_count;
    static StateTransition const ms_state_transition[];
    static unsigned int const ms_state_transition_count;

    Ast::Base * ReductionRuleHandler0000 ();
    Ast::Base * ReductionRuleHandler0001 ();
    Ast::Base * ReductionRuleHandler0002 ();
    Ast::Base * ReductionRuleHandler0003 ();
    Ast::Base * ReductionRuleHandler0004 ();
    Ast::Base * ReductionRuleHandler0005 ();
    Ast::Base * ReductionRuleHandler0006 ();
    Ast::Base * ReductionRuleHandler0007 ();
    Ast::Base * ReductionRuleHandler0008 ();
    Ast::Base * ReductionRuleHandler0009 ();
    Ast::Base * ReductionRuleHandler0010 ();
    Ast::Base * ReductionRuleHandler0011 ();
    Ast::Base * ReductionRuleHandler0012 ();
    Ast::Base * ReductionRuleHandler0013 ();
    Ast::Base * ReductionRuleHandler0014 ();
    Ast::Base * ReductionRuleHandler0015 ();
    Ast::Base * ReductionRuleHandler0016 ();
    Ast::Base * ReductionRuleHandler0017 ();
    Ast::Base * ReductionRuleHandler0018 ();
    Ast::Base * ReductionRuleHandler0019 ();
    Ast::Base * ReductionRuleHandler0020 ();
    Ast::Base * ReductionRuleHandler0021 ();
    Ast::Base * ReductionRuleHandler0022 ();
    Ast::Base * ReductionRuleHandler0023 ();
    Ast::Base * ReductionRuleHandler0024 ();
    Ast::Base * ReductionRuleHandler0025 ();
    Ast::Base * ReductionRuleHandler0026 ();
    Ast::Base * ReductionRuleHandler0027 ();
    Ast::Base * ReductionRuleHandler0028 ();
    Ast::Base * ReductionRuleHandler0029 ();
    Ast::Base * ReductionRuleHandler0030 ();
    Ast::Base * ReductionRuleHandler0031 ();
    Ast::Base * ReductionRuleHandler0032 ();
    Ast::Base * ReductionRuleHandler0033 ();
    Ast::Base * ReductionRuleHandler0034 ();
    Ast::Base * ReductionRuleHandler0035 ();
    Ast::Base * ReductionRuleHandler0036 ();
    Ast::Base * ReductionRuleHandler0037 ();
    Ast::Base * ReductionRuleHandler0038 ();
    Ast::Base * ReductionRuleHandler0039 ();
    Ast::Base * ReductionRuleHandler0040 ();
    Ast::Base * ReductionRuleHandler0041 ();
    Ast::Base * ReductionRuleHandler0042 ();
    Ast::Base * ReductionRuleHandler0043 ();
    Ast::Base * ReductionRuleHandler0044 ();
    Ast::Base * ReductionRuleHandler0045 ();
    Ast::Base * ReductionRuleHandler0046 ();
    Ast::Base * ReductionRuleHandler0047 ();
    Ast::Base * ReductionRuleHandler0048 ();
    Ast::Base * ReductionRuleHandler0049 ();
    Ast::Base * ReductionRuleHandler0050 ();
    Ast::Base * ReductionRuleHandler0051 ();
    Ast::Base * ReductionRuleHandler0052 ();
    Ast::Base * ReductionRuleHandler0053 ();

}; // end of class Parser

std::ostream &operator << (std::ostream &stream, Parser::Token::Type token_type);


#line 46 "trison_parser.trison"

} // end of namespace Trison

#endif // !defined(_TRISON_PARSER_HPP_)

#line 302 "trison_parser.hpp"
