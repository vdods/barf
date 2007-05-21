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
}

Parser::~Parser ()
{

#line 65 "barf_preprocessor_parser.trison"

    delete m_scanner;

#line 43 "barf_preprocessor_parser.cpp"
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

Parser::ParserReturnCode Parser::Parse (Ast::Base * *parsed_tree_root)
{


    ParserReturnCode return_code = PrivateParse(parsed_tree_root);



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

Parser::ParserReturnCode Parser::PrivateParse (Ast::Base * *parsed_tree_root)
{
    assert(parsed_tree_root && "the return-value pointer must be valid");

    m_state_stack.clear();
    m_token_stack.clear();

    m_lookahead_token_type = Token::INVALID_;
    m_lookahead_token = NULL;
    m_is_new_lookahead_token_required = true;
    m_in_error_handling_mode = false;

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
            if (m_in_error_handling_mode && state_transition.m_token_type == Token::ERROR_ && state_transition_token_type != Token::END_ ||
                !m_in_error_handling_mode && state_transition.m_token_type == state_transition_token_type)
            {
                if (state_transition.m_token_type == Token::ERROR_)
                {
                    ThrowAwayToken(m_lookahead_token);
                    m_lookahead_token = NULL;
                    m_lookahead_token_type = Token::ERROR_;
                }

                PrintStateTransition(state_transition_number);
                if (ProcessAction(state_transition.m_action) == ARC_ACCEPT_AND_RETURN)
                {
                    *parsed_tree_root = m_reduction_token;
                    return PRC_SUCCESS;
                }
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
                {
                    *parsed_tree_root = m_reduction_token;
                    return PRC_SUCCESS;
                }
            }
            // otherwise go into error recovery mode
            else
            {
                assert(!m_is_new_lookahead_token_required);
                assert(!m_in_error_handling_mode);

                DEBUG_SPEW_1("!!! error recovery: begin" << std::endl);
                m_in_error_handling_mode = true;

                // pop the stack until we reach an error-handling state, but only
                // if the lookahead token isn't END_ (to prevent an infinite loop).
                while (!GetDoesStateAcceptErrorToken(current_state_number) || m_lookahead_token_type == Token::END_)
                {
                    DEBUG_SPEW_1("!!! error recovery: popping state " << current_state_number << std::endl);
                    assert(m_token_stack.size() + 1 == m_state_stack.size());
                    if (m_token_stack.size() > 0)
                    {
                        ThrowAwayToken(m_token_stack.back());
                        m_token_stack.pop_back();
                    }
                    m_state_stack.pop_back();

                    if (m_state_stack.size() == 0)
                    {
                        DEBUG_SPEW_1("!!! error recovery: unhandled error -- quitting" << std::endl);
                        *parsed_tree_root = NULL;
                        return PRC_UNHANDLED_PARSE_ERROR;
                    }

                    current_state_number = m_state_stack.back();
                }

                DEBUG_SPEW_1("!!! error recovery: found state which accepts %error token" << std::endl);
                PrintStateStack();
            }
        }
    }

    // this should never happen because the above loop is infinite, but we'll do
    // stuff here anyway in case some compiler isn't smart enough to realize it.
    *parsed_tree_root = NULL;
    return PRC_UNHANDLED_PARSE_ERROR;
}

Parser::ActionReturnCode Parser::ProcessAction (Parser::Action const &action)
{
    if (action.m_transition_action == TA_SHIFT_AND_PUSH_STATE)
    {
        m_in_error_handling_mode = false;
        ShiftLookaheadToken();
        PushState(action.m_data);
    }
    else if (action.m_transition_action == TA_PUSH_STATE)
    {
        assert(!m_in_error_handling_mode);
        PushState(action.m_data);
    }
    else if (action.m_transition_action == TA_REDUCE_USING_RULE)
    {
        assert(!m_in_error_handling_mode);
        unsigned int reduction_rule_number = action.m_data;
        assert(reduction_rule_number < ms_reduction_rule_count);
        ReductionRule const &reduction_rule = ms_reduction_rule[reduction_rule_number];
        ReduceUsingRule(reduction_rule, false);
    }
    else if (action.m_transition_action == TA_REDUCE_AND_ACCEPT_USING_RULE)
    {
        assert(!m_in_error_handling_mode);
        unsigned int reduction_rule_number = action.m_data;
        assert(reduction_rule_number < ms_reduction_rule_count);
        ReductionRule const &reduction_rule = ms_reduction_rule[reduction_rule_number];
        ReduceUsingRule(reduction_rule, true);
        DEBUG_SPEW_1("*** accept" << std::endl);
        // everything is done, so just return.
        return ARC_ACCEPT_AND_RETURN;
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
    else
        m_reduction_token = NULL;
    // pop the states and tokens
    PopStates(reduction_rule.m_number_of_tokens_to_reduce_by, false);

    // only push the reduced token if we aren't accepting yet
    if (!and_accept)
    {
        // push the token that resulted from the reduction
        m_token_stack.push_back(m_reduction_token);
        m_reduction_token = NULL;
        PrintStateStack();
    }
}

void Parser::PopStates (unsigned int number_of_states_to_pop, bool print_state_stack)
{
    assert(m_token_stack.size() + 1 == m_state_stack.size());
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
    DEBUG_SPEW_1("*** throwing away token of type " << m_lookahead_token_type << std::endl);


#line 69 "barf_preprocessor_parser.trison"

    delete token;

#line 391 "barf_preprocessor_parser.cpp"
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
        "INTEGER_LITERAL",
        "IS_DEFINED",
        "KEYWORD_INT",
        "KEYWORD_STRING",
        "LOOP",
        "SANDBOX_INCLUDE",
        "SIZEOF",
        "START_CODE",
        "STRING_LENGTH",
        "STRING_LITERAL",
        "TEXT",
        "UNDEFINE",
        "WARNING",
        "END_",

        "body",
        "code",
        "code_body",
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
    
#line 507 "barf_preprocessor_parser.cpp"
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
    
#line 522 "barf_preprocessor_parser.cpp"
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
    
#line 539 "barf_preprocessor_parser.cpp"
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
    
#line 559 "barf_preprocessor_parser.cpp"
}

// rule 5: executable <- code:code    
Ast::Base * Parser::ReductionRuleHandler0005 ()
{
    assert(0 < m_reduction_rule_token_count);
    ExecutableAst * code = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 175 "barf_preprocessor_parser.trison"
 return code; 
#line 570 "barf_preprocessor_parser.cpp"
}

// rule 6: executable <- conditional_series:conditional    
Ast::Base * Parser::ReductionRuleHandler0006 ()
{
    assert(0 < m_reduction_rule_token_count);
    Conditional * conditional = Dsc< Conditional * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 177 "barf_preprocessor_parser.trison"
 return conditional; 
#line 581 "barf_preprocessor_parser.cpp"
}

// rule 7: executable <- define:define body:body end_define    
Ast::Base * Parser::ReductionRuleHandler0007 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 180 "barf_preprocessor_parser.trison"

        define->SetBody(body);
        return define;
    
#line 597 "barf_preprocessor_parser.cpp"
}

// rule 8: executable <- loop:loop body:body end_loop    
Ast::Base * Parser::ReductionRuleHandler0008 ()
{
    assert(0 < m_reduction_rule_token_count);
    Loop * loop = Dsc< Loop * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 186 "barf_preprocessor_parser.trison"

        loop->SetBody(body);
        return loop;
    
#line 613 "barf_preprocessor_parser.cpp"
}

// rule 9: executable <- for_each:for_each body:body end_for_each    
Ast::Base * Parser::ReductionRuleHandler0009 ()
{
    assert(0 < m_reduction_rule_token_count);
    ForEach * for_each = Dsc< ForEach * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 192 "barf_preprocessor_parser.trison"

        for_each->SetBody(body);
        return for_each;
    
#line 629 "barf_preprocessor_parser.cpp"
}

// rule 10: code <- START_CODE code_body:code_body END_CODE    
Ast::Base * Parser::ReductionRuleHandler0010 ()
{
    assert(1 < m_reduction_rule_token_count);
    ExecutableAst * code_body = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 200 "barf_preprocessor_parser.trison"
 return code_body; 
#line 640 "barf_preprocessor_parser.cpp"
}

// rule 11: code <- CODE_LINE code_body:code_body CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0011 ()
{
    assert(1 < m_reduction_rule_token_count);
    ExecutableAst * code_body = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 202 "barf_preprocessor_parser.trison"
 return code_body; 
#line 651 "barf_preprocessor_parser.cpp"
}

// rule 12: code_body <-     
Ast::Base * Parser::ReductionRuleHandler0012 ()
{

#line 208 "barf_preprocessor_parser.trison"
 return NULL; 
#line 660 "barf_preprocessor_parser.cpp"
}

// rule 13: code_body <- expression:expression    
Ast::Base * Parser::ReductionRuleHandler0013 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 211 "barf_preprocessor_parser.trison"
 return expression; 
#line 671 "barf_preprocessor_parser.cpp"
}

// rule 14: code_body <- DUMP_SYMBOL_TABLE '(' ')'    
Ast::Base * Parser::ReductionRuleHandler0014 ()
{

#line 214 "barf_preprocessor_parser.trison"
 return new DumpSymbolTable(); 
#line 680 "barf_preprocessor_parser.cpp"
}

// rule 15: code_body <- UNDEFINE '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0015 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 217 "barf_preprocessor_parser.trison"
 return new Undefine(id); 
#line 691 "barf_preprocessor_parser.cpp"
}

// rule 16: code_body <- DECLARE_ARRAY '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0016 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 220 "barf_preprocessor_parser.trison"
 return new DeclareArray(id); 
#line 702 "barf_preprocessor_parser.cpp"
}

// rule 17: code_body <- DECLARE_MAP '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0017 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 223 "barf_preprocessor_parser.trison"
 return new DeclareMap(id); 
#line 713 "barf_preprocessor_parser.cpp"
}

// rule 18: code_body <- INCLUDE '(' expression:include_filename_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0018 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * include_filename_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 226 "barf_preprocessor_parser.trison"
 return new Include(include_filename_expression, false); 
#line 724 "barf_preprocessor_parser.cpp"
}

// rule 19: code_body <- SANDBOX_INCLUDE '(' expression:include_filename_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0019 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * include_filename_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 229 "barf_preprocessor_parser.trison"
 return new Include(include_filename_expression, true); 
#line 735 "barf_preprocessor_parser.cpp"
}

// rule 20: code_body <- WARNING '(' expression:message_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0020 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * message_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 232 "barf_preprocessor_parser.trison"
 return new Message(message_expression, Message::WARNING); 
#line 746 "barf_preprocessor_parser.cpp"
}

// rule 21: code_body <- ERROR '(' expression:message_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0021 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * message_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 235 "barf_preprocessor_parser.trison"
 return new Message(message_expression, Message::ERROR); 
#line 757 "barf_preprocessor_parser.cpp"
}

// rule 22: code_body <- FATAL_ERROR '(' expression:message_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0022 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * message_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 238 "barf_preprocessor_parser.trison"
 return new Message(message_expression, Message::FATAL_ERROR); 
#line 768 "barf_preprocessor_parser.cpp"
}

// rule 23: conditional_series <- if_statement:conditional body:if_body conditional_series_end:else_body    
Ast::Base * Parser::ReductionRuleHandler0023 ()
{
    assert(0 < m_reduction_rule_token_count);
    Conditional * conditional = Dsc< Conditional * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * if_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Body * else_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 244 "barf_preprocessor_parser.trison"

        conditional->SetIfBody(if_body);
        conditional->SetElseBody(else_body);
        return conditional;
    
#line 787 "barf_preprocessor_parser.cpp"
}

// rule 24: conditional_series_end <- end_if    
Ast::Base * Parser::ReductionRuleHandler0024 ()
{

#line 253 "barf_preprocessor_parser.trison"
 return NULL; 
#line 796 "barf_preprocessor_parser.cpp"
}

// rule 25: conditional_series_end <- else_statement body:body end_if    
Ast::Base * Parser::ReductionRuleHandler0025 ()
{
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 255 "barf_preprocessor_parser.trison"
 return body; 
#line 807 "barf_preprocessor_parser.cpp"
}

// rule 26: conditional_series_end <- else_if_statement:conditional body:if_body conditional_series_end:else_body    
Ast::Base * Parser::ReductionRuleHandler0026 ()
{
    assert(0 < m_reduction_rule_token_count);
    Conditional * conditional = Dsc< Conditional * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * if_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Body * else_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 258 "barf_preprocessor_parser.trison"

        conditional->SetIfBody(if_body);
        conditional->SetElseBody(else_body);
        Body *body = new Body();
        body->Append(conditional);
        return body;
    
#line 828 "barf_preprocessor_parser.cpp"
}

// rule 27: if_statement <- START_CODE IF '(' expression:expression ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0027 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 270 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 839 "barf_preprocessor_parser.cpp"
}

// rule 28: if_statement <- CODE_LINE IF '(' expression:expression ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0028 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 273 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 850 "barf_preprocessor_parser.cpp"
}

// rule 29: else_statement <- START_CODE ELSE END_CODE    
Ast::Base * Parser::ReductionRuleHandler0029 ()
{

#line 278 "barf_preprocessor_parser.trison"
 return NULL; 
#line 859 "barf_preprocessor_parser.cpp"
}

// rule 30: else_statement <- CODE_LINE ELSE CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0030 ()
{

#line 280 "barf_preprocessor_parser.trison"
 return NULL; 
#line 868 "barf_preprocessor_parser.cpp"
}

// rule 31: else_if_statement <- START_CODE ELSE_IF '(' expression:expression ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0031 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 286 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 879 "barf_preprocessor_parser.cpp"
}

// rule 32: else_if_statement <- CODE_LINE ELSE_IF '(' expression:expression ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0032 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 289 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 890 "barf_preprocessor_parser.cpp"
}

// rule 33: end_if <- START_CODE END_IF END_CODE    
Ast::Base * Parser::ReductionRuleHandler0033 ()
{

#line 294 "barf_preprocessor_parser.trison"
 return NULL; 
#line 899 "barf_preprocessor_parser.cpp"
}

// rule 34: end_if <- CODE_LINE END_IF CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0034 ()
{

#line 296 "barf_preprocessor_parser.trison"
 return NULL; 
#line 908 "barf_preprocessor_parser.cpp"
}

// rule 35: define <- define_scalar:define    
Ast::Base * Parser::ReductionRuleHandler0035 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 301 "barf_preprocessor_parser.trison"
 return define; 
#line 919 "barf_preprocessor_parser.cpp"
}

// rule 36: define <- define_array_element:define    
Ast::Base * Parser::ReductionRuleHandler0036 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 303 "barf_preprocessor_parser.trison"
 return define; 
#line 930 "barf_preprocessor_parser.cpp"
}

// rule 37: define <- define_map_element:define    
Ast::Base * Parser::ReductionRuleHandler0037 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 305 "barf_preprocessor_parser.trison"
 return define; 
#line 941 "barf_preprocessor_parser.cpp"
}

// rule 38: define_scalar <- START_CODE DEFINE '(' ID:id ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0038 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 311 "barf_preprocessor_parser.trison"
 return new Define(id); 
#line 952 "barf_preprocessor_parser.cpp"
}

// rule 39: define_scalar <- CODE_LINE DEFINE '(' ID:id ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0039 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 314 "barf_preprocessor_parser.trison"
 return new Define(id); 
#line 963 "barf_preprocessor_parser.cpp"
}

// rule 40: define_array_element <- START_CODE DEFINE '(' ID:id '[' ']' ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0040 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 320 "barf_preprocessor_parser.trison"
 return new DefineArrayElement(id); 
#line 974 "barf_preprocessor_parser.cpp"
}

// rule 41: define_array_element <- CODE_LINE DEFINE '(' ID:id '[' ']' ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0041 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 323 "barf_preprocessor_parser.trison"
 return new DefineArrayElement(id); 
#line 985 "barf_preprocessor_parser.cpp"
}

// rule 42: define_map_element <- START_CODE DEFINE '(' ID:id '[' STRING_LITERAL:key ']' ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0042 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Text * key = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 329 "barf_preprocessor_parser.trison"
 return new DefineMapElement(id, key); 
#line 998 "barf_preprocessor_parser.cpp"
}

// rule 43: define_map_element <- CODE_LINE DEFINE '(' ID:id '[' STRING_LITERAL:key ']' ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0043 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Text * key = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 332 "barf_preprocessor_parser.trison"
 return new DefineMapElement(id, key); 
#line 1011 "barf_preprocessor_parser.cpp"
}

// rule 44: end_define <- START_CODE END_DEFINE END_CODE    
Ast::Base * Parser::ReductionRuleHandler0044 ()
{

#line 337 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1020 "barf_preprocessor_parser.cpp"
}

// rule 45: end_define <- CODE_LINE END_DEFINE CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0045 ()
{

#line 339 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1029 "barf_preprocessor_parser.cpp"
}

// rule 46: loop <- START_CODE LOOP '(' ID:iterator_id ',' expression:iteration_count_expression ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0046 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * iterator_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Expression * iteration_count_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 345 "barf_preprocessor_parser.trison"
 return new Loop(iterator_id, iteration_count_expression); 
#line 1042 "barf_preprocessor_parser.cpp"
}

// rule 47: loop <- CODE_LINE LOOP '(' ID:iterator_id ',' expression:iteration_count_expression ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0047 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * iterator_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Expression * iteration_count_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 348 "barf_preprocessor_parser.trison"
 return new Loop(iterator_id, iteration_count_expression); 
#line 1055 "barf_preprocessor_parser.cpp"
}

// rule 48: end_loop <- START_CODE END_LOOP END_CODE    
Ast::Base * Parser::ReductionRuleHandler0048 ()
{

#line 353 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1064 "barf_preprocessor_parser.cpp"
}

// rule 49: end_loop <- CODE_LINE END_LOOP CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0049 ()
{

#line 355 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1073 "barf_preprocessor_parser.cpp"
}

// rule 50: for_each <- START_CODE FOR_EACH '(' ID:key_id ',' ID:map_id ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0050 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * key_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Ast::Id * map_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 361 "barf_preprocessor_parser.trison"
 return new ForEach(key_id, map_id); 
#line 1086 "barf_preprocessor_parser.cpp"
}

// rule 51: for_each <- CODE_LINE FOR_EACH '(' ID:key_id ',' ID:map_id ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0051 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * key_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Ast::Id * map_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 364 "barf_preprocessor_parser.trison"
 return new ForEach(key_id, map_id); 
#line 1099 "barf_preprocessor_parser.cpp"
}

// rule 52: end_for_each <- START_CODE END_FOR_EACH END_CODE    
Ast::Base * Parser::ReductionRuleHandler0052 ()
{

#line 369 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1108 "barf_preprocessor_parser.cpp"
}

// rule 53: end_for_each <- CODE_LINE END_FOR_EACH CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0053 ()
{

#line 371 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1117 "barf_preprocessor_parser.cpp"
}

// rule 54: expression <- STRING_LITERAL:str    
Ast::Base * Parser::ReductionRuleHandler0054 ()
{
    assert(0 < m_reduction_rule_token_count);
    Text * str = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 377 "barf_preprocessor_parser.trison"
 return str; 
#line 1128 "barf_preprocessor_parser.cpp"
}

// rule 55: expression <- INTEGER_LITERAL:integer    
Ast::Base * Parser::ReductionRuleHandler0055 ()
{
    assert(0 < m_reduction_rule_token_count);
    Integer * integer = Dsc< Integer * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 380 "barf_preprocessor_parser.trison"
 return integer; 
#line 1139 "barf_preprocessor_parser.cpp"
}

// rule 56: expression <- SIZEOF '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0056 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 383 "barf_preprocessor_parser.trison"
 return new Sizeof(id); 
#line 1150 "barf_preprocessor_parser.cpp"
}

// rule 57: expression <- KEYWORD_INT '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0057 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 386 "barf_preprocessor_parser.trison"
 return new Operation(Operation::INT_CAST, expression); 
#line 1161 "barf_preprocessor_parser.cpp"
}

// rule 58: expression <- KEYWORD_STRING '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0058 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 389 "barf_preprocessor_parser.trison"
 return new Operation(Operation::STRING_CAST, expression); 
#line 1172 "barf_preprocessor_parser.cpp"
}

// rule 59: expression <- STRING_LENGTH '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0059 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 392 "barf_preprocessor_parser.trison"
 return new Operation(Operation::STRING_LENGTH, expression); 
#line 1183 "barf_preprocessor_parser.cpp"
}

// rule 60: expression <- IS_DEFINED '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0060 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 395 "barf_preprocessor_parser.trison"
 return new IsDefined(id, NULL); 
#line 1194 "barf_preprocessor_parser.cpp"
}

// rule 61: expression <- IS_DEFINED '(' ID:id '[' expression:element_index_expression ']' ')'    
Ast::Base * Parser::ReductionRuleHandler0061 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(4 < m_reduction_rule_token_count);
    Expression * element_index_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);

#line 398 "barf_preprocessor_parser.trison"
 return new IsDefined(id, element_index_expression); 
#line 1207 "barf_preprocessor_parser.cpp"
}

// rule 62: expression <- ID:id    
Ast::Base * Parser::ReductionRuleHandler0062 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 401 "barf_preprocessor_parser.trison"
 return new Dereference(id, NULL, DEREFERENCE_ALWAYS); 
#line 1218 "barf_preprocessor_parser.cpp"
}

// rule 63: expression <- ID:id '[' expression:element_index_expression ']'    
Ast::Base * Parser::ReductionRuleHandler0063 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * element_index_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 404 "barf_preprocessor_parser.trison"
 return new Dereference(id, element_index_expression, DEREFERENCE_ALWAYS); 
#line 1231 "barf_preprocessor_parser.cpp"
}

// rule 64: expression <- ID:id '?'    
Ast::Base * Parser::ReductionRuleHandler0064 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 407 "barf_preprocessor_parser.trison"
 return new Dereference(id, NULL, DEREFERENCE_IFF_DEFINED); 
#line 1242 "barf_preprocessor_parser.cpp"
}

// rule 65: expression <- ID:id '[' expression:element_index_expression ']' '?'    
Ast::Base * Parser::ReductionRuleHandler0065 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * element_index_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 410 "barf_preprocessor_parser.trison"
 return new Dereference(id, element_index_expression, DEREFERENCE_IFF_DEFINED); 
#line 1255 "barf_preprocessor_parser.cpp"
}

// rule 66: expression <- expression:left '.' expression:right    %left %prec CONCATENATION
Ast::Base * Parser::ReductionRuleHandler0066 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 413 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::CONCATENATE, right); 
#line 1268 "barf_preprocessor_parser.cpp"
}

// rule 67: expression <- expression:left '|' '|' expression:right     %prec LOGICAL_OR
Ast::Base * Parser::ReductionRuleHandler0067 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 416 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LOGICAL_OR, right); 
#line 1281 "barf_preprocessor_parser.cpp"
}

// rule 68: expression <- expression:left '&' '&' expression:right     %prec LOGICAL_AND
Ast::Base * Parser::ReductionRuleHandler0068 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 419 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LOGICAL_AND, right); 
#line 1294 "barf_preprocessor_parser.cpp"
}

// rule 69: expression <- expression:left '=' '=' expression:right     %prec EQUALITY
Ast::Base * Parser::ReductionRuleHandler0069 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 422 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::EQUAL, right); 
#line 1307 "barf_preprocessor_parser.cpp"
}

// rule 70: expression <- expression:left '!' '=' expression:right     %prec EQUALITY
Ast::Base * Parser::ReductionRuleHandler0070 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 425 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::NOT_EQUAL, right); 
#line 1320 "barf_preprocessor_parser.cpp"
}

// rule 71: expression <- expression:left '<' expression:right    %left %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0071 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 428 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LESS_THAN, right); 
#line 1333 "barf_preprocessor_parser.cpp"
}

// rule 72: expression <- expression:left '<' '=' expression:right     %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0072 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 431 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LESS_THAN_OR_EQUAL, right); 
#line 1346 "barf_preprocessor_parser.cpp"
}

// rule 73: expression <- expression:left '>' expression:right    %left %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0073 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 434 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::GREATER_THAN, right); 
#line 1359 "barf_preprocessor_parser.cpp"
}

// rule 74: expression <- expression:left '>' '=' expression:right     %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0074 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 437 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::GREATER_THAN_OR_EQUAL, right); 
#line 1372 "barf_preprocessor_parser.cpp"
}

// rule 75: expression <- expression:left '+' expression:right    %left %prec ADDITION
Ast::Base * Parser::ReductionRuleHandler0075 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 440 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::PLUS, right); 
#line 1385 "barf_preprocessor_parser.cpp"
}

// rule 76: expression <- expression:left '-' expression:right    %left %prec ADDITION
Ast::Base * Parser::ReductionRuleHandler0076 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 443 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::MINUS, right); 
#line 1398 "barf_preprocessor_parser.cpp"
}

// rule 77: expression <- expression:left '*' expression:right    %left %prec MULTIPLICATION
Ast::Base * Parser::ReductionRuleHandler0077 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 446 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::MULTIPLY, right); 
#line 1411 "barf_preprocessor_parser.cpp"
}

// rule 78: expression <- expression:left '/' expression:right    %left %prec MULTIPLICATION
Ast::Base * Parser::ReductionRuleHandler0078 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 449 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::DIVIDE, right); 
#line 1424 "barf_preprocessor_parser.cpp"
}

// rule 79: expression <- expression:left '%' expression:right    %left %prec MULTIPLICATION
Ast::Base * Parser::ReductionRuleHandler0079 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 452 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::REMAINDER, right); 
#line 1437 "barf_preprocessor_parser.cpp"
}

// rule 80: expression <- '-' expression:expression     %prec UNARY
Ast::Base * Parser::ReductionRuleHandler0080 ()
{
    assert(1 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 455 "barf_preprocessor_parser.trison"
 return new Operation(Operation::MINUS, expression); 
#line 1448 "barf_preprocessor_parser.cpp"
}

// rule 81: expression <- '!' expression:expression     %prec UNARY
Ast::Base * Parser::ReductionRuleHandler0081 ()
{
    assert(1 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 458 "barf_preprocessor_parser.trison"
 return new Operation(Operation::LOGICAL_NOT, expression); 
#line 1459 "barf_preprocessor_parser.cpp"
}

// rule 82: expression <- '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0082 ()
{
    assert(1 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 461 "barf_preprocessor_parser.trison"
 return expression; 
#line 1470 "barf_preprocessor_parser.cpp"
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
    {           Token::executable__,  1, &Parser::ReductionRuleHandler0005, "rule 5: executable <- code    "},
    {           Token::executable__,  1, &Parser::ReductionRuleHandler0006, "rule 6: executable <- conditional_series    "},
    {           Token::executable__,  3, &Parser::ReductionRuleHandler0007, "rule 7: executable <- define body end_define    "},
    {           Token::executable__,  3, &Parser::ReductionRuleHandler0008, "rule 8: executable <- loop body end_loop    "},
    {           Token::executable__,  3, &Parser::ReductionRuleHandler0009, "rule 9: executable <- for_each body end_for_each    "},
    {                 Token::code__,  3, &Parser::ReductionRuleHandler0010, "rule 10: code <- START_CODE code_body END_CODE    "},
    {                 Token::code__,  3, &Parser::ReductionRuleHandler0011, "rule 11: code <- CODE_LINE code_body CODE_NEWLINE    "},
    {            Token::code_body__,  0, &Parser::ReductionRuleHandler0012, "rule 12: code_body <-     "},
    {            Token::code_body__,  1, &Parser::ReductionRuleHandler0013, "rule 13: code_body <- expression    "},
    {            Token::code_body__,  3, &Parser::ReductionRuleHandler0014, "rule 14: code_body <- DUMP_SYMBOL_TABLE '(' ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0015, "rule 15: code_body <- UNDEFINE '(' ID ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0016, "rule 16: code_body <- DECLARE_ARRAY '(' ID ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0017, "rule 17: code_body <- DECLARE_MAP '(' ID ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0018, "rule 18: code_body <- INCLUDE '(' expression ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0019, "rule 19: code_body <- SANDBOX_INCLUDE '(' expression ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0020, "rule 20: code_body <- WARNING '(' expression ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0021, "rule 21: code_body <- ERROR '(' expression ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0022, "rule 22: code_body <- FATAL_ERROR '(' expression ')'    "},
    {   Token::conditional_series__,  3, &Parser::ReductionRuleHandler0023, "rule 23: conditional_series <- if_statement body conditional_series_end    "},
    {Token::conditional_series_end__,  1, &Parser::ReductionRuleHandler0024, "rule 24: conditional_series_end <- end_if    "},
    {Token::conditional_series_end__,  3, &Parser::ReductionRuleHandler0025, "rule 25: conditional_series_end <- else_statement body end_if    "},
    {Token::conditional_series_end__,  3, &Parser::ReductionRuleHandler0026, "rule 26: conditional_series_end <- else_if_statement body conditional_series_end    "},
    {         Token::if_statement__,  6, &Parser::ReductionRuleHandler0027, "rule 27: if_statement <- START_CODE IF '(' expression ')' END_CODE    "},
    {         Token::if_statement__,  6, &Parser::ReductionRuleHandler0028, "rule 28: if_statement <- CODE_LINE IF '(' expression ')' CODE_NEWLINE    "},
    {       Token::else_statement__,  3, &Parser::ReductionRuleHandler0029, "rule 29: else_statement <- START_CODE ELSE END_CODE    "},
    {       Token::else_statement__,  3, &Parser::ReductionRuleHandler0030, "rule 30: else_statement <- CODE_LINE ELSE CODE_NEWLINE    "},
    {    Token::else_if_statement__,  6, &Parser::ReductionRuleHandler0031, "rule 31: else_if_statement <- START_CODE ELSE_IF '(' expression ')' END_CODE    "},
    {    Token::else_if_statement__,  6, &Parser::ReductionRuleHandler0032, "rule 32: else_if_statement <- CODE_LINE ELSE_IF '(' expression ')' CODE_NEWLINE    "},
    {               Token::end_if__,  3, &Parser::ReductionRuleHandler0033, "rule 33: end_if <- START_CODE END_IF END_CODE    "},
    {               Token::end_if__,  3, &Parser::ReductionRuleHandler0034, "rule 34: end_if <- CODE_LINE END_IF CODE_NEWLINE    "},
    {               Token::define__,  1, &Parser::ReductionRuleHandler0035, "rule 35: define <- define_scalar    "},
    {               Token::define__,  1, &Parser::ReductionRuleHandler0036, "rule 36: define <- define_array_element    "},
    {               Token::define__,  1, &Parser::ReductionRuleHandler0037, "rule 37: define <- define_map_element    "},
    {        Token::define_scalar__,  6, &Parser::ReductionRuleHandler0038, "rule 38: define_scalar <- START_CODE DEFINE '(' ID ')' END_CODE    "},
    {        Token::define_scalar__,  6, &Parser::ReductionRuleHandler0039, "rule 39: define_scalar <- CODE_LINE DEFINE '(' ID ')' CODE_NEWLINE    "},
    { Token::define_array_element__,  8, &Parser::ReductionRuleHandler0040, "rule 40: define_array_element <- START_CODE DEFINE '(' ID '[' ']' ')' END_CODE    "},
    { Token::define_array_element__,  8, &Parser::ReductionRuleHandler0041, "rule 41: define_array_element <- CODE_LINE DEFINE '(' ID '[' ']' ')' CODE_NEWLINE    "},
    {   Token::define_map_element__,  9, &Parser::ReductionRuleHandler0042, "rule 42: define_map_element <- START_CODE DEFINE '(' ID '[' STRING_LITERAL ']' ')' END_CODE    "},
    {   Token::define_map_element__,  9, &Parser::ReductionRuleHandler0043, "rule 43: define_map_element <- CODE_LINE DEFINE '(' ID '[' STRING_LITERAL ']' ')' CODE_NEWLINE    "},
    {           Token::end_define__,  3, &Parser::ReductionRuleHandler0044, "rule 44: end_define <- START_CODE END_DEFINE END_CODE    "},
    {           Token::end_define__,  3, &Parser::ReductionRuleHandler0045, "rule 45: end_define <- CODE_LINE END_DEFINE CODE_NEWLINE    "},
    {                 Token::loop__,  8, &Parser::ReductionRuleHandler0046, "rule 46: loop <- START_CODE LOOP '(' ID ',' expression ')' END_CODE    "},
    {                 Token::loop__,  8, &Parser::ReductionRuleHandler0047, "rule 47: loop <- CODE_LINE LOOP '(' ID ',' expression ')' CODE_NEWLINE    "},
    {             Token::end_loop__,  3, &Parser::ReductionRuleHandler0048, "rule 48: end_loop <- START_CODE END_LOOP END_CODE    "},
    {             Token::end_loop__,  3, &Parser::ReductionRuleHandler0049, "rule 49: end_loop <- CODE_LINE END_LOOP CODE_NEWLINE    "},
    {             Token::for_each__,  8, &Parser::ReductionRuleHandler0050, "rule 50: for_each <- START_CODE FOR_EACH '(' ID ',' ID ')' END_CODE    "},
    {             Token::for_each__,  8, &Parser::ReductionRuleHandler0051, "rule 51: for_each <- CODE_LINE FOR_EACH '(' ID ',' ID ')' CODE_NEWLINE    "},
    {         Token::end_for_each__,  3, &Parser::ReductionRuleHandler0052, "rule 52: end_for_each <- START_CODE END_FOR_EACH END_CODE    "},
    {         Token::end_for_each__,  3, &Parser::ReductionRuleHandler0053, "rule 53: end_for_each <- CODE_LINE END_FOR_EACH CODE_NEWLINE    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0054, "rule 54: expression <- STRING_LITERAL    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0055, "rule 55: expression <- INTEGER_LITERAL    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0056, "rule 56: expression <- SIZEOF '(' ID ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0057, "rule 57: expression <- KEYWORD_INT '(' expression ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0058, "rule 58: expression <- KEYWORD_STRING '(' expression ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0059, "rule 59: expression <- STRING_LENGTH '(' expression ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0060, "rule 60: expression <- IS_DEFINED '(' ID ')'    "},
    {           Token::expression__,  7, &Parser::ReductionRuleHandler0061, "rule 61: expression <- IS_DEFINED '(' ID '[' expression ']' ')'    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0062, "rule 62: expression <- ID    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0063, "rule 63: expression <- ID '[' expression ']'    "},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0064, "rule 64: expression <- ID '?'    "},
    {           Token::expression__,  5, &Parser::ReductionRuleHandler0065, "rule 65: expression <- ID '[' expression ']' '?'    "},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0066, "rule 66: expression <- expression '.' expression    %left %prec CONCATENATION"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0067, "rule 67: expression <- expression '|' '|' expression     %prec LOGICAL_OR"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0068, "rule 68: expression <- expression '&' '&' expression     %prec LOGICAL_AND"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0069, "rule 69: expression <- expression '=' '=' expression     %prec EQUALITY"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0070, "rule 70: expression <- expression '!' '=' expression     %prec EQUALITY"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0071, "rule 71: expression <- expression '<' expression    %left %prec COMPARISON"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0072, "rule 72: expression <- expression '<' '=' expression     %prec COMPARISON"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0073, "rule 73: expression <- expression '>' expression    %left %prec COMPARISON"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0074, "rule 74: expression <- expression '>' '=' expression     %prec COMPARISON"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0075, "rule 75: expression <- expression '+' expression    %left %prec ADDITION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0076, "rule 76: expression <- expression '-' expression    %left %prec ADDITION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0077, "rule 77: expression <- expression '*' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0078, "rule 78: expression <- expression '/' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0079, "rule 79: expression <- expression '%' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0080, "rule 80: expression <- '-' expression     %prec UNARY"},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0081, "rule 81: expression <- '!' expression     %prec UNARY"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0082, "rule 82: expression <- '(' expression ')'    "},

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
    {   5,    3,    0,    8,   10}, // state    2
    {   0,    0,   18,    0,    0}, // state    3
    {  19,   24,   43,   44,    2}, // state    4
    {  46,   24,   70,   71,    2}, // state    5
    {  73,    1,   74,    0,    0}, // state    6
    {   0,    0,   75,    0,    0}, // state    7
    {   0,    0,   76,    0,    0}, // state    8
    {  77,    1,   78,   79,    1}, // state    9
    {  80,    1,   81,   82,    1}, // state   10
    {   0,    0,   83,    0,    0}, // state   11
    {   0,    0,   84,    0,    0}, // state   12
    {   0,    0,   85,    0,    0}, // state   13
    {  86,    1,   87,   88,    1}, // state   14
    {  89,    1,   90,   91,    1}, // state   15
    {  92,    2,   94,    0,    0}, // state   16
    {  95,    1,    0,    0,    0}, // state   17
    {  96,    1,    0,    0,    0}, // state   18
    {  97,    1,    0,    0,    0}, // state   19
    {  98,    1,    0,    0,    0}, // state   20
    {  99,    1,    0,    0,    0}, // state   21
    { 100,    1,    0,    0,    0}, // state   22
    { 101,    1,    0,    0,    0}, // state   23
    { 102,    1,    0,    0,    0}, // state   24
    { 103,    1,    0,    0,    0}, // state   25
    { 104,    1,    0,    0,    0}, // state   26
    { 105,    1,    0,    0,    0}, // state   27
    { 106,    1,    0,    0,    0}, // state   28
    { 107,    1,    0,    0,    0}, // state   29
    { 108,    1,    0,    0,    0}, // state   30
    { 109,    1,    0,    0,    0}, // state   31
    {   0,    0,  110,    0,    0}, // state   32
    {   0,    0,  111,    0,    0}, // state   33
    { 112,    1,    0,    0,    0}, // state   34
    { 113,    1,    0,    0,    0}, // state   35
    { 114,    1,    0,    0,    0}, // state   36
    { 115,   11,    0,  126,    1}, // state   37
    { 127,   11,    0,  138,    1}, // state   38
    { 139,   11,    0,  150,    1}, // state   39
    { 151,    1,    0,    0,    0}, // state   40
    { 152,   12,  164,    0,    0}, // state   41
    { 165,    1,    0,    0,    0}, // state   42
    { 166,    1,    0,    0,    0}, // state   43
    { 167,    1,    0,    0,    0}, // state   44
    { 168,    1,    0,    0,    0}, // state   45
    { 169,    1,    0,    0,    0}, // state   46
    {   0,    0,  170,    0,    0}, // state   47
    { 171,    2,    0,  173,   14}, // state   48
    { 187,    2,    0,  189,   11}, // state   49
    { 200,    2,    0,  202,   11}, // state   50
    { 213,    2,    0,  215,   11}, // state   51
    { 226,   11,    0,  237,    1}, // state   52
    {   0,    0,  238,    0,    0}, // state   53
    { 239,    1,    0,    0,    0}, // state   54
    { 240,   11,    0,  251,    1}, // state   55
    { 252,    1,    0,    0,    0}, // state   56
    { 253,    1,    0,    0,    0}, // state   57
    { 254,    1,    0,    0,    0}, // state   58
    { 255,    1,    0,    0,    0}, // state   59
    { 256,    1,    0,    0,    0}, // state   60
    { 257,    1,    0,    0,    0}, // state   61
    { 258,   11,    0,  269,    1}, // state   62
    { 270,   11,    0,  281,    1}, // state   63
    { 282,   11,    0,  293,    1}, // state   64
    { 294,   11,    0,  305,    1}, // state   65
    { 306,   11,    0,  317,    1}, // state   66
    { 318,    1,    0,    0,    0}, // state   67
    { 319,    1,    0,    0,    0}, // state   68
    { 320,   11,    0,  331,    1}, // state   69
    { 332,   11,    0,  343,    1}, // state   70
    { 344,   11,    0,  355,    1}, // state   71
    { 356,   13,    0,    0,    0}, // state   72
    {   0,    0,  369,    0,    0}, // state   73
    {   0,    0,  370,    0,    0}, // state   74
    {   0,    0,  371,    0,    0}, // state   75
    { 372,   11,    0,  383,    1}, // state   76
    { 384,   11,    0,  395,    1}, // state   77
    { 396,   11,    0,  407,    1}, // state   78
    { 408,   11,    0,  419,    1}, // state   79
    { 420,   11,    0,  431,    1}, // state   80
    { 432,   11,    0,  443,    1}, // state   81
    { 444,    1,    0,    0,    0}, // state   82
    { 445,    1,    0,    0,    0}, // state   83
    { 446,    1,    0,    0,    0}, // state   84
    { 447,    1,    0,    0,    0}, // state   85
    { 448,   12,    0,  460,    1}, // state   86
    { 461,   12,    0,  473,    1}, // state   87
    { 474,   11,    0,  485,    1}, // state   88
    { 486,    1,    0,    0,    0}, // state   89
    { 487,    1,    0,    0,    0}, // state   90
    { 488,    1,    0,    0,    0}, // state   91
    {   0,    0,  489,    0,    0}, // state   92
    { 490,   27,  517,  518,    2}, // state   93
    { 520,   27,  547,  548,    2}, // state   94
    {   0,    0,  550,    0,    0}, // state   95
    { 551,    1,  552,  553,    1}, // state   96
    { 554,    1,  555,  556,    1}, // state   97
    {   0,    0,  557,    0,    0}, // state   98
    { 558,   25,  583,  584,    2}, // state   99
    { 586,   25,  611,  612,    2}, // state  100
    {   0,    0,  614,    0,    0}, // state  101
    { 615,   25,  640,  641,    2}, // state  102
    { 643,   25,  668,  669,    2}, // state  103
    {   0,    0,  671,    0,    0}, // state  104
    { 672,   25,  697,  698,    2}, // state  105
    { 700,   25,  725,  726,    2}, // state  106
    {   0,    0,  728,    0,    0}, // state  107
    { 729,   13,    0,    0,    0}, // state  108
    {   0,    0,  742,    0,    0}, // state  109
    { 743,   13,    0,    0,    0}, // state  110
    { 756,    1,    0,    0,    0}, // state  111
    { 757,    1,    0,    0,    0}, // state  112
    { 758,    1,    0,    0,    0}, // state  113
    { 759,    2,    0,    0,    0}, // state  114
    { 761,    1,    0,    0,    0}, // state  115
    { 762,    1,    0,    0,    0}, // state  116
    { 763,   13,    0,    0,    0}, // state  117
    { 776,   13,    0,    0,    0}, // state  118
    { 789,   13,    0,    0,    0}, // state  119
    { 802,   13,    0,    0,    0}, // state  120
    { 815,   13,    0,    0,    0}, // state  121
    { 828,    1,    0,    0,    0}, // state  122
    { 829,    2,    0,    0,    0}, // state  123
    { 831,   13,    0,    0,    0}, // state  124
    { 844,   13,    0,    0,    0}, // state  125
    { 857,   13,    0,    0,    0}, // state  126
    {   0,    0,  870,    0,    0}, // state  127
    { 871,    5,  876,    0,    0}, // state  128
    { 877,    3,  880,    0,    0}, // state  129
    { 881,    3,  884,    0,    0}, // state  130
    {   0,    0,  885,    0,    0}, // state  131
    {   0,    0,  886,    0,    0}, // state  132
    {   0,    0,  887,    0,    0}, // state  133
    { 888,   11,    0,  899,    1}, // state  134
    { 900,   11,    0,  911,    1}, // state  135
    { 912,   11,    0,  923,    1}, // state  136
    { 924,   11,    0,  935,    1}, // state  137
    { 936,   11,    0,  947,    1}, // state  138
    { 948,    6,  954,    0,    0}, // state  139
    { 955,   11,    0,  966,    1}, // state  140
    { 967,    6,  973,    0,    0}, // state  141
    { 974,   13,    0,    0,    0}, // state  142
    { 987,    2,    0,    0,    0}, // state  143
    { 989,    1,    0,    0,    0}, // state  144
    { 990,    1,    0,    0,    0}, // state  145
    { 991,    1,    0,    0,    0}, // state  146
    { 992,    1,    0,    0,    0}, // state  147
    { 993,    1,    0,    0,    0}, // state  148
    { 994,    1,    0,    0,    0}, // state  149
    { 995,    1,    0,    0,    0}, // state  150
    { 996,    1,    0,    0,    0}, // state  151
    { 997,    2,    0,  999,   11}, // state  152
    {1010,    2,    0, 1012,   14}, // state  153
    {1026,    1,    0,    0,    0}, // state  154
    {1027,    1,    0,    0,    0}, // state  155
    {1028,    1,    0,    0,    0}, // state  156
    {1029,    1,    0,    0,    0}, // state  157
    {1030,    1,    0,    0,    0}, // state  158
    {1031,    1,    0,    0,    0}, // state  159
    {1032,    1, 1033,    0,    0}, // state  160
    {1034,    1,    0,    0,    0}, // state  161
    {   0,    0, 1035,    0,    0}, // state  162
    {   0,    0, 1036,    0,    0}, // state  163
    {   0,    0, 1037,    0,    0}, // state  164
    {1038,    1,    0,    0,    0}, // state  165
    {1039,    2,    0,    0,    0}, // state  166
    {1041,   11,    0, 1052,    1}, // state  167
    {1053,    1,    0,    0,    0}, // state  168
    {   0,    0, 1054,    0,    0}, // state  169
    {   0,    0, 1055,    0,    0}, // state  170
    {   0,    0, 1056,    0,    0}, // state  171
    {   0,    0, 1057,    0,    0}, // state  172
    {   0,    0, 1058,    0,    0}, // state  173
    {   0,    0, 1059,    0,    0}, // state  174
    {   0,    0, 1060,    0,    0}, // state  175
    {1061,   11,    0, 1072,    1}, // state  176
    {   0,    0, 1073,    0,    0}, // state  177
    {   0,    0, 1074,    0,    0}, // state  178
    {   0,    0, 1075,    0,    0}, // state  179
    {1076,   10, 1086,    0,    0}, // state  180
    {1087,   11, 1098,    0,    0}, // state  181
    {1099,   12, 1111,    0,    0}, // state  182
    {1112,   10, 1122,    0,    0}, // state  183
    {1123,    8, 1131,    0,    0}, // state  184
    {1132,    8, 1140,    0,    0}, // state  185
    {1141,    1,    0,    0,    0}, // state  186
    {1142,    1,    0,    0,    0}, // state  187
    {1143,    2,    0,    0,    0}, // state  188
    {1145,   11,    0, 1156,    1}, // state  189
    {1157,    1,    0,    0,    0}, // state  190
    {   0,    0, 1158,    0,    0}, // state  191
    {1159,   11,    0, 1170,    1}, // state  192
    {   0,    0, 1171,    0,    0}, // state  193
    {   0,    0, 1172,    0,    0}, // state  194
    {1173,   11,    0, 1184,    1}, // state  195
    {   0,    0, 1185,    0,    0}, // state  196
    {1186,   25, 1211, 1212,    2}, // state  197
    {1214,   25, 1239, 1240,    2}, // state  198
    {   0,    0, 1242,    0,    0}, // state  199
    {   0,    0, 1243,    0,    0}, // state  200
    {   0,    0, 1244,    0,    0}, // state  201
    {   0,    0, 1245,    0,    0}, // state  202
    {   0,    0, 1246,    0,    0}, // state  203
    {   0,    0, 1247,    0,    0}, // state  204
    {   0,    0, 1248,    0,    0}, // state  205
    {   0,    0, 1249,    0,    0}, // state  206
    {   0,    0, 1250,    0,    0}, // state  207
    {   0,    0, 1251,    0,    0}, // state  208
    {   0,    0, 1252,    0,    0}, // state  209
    {1253,    1,    0,    0,    0}, // state  210
    {1254,    1,    0,    0,    0}, // state  211
    {1255,   13,    0,    0,    0}, // state  212
    {1268,    1,    0,    0,    0}, // state  213
    {1269,   13,    0,    0,    0}, // state  214
    {   0,    0, 1282,    0,    0}, // state  215
    {   0,    0, 1283,    0,    0}, // state  216
    {1284,    1,    0,    0,    0}, // state  217
    {1285,    1,    0,    0,    0}, // state  218
    {1286,   13,    0,    0,    0}, // state  219
    {1299,    1,    0,    0,    0}, // state  220
    {1300,   13,    0,    0,    0}, // state  221
    {1313,   13,    0,    0,    0}, // state  222
    {1326,    1,    0,    0,    0}, // state  223
    {1327,    1,    0,    0,    0}, // state  224
    {1328,    1,    0,    0,    0}, // state  225
    {1329,    1,    0,    0,    0}, // state  226
    {1330,    1,    0,    0,    0}, // state  227
    {1331,    1,    0,    0,    0}, // state  228
    {1332,    1,    0,    0,    0}, // state  229
    {1333,    1,    0,    0,    0}, // state  230
    {1334,    1,    0,    0,    0}, // state  231
    {1335,    1,    0,    0,    0}, // state  232
    {1336,    1,    0,    0,    0}, // state  233
    {1337,    1,    0,    0,    0}, // state  234
    {   0,    0, 1338,    0,    0}, // state  235
    {   0,    0, 1339,    0,    0}, // state  236
    {   0,    0, 1340,    0,    0}, // state  237
    {   0,    0, 1341,    0,    0}, // state  238
    {1342,    1,    0,    0,    0}, // state  239
    {   0,    0, 1343,    0,    0}, // state  240
    {   0,    0, 1344,    0,    0}, // state  241
    {   0,    0, 1345,    0,    0}, // state  242
    {   0,    0, 1346,    0,    0}, // state  243
    {   0,    0, 1347,    0,    0}, // state  244
    {   0,    0, 1348,    0,    0}, // state  245
    {   0,    0, 1349,    0,    0}  // state  246

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
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state    3
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {TA_REDUCE_AND_ACCEPT_USING_RULE,    0}},

// ///////////////////////////////////////////////////////////////////////////
// state    4
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   40}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state    5
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   46}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state    6
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,   47}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    3}},

// ///////////////////////////////////////////////////////////////////////////
// state    7
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    5}},

// ///////////////////////////////////////////////////////////////////////////
// state    8
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    6}},

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
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,   49}},

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
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   37}},

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
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state   16
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   52}},
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,   53}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   62}},

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
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   68}},

// ///////////////////////////////////////////////////////////////////////////
// state   32
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   55}},

// ///////////////////////////////////////////////////////////////////////////
// state   33
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   54}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   71}},

// ///////////////////////////////////////////////////////////////////////////
// state   37
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   72}},

// ///////////////////////////////////////////////////////////////////////////
// state   38
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   73}},

// ///////////////////////////////////////////////////////////////////////////
// state   39
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   74}},

// ///////////////////////////////////////////////////////////////////////////
// state   40
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,   75}},

// ///////////////////////////////////////////////////////////////////////////
// state   41
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   13}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state   46
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   92}},

// ///////////////////////////////////////////////////////////////////////////
// state   47
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    4}},

// ///////////////////////////////////////////////////////////////////////////
// state   48
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,   93}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,   94}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {Token::conditional_series_end__, {                  TA_PUSH_STATE,   95}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {       Token::else_statement__, {                  TA_PUSH_STATE,   96}},
    {    Token::else_if_statement__, {                  TA_PUSH_STATE,   97}},
    {               Token::end_if__, {                  TA_PUSH_STATE,   98}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   49
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,   99}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  100}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {           Token::end_define__, {                  TA_PUSH_STATE,  101}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   50
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,  102}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  103}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::end_loop__, {                  TA_PUSH_STATE,  104}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   51
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,  105}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  106}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},
    {         Token::end_for_each__, {                  TA_PUSH_STATE,  107}},

// ///////////////////////////////////////////////////////////////////////////
// state   52
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  108}},

// ///////////////////////////////////////////////////////////////////////////
// state   53
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   64}},

// ///////////////////////////////////////////////////////////////////////////
// state   54
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  109}},

// ///////////////////////////////////////////////////////////////////////////
// state   55
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  110}},

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
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  116}},

// ///////////////////////////////////////////////////////////////////////////
// state   62
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  117}},

// ///////////////////////////////////////////////////////////////////////////
// state   63
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  118}},

// ///////////////////////////////////////////////////////////////////////////
// state   64
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  119}},

// ///////////////////////////////////////////////////////////////////////////
// state   65
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  120}},

// ///////////////////////////////////////////////////////////////////////////
// state   66
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  121}},

// ///////////////////////////////////////////////////////////////////////////
// state   67
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  122}},

// ///////////////////////////////////////////////////////////////////////////
// state   68
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  123}},

// ///////////////////////////////////////////////////////////////////////////
// state   69
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  124}},

// ///////////////////////////////////////////////////////////////////////////
// state   70
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  125}},

// ///////////////////////////////////////////////////////////////////////////
// state   71
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  126}},

// ///////////////////////////////////////////////////////////////////////////
// state   72
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  127}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state   73
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   80}},

// ///////////////////////////////////////////////////////////////////////////
// state   74
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   81}},

// ///////////////////////////////////////////////////////////////////////////
// state   75
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}},

// ///////////////////////////////////////////////////////////////////////////
// state   76
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  128}},

// ///////////////////////////////////////////////////////////////////////////
// state   77
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  129}},

// ///////////////////////////////////////////////////////////////////////////
// state   78
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  130}},

// ///////////////////////////////////////////////////////////////////////////
// state   79
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  131}},

// ///////////////////////////////////////////////////////////////////////////
// state   80
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  132}},

// ///////////////////////////////////////////////////////////////////////////
// state   81
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  133}},

// ///////////////////////////////////////////////////////////////////////////
// state   82
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  134}},

// ///////////////////////////////////////////////////////////////////////////
// state   83
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,  135}},

// ///////////////////////////////////////////////////////////////////////////
// state   84
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,  136}},

// ///////////////////////////////////////////////////////////////////////////
// state   85
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  137}},

// ///////////////////////////////////////////////////////////////////////////
// state   86
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  138}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  139}},

// ///////////////////////////////////////////////////////////////////////////
// state   87
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  140}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  141}},

// ///////////////////////////////////////////////////////////////////////////
// state   88
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  142}},

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
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  145}},

// ///////////////////////////////////////////////////////////////////////////
// state   92
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state   93
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {                   Token::ELSE, {        TA_SHIFT_AND_PUSH_STATE,  146}},
    {                Token::ELSE_IF, {        TA_SHIFT_AND_PUSH_STATE,  147}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  148}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   40}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state   94
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {                   Token::ELSE, {        TA_SHIFT_AND_PUSH_STATE,  149}},
    {                Token::ELSE_IF, {        TA_SHIFT_AND_PUSH_STATE,  150}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  151}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   46}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state   95
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   23}},

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
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,  153}},

// ///////////////////////////////////////////////////////////////////////////
// state   98
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   24}},

// ///////////////////////////////////////////////////////////////////////////
// state   99
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {             Token::END_DEFINE, {        TA_SHIFT_AND_PUSH_STATE,  154}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   40}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  100
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {             Token::END_DEFINE, {        TA_SHIFT_AND_PUSH_STATE,  155}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   46}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  101
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    7}},

// ///////////////////////////////////////////////////////////////////////////
// state  102
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {               Token::END_LOOP, {        TA_SHIFT_AND_PUSH_STATE,  156}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   40}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  103
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {               Token::END_LOOP, {        TA_SHIFT_AND_PUSH_STATE,  157}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   46}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  104
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    8}},

// ///////////////////////////////////////////////////////////////////////////
// state  105
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {           Token::END_FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,  158}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   40}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  106
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {           Token::END_FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,  159}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   46}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  107
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state  108
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  160}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  109
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state  110
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  161}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

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

// ///////////////////////////////////////////////////////////////////////////
// state  114
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  165}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,  166}},

// ///////////////////////////////////////////////////////////////////////////
// state  115
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  167}},

// ///////////////////////////////////////////////////////////////////////////
// state  116
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  168}},

// ///////////////////////////////////////////////////////////////////////////
// state  117
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  169}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  118
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  170}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  119
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  171}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  120
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  172}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  121
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  173}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  122
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  174}},

// ///////////////////////////////////////////////////////////////////////////
// state  123
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  175}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,  176}},

// ///////////////////////////////////////////////////////////////////////////
// state  124
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  177}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  125
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  178}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  126
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  179}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  127
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   82}},

// ///////////////////////////////////////////////////////////////////////////
// state  128
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   66}},

// ///////////////////////////////////////////////////////////////////////////
// state  129
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   75}},

// ///////////////////////////////////////////////////////////////////////////
// state  130
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
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
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   79}},

// ///////////////////////////////////////////////////////////////////////////
// state  134
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  180}},

// ///////////////////////////////////////////////////////////////////////////
// state  135
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  181}},

// ///////////////////////////////////////////////////////////////////////////
// state  136
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  182}},

// ///////////////////////////////////////////////////////////////////////////
// state  137
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  183}},

// ///////////////////////////////////////////////////////////////////////////
// state  138
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  184}},

// ///////////////////////////////////////////////////////////////////////////
// state  139
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   71}},

// ///////////////////////////////////////////////////////////////////////////
// state  140
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  185}},

// ///////////////////////////////////////////////////////////////////////////
// state  141
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   73}},

// ///////////////////////////////////////////////////////////////////////////
// state  142
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  186}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  143
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  187}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,  188}},

// ///////////////////////////////////////////////////////////////////////////
// state  144
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  189}},

// ///////////////////////////////////////////////////////////////////////////
// state  145
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  190}},

// ///////////////////////////////////////////////////////////////////////////
// state  146
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  191}},

// ///////////////////////////////////////////////////////////////////////////
// state  147
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,  192}},

// ///////////////////////////////////////////////////////////////////////////
// state  148
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  193}},

// ///////////////////////////////////////////////////////////////////////////
// state  149
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  194}},

// ///////////////////////////////////////////////////////////////////////////
// state  150
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,  195}},

// ///////////////////////////////////////////////////////////////////////////
// state  151
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  196}},

// ///////////////////////////////////////////////////////////////////////////
// state  152
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,  197}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  198}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {               Token::end_if__, {                  TA_PUSH_STATE,  199}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state  153
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,   93}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,   94}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {Token::conditional_series_end__, {                  TA_PUSH_STATE,  200}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {       Token::else_statement__, {                  TA_PUSH_STATE,   96}},
    {    Token::else_if_statement__, {                  TA_PUSH_STATE,   97}},
    {               Token::end_if__, {                  TA_PUSH_STATE,   98}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state  154
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  201}},

// ///////////////////////////////////////////////////////////////////////////
// state  155
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  202}},

// ///////////////////////////////////////////////////////////////////////////
// state  156
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  203}},

// ///////////////////////////////////////////////////////////////////////////
// state  157
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  204}},

// ///////////////////////////////////////////////////////////////////////////
// state  158
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  205}},

// ///////////////////////////////////////////////////////////////////////////
// state  159
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  206}},

// ///////////////////////////////////////////////////////////////////////////
// state  160
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,  207}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   63}},

// ///////////////////////////////////////////////////////////////////////////
// state  161
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  208}},

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
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   17}},

// ///////////////////////////////////////////////////////////////////////////
// state  165
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  209}},

// ///////////////////////////////////////////////////////////////////////////
// state  166
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,  210}},
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  211}},

// ///////////////////////////////////////////////////////////////////////////
// state  167
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  212}},

// ///////////////////////////////////////////////////////////////////////////
// state  168
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  213}},

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
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state  174
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   56}},

// ///////////////////////////////////////////////////////////////////////////
// state  175
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   60}},

// ///////////////////////////////////////////////////////////////////////////
// state  176
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  214}},

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
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state  180
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   70}},

// ///////////////////////////////////////////////////////////////////////////
// state  181
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   68}},

// ///////////////////////////////////////////////////////////////////////////
// state  182
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   67}},

// ///////////////////////////////////////////////////////////////////////////
// state  183
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   69}},

// ///////////////////////////////////////////////////////////////////////////
// state  184
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   72}},

// ///////////////////////////////////////////////////////////////////////////
// state  185
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   74}},

// ///////////////////////////////////////////////////////////////////////////
// state  186
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  215}},

// ///////////////////////////////////////////////////////////////////////////
// state  187
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  216}},

// ///////////////////////////////////////////////////////////////////////////
// state  188
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,  217}},
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  218}},

// ///////////////////////////////////////////////////////////////////////////
// state  189
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  219}},

// ///////////////////////////////////////////////////////////////////////////
// state  190
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  220}},

// ///////////////////////////////////////////////////////////////////////////
// state  191
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   29}},

// ///////////////////////////////////////////////////////////////////////////
// state  192
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  221}},

// ///////////////////////////////////////////////////////////////////////////
// state  193
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   33}},

// ///////////////////////////////////////////////////////////////////////////
// state  194
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   30}},

// ///////////////////////////////////////////////////////////////////////////
// state  195
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  222}},

// ///////////////////////////////////////////////////////////////////////////
// state  196
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state  197
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  148}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   40}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  198
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  151}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                Token::INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {        Token::SANDBOX_INCLUDE, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {                Token::WARNING, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                  Token::ERROR, {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {            Token::FATAL_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                 Token::SIZEOF, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {             Token::IS_DEFINED, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {        Token::INTEGER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {            Token::KEYWORD_INT, {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {         Token::KEYWORD_STRING, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {          Token::STRING_LENGTH, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   46}},
    {           Token::expression__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  199
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   25}},

// ///////////////////////////////////////////////////////////////////////////
// state  200
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   26}},

// ///////////////////////////////////////////////////////////////////////////
// state  201
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state  202
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   45}},

// ///////////////////////////////////////////////////////////////////////////
// state  203
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   48}},

// ///////////////////////////////////////////////////////////////////////////
// state  204
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   49}},

// ///////////////////////////////////////////////////////////////////////////
// state  205
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},

// ///////////////////////////////////////////////////////////////////////////
// state  206
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state  207
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   65}},

// ///////////////////////////////////////////////////////////////////////////
// state  208
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   27}},

// ///////////////////////////////////////////////////////////////////////////
// state  209
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   38}},

// ///////////////////////////////////////////////////////////////////////////
// state  210
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  223}},

// ///////////////////////////////////////////////////////////////////////////
// state  211
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  224}},

// ///////////////////////////////////////////////////////////////////////////
// state  212
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  225}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  213
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  226}},

// ///////////////////////////////////////////////////////////////////////////
// state  214
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  227}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  215
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state  216
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state  217
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  228}},

// ///////////////////////////////////////////////////////////////////////////
// state  218
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  229}},

// ///////////////////////////////////////////////////////////////////////////
// state  219
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  230}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  220
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  231}},

// ///////////////////////////////////////////////////////////////////////////
// state  221
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  232}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  222
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  233}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   77}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   78}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   87}},

// ///////////////////////////////////////////////////////////////////////////
// state  223
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  234}},

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
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  237}},

// ///////////////////////////////////////////////////////////////////////////
// state  227
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  238}},

// ///////////////////////////////////////////////////////////////////////////
// state  228
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  239}},

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
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  242}},

// ///////////////////////////////////////////////////////////////////////////
// state  232
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  243}},

// ///////////////////////////////////////////////////////////////////////////
// state  233
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  244}},

// ///////////////////////////////////////////////////////////////////////////
// state  234
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  245}},

// ///////////////////////////////////////////////////////////////////////////
// state  235
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state  236
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   46}},

// ///////////////////////////////////////////////////////////////////////////
// state  237
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   50}},

// ///////////////////////////////////////////////////////////////////////////
// state  238
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state  239
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  246}},

// ///////////////////////////////////////////////////////////////////////////
// state  240
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  241
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   47}},

// ///////////////////////////////////////////////////////////////////////////
// state  242
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state  243
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   31}},

// ///////////////////////////////////////////////////////////////////////////
// state  244
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   32}},

// ///////////////////////////////////////////////////////////////////////////
// state  245
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   42}},

// ///////////////////////////////////////////////////////////////////////////
// state  246
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   43}}

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

#line 4555 "barf_preprocessor_parser.cpp"

