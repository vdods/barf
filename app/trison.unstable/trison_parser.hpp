#include <ostream>
#include <string>
#include <vector>


#line 13 "trison_parser.trison"

#if !defined(_TRISON_PARSER_HPP_)
#define _TRISON_PARSER_HPP_

#include "trison.hpp"

#include "barf_commonlang_scanner.hpp"
#include "trison_message.hpp"

namespace Barf {
namespace Ast {

class Base;
class IdMap;
class IntegerMap;

} // end of namespace Ast

namespace CommonLang {

class TargetMap;

} // end of namespace CommonLang
} // end of namespace Barf

namespace Trison {

struct NonterminalList;
struct PrecedenceList;
struct PrecedenceMap;
struct TerminalMap;

#line 40 "trison_parser.hpp"

class Parser

{
public:

    struct Token
    {
        enum Type
        {
            // user-defined terminal, non-single-character tokens
            BAD_TOKEN = 0x100,
            CHAR_LITERAL,
            DIRECTIVE_DEFAULT_PARSE_NONTERMINAL,
            DIRECTIVE_ERROR,
            DIRECTIVE_NONTERMINAL,
            DIRECTIVE_PREC,
            DIRECTIVE_TARGET,
            DIRECTIVE_TARGETS,
            DIRECTIVE_TERMINAL,
            DIRECTIVE_TYPE,
            DUMB_CODE_BLOCK,
            END_PREAMBLE,
            ID,
            NEWLINE,
            STRICT_CODE_BLOCK,
            STRING_LITERAL,

            // special end-of-input terminal
            END_,

            // user-defined nonterminal tokens
            any_type_of_code_block__,
            at_least_one_newline__,
            at_least_zero_newlines__,
            nonterminal__,
            nonterminal_specification__,
            nonterminals__,
            precedence_directive__,
            precedence_directives__,
            root__,
            rule__,
            rule_handler__,
            rule_handlers__,
            rule_precedence_directive__,
            rule_specification__,
            rule_token__,
            rule_token_list__,
            rules__,
            start_directive__,
            target_directive__,
            target_directive_param__,
            target_directives__,
            target_ids__,
            targets_directive__,
            terminal__,
            terminal_directive__,
            terminal_directives__,
            terminals__,
            token_id__,
            type_spec__,

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


#line 46 "trison_parser.trison"

    inline FiLoc const &GetFiLoc () const { return m_scanner.GetFiLoc(); }

    bool ScannerDebugSpew () const { return m_scanner.DebugSpew(); }
    void ScannerDebugSpew (bool debug_spew) { m_scanner.DebugSpew(debug_spew); }

    bool OpenFile (string const &input_filename);

private:

    Token::Type Scan ();

    CommonLang::Scanner m_scanner;
    CommonLang::TargetMap *m_target_map;
    TerminalMap *m_terminal_map;
    PrecedenceList *m_precedence_list;
    PrecedenceMap *m_precedence_map;
    NonterminalList *m_nonterminal_list;
    Uint32 m_rule_count;

#line 155 "trison_parser.hpp"

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
    Ast::Base * ReductionRuleHandler0054 ();
    Ast::Base * ReductionRuleHandler0055 ();
    Ast::Base * ReductionRuleHandler0056 ();
    Ast::Base * ReductionRuleHandler0057 ();
    Ast::Base * ReductionRuleHandler0058 ();
    Ast::Base * ReductionRuleHandler0059 ();
    Ast::Base * ReductionRuleHandler0060 ();
    Ast::Base * ReductionRuleHandler0061 ();
    Ast::Base * ReductionRuleHandler0062 ();
    Ast::Base * ReductionRuleHandler0063 ();

}; // end of class Parser

std::ostream &operator << (std::ostream &stream, Parser::Token::Type token_type);


#line 67 "trison_parser.trison"

} // end of namespace Trison

#endif // !defined(_TRISON_PARSER_HPP_)

#line 344 "trison_parser.hpp"
