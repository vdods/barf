#include <ostream>
#include <string>
#include <vector>


#line 13 "barf_langspec_parser.trison"

#if !defined(_BARF_LANGSPEC_PARSER_HPP_)
#define _BARF_LANGSPEC_PARSER_HPP_

#include "barf.hpp"

#include "barf_commonlang_scanner.hpp"

namespace Barf {
namespace AstCommon {

class Ast;

} // end of namespace AstCommon

namespace LangSpec {

struct AddCodeSpecList;
struct AddDirectiveMap;

#line 28 "barf_langspec_parser.hpp"

class Parser

{
public:

    struct Token
    {
        enum Type
        {
            // user-defined terminal, non-single-character tokens
            BAD_TOKEN = 0x100,
            DIRECTIVE_ADD_CODESPEC,
            DIRECTIVE_ADD_OPTIONAL_DIRECTIVE,
            DIRECTIVE_ADD_REQUIRED_DIRECTIVE,
            DIRECTIVE_DEFAULT,
            DIRECTIVE_DUMB_CODE_BLOCK,
            DIRECTIVE_IDENTIFIER,
            DIRECTIVE_STRICT_CODE_BLOCK,
            DIRECTIVE_STRING,
            DIRECTIVE_TARGET_LANGUAGE,
            DUMB_CODE_BLOCK,
            IDENTIFIER,
            NEWLINE,
            STRICT_CODE_BLOCK,
            STRING_LITERAL,

            // special end-of-input terminal
            END_,

            // user-defined nonterminal tokens
            add_codespec__,
            add_directive__,
            at_least_one_newline__,
            at_least_zero_newlines__,
            default_value__,
            directives__,
            param_spec__,
            root__,
            target_language__,

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


#line 34 "barf_langspec_parser.trison"

    inline FiLoc const &GetFiLoc () const { return m_scanner.GetFiLoc(); }

    bool OpenFile (string const &input_filename);

private:

    Token::Type Scan ();

    CommonLang::Scanner m_scanner;
    AddCodeSpecList *m_add_codespec_list;
    AddDirectiveMap *m_add_directive_map;

#line 118 "barf_langspec_parser.hpp"

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

}; // end of class Parser

std::ostream &operator << (std::ostream &stream, Parser::Token::Type token_type);


#line 48 "barf_langspec_parser.trison"

} // end of namespace LangSpec
} // end of namespace Barf

#endif // !defined(_BARF_LANGSPEC_PARSER_HPP_)

#line 277 "barf_langspec_parser.hpp"
