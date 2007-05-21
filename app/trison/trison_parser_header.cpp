#include "trison_statemachine.hpp"

string const Trison::StateMachine::ms_header_file_template =
"\
#include <ostream>\n\
#include <string>\n\
#include <vector>\n\
\n\
$$HEADER_FILE_TOP$$\n\
\n\
class $$CLASS_NAME$$\n\
$$CLASS_INHERITANCE$$\n\
{\n\
public:\n\
\n\
    struct Token\n\
    {\n\
        enum Type\n\
        {\n\
            // user-defined terminal, non-single-character tokens\n\
$$TERMINAL_TOKEN_DECLARATIONS$$\n\
            // user-defined nonterminal tokens\n\
$$NONTERMINAL_TOKEN_DECLARATIONS$$\n\
            // parser's sentinel and special tokens\n\
            ERROR_,\n\
            DEFAULT_,\n\
            INVALID_\n\
        }; // end of enum $$CLASS_NAME$$::Token::Type\n\
    }; // end of struct $$CLASS_NAME$$::Token\n\
\n\
    $$CLASS_NAME$$ ();\n\
    $$FORCE_VIRTUAL_DESTRUCTOR$$~$$CLASS_NAME$$ ();\n\
\n\
    inline unsigned int GetDebugSpewLevel () const { return m_debug_spew_level; }\n\
    inline void SetDebugSpewLevel (unsigned int debug_spew_level) { m_debug_spew_level = debug_spew_level; }\n\
\n\
    static void CheckStateConsistency ();\n\
\n\
    enum ParserReturnCode\n\
    {\n\
        PRC_SUCCESS = 0,\n\
        PRC_UNHANDLED_PARSE_ERROR = 1\n\
    }; // end of enum ParserReturnCode\n\
\n\
$$PARSE_METHOD_ACCESS$$\n\
\n\
    ParserReturnCode Parse ($$BASE_ASSIGNED_TYPE$$ *parsed_tree_root);\n\
\n\
public:\n\
\n\
$$CLASS_METHODS_AND_MEMBERS$$\n\
\n\
private:\n\
\n\
    typedef unsigned int StateNumber;\n\
\n\
    enum TransitionAction\n\
    {\n\
        TA_SHIFT_AND_PUSH_STATE = 0,\n\
        TA_PUSH_STATE,\n\
        TA_REDUCE_USING_RULE,\n\
        TA_REDUCE_AND_ACCEPT_USING_RULE,\n\
\n\
        TA_COUNT\n\
    };\n\
\n\
    enum ActionReturnCode\n\
    {\n\
        ARC_CONTINUE_PARSING = 0,\n\
        ARC_ACCEPT_AND_RETURN\n\
    };\n\
\n\
    struct ReductionRule\n\
    {\n\
        typedef $$BASE_ASSIGNED_TYPE$$ ($$CLASS_NAME$$::*ReductionRuleHandler)();\n\
\n\
        Token::Type m_non_terminal_to_reduce_to;\n\
        unsigned int m_number_of_tokens_to_reduce_by;\n\
        ReductionRuleHandler m_handler;\n\
        std::string m_description;\n\
    };\n\
\n\
    struct Action\n\
    {\n\
        TransitionAction m_transition_action;\n\
        unsigned int m_data;\n\
    };\n\
\n\
    struct StateTransition\n\
    {\n\
        Token::Type m_token_type;\n\
        Action m_action;\n\
    };\n\
\n\
    struct State\n\
    {\n\
        unsigned int m_lookahead_transition_offset;\n\
        unsigned int m_lookahead_transition_count;\n\
        unsigned int m_default_action_offset;\n\
        unsigned int m_non_terminal_transition_offset;\n\
        unsigned int m_non_terminal_transition_count;\n\
    };\n\
\n\
    inline void GetNewLookaheadToken ()\n\
    {\n\
        if (m_is_new_lookahead_token_required)\n\
        {\n\
            m_is_new_lookahead_token_required = false;\n\
            ScanANewLookaheadToken();\n\
        }\n\
    }\n\
    inline Token::Type GetLookaheadTokenType ()\n\
    {\n\
        GetNewLookaheadToken();\n\
        return m_lookahead_token_type;\n\
    }\n\
    inline $$BASE_ASSIGNED_TYPE$$ const &GetLookaheadToken ()\n\
    {\n\
        GetNewLookaheadToken();\n\
        return m_lookahead_token;\n\
    }\n\
    bool GetDoesStateAcceptErrorToken (StateNumber state_number) const;\n\
\n\
    ParserReturnCode PrivateParse ($$BASE_ASSIGNED_TYPE$$ *parsed_tree_root);\n\
\n\
    ActionReturnCode ProcessAction (Action const &action);\n\
    void ShiftLookaheadToken ();\n\
    void PushState (StateNumber state_number);\n\
    void ReduceUsingRule (ReductionRule const &reduction_rule, bool and_accept);\n\
    void PopStates (unsigned int number_of_states_to_pop, bool print_state_stack = true);\n\
    void PrintStateStack () const;\n\
    void PrintTokenStack () const;\n\
    void PrintStateTransition (unsigned int state_transition_number) const;\n\
    void ScanANewLookaheadToken ();\n\
    void ThrowAwayToken ($$BASE_ASSIGNED_TYPE$$ token);\n\
    void ThrowAwayTokenStack ();\n\
\n\
    typedef std::vector<StateNumber> StateStack;\n\
    typedef std::vector< $$BASE_ASSIGNED_TYPE$$ > TokenStack;\n\
\n\
    unsigned int m_debug_spew_level;\n\
\n\
    StateStack m_state_stack;\n\
    TokenStack m_token_stack;\n\
\n\
    Token::Type m_lookahead_token_type;\n\
    $$BASE_ASSIGNED_TYPE$$ m_lookahead_token;\n\
    bool m_is_new_lookahead_token_required;\n\
    bool m_in_error_handling_mode;\n\
\n\
    bool m_is_returning_with_non_terminal;\n\
    Token::Type m_returning_with_this_non_terminal;\n\
\n\
    $$BASE_ASSIGNED_TYPE$$ m_reduction_token;\n\
    unsigned int m_reduction_rule_token_count;\n\
\n\
    static State const ms_state[];\n\
    static unsigned int const ms_state_count;\n\
    static ReductionRule const ms_reduction_rule[];\n\
    static unsigned int const ms_reduction_rule_count;\n\
    static StateTransition const ms_state_transition[];\n\
    static unsigned int const ms_state_transition_count;\n\
\n\
$$REDUCTION_RULE_HANDLER_DECLARATIONS$$\n\
}; // end of class $$CLASS_NAME$$\n\
\n\
std::ostream &operator << (std::ostream &stream, $$CLASS_NAME$$::Token::Type token_type);\n\
\n\
$$HEADER_FILE_BOTTOM$$\n\
";

