#include "barf_preprocessor_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x


#line 53 "barf_preprocessor_parser.trison"

#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_scanner.hpp"

namespace Barf {
namespace Preprocessor {

#line 21 "barf_preprocessor_parser.cpp"

Parser::Parser ()

{

#line 61 "barf_preprocessor_parser.trison"

    m_scanner = new Scanner();

#line 31 "barf_preprocessor_parser.cpp"
    m_debug_spew_level = 0;
    DEBUG_SPEW_2("### number of state transitions = " << ms_state_transition_count << std::endl);
    m_reduction_token = NULL;
}

Parser::~Parser ()
{

#line 65 "barf_preprocessor_parser.trison"

    delete m_scanner;

#line 44 "barf_preprocessor_parser.cpp"
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
    m_lookahead_token = NULL;
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
    m_lookahead_token = NULL;
    m_lookahead_token_type = Scan();
    DEBUG_SPEW_1("*** scanned a new lookahead token -- type " << m_lookahead_token_type << std::endl);
}

void Parser::ThrowAwayToken (Ast::Base * token)
{

#line 69 "barf_preprocessor_parser.trison"

    delete token;

#line 406 "barf_preprocessor_parser.cpp"
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
        "CODE_LINE",
        "CODE_NEWLINE",
        "DECLARE_ARRAY",
        "DECLARE_MAP",
        "DEFINE",
        "DUMP_SYMBOL_TABLE",
        "ELSE",
        "ELSE_IF",
        "END_CODE",
        "END_DEFINE",
        "END_FOR_EACH",
        "END_IF",
        "END_LOOP",
        "ERROR",
        "FATAL_ERROR",
        "FOR_EACH",
        "ID",
        "IF",
        "INCLUDE",
        "INTEGER",
        "IS_DEFINED",
        "KEYWORD_INT",
        "KEYWORD_STRING",
        "LOOP",
        "SANDBOX_INCLUDE",
        "SIZEOF",
        "START_CODE",
        "STRING",
        "STRING_LENGTH",
        "TEXT",
        "UNDEFINE",
        "WARNING",
        "END_",

        "body",
        "code",
        "conditional_series",
        "conditional_series_end",
        "define",
        "define_array_element",
        "define_map_element",
        "define_scalar",
        "else_if_statement",
        "else_statement",
        "end_define",
        "end_for_each",
        "end_if",
        "end_loop",
        "executable",
        "expression",
        "for_each",
        "if_statement",
        "loop",
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

// rule 0: %start <- body END_    
Ast::Base * Parser::ReductionRuleHandler0000 ()
{
    assert(0 < m_reduction_rule_token_count);
    return m_token_stack[m_token_stack.size() - m_reduction_rule_token_count];

}

// rule 1: body <-     
Ast::Base * Parser::ReductionRuleHandler0001 ()
{

#line 146 "barf_preprocessor_parser.trison"

        return new Body();
    
#line 521 "barf_preprocessor_parser.cpp"
}

// rule 2: body <- TEXT:text    
Ast::Base * Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    Text * text = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 151 "barf_preprocessor_parser.trison"

        Body *body = new Body();
        body->Append(text);
        return body;
    
#line 536 "barf_preprocessor_parser.cpp"
}

// rule 3: body <- body:body executable:executable    
Ast::Base * Parser::ReductionRuleHandler0003 ()
{
    assert(0 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    ExecutableAst * executable = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 158 "barf_preprocessor_parser.trison"

        if (executable != NULL)
            body->Append(executable);
        return body;
    
#line 553 "barf_preprocessor_parser.cpp"
}

// rule 4: body <- body:body executable:executable TEXT:text    
Ast::Base * Parser::ReductionRuleHandler0004 ()
{
    assert(0 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    ExecutableAst * executable = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Text * text = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 165 "barf_preprocessor_parser.trison"

        if (executable != NULL)
            body->Append(executable);
        body->Append(text);
        return body;
    
#line 573 "barf_preprocessor_parser.cpp"
}

// rule 5: executable <- START_CODE code:code END_CODE    
Ast::Base * Parser::ReductionRuleHandler0005 ()
{
    assert(1 < m_reduction_rule_token_count);
    ExecutableAst * code = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 175 "barf_preprocessor_parser.trison"
 return code; 
#line 584 "barf_preprocessor_parser.cpp"
}

// rule 6: executable <- CODE_LINE code:code CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0006 ()
{
    assert(1 < m_reduction_rule_token_count);
    ExecutableAst * code = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 177 "barf_preprocessor_parser.trison"
 return code; 
#line 595 "barf_preprocessor_parser.cpp"
}

// rule 7: executable <- conditional_series:conditional    
Ast::Base * Parser::ReductionRuleHandler0007 ()
{
    assert(0 < m_reduction_rule_token_count);
    Conditional * conditional = Dsc< Conditional * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 179 "barf_preprocessor_parser.trison"
 return conditional; 
#line 606 "barf_preprocessor_parser.cpp"
}

// rule 8: executable <- define:define body:body end_define    
Ast::Base * Parser::ReductionRuleHandler0008 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 182 "barf_preprocessor_parser.trison"

        define->SetBody(body);
        return define;
    
#line 622 "barf_preprocessor_parser.cpp"
}

// rule 9: executable <- loop:loop body:body end_loop    
Ast::Base * Parser::ReductionRuleHandler0009 ()
{
    assert(0 < m_reduction_rule_token_count);
    Loop * loop = Dsc< Loop * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 188 "barf_preprocessor_parser.trison"

        loop->SetBody(body);
        return loop;
    
#line 638 "barf_preprocessor_parser.cpp"
}

// rule 10: executable <- for_each:for_each body:body end_for_each    
Ast::Base * Parser::ReductionRuleHandler0010 ()
{
    assert(0 < m_reduction_rule_token_count);
    ForEach * for_each = Dsc< ForEach * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 194 "barf_preprocessor_parser.trison"

        for_each->SetBody(body);
        return for_each;
    
#line 654 "barf_preprocessor_parser.cpp"
}

// rule 11: code <-     
Ast::Base * Parser::ReductionRuleHandler0011 ()
{

#line 203 "barf_preprocessor_parser.trison"
 return NULL; 
#line 663 "barf_preprocessor_parser.cpp"
}

// rule 12: code <- expression:expression    
Ast::Base * Parser::ReductionRuleHandler0012 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 206 "barf_preprocessor_parser.trison"
 return expression; 
#line 674 "barf_preprocessor_parser.cpp"
}

// rule 13: code <- DUMP_SYMBOL_TABLE '(' ')'    
Ast::Base * Parser::ReductionRuleHandler0013 ()
{

#line 209 "barf_preprocessor_parser.trison"
 return new DumpSymbolTable(); 
#line 683 "barf_preprocessor_parser.cpp"
}

// rule 14: code <- UNDEFINE '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0014 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 212 "barf_preprocessor_parser.trison"
 return new Undefine(id); 
#line 694 "barf_preprocessor_parser.cpp"
}

// rule 15: code <- DECLARE_ARRAY '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0015 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 215 "barf_preprocessor_parser.trison"
 return new DeclareArray(id); 
#line 705 "barf_preprocessor_parser.cpp"
}

// rule 16: code <- DECLARE_MAP '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0016 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 218 "barf_preprocessor_parser.trison"
 return new DeclareMap(id); 
#line 716 "barf_preprocessor_parser.cpp"
}

// rule 17: code <- INCLUDE '(' expression:include_filename_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0017 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * include_filename_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 221 "barf_preprocessor_parser.trison"
 return new Include(include_filename_expression, false); 
#line 727 "barf_preprocessor_parser.cpp"
}

// rule 18: code <- SANDBOX_INCLUDE '(' expression:include_filename_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0018 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * include_filename_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 224 "barf_preprocessor_parser.trison"
 return new Include(include_filename_expression, true); 
#line 738 "barf_preprocessor_parser.cpp"
}

// rule 19: code <- WARNING '(' expression:message_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0019 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * message_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 227 "barf_preprocessor_parser.trison"
 return new Message(message_expression, Message::WARNING); 
#line 749 "barf_preprocessor_parser.cpp"
}

// rule 20: code <- ERROR '(' expression:message_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0020 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * message_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 230 "barf_preprocessor_parser.trison"
 return new Message(message_expression, Message::ERROR); 
#line 760 "barf_preprocessor_parser.cpp"
}

// rule 21: code <- FATAL_ERROR '(' expression:message_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0021 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * message_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 233 "barf_preprocessor_parser.trison"
 return new Message(message_expression, Message::FATAL_ERROR); 
#line 771 "barf_preprocessor_parser.cpp"
}

// rule 22: conditional_series <- if_statement:conditional body:if_body conditional_series_end:else_body    
Ast::Base * Parser::ReductionRuleHandler0022 ()
{
    assert(0 < m_reduction_rule_token_count);
    Conditional * conditional = Dsc< Conditional * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * if_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Body * else_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 239 "barf_preprocessor_parser.trison"

        conditional->SetIfBody(if_body);
        conditional->SetElseBody(else_body);
        return conditional;
    
#line 790 "barf_preprocessor_parser.cpp"
}

// rule 23: conditional_series_end <- end_if    
Ast::Base * Parser::ReductionRuleHandler0023 ()
{

#line 248 "barf_preprocessor_parser.trison"
 return NULL; 
#line 799 "barf_preprocessor_parser.cpp"
}

// rule 24: conditional_series_end <- else_statement body:body end_if    
Ast::Base * Parser::ReductionRuleHandler0024 ()
{
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 250 "barf_preprocessor_parser.trison"
 return body; 
#line 810 "barf_preprocessor_parser.cpp"
}

// rule 25: conditional_series_end <- else_if_statement:conditional body:if_body conditional_series_end:else_body    
Ast::Base * Parser::ReductionRuleHandler0025 ()
{
    assert(0 < m_reduction_rule_token_count);
    Conditional * conditional = Dsc< Conditional * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * if_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Body * else_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 253 "barf_preprocessor_parser.trison"

        conditional->SetIfBody(if_body);
        conditional->SetElseBody(else_body);
        Body *body = new Body();
        body->Append(conditional);
        return body;
    
#line 831 "barf_preprocessor_parser.cpp"
}

// rule 26: if_statement <- START_CODE IF '(' expression:expression ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0026 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 265 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 842 "barf_preprocessor_parser.cpp"
}

// rule 27: if_statement <- CODE_LINE IF '(' expression:expression ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0027 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 268 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 853 "barf_preprocessor_parser.cpp"
}

// rule 28: else_statement <- START_CODE ELSE END_CODE    
Ast::Base * Parser::ReductionRuleHandler0028 ()
{

#line 273 "barf_preprocessor_parser.trison"
 return NULL; 
#line 862 "barf_preprocessor_parser.cpp"
}

// rule 29: else_statement <- CODE_LINE ELSE CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0029 ()
{

#line 275 "barf_preprocessor_parser.trison"
 return NULL; 
#line 871 "barf_preprocessor_parser.cpp"
}

// rule 30: else_if_statement <- START_CODE ELSE_IF '(' expression:expression ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0030 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 281 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 882 "barf_preprocessor_parser.cpp"
}

// rule 31: else_if_statement <- CODE_LINE ELSE_IF '(' expression:expression ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0031 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 284 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 893 "barf_preprocessor_parser.cpp"
}

// rule 32: end_if <- START_CODE END_IF END_CODE    
Ast::Base * Parser::ReductionRuleHandler0032 ()
{

#line 289 "barf_preprocessor_parser.trison"
 return NULL; 
#line 902 "barf_preprocessor_parser.cpp"
}

// rule 33: end_if <- CODE_LINE END_IF CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0033 ()
{

#line 291 "barf_preprocessor_parser.trison"
 return NULL; 
#line 911 "barf_preprocessor_parser.cpp"
}

// rule 34: define <- define_scalar:define    
Ast::Base * Parser::ReductionRuleHandler0034 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 296 "barf_preprocessor_parser.trison"
 return define; 
#line 922 "barf_preprocessor_parser.cpp"
}

// rule 35: define <- define_array_element:define    
Ast::Base * Parser::ReductionRuleHandler0035 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 298 "barf_preprocessor_parser.trison"
 return define; 
#line 933 "barf_preprocessor_parser.cpp"
}

// rule 36: define <- define_map_element:define    
Ast::Base * Parser::ReductionRuleHandler0036 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 300 "barf_preprocessor_parser.trison"
 return define; 
#line 944 "barf_preprocessor_parser.cpp"
}

// rule 37: define_scalar <- START_CODE DEFINE '(' ID:id ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0037 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 306 "barf_preprocessor_parser.trison"
 return new Define(id); 
#line 955 "barf_preprocessor_parser.cpp"
}

// rule 38: define_scalar <- CODE_LINE DEFINE '(' ID:id ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0038 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 309 "barf_preprocessor_parser.trison"
 return new Define(id); 
#line 966 "barf_preprocessor_parser.cpp"
}

// rule 39: define_array_element <- START_CODE DEFINE '(' ID:id '[' ']' ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0039 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 315 "barf_preprocessor_parser.trison"
 return new DefineArrayElement(id); 
#line 977 "barf_preprocessor_parser.cpp"
}

// rule 40: define_array_element <- CODE_LINE DEFINE '(' ID:id '[' ']' ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0040 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 318 "barf_preprocessor_parser.trison"
 return new DefineArrayElement(id); 
#line 988 "barf_preprocessor_parser.cpp"
}

// rule 41: define_map_element <- START_CODE DEFINE '(' ID:id '[' STRING:key ']' ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0041 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Text * key = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 324 "barf_preprocessor_parser.trison"
 return new DefineMapElement(id, key); 
#line 1001 "barf_preprocessor_parser.cpp"
}

// rule 42: define_map_element <- CODE_LINE DEFINE '(' ID:id '[' STRING:key ']' ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0042 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Text * key = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 327 "barf_preprocessor_parser.trison"
 return new DefineMapElement(id, key); 
#line 1014 "barf_preprocessor_parser.cpp"
}

// rule 43: end_define <- START_CODE END_DEFINE END_CODE    
Ast::Base * Parser::ReductionRuleHandler0043 ()
{

#line 332 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1023 "barf_preprocessor_parser.cpp"
}

// rule 44: end_define <- CODE_LINE END_DEFINE CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0044 ()
{

#line 334 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1032 "barf_preprocessor_parser.cpp"
}

// rule 45: loop <- START_CODE LOOP '(' ID:iterator_id ',' expression:iteration_count_expression ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0045 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * iterator_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Expression * iteration_count_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 340 "barf_preprocessor_parser.trison"
 return new Loop(iterator_id, iteration_count_expression); 
#line 1045 "barf_preprocessor_parser.cpp"
}

// rule 46: loop <- CODE_LINE LOOP '(' ID:iterator_id ',' expression:iteration_count_expression ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0046 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * iterator_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Expression * iteration_count_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 343 "barf_preprocessor_parser.trison"
 return new Loop(iterator_id, iteration_count_expression); 
#line 1058 "barf_preprocessor_parser.cpp"
}

// rule 47: end_loop <- START_CODE END_LOOP END_CODE    
Ast::Base * Parser::ReductionRuleHandler0047 ()
{

#line 348 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1067 "barf_preprocessor_parser.cpp"
}

// rule 48: end_loop <- CODE_LINE END_LOOP CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0048 ()
{

#line 350 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1076 "barf_preprocessor_parser.cpp"
}

// rule 49: for_each <- START_CODE FOR_EACH '(' ID:key_id ',' ID:map_id ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0049 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * key_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Ast::Id * map_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 356 "barf_preprocessor_parser.trison"
 return new ForEach(key_id, map_id); 
#line 1089 "barf_preprocessor_parser.cpp"
}

// rule 50: for_each <- CODE_LINE FOR_EACH '(' ID:key_id ',' ID:map_id ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0050 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * key_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Ast::Id * map_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 359 "barf_preprocessor_parser.trison"
 return new ForEach(key_id, map_id); 
#line 1102 "barf_preprocessor_parser.cpp"
}

// rule 51: end_for_each <- START_CODE END_FOR_EACH END_CODE    
Ast::Base * Parser::ReductionRuleHandler0051 ()
{

#line 364 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1111 "barf_preprocessor_parser.cpp"
}

// rule 52: end_for_each <- CODE_LINE END_FOR_EACH CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0052 ()
{

#line 366 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1120 "barf_preprocessor_parser.cpp"
}

// rule 53: expression <- STRING:str    
Ast::Base * Parser::ReductionRuleHandler0053 ()
{
    assert(0 < m_reduction_rule_token_count);
    Text * str = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 372 "barf_preprocessor_parser.trison"
 return str; 
#line 1131 "barf_preprocessor_parser.cpp"
}

// rule 54: expression <- INTEGER:integer    
Ast::Base * Parser::ReductionRuleHandler0054 ()
{
    assert(0 < m_reduction_rule_token_count);
    Integer * integer = Dsc< Integer * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 375 "barf_preprocessor_parser.trison"
 return integer; 
#line 1142 "barf_preprocessor_parser.cpp"
}

// rule 55: expression <- SIZEOF '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0055 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 378 "barf_preprocessor_parser.trison"
 return new Sizeof(id); 
#line 1153 "barf_preprocessor_parser.cpp"
}

// rule 56: expression <- KEYWORD_INT '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0056 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 381 "barf_preprocessor_parser.trison"
 return new Operation(Operation::INT_CAST, expression); 
#line 1164 "barf_preprocessor_parser.cpp"
}

// rule 57: expression <- KEYWORD_STRING '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0057 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 384 "barf_preprocessor_parser.trison"
 return new Operation(Operation::STRING_CAST, expression); 
#line 1175 "barf_preprocessor_parser.cpp"
}

// rule 58: expression <- STRING_LENGTH '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0058 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 387 "barf_preprocessor_parser.trison"
 return new Operation(Operation::STRING_LENGTH, expression); 
#line 1186 "barf_preprocessor_parser.cpp"
}

// rule 59: expression <- IS_DEFINED '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0059 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 390 "barf_preprocessor_parser.trison"
 return new IsDefined(id, NULL); 
#line 1197 "barf_preprocessor_parser.cpp"
}

// rule 60: expression <- IS_DEFINED '(' ID:id '[' expression:element_index_expression ']' ')'    
Ast::Base * Parser::ReductionRuleHandler0060 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(4 < m_reduction_rule_token_count);
    Expression * element_index_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);

#line 393 "barf_preprocessor_parser.trison"
 return new IsDefined(id, element_index_expression); 
#line 1210 "barf_preprocessor_parser.cpp"
}

// rule 61: expression <- ID:id    
Ast::Base * Parser::ReductionRuleHandler0061 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 396 "barf_preprocessor_parser.trison"
 return new Dereference(id, NULL, DEREFERENCE_ALWAYS); 
#line 1221 "barf_preprocessor_parser.cpp"
}

// rule 62: expression <- ID:id '[' expression:element_index_expression ']'    
Ast::Base * Parser::ReductionRuleHandler0062 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * element_index_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 399 "barf_preprocessor_parser.trison"
 return new Dereference(id, element_index_expression, DEREFERENCE_ALWAYS); 
#line 1234 "barf_preprocessor_parser.cpp"
}

// rule 63: expression <- ID:id '?'    
Ast::Base * Parser::ReductionRuleHandler0063 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 402 "barf_preprocessor_parser.trison"
 return new Dereference(id, NULL, DEREFERENCE_IFF_DEFINED); 
#line 1245 "barf_preprocessor_parser.cpp"
}

// rule 64: expression <- ID:id '[' expression:element_index_expression ']' '?'    
Ast::Base * Parser::ReductionRuleHandler0064 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * element_index_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 405 "barf_preprocessor_parser.trison"
 return new Dereference(id, element_index_expression, DEREFERENCE_IFF_DEFINED); 
#line 1258 "barf_preprocessor_parser.cpp"
}

// rule 65: expression <- expression:left '.' expression:right    %left %prec CONCATENATION
Ast::Base * Parser::ReductionRuleHandler0065 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 408 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::CONCATENATE, right); 
#line 1271 "barf_preprocessor_parser.cpp"
}

// rule 66: expression <- expression:left '|' '|' expression:right     %prec LOGICAL_OR
Ast::Base * Parser::ReductionRuleHandler0066 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 411 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LOGICAL_OR, right); 
#line 1284 "barf_preprocessor_parser.cpp"
}

// rule 67: expression <- expression:left '&' '&' expression:right     %prec LOGICAL_AND
Ast::Base * Parser::ReductionRuleHandler0067 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 414 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LOGICAL_AND, right); 
#line 1297 "barf_preprocessor_parser.cpp"
}

// rule 68: expression <- expression:left '=' '=' expression:right     %prec EQUALITY
Ast::Base * Parser::ReductionRuleHandler0068 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 417 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::EQUAL, right); 
#line 1310 "barf_preprocessor_parser.cpp"
}

// rule 69: expression <- expression:left '!' '=' expression:right     %prec EQUALITY
Ast::Base * Parser::ReductionRuleHandler0069 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 420 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::NOT_EQUAL, right); 
#line 1323 "barf_preprocessor_parser.cpp"
}

// rule 70: expression <- expression:left '<' expression:right    %left %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0070 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 423 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LESS_THAN, right); 
#line 1336 "barf_preprocessor_parser.cpp"
}

// rule 71: expression <- expression:left '<' '=' expression:right     %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0071 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 426 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LESS_THAN_OR_EQUAL, right); 
#line 1349 "barf_preprocessor_parser.cpp"
}

// rule 72: expression <- expression:left '>' expression:right    %left %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0072 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 429 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::GREATER_THAN, right); 
#line 1362 "barf_preprocessor_parser.cpp"
}

// rule 73: expression <- expression:left '>' '=' expression:right     %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0073 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 432 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::GREATER_THAN_OR_EQUAL, right); 
#line 1375 "barf_preprocessor_parser.cpp"
}

// rule 74: expression <- expression:left '+' expression:right    %left %prec ADDITION
Ast::Base * Parser::ReductionRuleHandler0074 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 435 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::PLUS, right); 
#line 1388 "barf_preprocessor_parser.cpp"
}

// rule 75: expression <- expression:left '-' expression:right    %left %prec ADDITION
Ast::Base * Parser::ReductionRuleHandler0075 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 438 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::MINUS, right); 
#line 1401 "barf_preprocessor_parser.cpp"
}

// rule 76: expression <- expression:left '*' expression:right    %left %prec MULTIPLICATION
Ast::Base * Parser::ReductionRuleHandler0076 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 441 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::MULTIPLY, right); 
#line 1414 "barf_preprocessor_parser.cpp"
}

// rule 77: expression <- expression:left '/' expression:right    %left %prec MULTIPLICATION
Ast::Base * Parser::ReductionRuleHandler0077 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 444 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::DIVIDE, right); 
#line 1427 "barf_preprocessor_parser.cpp"
}

// rule 78: expression <- expression:left '%' expression:right    %left %prec MULTIPLICATION
Ast::Base * Parser::ReductionRuleHandler0078 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 447 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::REMAINDER, right); 
#line 1440 "barf_preprocessor_parser.cpp"
}

// rule 79: expression <- '-' expression:expression     %prec UNARY
Ast::Base * Parser::ReductionRuleHandler0079 ()
{
    assert(1 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 450 "barf_preprocessor_parser.trison"
 return new Operation(Operation::MINUS, expression); 
#line 1451 "barf_preprocessor_parser.cpp"
}

// rule 80: expression <- '!' expression:expression     %prec UNARY
Ast::Base * Parser::ReductionRuleHandler0080 ()
{
    assert(1 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 453 "barf_preprocessor_parser.trison"
 return new Operation(Operation::LOGICAL_NOT, expression); 
#line 1462 "barf_preprocessor_parser.cpp"
}

// rule 81: expression <- '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0081 ()
{
    assert(1 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 456 "barf_preprocessor_parser.trison"
 return expression; 
#line 1473 "barf_preprocessor_parser.cpp"
}



// ///////////////////////////////////////////////////////////////////////////
// reduction rule lookup table
// ///////////////////////////////////////////////////////////////////////////

Parser::ReductionRule const Parser::ms_reduction_rule[] =
{
    {                 Token::START_,  2, &Parser::ReductionRuleHandler0000, "rule 0: %start <- body END_    "},
    {                 Token::body__,  0, &Parser::ReductionRuleHandler0001, "rule 1: body <-     "},
    {                 Token::body__,  1, &Parser::ReductionRuleHandler0002, "rule 2: body <- TEXT    "},
    {                 Token::body__,  2, &Parser::ReductionRuleHandler0003, "rule 3: body <- body executable    "},
    {                 Token::body__,  3, &Parser::ReductionRuleHandler0004, "rule 4: body <- body executable TEXT    "},
    {           Token::executable__,  3, &Parser::ReductionRuleHandler0005, "rule 5: executable <- START_CODE code END_CODE    "},
    {           Token::executable__,  3, &Parser::ReductionRuleHandler0006, "rule 6: executable <- CODE_LINE code CODE_NEWLINE    "},
    {           Token::executable__,  1, &Parser::ReductionRuleHandler0007, "rule 7: executable <- conditional_series    "},
    {           Token::executable__,  3, &Parser::ReductionRuleHandler0008, "rule 8: executable <- define body end_define    "},
    {           Token::executable__,  3, &Parser::ReductionRuleHandler0009, "rule 9: executable <- loop body end_loop    "},
    {           Token::executable__,  3, &Parser::ReductionRuleHandler0010, "rule 10: executable <- for_each body end_for_each    "},
    {                 Token::code__,  0, &Parser::ReductionRuleHandler0011, "rule 11: code <-     "},
    {                 Token::code__,  1, &Parser::ReductionRuleHandler0012, "rule 12: code <- expression    "},
    {                 Token::code__,  3, &Parser::ReductionRuleHandler0013, "rule 13: code <- DUMP_SYMBOL_TABLE '(' ')'    "},
    {                 Token::code__,  4, &Parser::ReductionRuleHandler0014, "rule 14: code <- UNDEFINE '(' ID ')'    "},
    {                 Token::code__,  4, &Parser::ReductionRuleHandler0015, "rule 15: code <- DECLARE_ARRAY '(' ID ')'    "},
    {                 Token::code__,  4, &Parser::ReductionRuleHandler0016, "rule 16: code <- DECLARE_MAP '(' ID ')'    "},
    {                 Token::code__,  4, &Parser::ReductionRuleHandler0017, "rule 17: code <- INCLUDE '(' expression ')'    "},
    {                 Token::code__,  4, &Parser::ReductionRuleHandler0018, "rule 18: code <- SANDBOX_INCLUDE '(' expression ')'    "},
    {                 Token::code__,  4, &Parser::ReductionRuleHandler0019, "rule 19: code <- WARNING '(' expression ')'    "},
    {                 Token::code__,  4, &Parser::ReductionRuleHandler0020, "rule 20: code <- ERROR '(' expression ')'    "},
    {                 Token::code__,  4, &Parser::ReductionRuleHandler0021, "rule 21: code <- FATAL_ERROR '(' expression ')'    "},
    {   Token::conditional_series__,  3, &Parser::ReductionRuleHandler0022, "rule 22: conditional_series <- if_statement body conditional_series_end    "},
    {Token::conditional_series_end__,  1, &Parser::ReductionRuleHandler0023, "rule 23: conditional_series_end <- end_if    "},
    {Token::conditional_series_end__,  3, &Parser::ReductionRuleHandler0024, "rule 24: conditional_series_end <- else_statement body end_if    "},
    {Token::conditional_series_end__,  3, &Parser::ReductionRuleHandler0025, "rule 25: conditional_series_end <- else_if_statement body conditional_series_end    "},
    {         Token::if_statement__,  6, &Parser::ReductionRuleHandler0026, "rule 26: if_statement <- START_CODE IF '(' expression ')' END_CODE    "},
    {         Token::if_statement__,  6, &Parser::ReductionRuleHandler0027, "rule 27: if_statement <- CODE_LINE IF '(' expression ')' CODE_NEWLINE    "},
    {       Token::else_statement__,  3, &Parser::ReductionRuleHandler0028, "rule 28: else_statement <- START_CODE ELSE END_CODE    "},
    {       Token::else_statement__,  3, &Parser::ReductionRuleHandler0029, "rule 29: else_statement <- CODE_LINE ELSE CODE_NEWLINE    "},
    {    Token::else_if_statement__,  6, &Parser::ReductionRuleHandler0030, "rule 30: else_if_statement <- START_CODE ELSE_IF '(' expression ')' END_CODE    "},
    {    Token::else_if_statement__,  6, &Parser::ReductionRuleHandler0031, "rule 31: else_if_statement <- CODE_LINE ELSE_IF '(' expression ')' CODE_NEWLINE    "},
    {               Token::end_if__,  3, &Parser::ReductionRuleHandler0032, "rule 32: end_if <- START_CODE END_IF END_CODE    "},
    {               Token::end_if__,  3, &Parser::ReductionRuleHandler0033, "rule 33: end_if <- CODE_LINE END_IF CODE_NEWLINE    "},
    {               Token::define__,  1, &Parser::ReductionRuleHandler0034, "rule 34: define <- define_scalar    "},
    {               Token::define__,  1, &Parser::ReductionRuleHandler0035, "rule 35: define <- define_array_element    "},
    {               Token::define__,  1, &Parser::ReductionRuleHandler0036, "rule 36: define <- define_map_element    "},
    {        Token::define_scalar__,  6, &Parser::ReductionRuleHandler0037, "rule 37: define_scalar <- START_CODE DEFINE '(' ID ')' END_CODE    "},
    {        Token::define_scalar__,  6, &Parser::ReductionRuleHandler0038, "rule 38: define_scalar <- CODE_LINE DEFINE '(' ID ')' CODE_NEWLINE    "},
    { Token::define_array_element__,  8, &Parser::ReductionRuleHandler0039, "rule 39: define_array_element <- START_CODE DEFINE '(' ID '[' ']' ')' END_CODE    "},
    { Token::define_array_element__,  8, &Parser::ReductionRuleHandler0040, "rule 40: define_array_element <- CODE_LINE DEFINE '(' ID '[' ']' ')' CODE_NEWLINE    "},
    {   Token::define_map_element__,  9, &Parser::ReductionRuleHandler0041, "rule 41: define_map_element <- START_CODE DEFINE '(' ID '[' STRING ']' ')' END_CODE    "},
    {   Token::define_map_element__,  9, &Parser::ReductionRuleHandler0042, "rule 42: define_map_element <- CODE_LINE DEFINE '(' ID '[' STRING ']' ')' CODE_NEWLINE    "},
    {           Token::end_define__,  3, &Parser::ReductionRuleHandler0043, "rule 43: end_define <- START_CODE END_DEFINE END_CODE    "},
    {           Token::end_define__,  3, &Parser::ReductionRuleHandler0044, "rule 44: end_define <- CODE_LINE END_DEFINE CODE_NEWLINE    "},
    {                 Token::loop__,  8, &Parser::ReductionRuleHandler0045, "rule 45: loop <- START_CODE LOOP '(' ID ',' expression ')' END_CODE    "},
    {                 Token::loop__,  8, &Parser::ReductionRuleHandler0046, "rule 46: loop <- CODE_LINE LOOP '(' ID ',' expression ')' CODE_NEWLINE    "},
    {             Token::end_loop__,  3, &Parser::ReductionRuleHandler0047, "rule 47: end_loop <- START_CODE END_LOOP END_CODE    "},
    {             Token::end_loop__,  3, &Parser::ReductionRuleHandler0048, "rule 48: end_loop <- CODE_LINE END_LOOP CODE_NEWLINE    "},
    {             Token::for_each__,  8, &Parser::ReductionRuleHandler0049, "rule 49: for_each <- START_CODE FOR_EACH '(' ID ',' ID ')' END_CODE    "},
    {             Token::for_each__,  8, &Parser::ReductionRuleHandler0050, "rule 50: for_each <- CODE_LINE FOR_EACH '(' ID ',' ID ')' CODE_NEWLINE    "},
    {         Token::end_for_each__,  3, &Parser::ReductionRuleHandler0051, "rule 51: end_for_each <- START_CODE END_FOR_EACH END_CODE    "},
    {         Token::end_for_each__,  3, &Parser::ReductionRuleHandler0052, "rule 52: end_for_each <- CODE_LINE END_FOR_EACH CODE_NEWLINE    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0053, "rule 53: expression <- STRING    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0054, "rule 54: expression <- INTEGER    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0055, "rule 55: expression <- SIZEOF '(' ID ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0056, "rule 56: expression <- KEYWORD_INT '(' expression ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0057, "rule 57: expression <- KEYWORD_STRING '(' expression ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0058, "rule 58: expression <- STRING_LENGTH '(' expression ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0059, "rule 59: expression <- IS_DEFINED '(' ID ')'    "},
    {           Token::expression__,  7, &Parser::ReductionRuleHandler0060, "rule 60: expression <- IS_DEFINED '(' ID '[' expression ']' ')'    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0061, "rule 61: expression <- ID    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0062, "rule 62: expression <- ID '[' expression ']'    "},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0063, "rule 63: expression <- ID '?'    "},
    {           Token::expression__,  5, &Parser::ReductionRuleHandler0064, "rule 64: expression <- ID '[' expression ']' '?'    "},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0065, "rule 65: expression <- expression '.' expression    %left %prec CONCATENATION"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0066, "rule 66: expression <- expression '|' '|' expression     %prec LOGICAL_OR"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0067, "rule 67: expression <- expression '&' '&' expression     %prec LOGICAL_AND"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0068, "rule 68: expression <- expression '=' '=' expression     %prec EQUALITY"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0069, "rule 69: expression <- expression '!' '=' expression     %prec EQUALITY"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0070, "rule 70: expression <- expression '<' expression    %left %prec COMPARISON"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0071, "rule 71: expression <- expression '<' '=' expression     %prec COMPARISON"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0072, "rule 72: expression <- expression '>' expression    %left %prec COMPARISON"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0073, "rule 73: expression <- expression '>' '=' expression     %prec COMPARISON"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0074, "rule 74: expression <- expression '+' expression    %left %prec ADDITION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0075, "rule 75: expression <- expression '-' expression    %left %prec ADDITION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0076, "rule 76: expression <- expression '*' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0077, "rule 77: expression <- expression '/' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0078, "rule 78: expression <- expression '%' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0079, "rule 79: expression <- '-' expression     %prec UNARY"},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0080, "rule 80: expression <- '!' expression     %prec UNARY"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0081, "rule 81: expression <- '(' expression ')'    "},

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
    {   1,    1,    2,    3,    1}, // state    0
    {   0,    0,    4,    0,    0}, // state    1
    {   5,    3,    0,    8,    9}, // state    2
    {   0,    0,   17,    0,    0}, // state    3
    {  18,   24,   42,   43,    2}, // state    4
    {  45,   24,   69,   70,    2}, // state    5
    {  72,    1,   73,    0,    0}, // state    6
    {   0,    0,   74,    0,    0}, // state    7
    {  75,    1,   76,   77,    1}, // state    8
    {  78,    1,   79,   80,    1}, // state    9
    {   0,    0,   81,    0,    0}, // state   10
    {   0,    0,   82,    0,    0}, // state   11
    {   0,    0,   83,    0,    0}, // state   12
    {  84,    1,   85,   86,    1}, // state   13
    {  87,    1,   88,   89,    1}, // state   14
    {  90,    2,   92,    0,    0}, // state   15
    {  93,    1,    0,    0,    0}, // state   16
    {  94,    1,    0,    0,    0}, // state   17
    {  95,    1,    0,    0,    0}, // state   18
    {  96,    1,    0,    0,    0}, // state   19
    {  97,    1,    0,    0,    0}, // state   20
    {  98,    1,    0,    0,    0}, // state   21
    {  99,    1,    0,    0,    0}, // state   22
    { 100,    1,    0,    0,    0}, // state   23
    { 101,    1,    0,    0,    0}, // state   24
    { 102,    1,    0,    0,    0}, // state   25
    { 103,    1,    0,    0,    0}, // state   26
    { 104,    1,    0,    0,    0}, // state   27
    { 105,    1,    0,    0,    0}, // state   28
    { 106,    1,    0,    0,    0}, // state   29
    { 107,    1,    0,    0,    0}, // state   30
    {   0,    0,  108,    0,    0}, // state   31
    {   0,    0,  109,    0,    0}, // state   32
    { 110,    1,    0,    0,    0}, // state   33
    { 111,    1,    0,    0,    0}, // state   34
    { 112,    1,    0,    0,    0}, // state   35
    { 113,   11,    0,  124,    1}, // state   36
    { 125,   11,    0,  136,    1}, // state   37
    { 137,   11,    0,  148,    1}, // state   38
    { 149,    1,    0,    0,    0}, // state   39
    { 150,   12,  162,    0,    0}, // state   40
    { 163,    1,    0,    0,    0}, // state   41
    { 164,    1,    0,    0,    0}, // state   42
    { 165,    1,    0,    0,    0}, // state   43
    { 166,    1,    0,    0,    0}, // state   44
    { 167,    1,    0,    0,    0}, // state   45
    {   0,    0,  168,    0,    0}, // state   46
    { 169,    2,    0,  171,   13}, // state   47
    { 184,    2,    0,  186,   10}, // state   48
    { 196,    2,    0,  198,   10}, // state   49
    { 208,    2,    0,  210,   10}, // state   50
    { 220,   11,    0,  231,    1}, // state   51
    {   0,    0,  232,    0,    0}, // state   52
    { 233,    1,    0,    0,    0}, // state   53
    { 234,   11,    0,  245,    1}, // state   54
    { 246,    1,    0,    0,    0}, // state   55
    { 247,    1,    0,    0,    0}, // state   56
    { 248,    1,    0,    0,    0}, // state   57
    { 249,    1,    0,    0,    0}, // state   58
    { 250,    1,    0,    0,    0}, // state   59
    { 251,    1,    0,    0,    0}, // state   60
    { 252,   11,    0,  263,    1}, // state   61
    { 264,   11,    0,  275,    1}, // state   62
    { 276,   11,    0,  287,    1}, // state   63
    { 288,   11,    0,  299,    1}, // state   64
    { 300,   11,    0,  311,    1}, // state   65
    { 312,    1,    0,    0,    0}, // state   66
    { 313,    1,    0,    0,    0}, // state   67
    { 314,   11,    0,  325,    1}, // state   68
    { 326,   11,    0,  337,    1}, // state   69
    { 338,   11,    0,  349,    1}, // state   70
    { 350,   13,    0,    0,    0}, // state   71
    {   0,    0,  363,    0,    0}, // state   72
    {   0,    0,  364,    0,    0}, // state   73
    {   0,    0,  365,    0,    0}, // state   74
    { 366,   11,    0,  377,    1}, // state   75
    { 378,   11,    0,  389,    1}, // state   76
    { 390,   11,    0,  401,    1}, // state   77
    { 402,   11,    0,  413,    1}, // state   78
    { 414,   11,    0,  425,    1}, // state   79
    { 426,   11,    0,  437,    1}, // state   80
    { 438,    1,    0,    0,    0}, // state   81
    { 439,    1,    0,    0,    0}, // state   82
    { 440,    1,    0,    0,    0}, // state   83
    { 441,    1,    0,    0,    0}, // state   84
    { 442,   12,    0,  454,    1}, // state   85
    { 455,   12,    0,  467,    1}, // state   86
    { 468,   11,    0,  479,    1}, // state   87
    { 480,    1,    0,    0,    0}, // state   88
    { 481,    1,    0,    0,    0}, // state   89
    { 482,    1,    0,    0,    0}, // state   90
    {   0,    0,  483,    0,    0}, // state   91
    { 484,   27,  511,  512,    2}, // state   92
    { 514,   27,  541,  542,    2}, // state   93
    {   0,    0,  544,    0,    0}, // state   94
    { 545,    1,  546,  547,    1}, // state   95
    { 548,    1,  549,  550,    1}, // state   96
    {   0,    0,  551,    0,    0}, // state   97
    { 552,   25,  577,  578,    2}, // state   98
    { 580,   25,  605,  606,    2}, // state   99
    {   0,    0,  608,    0,    0}, // state  100
    { 609,   25,  634,  635,    2}, // state  101
    { 637,   25,  662,  663,    2}, // state  102
    {   0,    0,  665,    0,    0}, // state  103
    { 666,   25,  691,  692,    2}, // state  104
    { 694,   25,  719,  720,    2}, // state  105
    {   0,    0,  722,    0,    0}, // state  106
    { 723,   13,    0,    0,    0}, // state  107
    {   0,    0,  736,    0,    0}, // state  108
    { 737,   13,    0,    0,    0}, // state  109
    { 750,    1,    0,    0,    0}, // state  110
    { 751,    1,    0,    0,    0}, // state  111
    { 752,    1,    0,    0,    0}, // state  112
    { 753,    2,    0,    0,    0}, // state  113
    { 755,    1,    0,    0,    0}, // state  114
    { 756,    1,    0,    0,    0}, // state  115
    { 757,   13,    0,    0,    0}, // state  116
    { 770,   13,    0,    0,    0}, // state  117
    { 783,   13,    0,    0,    0}, // state  118
    { 796,   13,    0,    0,    0}, // state  119
    { 809,   13,    0,    0,    0}, // state  120
    { 822,    1,    0,    0,    0}, // state  121
    { 823,    2,    0,    0,    0}, // state  122
    { 825,   13,    0,    0,    0}, // state  123
    { 838,   13,    0,    0,    0}, // state  124
    { 851,   13,    0,    0,    0}, // state  125
    {   0,    0,  864,    0,    0}, // state  126
    { 865,    5,  870,    0,    0}, // state  127
    { 871,    3,  874,    0,    0}, // state  128
    { 875,    3,  878,    0,    0}, // state  129
    {   0,    0,  879,    0,    0}, // state  130
    {   0,    0,  880,    0,    0}, // state  131
    {   0,    0,  881,    0,    0}, // state  132
    { 882,   11,    0,  893,    1}, // state  133
    { 894,   11,    0,  905,    1}, // state  134
    { 906,   11,    0,  917,    1}, // state  135
    { 918,   11,    0,  929,    1}, // state  136
    { 930,   11,    0,  941,    1}, // state  137
    { 942,    6,  948,    0,    0}, // state  138
    { 949,   11,    0,  960,    1}, // state  139
    { 961,    6,  967,    0,    0}, // state  140
    { 968,   13,    0,    0,    0}, // state  141
    { 981,    2,    0,    0,    0}, // state  142
    { 983,    1,    0,    0,    0}, // state  143
    { 984,    1,    0,    0,    0}, // state  144
    { 985,    1,    0,    0,    0}, // state  145
    { 986,    1,    0,    0,    0}, // state  146
    { 987,    1,    0,    0,    0}, // state  147
    { 988,    1,    0,    0,    0}, // state  148
    { 989,    1,    0,    0,    0}, // state  149
    { 990,    1,    0,    0,    0}, // state  150
    { 991,    2,    0,  993,   10}, // state  151
    {1003,    2,    0, 1005,   13}, // state  152
    {1018,    1,    0,    0,    0}, // state  153
    {1019,    1,    0,    0,    0}, // state  154
    {1020,    1,    0,    0,    0}, // state  155
    {1021,    1,    0,    0,    0}, // state  156
    {1022,    1,    0,    0,    0}, // state  157
    {1023,    1,    0,    0,    0}, // state  158
    {1024,    1, 1025,    0,    0}, // state  159
    {1026,    1,    0,    0,    0}, // state  160
    {   0,    0, 1027,    0,    0}, // state  161
    {   0,    0, 1028,    0,    0}, // state  162
    {   0,    0, 1029,    0,    0}, // state  163
    {1030,    1,    0,    0,    0}, // state  164
    {1031,    2,    0,    0,    0}, // state  165
    {1033,   11,    0, 1044,    1}, // state  166
    {1045,    1,    0,    0,    0}, // state  167
    {   0,    0, 1046,    0,    0}, // state  168
    {   0,    0, 1047,    0,    0}, // state  169
    {   0,    0, 1048,    0,    0}, // state  170
    {   0,    0, 1049,    0,    0}, // state  171
    {   0,    0, 1050,    0,    0}, // state  172
    {   0,    0, 1051,    0,    0}, // state  173
    {   0,    0, 1052,    0,    0}, // state  174
    {1053,   11,    0, 1064,    1}, // state  175
    {   0,    0, 1065,    0,    0}, // state  176
    {   0,    0, 1066,    0,    0}, // state  177
    {   0,    0, 1067,    0,    0}, // state  178
    {1068,   10, 1078,    0,    0}, // state  179
    {1079,   11, 1090,    0,    0}, // state  180
    {1091,   12, 1103,    0,    0}, // state  181
    {1104,   10, 1114,    0,    0}, // state  182
    {1115,    8, 1123,    0,    0}, // state  183
    {1124,    8, 1132,    0,    0}, // state  184
    {1133,    1,    0,    0,    0}, // state  185
    {1134,    1,    0,    0,    0}, // state  186
    {1135,    2,    0,    0,    0}, // state  187
    {1137,   11,    0, 1148,    1}, // state  188
    {1149,    1,    0,    0,    0}, // state  189
    {   0,    0, 1150,    0,    0}, // state  190
    {1151,   11,    0, 1162,    1}, // state  191
    {   0,    0, 1163,    0,    0}, // state  192
    {   0,    0, 1164,    0,    0}, // state  193
    {1165,   11,    0, 1176,    1}, // state  194
    {   0,    0, 1177,    0,    0}, // state  195
    {1178,   25, 1203, 1204,    2}, // state  196
    {1206,   25, 1231, 1232,    2}, // state  197
    {   0,    0, 1234,    0,    0}, // state  198
    {   0,    0, 1235,    0,    0}, // state  199
    {   0,    0, 1236,    0,    0}, // state  200
    {   0,    0, 1237,    0,    0}, // state  201
    {   0,    0, 1238,    0,    0}, // state  202
    {   0,    0, 1239,    0,    0}, // state  203
    {   0,    0, 1240,    0,    0}, // state  204
    {   0,    0, 1241,    0,    0}, // state  205
    {   0,    0, 1242,    0,    0}, // state  206
    {   0,    0, 1243,    0,    0}, // state  207
    {   0,    0, 1244,    0,    0}, // state  208
    {1245,    1,    0,    0,    0}, // state  209
    {1246,    1,    0,    0,    0}, // state  210
    {1247,   13,    0,    0,    0}, // state  211
    {1260,    1,    0,    0,    0}, // state  212
    {1261,   13,    0,    0,    0}, // state  213
    {   0,    0, 1274,    0,    0}, // state  214
    {   0,    0, 1275,    0,    0}, // state  215
    {1276,    1,    0,    0,    0}, // state  216
    {1277,    1,    0,    0,    0}, // state  217
    {1278,   13,    0,    0,    0}, // state  218
    {1291,    1,    0,    0,    0}, // state  219
    {1292,   13,    0,    0,    0}, // state  220
    {1305,   13,    0,    0,    0}, // state  221
    {1318,    1,    0,    0,    0}, // state  222
    {1319,    1,    0,    0,    0}, // state  223
    {1320,    1,    0,    0,    0}, // state  224
    {1321,    1,    0,    0,    0}, // state  225
    {1322,    1,    0,    0,    0}, // state  226
    {1323,    1,    0,    0,    0}, // state  227
    {1324,    1,    0,    0,    0}, // state  228
    {1325,    1,    0,    0,    0}, // state  229
    {1326,    1,    0,    0,    0}, // state  230
    {1327,    1,    0,    0,    0}, // state  231
    {1328,    1,    0,    0,    0}, // state  232
    {1329,    1,    0,    0,    0}, // state  233
    {   0,    0, 1330,    0,    0}, // state  234
    {   0,    0, 1331,    0,    0}, // state  235
    {   0,    0, 1332,    0,    0}, // state  236
    {   0,    0, 1333,    0,    0}, // state  237
    {1334,    1,    0,    0,    0}, // state  238
    {   0,    0, 1335,    0,    0}, // state  239
    {   0,    0, 1336,    0,    0}, // state  240
    {   0,    0, 1337,    0,    0}, // state  241
    {   0,    0, 1338,    0,    0}, // state  242
    {   0,    0, 1339,    0,    0}, // state  243
    {   0,    0, 1340,    0,    0}, // state  244
    {   0,    0, 1341,    0,    0}  // state  245

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
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,    2}},

// ///////////////////////////////////////////////////////////////////////////
// state    1
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    2}},

// ///////////////////////////////////////////////////////////////////////////
// state    2
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::END_, {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    7}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    8}},
    {               Token::define__, {                  TA_PUSH_STATE,    9}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   10}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   11}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   12}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   13}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state    3
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {TA_REDUCE_AND_ACCEPT_USING_RULE,    0}},

// ///////////////////////////////////////////////////////////////////////////
// state    4
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   39}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state    5
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   41}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   45}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state    6
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,   46}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    3}},

// ///////////////////////////////////////////////////////////////////////////
// state    7
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    7}},

// ///////////////////////////////////////////////////////////////////////////
// state    8
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,   47}},

// ///////////////////////////////////////////////////////////////////////////
// state    9
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,   48}},

// ///////////////////////////////////////////////////////////////////////////
// state   10
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state   11
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   35}},

// ///////////////////////////////////////////////////////////////////////////
// state   12
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   36}},

// ///////////////////////////////////////////////////////////////////////////
// state   13
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,   49}},

// ///////////////////////////////////////////////////////////////////////////
// state   14
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,   50}},

// ///////////////////////////////////////////////////////////////////////////
// state   15
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   51}},
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,   52}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state   16
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state   17
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   54}},

// ///////////////////////////////////////////////////////////////////////////
// state   18
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   55}},

// ///////////////////////////////////////////////////////////////////////////
// state   19
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   56}},

// ///////////////////////////////////////////////////////////////////////////
// state   20
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   57}},

// ///////////////////////////////////////////////////////////////////////////
// state   21
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   58}},

// ///////////////////////////////////////////////////////////////////////////
// state   22
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state   23
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   60}},

// ///////////////////////////////////////////////////////////////////////////
// state   24
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state   25
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   62}},

// ///////////////////////////////////////////////////////////////////////////
// state   26
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   63}},

// ///////////////////////////////////////////////////////////////////////////
// state   27
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   64}},

// ///////////////////////////////////////////////////////////////////////////
// state   28
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   65}},

// ///////////////////////////////////////////////////////////////////////////
// state   29
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   66}},

// ///////////////////////////////////////////////////////////////////////////
// state   30
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   67}},

// ///////////////////////////////////////////////////////////////////////////
// state   31
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   54}},

// ///////////////////////////////////////////////////////////////////////////
// state   32
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state   33
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   68}},

// ///////////////////////////////////////////////////////////////////////////
// state   34
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   69}},

// ///////////////////////////////////////////////////////////////////////////
// state   35
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   70}},

// ///////////////////////////////////////////////////////////////////////////
// state   36
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   71}},

// ///////////////////////////////////////////////////////////////////////////
// state   37
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   72}},

// ///////////////////////////////////////////////////////////////////////////
// state   38
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   73}},

// ///////////////////////////////////////////////////////////////////////////
// state   39
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,   74}},

// ///////////////////////////////////////////////////////////////////////////
// state   40
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},

// ///////////////////////////////////////////////////////////////////////////
// state   41
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state   42
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   88}},

// ///////////////////////////////////////////////////////////////////////////
// state   43
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   89}},

// ///////////////////////////////////////////////////////////////////////////
// state   44
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   90}},

// ///////////////////////////////////////////////////////////////////////////
// state   45
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state   46
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    4}},

// ///////////////////////////////////////////////////////////////////////////
// state   47
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,   92}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,   93}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    7}},
    {Token::conditional_series_end__, {                  TA_PUSH_STATE,   94}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    8}},
    {       Token::else_statement__, {                  TA_PUSH_STATE,   95}},
    {    Token::else_if_statement__, {                  TA_PUSH_STATE,   96}},
    {               Token::end_if__, {                  TA_PUSH_STATE,   97}},
    {               Token::define__, {                  TA_PUSH_STATE,    9}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   10}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   11}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   12}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   13}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state   48
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,   98}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,   99}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    7}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    8}},
    {               Token::define__, {                  TA_PUSH_STATE,    9}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   10}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   11}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   12}},
    {           Token::end_define__, {                  TA_PUSH_STATE,  100}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   13}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state   49
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,  101}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  102}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    7}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    8}},
    {               Token::define__, {                  TA_PUSH_STATE,    9}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   10}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   11}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   12}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   13}},
    {             Token::end_loop__, {                  TA_PUSH_STATE,  103}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state   50
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,  104}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  105}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    7}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    8}},
    {               Token::define__, {                  TA_PUSH_STATE,    9}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   10}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   11}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   12}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   13}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   14}},
    {         Token::end_for_each__, {                  TA_PUSH_STATE,  106}},

// ///////////////////////////////////////////////////////////////////////////
// state   51
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  107}},

// ///////////////////////////////////////////////////////////////////////////
// state   52
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   63}},

// ///////////////////////////////////////////////////////////////////////////
// state   53
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  108}},

// ///////////////////////////////////////////////////////////////////////////
// state   54
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  109}},

// ///////////////////////////////////////////////////////////////////////////
// state   55
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  110}},

// ///////////////////////////////////////////////////////////////////////////
// state   56
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  111}},

// ///////////////////////////////////////////////////////////////////////////
// state   57
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  112}},

// ///////////////////////////////////////////////////////////////////////////
// state   58
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  113}},

// ///////////////////////////////////////////////////////////////////////////
// state   59
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  114}},

// ///////////////////////////////////////////////////////////////////////////
// state   60
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  115}},

// ///////////////////////////////////////////////////////////////////////////
// state   61
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  116}},

// ///////////////////////////////////////////////////////////////////////////
// state   62
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  117}},

// ///////////////////////////////////////////////////////////////////////////
// state   63
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  118}},

// ///////////////////////////////////////////////////////////////////////////
// state   64
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  119}},

// ///////////////////////////////////////////////////////////////////////////
// state   65
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  120}},

// ///////////////////////////////////////////////////////////////////////////
// state   66
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  121}},

// ///////////////////////////////////////////////////////////////////////////
// state   67
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  122}},

// ///////////////////////////////////////////////////////////////////////////
// state   68
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  123}},

// ///////////////////////////////////////////////////////////////////////////
// state   69
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  124}},

// ///////////////////////////////////////////////////////////////////////////
// state   70
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  125}},

// ///////////////////////////////////////////////////////////////////////////
// state   71
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  126}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state   72
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   79}},

// ///////////////////////////////////////////////////////////////////////////
// state   73
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   80}},

// ///////////////////////////////////////////////////////////////////////////
// state   74
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    5}},

// ///////////////////////////////////////////////////////////////////////////
// state   75
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  127}},

// ///////////////////////////////////////////////////////////////////////////
// state   76
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  128}},

// ///////////////////////////////////////////////////////////////////////////
// state   77
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  129}},

// ///////////////////////////////////////////////////////////////////////////
// state   78
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  130}},

// ///////////////////////////////////////////////////////////////////////////
// state   79
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  131}},

// ///////////////////////////////////////////////////////////////////////////
// state   80
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  132}},

// ///////////////////////////////////////////////////////////////////////////
// state   81
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  133}},

// ///////////////////////////////////////////////////////////////////////////
// state   82
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,  134}},

// ///////////////////////////////////////////////////////////////////////////
// state   83
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,  135}},

// ///////////////////////////////////////////////////////////////////////////
// state   84
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  136}},

// ///////////////////////////////////////////////////////////////////////////
// state   85
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  137}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  138}},

// ///////////////////////////////////////////////////////////////////////////
// state   86
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  139}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  140}},

// ///////////////////////////////////////////////////////////////////////////
// state   87
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  141}},

// ///////////////////////////////////////////////////////////////////////////
// state   88
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  142}},

// ///////////////////////////////////////////////////////////////////////////
// state   89
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  143}},

// ///////////////////////////////////////////////////////////////////////////
// state   90
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  144}},

// ///////////////////////////////////////////////////////////////////////////
// state   91
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    6}},

// ///////////////////////////////////////////////////////////////////////////
// state   92
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                   Token::ELSE, {        TA_SHIFT_AND_PUSH_STATE,  145}},
    {                Token::ELSE_IF, {        TA_SHIFT_AND_PUSH_STATE,  146}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  147}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   39}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state   93
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   41}},
    {                   Token::ELSE, {        TA_SHIFT_AND_PUSH_STATE,  148}},
    {                Token::ELSE_IF, {        TA_SHIFT_AND_PUSH_STATE,  149}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  150}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   45}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state   94
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   95
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,  151}},

// ///////////////////////////////////////////////////////////////////////////
// state   96
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,  152}},

// ///////////////////////////////////////////////////////////////////////////
// state   97
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   23}},

// ///////////////////////////////////////////////////////////////////////////
// state   98
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {             Token::END_DEFINE, {        TA_SHIFT_AND_PUSH_STATE,  153}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   39}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state   99
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   41}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {             Token::END_DEFINE, {        TA_SHIFT_AND_PUSH_STATE,  154}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   45}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state  100
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    8}},

// ///////////////////////////////////////////////////////////////////////////
// state  101
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {               Token::END_LOOP, {        TA_SHIFT_AND_PUSH_STATE,  155}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   39}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state  102
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   41}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {               Token::END_LOOP, {        TA_SHIFT_AND_PUSH_STATE,  156}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   45}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state  103
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state  104
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {           Token::END_FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,  157}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   39}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state  105
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   41}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {           Token::END_FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,  158}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   45}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state  106
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}},

// ///////////////////////////////////////////////////////////////////////////
// state  107
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  159}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  108
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state  109
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  160}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  110
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  161}},

// ///////////////////////////////////////////////////////////////////////////
// state  111
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  162}},

// ///////////////////////////////////////////////////////////////////////////
// state  112
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  163}},

// ///////////////////////////////////////////////////////////////////////////
// state  113
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  164}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,  165}},

// ///////////////////////////////////////////////////////////////////////////
// state  114
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  166}},

// ///////////////////////////////////////////////////////////////////////////
// state  115
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  167}},

// ///////////////////////////////////////////////////////////////////////////
// state  116
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  168}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  117
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  169}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  118
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  170}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  119
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  171}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  120
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  172}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  121
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  173}},

// ///////////////////////////////////////////////////////////////////////////
// state  122
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  174}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,  175}},

// ///////////////////////////////////////////////////////////////////////////
// state  123
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  176}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  124
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  177}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  125
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  178}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  126
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   81}},

// ///////////////////////////////////////////////////////////////////////////
// state  127
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   65}},

// ///////////////////////////////////////////////////////////////////////////
// state  128
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   74}},

// ///////////////////////////////////////////////////////////////////////////
// state  129
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   75}},

// ///////////////////////////////////////////////////////////////////////////
// state  130
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   76}},

// ///////////////////////////////////////////////////////////////////////////
// state  131
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   77}},

// ///////////////////////////////////////////////////////////////////////////
// state  132
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state  133
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  179}},

// ///////////////////////////////////////////////////////////////////////////
// state  134
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  180}},

// ///////////////////////////////////////////////////////////////////////////
// state  135
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  181}},

// ///////////////////////////////////////////////////////////////////////////
// state  136
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  182}},

// ///////////////////////////////////////////////////////////////////////////
// state  137
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  183}},

// ///////////////////////////////////////////////////////////////////////////
// state  138
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   70}},

// ///////////////////////////////////////////////////////////////////////////
// state  139
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  184}},

// ///////////////////////////////////////////////////////////////////////////
// state  140
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   72}},

// ///////////////////////////////////////////////////////////////////////////
// state  141
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  185}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  142
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  186}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,  187}},

// ///////////////////////////////////////////////////////////////////////////
// state  143
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  188}},

// ///////////////////////////////////////////////////////////////////////////
// state  144
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  189}},

// ///////////////////////////////////////////////////////////////////////////
// state  145
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  190}},

// ///////////////////////////////////////////////////////////////////////////
// state  146
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,  191}},

// ///////////////////////////////////////////////////////////////////////////
// state  147
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  192}},

// ///////////////////////////////////////////////////////////////////////////
// state  148
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  193}},

// ///////////////////////////////////////////////////////////////////////////
// state  149
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,  194}},

// ///////////////////////////////////////////////////////////////////////////
// state  150
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  195}},

// ///////////////////////////////////////////////////////////////////////////
// state  151
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,  196}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  197}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    7}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    8}},
    {               Token::end_if__, {                  TA_PUSH_STATE,  198}},
    {               Token::define__, {                  TA_PUSH_STATE,    9}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   10}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   11}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   12}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   13}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state  152
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,   92}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,   93}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    7}},
    {Token::conditional_series_end__, {                  TA_PUSH_STATE,  199}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    8}},
    {       Token::else_statement__, {                  TA_PUSH_STATE,   95}},
    {    Token::else_if_statement__, {                  TA_PUSH_STATE,   96}},
    {               Token::end_if__, {                  TA_PUSH_STATE,   97}},
    {               Token::define__, {                  TA_PUSH_STATE,    9}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   10}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   11}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   12}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   13}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state  153
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  200}},

// ///////////////////////////////////////////////////////////////////////////
// state  154
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  201}},

// ///////////////////////////////////////////////////////////////////////////
// state  155
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  202}},

// ///////////////////////////////////////////////////////////////////////////
// state  156
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  203}},

// ///////////////////////////////////////////////////////////////////////////
// state  157
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  204}},

// ///////////////////////////////////////////////////////////////////////////
// state  158
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  205}},

// ///////////////////////////////////////////////////////////////////////////
// state  159
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,  206}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   62}},

// ///////////////////////////////////////////////////////////////////////////
// state  160
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  207}},

// ///////////////////////////////////////////////////////////////////////////
// state  161
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state  162
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state  163
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state  164
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  208}},

// ///////////////////////////////////////////////////////////////////////////
// state  165
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,  209}},
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  210}},

// ///////////////////////////////////////////////////////////////////////////
// state  166
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  211}},

// ///////////////////////////////////////////////////////////////////////////
// state  167
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  212}},

// ///////////////////////////////////////////////////////////////////////////
// state  168
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   17}},

// ///////////////////////////////////////////////////////////////////////////
// state  169
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   18}},

// ///////////////////////////////////////////////////////////////////////////
// state  170
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   19}},

// ///////////////////////////////////////////////////////////////////////////
// state  171
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state  172
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   21}},

// ///////////////////////////////////////////////////////////////////////////
// state  173
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   55}},

// ///////////////////////////////////////////////////////////////////////////
// state  174
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state  175
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  213}},

// ///////////////////////////////////////////////////////////////////////////
// state  176
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   56}},

// ///////////////////////////////////////////////////////////////////////////
// state  177
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   57}},

// ///////////////////////////////////////////////////////////////////////////
// state  178
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   58}},

// ///////////////////////////////////////////////////////////////////////////
// state  179
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   69}},

// ///////////////////////////////////////////////////////////////////////////
// state  180
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   67}},

// ///////////////////////////////////////////////////////////////////////////
// state  181
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   66}},

// ///////////////////////////////////////////////////////////////////////////
// state  182
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   68}},

// ///////////////////////////////////////////////////////////////////////////
// state  183
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   71}},

// ///////////////////////////////////////////////////////////////////////////
// state  184
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   73}},

// ///////////////////////////////////////////////////////////////////////////
// state  185
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  214}},

// ///////////////////////////////////////////////////////////////////////////
// state  186
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  215}},

// ///////////////////////////////////////////////////////////////////////////
// state  187
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,  216}},
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  217}},

// ///////////////////////////////////////////////////////////////////////////
// state  188
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  218}},

// ///////////////////////////////////////////////////////////////////////////
// state  189
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  219}},

// ///////////////////////////////////////////////////////////////////////////
// state  190
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state  191
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  220}},

// ///////////////////////////////////////////////////////////////////////////
// state  192
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   32}},

// ///////////////////////////////////////////////////////////////////////////
// state  193
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   29}},

// ///////////////////////////////////////////////////////////////////////////
// state  194
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  221}},

// ///////////////////////////////////////////////////////////////////////////
// state  195
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   33}},

// ///////////////////////////////////////////////////////////////////////////
// state  196
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  147}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   39}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state  197
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   41}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  150}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                Token::INTEGER, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},
    // nonterminal transitions
    {                 Token::code__, {                  TA_PUSH_STATE,   45}},
    {           Token::expression__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state  198
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   24}},

// ///////////////////////////////////////////////////////////////////////////
// state  199
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   25}},

// ///////////////////////////////////////////////////////////////////////////
// state  200
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state  201
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state  202
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   47}},

// ///////////////////////////////////////////////////////////////////////////
// state  203
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   48}},

// ///////////////////////////////////////////////////////////////////////////
// state  204
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state  205
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},

// ///////////////////////////////////////////////////////////////////////////
// state  206
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   64}},

// ///////////////////////////////////////////////////////////////////////////
// state  207
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   26}},

// ///////////////////////////////////////////////////////////////////////////
// state  208
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state  209
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  222}},

// ///////////////////////////////////////////////////////////////////////////
// state  210
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  223}},

// ///////////////////////////////////////////////////////////////////////////
// state  211
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  224}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  212
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  225}},

// ///////////////////////////////////////////////////////////////////////////
// state  213
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  226}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  214
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   27}},

// ///////////////////////////////////////////////////////////////////////////
// state  215
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   38}},

// ///////////////////////////////////////////////////////////////////////////
// state  216
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  227}},

// ///////////////////////////////////////////////////////////////////////////
// state  217
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  228}},

// ///////////////////////////////////////////////////////////////////////////
// state  218
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  229}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  219
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  230}},

// ///////////////////////////////////////////////////////////////////////////
// state  220
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  231}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  221
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  232}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   86}},

// ///////////////////////////////////////////////////////////////////////////
// state  222
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  233}},

// ///////////////////////////////////////////////////////////////////////////
// state  223
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  234}},

// ///////////////////////////////////////////////////////////////////////////
// state  224
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  235}},

// ///////////////////////////////////////////////////////////////////////////
// state  225
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  236}},

// ///////////////////////////////////////////////////////////////////////////
// state  226
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  237}},

// ///////////////////////////////////////////////////////////////////////////
// state  227
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  238}},

// ///////////////////////////////////////////////////////////////////////////
// state  228
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  239}},

// ///////////////////////////////////////////////////////////////////////////
// state  229
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  240}},

// ///////////////////////////////////////////////////////////////////////////
// state  230
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  241}},

// ///////////////////////////////////////////////////////////////////////////
// state  231
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  242}},

// ///////////////////////////////////////////////////////////////////////////
// state  232
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  243}},

// ///////////////////////////////////////////////////////////////////////////
// state  233
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  244}},

// ///////////////////////////////////////////////////////////////////////////
// state  234
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state  235
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   45}},

// ///////////////////////////////////////////////////////////////////////////
// state  236
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   49}},

// ///////////////////////////////////////////////////////////////////////////
// state  237
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   60}},

// ///////////////////////////////////////////////////////////////////////////
// state  238
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  245}},

// ///////////////////////////////////////////////////////////////////////////
// state  239
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state  240
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   46}},

// ///////////////////////////////////////////////////////////////////////////
// state  241
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   50}},

// ///////////////////////////////////////////////////////////////////////////
// state  242
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   30}},

// ///////////////////////////////////////////////////////////////////////////
// state  243
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   31}},

// ///////////////////////////////////////////////////////////////////////////
// state  244
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  245
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   42}}

};

unsigned int const Parser::ms_state_transition_count =
    sizeof(Parser::ms_state_transition) /
    sizeof(Parser::StateTransition);


#line 73 "barf_preprocessor_parser.trison"

bool Parser::OpenFile (string const &input_filename)
{
    assert(m_scanner != NULL);
    return m_scanner->OpenFile(input_filename);
}

void Parser::OpenString (string const &input_string, string const &input_name, bool use_line_numbers)
{
    assert(m_scanner != NULL);
    return m_scanner->OpenString(input_string, input_name, use_line_numbers);
}

void Parser::OpenUsingStream (istream *input_stream, string const &input_name, bool use_line_numbers)
{
    assert(m_scanner != NULL);
    return m_scanner->OpenUsingStream(input_stream, input_name, use_line_numbers);
}

Parser::Token::Type Parser::Scan ()
{
    assert(m_scanner != NULL);
    return m_scanner->Scan(&m_lookahead_token);
}

} // end of namespace Preprocessor
} // end of namespace Barf

#line 4543 "barf_preprocessor_parser.cpp"

