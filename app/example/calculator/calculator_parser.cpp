#include "calculator_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x


#line 47 "calculator_parser.trison"

#include "calculator_scanner.hpp"

#define MODULO(x) (zerop(m_modulus) ? x : cl_N(mod(realpart(x), m_modulus)))

namespace Calculator {

#line 21 "calculator_parser.cpp"

Parser::Parser ()

{

#line 55 "calculator_parser.trison"

    m_scanner = NULL;
    m_last_result = cl_I(0);
    m_modulus = cl_I(0);
    m_should_print_result = true;

#line 34 "calculator_parser.cpp"
    m_debug_spew_level = 0;
    DEBUG_SPEW_2("### number of state transitions = " << ms_state_transition_count << std::endl);
    m_reduction_token = 0;
}

Parser::~Parser ()
{

#line 62 "calculator_parser.trison"

    delete m_scanner;
    m_scanner = NULL;

#line 48 "calculator_parser.cpp"
}

void Parser::CheckStateConsistency ()
{
    unsigned int counter = 1;
    for (unsigned int i = 0; i < ms_state_count; ++i)
    {
        if (ms_state[i].m_lookahead_transition_offset > 0)
        {
            assert(counter == ms_state[i].m_lookahead_transition_offset);
            assert(ms_state[i].m_lookahead_transition_count > 0);
        }
        else
            assert(ms_state[i].m_lookahead_transition_count == 0);

        counter += ms_state[i].m_lookahead_transition_count;

        if (ms_state[i].m_default_action_offset > 0)
            ++counter;

        if (ms_state[i].m_non_terminal_transition_offset > 0)
        {
            assert(counter == ms_state[i].m_non_terminal_transition_offset);
            assert(ms_state[i].m_non_terminal_transition_offset > 0);
        }
        else
            assert(ms_state[i].m_non_terminal_transition_offset == 0);

        counter += ms_state[i].m_non_terminal_transition_count;
    }
    assert(counter == ms_state_transition_count);
}

Parser::ParserReturnCode Parser::Parse ()
{

#line 67 "calculator_parser.trison"

    m_should_print_result = true;

#line 89 "calculator_parser.cpp"

    ParserReturnCode return_code = PrivateParse();



    return return_code;
}

bool Parser::GetDoesStateAcceptErrorToken (Parser::StateNumber state_number) const
{
    assert(state_number < ms_state_count);
    State const &state = ms_state[state_number];

    for (unsigned int transition = state.m_lookahead_transition_offset,
                      transition_end = state.m_lookahead_transition_offset +
                                       state.m_lookahead_transition_count;
         transition < transition_end;
         ++transition)
    {
        if (ms_state_transition[transition].m_token_type == Token::ERROR_)
            return true;
    }

    return false;
}

Parser::ParserReturnCode Parser::PrivateParse ()
{
    m_state_stack.clear();
    m_token_stack.clear();

    m_lookahead_token_type = Token::INVALID_;
    m_lookahead_token = 0;
    m_is_new_lookahead_token_required = true;

    m_saved_lookahead_token_type = Token::INVALID_;
    m_get_new_lookahead_token_type_from_saved = false;
    m_previous_transition_accepted_error_token = false;

    m_is_returning_with_non_terminal = false;
    m_returning_with_this_non_terminal = Token::INVALID_;

    // start in state 0
    PushState(0);

    while (true)
    {
        StateNumber current_state_number = m_state_stack.back();
        assert(current_state_number < ms_state_count);
        State const &current_state = ms_state[current_state_number];

        unsigned int state_transition_number;
        unsigned int state_transition_count;
        unsigned int default_action_state_transition_number;
        Token::Type state_transition_token_type = Token::INVALID_;

        // if we've just reduced to a non-terminal, coming from
        // another state, use the non-terminal transitions.
        if (m_is_returning_with_non_terminal)
        {
            m_is_returning_with_non_terminal = false;
            state_transition_number = current_state.m_non_terminal_transition_offset;
            state_transition_count = current_state.m_non_terminal_transition_count;
            default_action_state_transition_number = 0;
            state_transition_token_type = m_returning_with_this_non_terminal;
        }
        // otherwise use the lookahead transitions, with the default action
        else
        {
            state_transition_number = current_state.m_lookahead_transition_offset;
            state_transition_count = current_state.m_lookahead_transition_count;
            default_action_state_transition_number = current_state.m_default_action_offset;
            // GetLookaheadTokenType may cause Scan to be called, which may
            // block execution.  only scan a token if necessary.
            if (state_transition_count != 0)
            {
                state_transition_token_type = GetLookaheadTokenType();
                DEBUG_SPEW_1("*** lookahead token type: " << state_transition_token_type << std::endl);
            }
        }

        unsigned int i;
        for (i = 0;
             i < state_transition_count;
             ++i, ++state_transition_number)
        {
            StateTransition const &state_transition =
                ms_state_transition[state_transition_number];
            // if this token matches the current transition, do its action
            if (state_transition.m_token_type == state_transition_token_type)
            {
                if (state_transition.m_token_type == Token::ERROR_)
                    m_previous_transition_accepted_error_token = true;
                else
                    m_previous_transition_accepted_error_token = false;

                PrintStateTransition(state_transition_number);
                if (ProcessAction(state_transition.m_action) == ARC_ACCEPT_AND_RETURN)
                    return PRC_SUCCESS; // the accepted token is in m_reduction_token
                else
                    break;
            }
        }

        // if no transition matched, check for a default action.
        if (i == state_transition_count)
        {
            // check for the default action
            if (default_action_state_transition_number != 0)
            {
                PrintStateTransition(default_action_state_transition_number);
                Action const &default_action =
                    ms_state_transition[default_action_state_transition_number].m_action;
                if (ProcessAction(default_action) == ARC_ACCEPT_AND_RETURN)
                    return PRC_SUCCESS; // the accepted token is in m_reduction_token
            }
            // otherwise go into error recovery mode
            else
            {
                assert(!m_is_new_lookahead_token_required);

                DEBUG_SPEW_1("!!! error recovery: begin" << std::endl);

                // if an error was encountered, and this state accepts the %error
                // token, then we don't need to pop states
                if (GetDoesStateAcceptErrorToken(current_state_number))
                {
                    // if an error token was previously accepted, then throw
                    // away the lookahead token, because whatever the lookahead
                    // was didn't match.  this prevents an infinite loop.
                    if (m_previous_transition_accepted_error_token)
                    {
                        ThrowAwayToken(m_lookahead_token);
                        m_is_new_lookahead_token_required = true;
                    }
                    // otherwise, save off the lookahead token so that once the
                    // %error token has been shifted, the lookahead can be
                    // re-analyzed.
                    else
                    {
                        m_saved_lookahead_token_type = m_lookahead_token_type;
                        m_get_new_lookahead_token_type_from_saved = true;
                        m_lookahead_token_type = Token::ERROR_;
                    }
                }
                // otherwise save off the lookahead token for the error panic popping
                else
                {
                    // save off the lookahead token type and set the current to Token::ERROR_
                    m_saved_lookahead_token_type = m_lookahead_token_type;
                    m_get_new_lookahead_token_type_from_saved = true;
                    m_lookahead_token_type = Token::ERROR_;

                    // pop until we either run off the stack, or find a state
                    // which accepts the %error token.
                    assert(m_state_stack.size() > 0);
                    do
                    {
                        DEBUG_SPEW_1("!!! error recovery: popping state " << current_state_number << std::endl);
                        m_state_stack.pop_back();

                        if (m_state_stack.size() == 0)
                        {
                            ThrowAwayTokenStack();
                            DEBUG_SPEW_1("!!! error recovery: unhandled error -- quitting" << std::endl);
                            return PRC_UNHANDLED_PARSE_ERROR;
                        }

                        assert(m_token_stack.size() > 0);
                        ThrowAwayToken(m_token_stack.back());
                        m_token_stack.pop_back();
                        current_state_number = m_state_stack.back();
                    }
                    while (!GetDoesStateAcceptErrorToken(current_state_number));
                }

                DEBUG_SPEW_1("!!! error recovery: found state which accepts %error token" << std::endl);
                PrintStateStack();
            }
        }
    }

    // this should never happen because the above loop is infinite
    return PRC_UNHANDLED_PARSE_ERROR;
}

Parser::ActionReturnCode Parser::ProcessAction (Parser::Action const &action)
{
    if (action.m_transition_action == TA_SHIFT_AND_PUSH_STATE)
    {
        ShiftLookaheadToken();
        PushState(action.m_data);
    }
    else if (action.m_transition_action == TA_PUSH_STATE)
    {
        PushState(action.m_data);
    }
    else if (action.m_transition_action == TA_REDUCE_USING_RULE)
    {
        unsigned int reduction_rule_number = action.m_data;
        assert(reduction_rule_number < ms_reduction_rule_count);
        ReductionRule const &reduction_rule = ms_reduction_rule[reduction_rule_number];
        ReduceUsingRule(reduction_rule, false);
    }
    else if (action.m_transition_action == TA_REDUCE_AND_ACCEPT_USING_RULE)
    {
        unsigned int reduction_rule_number = action.m_data;
        assert(reduction_rule_number < ms_reduction_rule_count);
        ReductionRule const &reduction_rule = ms_reduction_rule[reduction_rule_number];
        ReduceUsingRule(reduction_rule, true);
        DEBUG_SPEW_1("*** accept" << std::endl);
        // everything is done, so just return.
        return ARC_ACCEPT_AND_RETURN;
    }
    else if (action.m_transition_action == TA_THROW_AWAY_LOOKAHEAD_TOKEN)
    {
        assert(!m_is_new_lookahead_token_required);
        ThrowAwayToken(m_lookahead_token);
        m_is_new_lookahead_token_required = true;
    }

    return ARC_CONTINUE_PARSING;
}

void Parser::ShiftLookaheadToken ()
{
    assert(m_lookahead_token_type != Token::DEFAULT_);
    assert(m_lookahead_token_type != Token::INVALID_);
    DEBUG_SPEW_1("*** shifting lookahead token -- type " << m_lookahead_token_type << std::endl);
    m_token_stack.push_back(m_lookahead_token);
    m_is_new_lookahead_token_required = true;
}

void Parser::PushState (StateNumber const state_number)
{
    assert(state_number < ms_state_count);

    DEBUG_SPEW_1("*** going to state " << state_number << std::endl);
    m_state_stack.push_back(state_number);
    PrintStateStack();
}

void Parser::ReduceUsingRule (ReductionRule const &reduction_rule, bool and_accept)
{
    if (and_accept)
    {
        assert(reduction_rule.m_number_of_tokens_to_reduce_by == m_state_stack.size() - 1);
        assert(reduction_rule.m_number_of_tokens_to_reduce_by == m_token_stack.size());
    }
    else
    {
        assert(reduction_rule.m_number_of_tokens_to_reduce_by < m_state_stack.size());
        assert(reduction_rule.m_number_of_tokens_to_reduce_by <= m_token_stack.size());
    }

    DEBUG_SPEW_1("*** reducing: " << reduction_rule.m_description << std::endl);

    m_is_returning_with_non_terminal = true;
    m_returning_with_this_non_terminal = reduction_rule.m_non_terminal_to_reduce_to;
    m_reduction_rule_token_count = reduction_rule.m_number_of_tokens_to_reduce_by;

    // call the reduction rule handler if it exists
    if (reduction_rule.m_handler != NULL)
        m_reduction_token = (this->*(reduction_rule.m_handler))();
    // pop the states and tokens
    PopStates(reduction_rule.m_number_of_tokens_to_reduce_by, false);

    // only push the reduced token if we aren't accepting yet
    if (!and_accept)
    {
        // push the token that resulted from the reduction
        m_token_stack.push_back(m_reduction_token);
        PrintStateStack();
    }
}

void Parser::PopStates (unsigned int number_of_states_to_pop, bool print_state_stack)
{
    assert(number_of_states_to_pop < m_state_stack.size());
    assert(number_of_states_to_pop <= m_token_stack.size());

    while (number_of_states_to_pop-- > 0)
    {
        m_state_stack.pop_back();
        m_token_stack.pop_back();
    }

    if (print_state_stack)
        PrintStateStack();
}

void Parser::PrintStateStack () const
{
    DEBUG_SPEW_2("*** state stack: ");
    for (StateStack::const_iterator it = m_state_stack.begin(),
                                 it_end = m_state_stack.end();
         it != it_end;
         ++it)
    {
        DEBUG_SPEW_2(*it << " ");
    }
    DEBUG_SPEW_2(std::endl);
}

void Parser::PrintStateTransition (unsigned int const state_transition_number) const
{
    assert(state_transition_number < ms_state_transition_count);
    DEBUG_SPEW_2("&&& exercising state transition " << std::setw(4) << std::right << state_transition_number << std::endl);
}

void Parser::ScanANewLookaheadToken ()
{
    assert(!m_is_new_lookahead_token_required);
    m_lookahead_token = 0;
    m_lookahead_token_type = Scan();
    DEBUG_SPEW_1("*** scanned a new lookahead token -- type " << m_lookahead_token_type << std::endl);
}

void Parser::ThrowAwayToken (cl_N token)
{

#line 71 "calculator_parser.trison"


#line 414 "calculator_parser.cpp"
}

void Parser::ThrowAwayTokenStack ()
{
    while (!m_token_stack.empty())
    {
        ThrowAwayToken(m_token_stack.back());
        m_token_stack.pop_back();
    }
}

std::ostream &operator << (std::ostream &stream, Parser::Token::Type token_type)
{
    static std::string const s_token_type_string[] =
    {
        "BAD_TOKEN",
        "FLOAT",
        "MOD",
        "NEWLINE",
        "NUMBER",
        "RESULT",
        "END_",

        "at_least_zero_newlines",
        "command",
        "expression",
        "root",
        "START_",

        "%error",
        "DEFAULT_",
        "INVALID_"
    };
    static unsigned int const s_token_type_string_count =
        sizeof(s_token_type_string) /
        sizeof(std::string);

    unsigned token_type_value = static_cast<unsigned int>(token_type);
    if (token_type_value < 0x20)
        stream << token_type_value;
    else if (token_type_value < 0x7F)
        stream << "'" << static_cast<char>(token_type) << "'";
    else if (token_type_value < 0x100)
        stream << token_type_value;
    else if (token_type_value < 0x100 + s_token_type_string_count)
        stream << s_token_type_string[token_type_value - 0x100];
    else
        stream << token_type_value;

    return stream;
}

// ///////////////////////////////////////////////////////////////////////////
// state machine reduction rule handlers
// ///////////////////////////////////////////////////////////////////////////

// rule 0: %start <- root END_    
cl_N Parser::ReductionRuleHandler0000 ()
{
    assert(0 < m_reduction_rule_token_count);
    return m_token_stack[m_token_stack.size() - m_reduction_rule_token_count];

    return 0;
}

// rule 1: root <-     
cl_N Parser::ReductionRuleHandler0001 ()
{

#line 113 "calculator_parser.trison"

        m_should_print_result = false;
        return 0;
    
#line 489 "calculator_parser.cpp"
    return 0;
}

// rule 2: root <- expression:result    
cl_N Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    cl_N result = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 119 "calculator_parser.trison"

        result = MODULO(result);
        if (m_should_print_result)
            m_last_result = result;
        return result;
    
#line 506 "calculator_parser.cpp"
    return 0;
}

// rule 3: root <- command    
cl_N Parser::ReductionRuleHandler0003 ()
{

#line 127 "calculator_parser.trison"

        m_should_print_result = false;
        return 0;
    
#line 519 "calculator_parser.cpp"
    return 0;
}

// rule 4: expression <- expression:lhs '+' expression:rhs    %left %prec ADDITION
cl_N Parser::ReductionRuleHandler0004 ()
{
    assert(0 < m_reduction_rule_token_count);
    cl_N lhs = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    cl_N rhs = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 135 "calculator_parser.trison"
 return MODULO(lhs + rhs); 
#line 533 "calculator_parser.cpp"
    return 0;
}

// rule 5: expression <- expression:lhs '-' expression:rhs    %left %prec ADDITION
cl_N Parser::ReductionRuleHandler0005 ()
{
    assert(0 < m_reduction_rule_token_count);
    cl_N lhs = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    cl_N rhs = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 137 "calculator_parser.trison"
 return MODULO(lhs - rhs); 
#line 547 "calculator_parser.cpp"
    return 0;
}

// rule 6: expression <- expression:lhs '*' expression:rhs    %left %prec MULTIPLICATION
cl_N Parser::ReductionRuleHandler0006 ()
{
    assert(0 < m_reduction_rule_token_count);
    cl_N lhs = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    cl_N rhs = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 139 "calculator_parser.trison"
 return MODULO(lhs * rhs); 
#line 561 "calculator_parser.cpp"
    return 0;
}

// rule 7: expression <- expression:lhs '/' expression:rhs    %left %prec MULTIPLICATION
cl_N Parser::ReductionRuleHandler0007 ()
{
    assert(0 < m_reduction_rule_token_count);
    cl_N lhs = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    cl_N rhs = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 142 "calculator_parser.trison"

        if (zerop(rhs))
        {
            cerr << "error: divide by zero" << endl;
            m_should_print_result = false;
            return rhs; // arbitrary return value
        }
        else
            return lhs / rhs;
    
#line 584 "calculator_parser.cpp"
    return 0;
}

// rule 8: expression <- '+' expression:exp     %prec UNARY
cl_N Parser::ReductionRuleHandler0008 ()
{
    assert(1 < m_reduction_rule_token_count);
    cl_N exp = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 153 "calculator_parser.trison"
 return MODULO(exp); 
#line 596 "calculator_parser.cpp"
    return 0;
}

// rule 9: expression <- '-' expression:exp     %prec UNARY
cl_N Parser::ReductionRuleHandler0009 ()
{
    assert(1 < m_reduction_rule_token_count);
    cl_N exp = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 155 "calculator_parser.trison"
 return MODULO(-exp); 
#line 608 "calculator_parser.cpp"
    return 0;
}

// rule 10: expression <- expression:base '^' expression:power    %right %prec EXPONENTIATION
cl_N Parser::ReductionRuleHandler0010 ()
{
    assert(0 < m_reduction_rule_token_count);
    cl_N base = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    cl_N power = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 158 "calculator_parser.trison"

        if (zerop(base) && zerop(power))
        {
            cerr << "error: taking 0 to the 0 power" << endl;
            m_should_print_result = false;
            return base; // arbitrary return value
        }
        else
            return MODULO(expt(base, power));
    
#line 631 "calculator_parser.cpp"
    return 0;
}

// rule 11: expression <- '(' expression:exp ')'    
cl_N Parser::ReductionRuleHandler0011 ()
{
    assert(1 < m_reduction_rule_token_count);
    cl_N exp = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 169 "calculator_parser.trison"
 return exp; 
#line 643 "calculator_parser.cpp"
    return 0;
}

// rule 12: expression <- NUMBER:number    
cl_N Parser::ReductionRuleHandler0012 ()
{
    assert(0 < m_reduction_rule_token_count);
    cl_N number = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 173 "calculator_parser.trison"
 return MODULO(number); 
#line 655 "calculator_parser.cpp"
    return 0;
}

// rule 13: expression <- RESULT    
cl_N Parser::ReductionRuleHandler0013 ()
{

#line 175 "calculator_parser.trison"
 return m_last_result; 
#line 665 "calculator_parser.cpp"
    return 0;
}

// rule 14: command <- '\\' MOD NUMBER:number    
cl_N Parser::ReductionRuleHandler0014 ()
{
    assert(2 < m_reduction_rule_token_count);
    cl_N number = static_cast< cl_N >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 181 "calculator_parser.trison"

        if (realpart(number) >= 0)
            m_modulus = realpart(number);
        else
            cerr << "error: invalid modulus (must be non-negative)" << endl;
        return 0;
    
#line 683 "calculator_parser.cpp"
    return 0;
}

// rule 15: command <- '\\' MOD    
cl_N Parser::ReductionRuleHandler0015 ()
{

#line 190 "calculator_parser.trison"

        cout << "current modulus: " << m_modulus << endl;
        return 0;
    
#line 696 "calculator_parser.cpp"
    return 0;
}

// rule 16: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    
cl_N Parser::ReductionRuleHandler0016 ()
{

#line 198 "calculator_parser.trison"
 return 0; 
#line 706 "calculator_parser.cpp"
    return 0;
}

// rule 17: at_least_zero_newlines <-     
cl_N Parser::ReductionRuleHandler0017 ()
{

#line 200 "calculator_parser.trison"
 return 0; 
#line 716 "calculator_parser.cpp"
    return 0;
}



// ///////////////////////////////////////////////////////////////////////////
// reduction rule lookup table
// ///////////////////////////////////////////////////////////////////////////

Parser::ReductionRule const Parser::ms_reduction_rule[] =
{
    {                 Token::START_,  2, &Parser::ReductionRuleHandler0000, "rule 0: %start <- root END_    "},
    {                 Token::root__,  0, &Parser::ReductionRuleHandler0001, "rule 1: root <-     "},
    {                 Token::root__,  1, &Parser::ReductionRuleHandler0002, "rule 2: root <- expression    "},
    {                 Token::root__,  1, &Parser::ReductionRuleHandler0003, "rule 3: root <- command    "},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0004, "rule 4: expression <- expression '+' expression    %left %prec ADDITION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0005, "rule 5: expression <- expression '-' expression    %left %prec ADDITION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0006, "rule 6: expression <- expression '*' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0007, "rule 7: expression <- expression '/' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0008, "rule 8: expression <- '+' expression     %prec UNARY"},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0009, "rule 9: expression <- '-' expression     %prec UNARY"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0010, "rule 10: expression <- expression '^' expression    %right %prec EXPONENTIATION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0011, "rule 11: expression <- '(' expression ')'    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0012, "rule 12: expression <- NUMBER    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0013, "rule 13: expression <- RESULT    "},
    {              Token::command__,  3, &Parser::ReductionRuleHandler0014, "rule 14: command <- '\\' MOD NUMBER    "},
    {              Token::command__,  2, &Parser::ReductionRuleHandler0015, "rule 15: command <- '\\' MOD    "},
    {Token::at_least_zero_newlines__,  2, &Parser::ReductionRuleHandler0016, "rule 16: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    "},
    {Token::at_least_zero_newlines__,  0, &Parser::ReductionRuleHandler0017, "rule 17: at_least_zero_newlines <-     "},

    // special error panic reduction rule
    {                 Token::ERROR_,  1,                                     NULL, "* -> error"}
};

unsigned int const Parser::ms_reduction_rule_count =
    sizeof(Parser::ms_reduction_rule) /
    sizeof(Parser::ReductionRule);

// ///////////////////////////////////////////////////////////////////////////
// state transition lookup table
// ///////////////////////////////////////////////////////////////////////////

Parser::State const Parser::ms_state[] =
{
    {   1,    6,    7,    8,    3}, // state    0
    {   0,    0,   11,    0,    0}, // state    1
    {   0,    0,   12,    0,    0}, // state    2
    {  13,    5,    0,   18,    1}, // state    3
    {  19,    5,    0,   24,    1}, // state    4
    {  25,    5,    0,   30,    1}, // state    5
    {  31,    1,    0,    0,    0}, // state    6
    {  32,    1,    0,    0,    0}, // state    7
    {  33,    5,   38,    0,    0}, // state    8
    {   0,    0,   39,    0,    0}, // state    9
    {  40,    1,   41,    0,    0}, // state   10
    {  42,    1,   43,    0,    0}, // state   11
    {  44,    6,    0,    0,    0}, // state   12
    {  50,    1,   51,    0,    0}, // state   13
    {   0,    0,   52,    0,    0}, // state   14
    {  53,    5,    0,   58,    1}, // state   15
    {  59,    5,    0,   64,    1}, // state   16
    {  65,    5,    0,   70,    1}, // state   17
    {  71,    5,    0,   76,    1}, // state   18
    {  77,    5,    0,   82,    1}, // state   19
    {   0,    0,   83,    0,    0}, // state   20
    {   0,    0,   84,    0,    0}, // state   21
    {  85,    3,   88,    0,    0}, // state   22
    {  89,    3,   92,    0,    0}, // state   23
    {  93,    1,   94,    0,    0}, // state   24
    {  95,    1,   96,    0,    0}, // state   25
    {  97,    1,   98,    0,    0}  // state   26

};

unsigned int const Parser::ms_state_count =
    sizeof(Parser::ms_state) /
    sizeof(Parser::State);

// ///////////////////////////////////////////////////////////////////////////
// state transition table
// ///////////////////////////////////////////////////////////////////////////

Parser::StateTransition const Parser::ms_state_transition[] =
{
    // dummy transition in the NULL transition
    {               Token::INVALID_, {                       TA_COUNT,    0}},

// ///////////////////////////////////////////////////////////////////////////
// state    0
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::NUMBER, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                 Token::RESULT, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,    6}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::root__, {                  TA_PUSH_STATE,    7}},
    {           Token::expression__, {                  TA_PUSH_STATE,    8}},
    {              Token::command__, {                  TA_PUSH_STATE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state    1
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},

// ///////////////////////////////////////////////////////////////////////////
// state    2
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state    3
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::NUMBER, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                 Token::RESULT, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   10}},

// ///////////////////////////////////////////////////////////////////////////
// state    4
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::NUMBER, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                 Token::RESULT, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state    5
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::NUMBER, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                 Token::RESULT, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   12}},

// ///////////////////////////////////////////////////////////////////////////
// state    6
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                    Token::MOD, {        TA_SHIFT_AND_PUSH_STATE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state    7
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::END_, {        TA_SHIFT_AND_PUSH_STATE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state    8
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   19}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    2}},

// ///////////////////////////////////////////////////////////////////////////
// state    9
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    3}},

// ///////////////////////////////////////////////////////////////////////////
// state   10
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   19}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    8}},

// ///////////////////////////////////////////////////////////////////////////
// state   11
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   19}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state   12
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state   13
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::NUMBER, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   14
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {TA_REDUCE_AND_ACCEPT_USING_RULE,    0}},

// ///////////////////////////////////////////////////////////////////////////
// state   15
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::NUMBER, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                 Token::RESULT, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   16
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::NUMBER, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                 Token::RESULT, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   23}},

// ///////////////////////////////////////////////////////////////////////////
// state   17
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::NUMBER, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                 Token::RESULT, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   24}},

// ///////////////////////////////////////////////////////////////////////////
// state   18
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::NUMBER, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                 Token::RESULT, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   25}},

// ///////////////////////////////////////////////////////////////////////////
// state   19
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::NUMBER, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                 Token::RESULT, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   26}},

// ///////////////////////////////////////////////////////////////////////////
// state   20
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state   21
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state   22
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   19}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    4}},

// ///////////////////////////////////////////////////////////////////////////
// state   23
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   19}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    5}},

// ///////////////////////////////////////////////////////////////////////////
// state   24
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   19}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    6}},

// ///////////////////////////////////////////////////////////////////////////
// state   25
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   19}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    7}},

// ///////////////////////////////////////////////////////////////////////////
// state   26
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   19}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}}

};

unsigned int const Parser::ms_state_transition_count =
    sizeof(Parser::ms_state_transition) /
    sizeof(Parser::StateTransition);


#line 74 "calculator_parser.trison"

void Parser::SetInputString (string const &input_string)
{
    if (m_scanner != NULL)
        delete m_scanner;
    m_scanner = new Scanner(input_string);
}

Parser::Token::Type Parser::Scan ()
{
    assert(m_scanner != NULL);
    return m_scanner->Scan(m_lookahead_token);
}

} // end of namespace Calculator

#line 1080 "calculator_parser.cpp"

