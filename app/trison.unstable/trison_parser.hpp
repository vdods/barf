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
namespace AstCommon {

class Ast;
class IdentifierMap;
class IntegerMap;

} // end of namespace AstCommon

namespace CommonLang {

class TargetLanguageMap;

} // end of namespace CommonLang
} // end of namespace Barf

namespace Trison {

struct NonterminalList;
struct PrecedenceList;
struct PrecedenceMap;
struct TokenMap;

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
            CHARACTER_LITERAL,
            DIRECTIVE_ERROR,
            DIRECTIVE_LANGUAGE,
            DIRECTIVE_LEFT,
            DIRECTIVE_NONASSOC,
            DIRECTIVE_PREC,
            DIRECTIVE_RIGHT,
            DIRECTIVE_START,
            DIRECTIVE_TARGET_LANGUAGES,
            DIRECTIVE_TOKEN,
            DIRECTIVE_TYPE,
            DUMB_CODE_BLOCK,
            END_PREAMBLE,
            IDENTIFIER,
            NEWLINE,
            STRICT_CODE_BLOCK,
            STRING_LITERAL,

            // special end-of-input terminal
            END_,

            // user-defined nonterminal tokens
            any_type_of_code_block__,
            at_least_one_newline__,
            at_least_zero_newlines__,
            language_directive__,
            language_directive_param__,
            language_directives__,
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
            target_language_identifiers__,
            target_languages_directive__,
            token__,
            token_directive__,
            token_directives__,
            token_type_spec__,
            tokens__,

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


#line 46 "trison_parser.trison"

    inline FileLocation const &GetFileLocation () const { return m_scanner.GetFileLocation(); }

    bool ScannerDebugSpew () const { return m_scanner.DebugSpew(); }
    void ScannerDebugSpew (bool debug_spew) { m_scanner.DebugSpew(debug_spew); }

    bool OpenFile (string const &input_filename);

private:

    Token::Type Scan ();

    CommonLang::Scanner m_scanner;
    CommonLang::TargetLanguageMap *m_target_language_map;
    TokenMap *m_token_map;
    PrecedenceList *m_precedence_list;
    PrecedenceMap *m_precedence_map;
    NonterminalList *m_nonterminal_list;

#line 158 "trison_parser.hpp"

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

}; // end of class Parser

std::ostream &operator << (std::ostream &stream, Parser::Token::Type token_type);


#line 66 "trison_parser.trison"

} // end of namespace Trison

#endif // !defined(_TRISON_PARSER_HPP_)

#line 358 "trison_parser.hpp"
