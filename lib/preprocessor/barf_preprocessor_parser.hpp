#include <ostream>
#include <string>
#include <vector>


#line 13 "barf_preprocessor_parser.trison"

#if !defined(_BARF_PREPROCESSOR_PARSER_HPP_)
#define _BARF_PREPROCESSOR_PARSER_HPP_

#include "barf_preprocessor.hpp"

namespace Barf {
namespace AstCommon {

class Ast;

} // end of namespace AstCommon

namespace Preprocessor {

class Scanner;

#line 25 "barf_preprocessor_parser.hpp"

class Parser

{
public:

    struct Token
    {
        enum Type
        {
            // user-defined terminal, non-single-character tokens
            BAD_TOKEN = 0x100,
            CODE_LINE,
            CODE_NEWLINE,
            DECLARE_ARRAY,
            DECLARE_MAP,
            DEFINE,
            DUMP_SYMBOL_TABLE,
            ELSE,
            ELSE_IF,
            END_CODE,
            END_DEFINE,
            END_FOR_EACH,
            END_IF,
            END_LOOP,
            ERROR,
            FATAL_ERROR,
            FOR_EACH,
            ID,
            IF,
            INCLUDE,
            INTEGER,
            IS_DEFINED,
            KEYWORD_INT,
            KEYWORD_STRING,
            LOOP,
            SANDBOX_INCLUDE,
            SIZEOF,
            START_CODE,
            STRING,
            STRING_LENGTH,
            TEXT,
            UNDEFINE,
            WARNING,

            // special end-of-input terminal
            END_,

            // user-defined nonterminal tokens
            body__,
            code__,
            conditional_series__,
            conditional_series_end__,
            define__,
            define_array_element__,
            define_map_element__,
            define_scalar__,
            else_if_statement__,
            else_statement__,
            end_define__,
            end_for_each__,
            end_if__,
            end_loop__,
            executable__,
            expression__,
            for_each__,
            if_statement__,
            loop__,

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

    inline AstCommon::Ast * const &GetAcceptedToken () const { return m_reduction_token; }
    inline void ClearAcceptedToken () { m_reduction_token = NULL; }

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


#line 31 "barf_preprocessor_parser.trison"

    bool OpenFile (string const &input_filename);
    void OpenString (string const &input_string, string const &input_name, bool use_line_numbers = false);
    void OpenUsingStream (istream *input_stream, string const &input_name, bool use_line_numbers);

private:

    Token::Type Scan ();

    Scanner *m_scanner;

#line 141 "barf_preprocessor_parser.hpp"

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
        typedef AstCommon::Ast * (Parser::*ReductionRuleHandler)();

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
    inline AstCommon::Ast * const &GetLookaheadToken ()
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
    void ThrowAwayToken (AstCommon::Ast * token);
    void ThrowAwayTokenStack ();

    typedef std::vector<StateNumber> StateStack;
    typedef std::vector< AstCommon::Ast * > TokenStack;

    unsigned int m_debug_spew_level;

    StateStack m_state_stack;
    TokenStack m_token_stack;

    Token::Type m_lookahead_token_type;
    AstCommon::Ast * m_lookahead_token;
    bool m_is_new_lookahead_token_required;

    Token::Type m_saved_lookahead_token_type;
    bool m_get_new_lookahead_token_type_from_saved;
    bool m_previous_transition_accepted_error_token;

    bool m_is_returning_with_non_terminal;
    Token::Type m_returning_with_this_non_terminal;

    AstCommon::Ast * m_reduction_token;
    unsigned int m_reduction_rule_token_count;

    static State const ms_state[];
    static unsigned int const ms_state_count;
    static ReductionRule const ms_reduction_rule[];
    static unsigned int const ms_reduction_rule_count;
    static StateTransition const ms_state_transition[];
    static unsigned int const ms_state_transition_count;

    AstCommon::Ast * ReductionRuleHandler0000 ();
    AstCommon::Ast * ReductionRuleHandler0001 ();
    AstCommon::Ast * ReductionRuleHandler0002 ();
    AstCommon::Ast * ReductionRuleHandler0003 ();
    AstCommon::Ast * ReductionRuleHandler0004 ();
    AstCommon::Ast * ReductionRuleHandler0005 ();
    AstCommon::Ast * ReductionRuleHandler0006 ();
    AstCommon::Ast * ReductionRuleHandler0007 ();
    AstCommon::Ast * ReductionRuleHandler0008 ();
    AstCommon::Ast * ReductionRuleHandler0009 ();
    AstCommon::Ast * ReductionRuleHandler0010 ();
    AstCommon::Ast * ReductionRuleHandler0011 ();
    AstCommon::Ast * ReductionRuleHandler0012 ();
    AstCommon::Ast * ReductionRuleHandler0013 ();
    AstCommon::Ast * ReductionRuleHandler0014 ();
    AstCommon::Ast * ReductionRuleHandler0015 ();
    AstCommon::Ast * ReductionRuleHandler0016 ();
    AstCommon::Ast * ReductionRuleHandler0017 ();
    AstCommon::Ast * ReductionRuleHandler0018 ();
    AstCommon::Ast * ReductionRuleHandler0019 ();
    AstCommon::Ast * ReductionRuleHandler0020 ();
    AstCommon::Ast * ReductionRuleHandler0021 ();
    AstCommon::Ast * ReductionRuleHandler0022 ();
    AstCommon::Ast * ReductionRuleHandler0023 ();
    AstCommon::Ast * ReductionRuleHandler0024 ();
    AstCommon::Ast * ReductionRuleHandler0025 ();
    AstCommon::Ast * ReductionRuleHandler0026 ();
    AstCommon::Ast * ReductionRuleHandler0027 ();
    AstCommon::Ast * ReductionRuleHandler0028 ();
    AstCommon::Ast * ReductionRuleHandler0029 ();
    AstCommon::Ast * ReductionRuleHandler0030 ();
    AstCommon::Ast * ReductionRuleHandler0031 ();
    AstCommon::Ast * ReductionRuleHandler0032 ();
    AstCommon::Ast * ReductionRuleHandler0033 ();
    AstCommon::Ast * ReductionRuleHandler0034 ();
    AstCommon::Ast * ReductionRuleHandler0035 ();
    AstCommon::Ast * ReductionRuleHandler0036 ();
    AstCommon::Ast * ReductionRuleHandler0037 ();
    AstCommon::Ast * ReductionRuleHandler0038 ();
    AstCommon::Ast * ReductionRuleHandler0039 ();
    AstCommon::Ast * ReductionRuleHandler0040 ();
    AstCommon::Ast * ReductionRuleHandler0041 ();
    AstCommon::Ast * ReductionRuleHandler0042 ();
    AstCommon::Ast * ReductionRuleHandler0043 ();
    AstCommon::Ast * ReductionRuleHandler0044 ();
    AstCommon::Ast * ReductionRuleHandler0045 ();
    AstCommon::Ast * ReductionRuleHandler0046 ();
    AstCommon::Ast * ReductionRuleHandler0047 ();
    AstCommon::Ast * ReductionRuleHandler0048 ();
    AstCommon::Ast * ReductionRuleHandler0049 ();
    AstCommon::Ast * ReductionRuleHandler0050 ();
    AstCommon::Ast * ReductionRuleHandler0051 ();
    AstCommon::Ast * ReductionRuleHandler0052 ();
    AstCommon::Ast * ReductionRuleHandler0053 ();
    AstCommon::Ast * ReductionRuleHandler0054 ();
    AstCommon::Ast * ReductionRuleHandler0055 ();
    AstCommon::Ast * ReductionRuleHandler0056 ();
    AstCommon::Ast * ReductionRuleHandler0057 ();
    AstCommon::Ast * ReductionRuleHandler0058 ();
    AstCommon::Ast * ReductionRuleHandler0059 ();
    AstCommon::Ast * ReductionRuleHandler0060 ();
    AstCommon::Ast * ReductionRuleHandler0061 ();
    AstCommon::Ast * ReductionRuleHandler0062 ();
    AstCommon::Ast * ReductionRuleHandler0063 ();
    AstCommon::Ast * ReductionRuleHandler0064 ();
    AstCommon::Ast * ReductionRuleHandler0065 ();
    AstCommon::Ast * ReductionRuleHandler0066 ();
    AstCommon::Ast * ReductionRuleHandler0067 ();
    AstCommon::Ast * ReductionRuleHandler0068 ();
    AstCommon::Ast * ReductionRuleHandler0069 ();
    AstCommon::Ast * ReductionRuleHandler0070 ();
    AstCommon::Ast * ReductionRuleHandler0071 ();
    AstCommon::Ast * ReductionRuleHandler0072 ();
    AstCommon::Ast * ReductionRuleHandler0073 ();
    AstCommon::Ast * ReductionRuleHandler0074 ();
    AstCommon::Ast * ReductionRuleHandler0075 ();
    AstCommon::Ast * ReductionRuleHandler0076 ();
    AstCommon::Ast * ReductionRuleHandler0077 ();
    AstCommon::Ast * ReductionRuleHandler0078 ();
    AstCommon::Ast * ReductionRuleHandler0079 ();
    AstCommon::Ast * ReductionRuleHandler0080 ();
    AstCommon::Ast * ReductionRuleHandler0081 ();

}; // end of class Parser

std::ostream &operator << (std::ostream &stream, Parser::Token::Type token_type);


#line 43 "barf_preprocessor_parser.trison"

} // end of namespace Preprocessor
} // end of namespace Barf

#endif // !defined(_BARF_PREPROCESSOR_PARSER_HPP_)

#line 359 "barf_preprocessor_parser.hpp"
