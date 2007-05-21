#include "trison_statemachine.hpp"

string const Trison::StateMachine::ms_implementation_file_template =
"\
#include \"$$HEADER_FILENAME$$\"\n\
\n\
#include <cassert>\n\
#include <cstdio>\n\
#include <iomanip>\n\
#include <iostream>\n\
\n\
#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x\n\
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x\n\
\n\
$$IMPLEMENTATION_FILE_TOP$$\n\
\n\
$$CLASS_NAME$$::$$CLASS_NAME$$ ()\n\
$$SUPERCLASS_AND_MEMBER_CONSTRUCTORS$$\n\
{\n\
$$CONSTRUCTOR_ACTIONS$$\n\
    m_debug_spew_level = 0;\n\
    DEBUG_SPEW_2(\"### number of state transitions = \" << ms_state_transition_count << std::endl);\n\
}\n\
\n\
$$CLASS_NAME$$::~$$CLASS_NAME$$ ()\n\
{\n\
$$DESTRUCTOR_ACTIONS$$\n\
}\n\
\n\
void $$CLASS_NAME$$::CheckStateConsistency ()\n\
{\n\
    unsigned int counter = 1;\n\
    for (unsigned int i = 0; i < ms_state_count; ++i)\n\
    {\n\
        if (ms_state[i].m_lookahead_transition_offset > 0)\n\
        {\n\
            assert(counter == ms_state[i].m_lookahead_transition_offset);\n\
            assert(ms_state[i].m_lookahead_transition_count > 0);\n\
        }\n\
        else\n\
            assert(ms_state[i].m_lookahead_transition_count == 0);\n\
\n\
        counter += ms_state[i].m_lookahead_transition_count;\n\
\n\
        if (ms_state[i].m_default_action_offset > 0)\n\
            ++counter;\n\
\n\
        if (ms_state[i].m_non_terminal_transition_offset > 0)\n\
        {\n\
            assert(counter == ms_state[i].m_non_terminal_transition_offset);\n\
            assert(ms_state[i].m_non_terminal_transition_offset > 0);\n\
        }\n\
        else\n\
            assert(ms_state[i].m_non_terminal_transition_offset == 0);\n\
\n\
        counter += ms_state[i].m_non_terminal_transition_count;\n\
    }\n\
    assert(counter == ms_state_transition_count);\n\
}\n\
\n\
$$CLASS_NAME$$::ParserReturnCode $$CLASS_NAME$$::Parse ($$BASE_ASSIGNED_TYPE$$ *return_token)\n\
{\n\
$$START_OF_PARSE_METHOD_ACTIONS$$\n\
\n\
    ParserReturnCode return_code = PrivateParse(return_token);\n\
\n\
$$END_OF_PARSE_METHOD_ACTIONS$$\n\
\n\
    return return_code;\n\
}\n\
\n\
bool $$CLASS_NAME$$::GetDoesStateAcceptErrorToken ($$CLASS_NAME$$::StateNumber state_number) const\n\
{\n\
    assert(state_number < ms_state_count);\n\
    State const &state = ms_state[state_number];\n\
\n\
    for (unsigned int transition = state.m_lookahead_transition_offset,\n\
                      transition_end = state.m_lookahead_transition_offset +\n\
                                       state.m_lookahead_transition_count;\n\
         transition < transition_end;\n\
         ++transition)\n\
    {\n\
        if (ms_state_transition[transition].m_token_type == Token::ERROR_)\n\
            return true;\n\
    }\n\
\n\
    return false;\n\
}\n\
\n\
$$CLASS_NAME$$::ParserReturnCode $$CLASS_NAME$$::PrivateParse ($$BASE_ASSIGNED_TYPE$$ *return_token)\n\
{\n\
    assert(return_token && \"the return-token pointer must be valid\");\n\
\n\
    m_state_stack.clear();\n\
    m_token_stack.clear();\n\
\n\
    m_lookahead_token_type = Token::INVALID_;\n\
    m_lookahead_token = $$BASE_ASSIGNED_TYPE_SENTINEL$$;\n\
    m_is_new_lookahead_token_required = true;\n\
    m_in_error_handling_mode = false;\n\
\n\
    m_is_returning_with_non_terminal = false;\n\
    m_returning_with_this_non_terminal = Token::INVALID_;\n\
\n\
    // start in state 0\n\
    PushState(0);\n\
\n\
    while (true)\n\
    {\n\
        StateNumber current_state_number = m_state_stack.back();\n\
        assert(current_state_number < ms_state_count);\n\
        State const &current_state = ms_state[current_state_number];\n\
\n\
        unsigned int state_transition_number;\n\
        unsigned int state_transition_count;\n\
        unsigned int default_action_state_transition_number;\n\
        Token::Type state_transition_token_type = Token::INVALID_;\n\
\n\
        // if we've just reduced to a non-terminal, coming from\n\
        // another state, use the non-terminal transitions.\n\
        if (m_is_returning_with_non_terminal)\n\
        {\n\
            m_is_returning_with_non_terminal = false;\n\
            state_transition_number = current_state.m_non_terminal_transition_offset;\n\
            state_transition_count = current_state.m_non_terminal_transition_count;\n\
            default_action_state_transition_number = 0;\n\
            state_transition_token_type = m_returning_with_this_non_terminal;\n\
        }\n\
        // otherwise use the lookahead transitions, with the default action\n\
        else\n\
        {\n\
            state_transition_number = current_state.m_lookahead_transition_offset;\n\
            state_transition_count = current_state.m_lookahead_transition_count;\n\
            default_action_state_transition_number = current_state.m_default_action_offset;\n\
            // GetLookaheadTokenType may cause Scan to be called, which may\n\
            // block execution.  only scan a token if necessary.\n\
            if (state_transition_count != 0)\n\
            {\n\
                state_transition_token_type = GetLookaheadTokenType();\n\
                DEBUG_SPEW_1(\"*** lookahead token type: \" << state_transition_token_type << std::endl);\n\
            }\n\
        }\n\
\n\
        unsigned int i;\n\
        for (i = 0;\n\
             i < state_transition_count;\n\
             ++i, ++state_transition_number)\n\
        {\n\
            StateTransition const &state_transition =\n\
                ms_state_transition[state_transition_number];\n\
            // if this token matches the current transition, do its action\n\
            if (m_in_error_handling_mode && state_transition.m_token_type == Token::ERROR_ && state_transition_token_type != Token::END_ ||\n\
                !m_in_error_handling_mode && state_transition.m_token_type == state_transition_token_type)\n\
            {\n\
                if (state_transition.m_token_type == Token::ERROR_)\n\
                {\n\
                    ThrowAwayToken(m_lookahead_token);\n\
                    m_lookahead_token = $$BASE_ASSIGNED_TYPE_SENTINEL$$;\n\
                    m_lookahead_token_type = Token::ERROR_;\n\
                }\n\
\n\
                PrintStateTransition(state_transition_number);\n\
                if (ProcessAction(state_transition.m_action) == ARC_ACCEPT_AND_RETURN)\n\
                {\n\
                    *return_token = m_reduction_token;\n\
                    return PRC_SUCCESS;\n\
                }\n\
                else\n\
                    break;\n\
            }\n\
        }\n\
\n\
        // if no transition matched, check for a default action.\n\
        if (i == state_transition_count)\n\
        {\n\
            // check for the default action\n\
            if (default_action_state_transition_number != 0)\n\
            {\n\
                PrintStateTransition(default_action_state_transition_number);\n\
                Action const &default_action =\n\
                    ms_state_transition[default_action_state_transition_number].m_action;\n\
                if (ProcessAction(default_action) == ARC_ACCEPT_AND_RETURN)\n\
                {\n\
                    *return_token = m_reduction_token;\n\
                    return PRC_SUCCESS;\n\
                }\n\
            }\n\
            // otherwise go into error recovery mode\n\
            else\n\
            {\n\
                assert(!m_is_new_lookahead_token_required);\n\
                assert(!m_in_error_handling_mode);\n\
\n\
                DEBUG_SPEW_1(\"!!! error recovery: begin\" << std::endl);\n\
                m_in_error_handling_mode = true;\n\
\n\
                // pop the stack until we reach an error-handling state, but only\n\
                // if the lookahead token isn't END_ (to prevent an infinite loop).\n\
                while (!GetDoesStateAcceptErrorToken(current_state_number) || m_lookahead_token_type == Token::END_)\n\
                {\n\
                    DEBUG_SPEW_1(\"!!! error recovery: popping state \" << current_state_number << std::endl);\n\
                    assert(m_token_stack.size() + 1 == m_state_stack.size());\n\
                    if (m_token_stack.size() > 0)\n\
                    {\n\
                        ThrowAwayToken(m_token_stack.back());\n\
                        m_token_stack.pop_back();\n\
                    }\n\
                    m_state_stack.pop_back();\n\
\n\
                    if (m_state_stack.size() == 0)\n\
                    {\n\
                        DEBUG_SPEW_1(\"!!! error recovery: unhandled error -- quitting\" << std::endl);\n\
                        *return_token = $$BASE_ASSIGNED_TYPE_SENTINEL$$;\n\
                        return PRC_UNHANDLED_PARSE_ERROR;\n\
                    }\n\
\n\
                    current_state_number = m_state_stack.back();\n\
                }\n\
\n\
                DEBUG_SPEW_1(\"!!! error recovery: found state which accepts %error token\" << std::endl);\n\
                PrintStateStack();\n\
            }\n\
        }\n\
    }\n\
\n\
    // this should never happen because the above loop is infinite, but we'll do\n\
    // stuff here anyway in case some compiler isn't smart enough to realize it.\n\
    *return_token = $$BASE_ASSIGNED_TYPE_SENTINEL$$;\n\
    return PRC_UNHANDLED_PARSE_ERROR;\n\
}\n\
\n\
$$CLASS_NAME$$::ActionReturnCode $$CLASS_NAME$$::ProcessAction ($$CLASS_NAME$$::Action const &action)\n\
{\n\
    if (action.m_transition_action == TA_SHIFT_AND_PUSH_STATE)\n\
    {\n\
        m_in_error_handling_mode = false;\n\
        ShiftLookaheadToken();\n\
        PushState(action.m_data);\n\
    }\n\
    else if (action.m_transition_action == TA_PUSH_STATE)\n\
    {\n\
        assert(!m_in_error_handling_mode);\n\
        PushState(action.m_data);\n\
    }\n\
    else if (action.m_transition_action == TA_REDUCE_USING_RULE)\n\
    {\n\
        assert(!m_in_error_handling_mode);\n\
        unsigned int reduction_rule_number = action.m_data;\n\
        assert(reduction_rule_number < ms_reduction_rule_count);\n\
        ReductionRule const &reduction_rule = ms_reduction_rule[reduction_rule_number];\n\
        ReduceUsingRule(reduction_rule, false);\n\
    }\n\
    else if (action.m_transition_action == TA_REDUCE_AND_ACCEPT_USING_RULE)\n\
    {\n\
        assert(!m_in_error_handling_mode);\n\
        unsigned int reduction_rule_number = action.m_data;\n\
        assert(reduction_rule_number < ms_reduction_rule_count);\n\
        ReductionRule const &reduction_rule = ms_reduction_rule[reduction_rule_number];\n\
        ReduceUsingRule(reduction_rule, true);\n\
        DEBUG_SPEW_1(\"*** accept\" << std::endl);\n\
        // everything is done, so just return.\n\
        return ARC_ACCEPT_AND_RETURN;\n\
    }\n\
\n\
    return ARC_CONTINUE_PARSING;\n\
}\n\
\n\
void $$CLASS_NAME$$::ShiftLookaheadToken ()\n\
{\n\
    assert(m_lookahead_token_type != Token::DEFAULT_);\n\
    assert(m_lookahead_token_type != Token::INVALID_);\n\
    DEBUG_SPEW_1(\"*** shifting lookahead token -- type \" << m_lookahead_token_type << std::endl);\n\
    m_token_stack.push_back(m_lookahead_token);\n\
    m_is_new_lookahead_token_required = true;\n\
}\n\
\n\
void $$CLASS_NAME$$::PushState (StateNumber const state_number)\n\
{\n\
    assert(state_number < ms_state_count);\n\
\n\
    DEBUG_SPEW_1(\"*** going to state \" << state_number << std::endl);\n\
    m_state_stack.push_back(state_number);\n\
    PrintStateStack();\n\
}\n\
\n\
void $$CLASS_NAME$$::ReduceUsingRule (ReductionRule const &reduction_rule, bool and_accept)\n\
{\n\
    if (and_accept)\n\
    {\n\
        assert(reduction_rule.m_number_of_tokens_to_reduce_by == m_state_stack.size() - 1);\n\
        assert(reduction_rule.m_number_of_tokens_to_reduce_by == m_token_stack.size());\n\
    }\n\
    else\n\
    {\n\
        assert(reduction_rule.m_number_of_tokens_to_reduce_by < m_state_stack.size());\n\
        assert(reduction_rule.m_number_of_tokens_to_reduce_by <= m_token_stack.size());\n\
    }\n\
\n\
    DEBUG_SPEW_1(\"*** reducing: \" << reduction_rule.m_description << std::endl);\n\
\n\
    m_is_returning_with_non_terminal = true;\n\
    m_returning_with_this_non_terminal = reduction_rule.m_non_terminal_to_reduce_to;\n\
    m_reduction_rule_token_count = reduction_rule.m_number_of_tokens_to_reduce_by;\n\
\n\
    // call the reduction rule handler if it exists\n\
    if (reduction_rule.m_handler != NULL)\n\
        m_reduction_token = (this->*(reduction_rule.m_handler))();\n\
    else\n\
        m_reduction_token = $$BASE_ASSIGNED_TYPE_SENTINEL$$;\n\
    // pop the states and tokens\n\
    PopStates(reduction_rule.m_number_of_tokens_to_reduce_by, false);\n\
\n\
    // only push the reduced token if we aren't accepting yet\n\
    if (!and_accept)\n\
    {\n\
        // push the token that resulted from the reduction\n\
        m_token_stack.push_back(m_reduction_token);\n\
        m_reduction_token = $$BASE_ASSIGNED_TYPE_SENTINEL$$;\n\
        PrintStateStack();\n\
    }\n\
}\n\
\n\
void $$CLASS_NAME$$::PopStates (unsigned int number_of_states_to_pop, bool print_state_stack)\n\
{\n\
    assert(m_token_stack.size() + 1 == m_state_stack.size());\n\
    assert(number_of_states_to_pop < m_state_stack.size());\n\
    assert(number_of_states_to_pop <= m_token_stack.size());\n\
\n\
    while (number_of_states_to_pop-- > 0)\n\
    {\n\
        m_state_stack.pop_back();\n\
        m_token_stack.pop_back();\n\
    }\n\
\n\
    if (print_state_stack)\n\
        PrintStateStack();\n\
}\n\
\n\
void $$CLASS_NAME$$::PrintStateStack () const\n\
{\n\
    DEBUG_SPEW_2(\"*** state stack: \");\n\
    for (StateStack::const_iterator it = m_state_stack.begin(),\n\
                                 it_end = m_state_stack.end();\n\
         it != it_end;\n\
         ++it)\n\
    {\n\
        DEBUG_SPEW_2(*it << \" \");\n\
    }\n\
    DEBUG_SPEW_2(std::endl);\n\
}\n\
\n\
void $$CLASS_NAME$$::PrintStateTransition (unsigned int const state_transition_number) const\n\
{\n\
    assert(state_transition_number < ms_state_transition_count);\n\
    DEBUG_SPEW_2(\"&&& exercising state transition \" << std::setw(4) << std::right << state_transition_number << std::endl);\n\
}\n\
\n\
void $$CLASS_NAME$$::ScanANewLookaheadToken ()\n\
{\n\
    assert(!m_is_new_lookahead_token_required);\n\
    m_lookahead_token = $$BASE_ASSIGNED_TYPE_SENTINEL$$;\n\
    m_lookahead_token_type = Scan();\n\
    DEBUG_SPEW_1(\"*** scanned a new lookahead token -- type \" << m_lookahead_token_type << std::endl);\n\
}\n\
\n\
void $$CLASS_NAME$$::ThrowAwayToken ($$BASE_ASSIGNED_TYPE$$ token)\n\
{\n\
    DEBUG_SPEW_1(\"*** throwing away token of type \" << m_lookahead_token_type << std::endl);\n\
\n\
$$THROW_AWAY_TOKEN_ACTIONS$$\n\
}\n\
\n\
void $$CLASS_NAME$$::ThrowAwayTokenStack ()\n\
{\n\
    while (!m_token_stack.empty())\n\
    {\n\
        ThrowAwayToken(m_token_stack.back());\n\
        m_token_stack.pop_back();\n\
    }\n\
}\n\
\n\
std::ostream &operator << (std::ostream &stream, $$CLASS_NAME$$::Token::Type token_type)\n\
{\n\
    static std::string const s_token_type_string[] =\n\
    {\n\
$$TERMINAL_TOKEN_STRINGS$$\n\
$$NONTERMINAL_TOKEN_STRINGS$$\n\
        \"%error\",\n\
        \"DEFAULT_\",\n\
        \"INVALID_\"\n\
    };\n\
    static unsigned int const s_token_type_string_count =\n\
        sizeof(s_token_type_string) /\n\
        sizeof(std::string);\n\
\n\
    unsigned token_type_value = static_cast<unsigned int>(token_type);\n\
    if (token_type_value < 0x20)\n\
        stream << token_type_value;\n\
    else if (token_type_value < 0x7F)\n\
        stream << \"'\" << static_cast<char>(token_type) << \"'\";\n\
    else if (token_type_value < 0x100)\n\
        stream << token_type_value;\n\
    else if (token_type_value < 0x100 + s_token_type_string_count)\n\
        stream << s_token_type_string[token_type_value - 0x100];\n\
    else\n\
        stream << token_type_value;\n\
\n\
    return stream;\n\
}\n\
\n\
// ///////////////////////////////////////////////////////////////////////////\n\
// state machine reduction rule handlers\n\
// ///////////////////////////////////////////////////////////////////////////\n\
\n\
$$REDUCTION_RULE_HANDLER_DEFINITIONS$$\n\
\n\
// ///////////////////////////////////////////////////////////////////////////\n\
// reduction rule lookup table\n\
// ///////////////////////////////////////////////////////////////////////////\n\
\n\
$$CLASS_NAME$$::ReductionRule const $$CLASS_NAME$$::ms_reduction_rule[] =\n\
{\n\
$$REDUCTION_RULE_LOOKUP_TABLE$$\n\
    // special error panic reduction rule\n\
    {                 Token::ERROR_,  1,                                     NULL, \"* -> error\"}\n\
};\n\
\n\
unsigned int const $$CLASS_NAME$$::ms_reduction_rule_count =\n\
    sizeof($$CLASS_NAME$$::ms_reduction_rule) /\n\
    sizeof($$CLASS_NAME$$::ReductionRule);\n\
\n\
// ///////////////////////////////////////////////////////////////////////////\n\
// state transition lookup table\n\
// ///////////////////////////////////////////////////////////////////////////\n\
\n\
$$CLASS_NAME$$::State const $$CLASS_NAME$$::ms_state[] =\n\
{\n\
$$STATE_TRANSITION_LOOKUP_TABLE$$\n\
};\n\
\n\
unsigned int const $$CLASS_NAME$$::ms_state_count =\n\
    sizeof($$CLASS_NAME$$::ms_state) /\n\
    sizeof($$CLASS_NAME$$::State);\n\
\n\
// ///////////////////////////////////////////////////////////////////////////\n\
// state transition table\n\
// ///////////////////////////////////////////////////////////////////////////\n\
\n\
$$CLASS_NAME$$::StateTransition const $$CLASS_NAME$$::ms_state_transition[] =\n\
{\n\
    // dummy transition in the NULL transition\n\
    {               Token::INVALID_, {                       TA_COUNT,    0}},\n\
\n\
$$STATE_TRANSITION_TABLE$$\n\
};\n\
\n\
unsigned int const $$CLASS_NAME$$::ms_state_transition_count =\n\
    sizeof($$CLASS_NAME$$::ms_state_transition) /\n\
    sizeof($$CLASS_NAME$$::StateTransition);\n\
\n\
$$IMPLEMENTATION_FILE_BOTTOM$$\n\
\n\
";

