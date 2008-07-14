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

Parser::ParserReturnCode Parser::Parse (Ast::Base * *return_token)
{


    ParserReturnCode return_code = PrivateParse(return_token);



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

Parser::ParserReturnCode Parser::PrivateParse (Ast::Base * *return_token)
{
    assert(return_token && "the return-token pointer must be valid");

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
                    *return_token = m_reduction_token;
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
                    *return_token = m_reduction_token;
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
                        *return_token = NULL;
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
    *return_token = NULL;
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
        "TO_CHARACTER_LITERAL",
        "TO_STRING_LITERAL",
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

#line 148 "barf_preprocessor_parser.trison"

        return new Body();
    
#line 509 "barf_preprocessor_parser.cpp"
}

// rule 2: body <- TEXT:text    
Ast::Base * Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    Text * text = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 153 "barf_preprocessor_parser.trison"

        Body *body = new Body();
        body->Append(text);
        return body;
    
#line 524 "barf_preprocessor_parser.cpp"
}

// rule 3: body <- body:body executable:executable    
Ast::Base * Parser::ReductionRuleHandler0003 ()
{
    assert(0 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    ExecutableAst * executable = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 160 "barf_preprocessor_parser.trison"

        if (executable != NULL)
            body->Append(executable);
        return body;
    
#line 541 "barf_preprocessor_parser.cpp"
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

#line 167 "barf_preprocessor_parser.trison"

        if (executable != NULL)
            body->Append(executable);
        body->Append(text);
        return body;
    
#line 561 "barf_preprocessor_parser.cpp"
}

// rule 5: executable <- code:code    
Ast::Base * Parser::ReductionRuleHandler0005 ()
{
    assert(0 < m_reduction_rule_token_count);
    ExecutableAst * code = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 177 "barf_preprocessor_parser.trison"
 return code; 
#line 572 "barf_preprocessor_parser.cpp"
}

// rule 6: executable <- conditional_series:conditional    
Ast::Base * Parser::ReductionRuleHandler0006 ()
{
    assert(0 < m_reduction_rule_token_count);
    Conditional * conditional = Dsc< Conditional * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 179 "barf_preprocessor_parser.trison"
 return conditional; 
#line 583 "barf_preprocessor_parser.cpp"
}

// rule 7: executable <- define:define body:body end_define    
Ast::Base * Parser::ReductionRuleHandler0007 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 182 "barf_preprocessor_parser.trison"

        define->SetBody(body);
        return define;
    
#line 599 "barf_preprocessor_parser.cpp"
}

// rule 8: executable <- loop:loop body:body end_loop    
Ast::Base * Parser::ReductionRuleHandler0008 ()
{
    assert(0 < m_reduction_rule_token_count);
    Loop * loop = Dsc< Loop * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 188 "barf_preprocessor_parser.trison"

        loop->SetBody(body);
        return loop;
    
#line 615 "barf_preprocessor_parser.cpp"
}

// rule 9: executable <- for_each:for_each body:body end_for_each    
Ast::Base * Parser::ReductionRuleHandler0009 ()
{
    assert(0 < m_reduction_rule_token_count);
    ForEach * for_each = Dsc< ForEach * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 194 "barf_preprocessor_parser.trison"

        for_each->SetBody(body);
        return for_each;
    
#line 631 "barf_preprocessor_parser.cpp"
}

// rule 10: code <- START_CODE code_body:code_body END_CODE    
Ast::Base * Parser::ReductionRuleHandler0010 ()
{
    assert(1 < m_reduction_rule_token_count);
    ExecutableAst * code_body = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 202 "barf_preprocessor_parser.trison"
 return code_body; 
#line 642 "barf_preprocessor_parser.cpp"
}

// rule 11: code <- CODE_LINE code_body:code_body CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0011 ()
{
    assert(1 < m_reduction_rule_token_count);
    ExecutableAst * code_body = Dsc< ExecutableAst * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 204 "barf_preprocessor_parser.trison"
 return code_body; 
#line 653 "barf_preprocessor_parser.cpp"
}

// rule 12: code_body <-     
Ast::Base * Parser::ReductionRuleHandler0012 ()
{

#line 210 "barf_preprocessor_parser.trison"
 return NULL; 
#line 662 "barf_preprocessor_parser.cpp"
}

// rule 13: code_body <- expression:expression    
Ast::Base * Parser::ReductionRuleHandler0013 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 213 "barf_preprocessor_parser.trison"
 return expression; 
#line 673 "barf_preprocessor_parser.cpp"
}

// rule 14: code_body <- DUMP_SYMBOL_TABLE '(' ')'    
Ast::Base * Parser::ReductionRuleHandler0014 ()
{

#line 216 "barf_preprocessor_parser.trison"
 return new DumpSymbolTable(); 
#line 682 "barf_preprocessor_parser.cpp"
}

// rule 15: code_body <- TO_CHARACTER_LITERAL '(' expression:character_index_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0015 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * character_index_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 219 "barf_preprocessor_parser.trison"
 return new ToCharacterLiteral(character_index_expression); 
#line 693 "barf_preprocessor_parser.cpp"
}

// rule 16: code_body <- TO_STRING_LITERAL '(' expression:string_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0016 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * string_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 222 "barf_preprocessor_parser.trison"
 return new ToStringLiteral(string_expression); 
#line 704 "barf_preprocessor_parser.cpp"
}

// rule 17: code_body <- UNDEFINE '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0017 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 225 "barf_preprocessor_parser.trison"
 return new Undefine(id); 
#line 715 "barf_preprocessor_parser.cpp"
}

// rule 18: code_body <- DECLARE_ARRAY '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0018 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 228 "barf_preprocessor_parser.trison"
 return new DeclareArray(id); 
#line 726 "barf_preprocessor_parser.cpp"
}

// rule 19: code_body <- DECLARE_MAP '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0019 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 231 "barf_preprocessor_parser.trison"
 return new DeclareMap(id); 
#line 737 "barf_preprocessor_parser.cpp"
}

// rule 20: code_body <- INCLUDE '(' expression:include_filename_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0020 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * include_filename_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 234 "barf_preprocessor_parser.trison"
 return new Include(include_filename_expression, false); 
#line 748 "barf_preprocessor_parser.cpp"
}

// rule 21: code_body <- SANDBOX_INCLUDE '(' expression:include_filename_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0021 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * include_filename_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 237 "barf_preprocessor_parser.trison"
 return new Include(include_filename_expression, true); 
#line 759 "barf_preprocessor_parser.cpp"
}

// rule 22: code_body <- WARNING '(' expression:message_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0022 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * message_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 240 "barf_preprocessor_parser.trison"
 return new Message(message_expression, Message::WARNING); 
#line 770 "barf_preprocessor_parser.cpp"
}

// rule 23: code_body <- ERROR '(' expression:message_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0023 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * message_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 243 "barf_preprocessor_parser.trison"
 return new Message(message_expression, Message::ERROR); 
#line 781 "barf_preprocessor_parser.cpp"
}

// rule 24: code_body <- FATAL_ERROR '(' expression:message_expression ')'    
Ast::Base * Parser::ReductionRuleHandler0024 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * message_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 246 "barf_preprocessor_parser.trison"
 return new Message(message_expression, Message::FATAL_ERROR); 
#line 792 "barf_preprocessor_parser.cpp"
}

// rule 25: conditional_series <- if_statement:conditional body:if_body conditional_series_end:else_body    
Ast::Base * Parser::ReductionRuleHandler0025 ()
{
    assert(0 < m_reduction_rule_token_count);
    Conditional * conditional = Dsc< Conditional * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * if_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Body * else_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 252 "barf_preprocessor_parser.trison"

        conditional->SetIfBody(if_body);
        conditional->SetElseBody(else_body);
        return conditional;
    
#line 811 "barf_preprocessor_parser.cpp"
}

// rule 26: conditional_series_end <- end_if    
Ast::Base * Parser::ReductionRuleHandler0026 ()
{

#line 261 "barf_preprocessor_parser.trison"
 return NULL; 
#line 820 "barf_preprocessor_parser.cpp"
}

// rule 27: conditional_series_end <- else_statement body:body end_if    
Ast::Base * Parser::ReductionRuleHandler0027 ()
{
    assert(1 < m_reduction_rule_token_count);
    Body * body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 263 "barf_preprocessor_parser.trison"
 return body; 
#line 831 "barf_preprocessor_parser.cpp"
}

// rule 28: conditional_series_end <- else_if_statement:conditional body:if_body conditional_series_end:else_body    
Ast::Base * Parser::ReductionRuleHandler0028 ()
{
    assert(0 < m_reduction_rule_token_count);
    Conditional * conditional = Dsc< Conditional * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Body * if_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Body * else_body = Dsc< Body * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 266 "barf_preprocessor_parser.trison"

        conditional->SetIfBody(if_body);
        conditional->SetElseBody(else_body);
        Body *body = new Body();
        body->Append(conditional);
        return body;
    
#line 852 "barf_preprocessor_parser.cpp"
}

// rule 29: if_statement <- START_CODE IF '(' expression:expression ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0029 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 278 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 863 "barf_preprocessor_parser.cpp"
}

// rule 30: if_statement <- CODE_LINE IF '(' expression:expression ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0030 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 281 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 874 "barf_preprocessor_parser.cpp"
}

// rule 31: else_statement <- START_CODE ELSE END_CODE    
Ast::Base * Parser::ReductionRuleHandler0031 ()
{

#line 286 "barf_preprocessor_parser.trison"
 return NULL; 
#line 883 "barf_preprocessor_parser.cpp"
}

// rule 32: else_statement <- CODE_LINE ELSE CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0032 ()
{

#line 288 "barf_preprocessor_parser.trison"
 return NULL; 
#line 892 "barf_preprocessor_parser.cpp"
}

// rule 33: else_if_statement <- START_CODE ELSE_IF '(' expression:expression ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0033 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 294 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 903 "barf_preprocessor_parser.cpp"
}

// rule 34: else_if_statement <- CODE_LINE ELSE_IF '(' expression:expression ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0034 ()
{
    assert(3 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 297 "barf_preprocessor_parser.trison"
 return new Conditional(expression); 
#line 914 "barf_preprocessor_parser.cpp"
}

// rule 35: end_if <- START_CODE END_IF END_CODE    
Ast::Base * Parser::ReductionRuleHandler0035 ()
{

#line 302 "barf_preprocessor_parser.trison"
 return NULL; 
#line 923 "barf_preprocessor_parser.cpp"
}

// rule 36: end_if <- CODE_LINE END_IF CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0036 ()
{

#line 304 "barf_preprocessor_parser.trison"
 return NULL; 
#line 932 "barf_preprocessor_parser.cpp"
}

// rule 37: define <- define_scalar:define    
Ast::Base * Parser::ReductionRuleHandler0037 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 309 "barf_preprocessor_parser.trison"
 return define; 
#line 943 "barf_preprocessor_parser.cpp"
}

// rule 38: define <- define_array_element:define    
Ast::Base * Parser::ReductionRuleHandler0038 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 311 "barf_preprocessor_parser.trison"
 return define; 
#line 954 "barf_preprocessor_parser.cpp"
}

// rule 39: define <- define_map_element:define    
Ast::Base * Parser::ReductionRuleHandler0039 ()
{
    assert(0 < m_reduction_rule_token_count);
    Define * define = Dsc< Define * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 313 "barf_preprocessor_parser.trison"
 return define; 
#line 965 "barf_preprocessor_parser.cpp"
}

// rule 40: define_scalar <- START_CODE DEFINE '(' ID:id ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0040 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 319 "barf_preprocessor_parser.trison"
 return new Define(id); 
#line 976 "barf_preprocessor_parser.cpp"
}

// rule 41: define_scalar <- CODE_LINE DEFINE '(' ID:id ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0041 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 322 "barf_preprocessor_parser.trison"
 return new Define(id); 
#line 987 "barf_preprocessor_parser.cpp"
}

// rule 42: define_array_element <- START_CODE DEFINE '(' ID:id '[' ']' ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0042 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 328 "barf_preprocessor_parser.trison"
 return new DefineArrayElement(id); 
#line 998 "barf_preprocessor_parser.cpp"
}

// rule 43: define_array_element <- CODE_LINE DEFINE '(' ID:id '[' ']' ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0043 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 331 "barf_preprocessor_parser.trison"
 return new DefineArrayElement(id); 
#line 1009 "barf_preprocessor_parser.cpp"
}

// rule 44: define_map_element <- START_CODE DEFINE '(' ID:id '[' STRING_LITERAL:key ']' ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0044 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Text * key = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 337 "barf_preprocessor_parser.trison"
 return new DefineMapElement(id, key); 
#line 1022 "barf_preprocessor_parser.cpp"
}

// rule 45: define_map_element <- CODE_LINE DEFINE '(' ID:id '[' STRING_LITERAL:key ']' ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0045 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Text * key = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 340 "barf_preprocessor_parser.trison"
 return new DefineMapElement(id, key); 
#line 1035 "barf_preprocessor_parser.cpp"
}

// rule 46: end_define <- START_CODE END_DEFINE END_CODE    
Ast::Base * Parser::ReductionRuleHandler0046 ()
{

#line 345 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1044 "barf_preprocessor_parser.cpp"
}

// rule 47: end_define <- CODE_LINE END_DEFINE CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0047 ()
{

#line 347 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1053 "barf_preprocessor_parser.cpp"
}

// rule 48: loop <- START_CODE LOOP '(' ID:iterator_id ',' expression:iteration_count_expression ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0048 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * iterator_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Expression * iteration_count_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 353 "barf_preprocessor_parser.trison"
 return new Loop(iterator_id, iteration_count_expression); 
#line 1066 "barf_preprocessor_parser.cpp"
}

// rule 49: loop <- CODE_LINE LOOP '(' ID:iterator_id ',' expression:iteration_count_expression ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0049 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * iterator_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Expression * iteration_count_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 356 "barf_preprocessor_parser.trison"
 return new Loop(iterator_id, iteration_count_expression); 
#line 1079 "barf_preprocessor_parser.cpp"
}

// rule 50: end_loop <- START_CODE END_LOOP END_CODE    
Ast::Base * Parser::ReductionRuleHandler0050 ()
{

#line 361 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1088 "barf_preprocessor_parser.cpp"
}

// rule 51: end_loop <- CODE_LINE END_LOOP CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0051 ()
{

#line 363 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1097 "barf_preprocessor_parser.cpp"
}

// rule 52: for_each <- START_CODE FOR_EACH '(' ID:key_id ',' ID:map_id ')' END_CODE    
Ast::Base * Parser::ReductionRuleHandler0052 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * key_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Ast::Id * map_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 369 "barf_preprocessor_parser.trison"
 return new ForEach(key_id, map_id); 
#line 1110 "barf_preprocessor_parser.cpp"
}

// rule 53: for_each <- CODE_LINE FOR_EACH '(' ID:key_id ',' ID:map_id ')' CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0053 ()
{
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * key_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(5 < m_reduction_rule_token_count);
    Ast::Id * map_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 372 "barf_preprocessor_parser.trison"
 return new ForEach(key_id, map_id); 
#line 1123 "barf_preprocessor_parser.cpp"
}

// rule 54: end_for_each <- START_CODE END_FOR_EACH END_CODE    
Ast::Base * Parser::ReductionRuleHandler0054 ()
{

#line 377 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1132 "barf_preprocessor_parser.cpp"
}

// rule 55: end_for_each <- CODE_LINE END_FOR_EACH CODE_NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0055 ()
{

#line 379 "barf_preprocessor_parser.trison"
 return NULL; 
#line 1141 "barf_preprocessor_parser.cpp"
}

// rule 56: expression <- STRING_LITERAL:str    
Ast::Base * Parser::ReductionRuleHandler0056 ()
{
    assert(0 < m_reduction_rule_token_count);
    Text * str = Dsc< Text * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 385 "barf_preprocessor_parser.trison"
 return str; 
#line 1152 "barf_preprocessor_parser.cpp"
}

// rule 57: expression <- INTEGER_LITERAL:integer    
Ast::Base * Parser::ReductionRuleHandler0057 ()
{
    assert(0 < m_reduction_rule_token_count);
    Integer * integer = Dsc< Integer * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 388 "barf_preprocessor_parser.trison"
 return integer; 
#line 1163 "barf_preprocessor_parser.cpp"
}

// rule 58: expression <- SIZEOF '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0058 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 391 "barf_preprocessor_parser.trison"
 return new Sizeof(id); 
#line 1174 "barf_preprocessor_parser.cpp"
}

// rule 59: expression <- KEYWORD_INT '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0059 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 394 "barf_preprocessor_parser.trison"
 return new Operation(Operation::INT_CAST, expression); 
#line 1185 "barf_preprocessor_parser.cpp"
}

// rule 60: expression <- KEYWORD_STRING '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0060 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 397 "barf_preprocessor_parser.trison"
 return new Operation(Operation::STRING_CAST, expression); 
#line 1196 "barf_preprocessor_parser.cpp"
}

// rule 61: expression <- STRING_LENGTH '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0061 ()
{
    assert(2 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 400 "barf_preprocessor_parser.trison"
 return new Operation(Operation::STRING_LENGTH, expression); 
#line 1207 "barf_preprocessor_parser.cpp"
}

// rule 62: expression <- IS_DEFINED '(' ID:id ')'    
Ast::Base * Parser::ReductionRuleHandler0062 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 403 "barf_preprocessor_parser.trison"
 return new IsDefined(id, NULL); 
#line 1218 "barf_preprocessor_parser.cpp"
}

// rule 63: expression <- IS_DEFINED '(' ID:id '[' expression:element_index_expression ']' ')'    
Ast::Base * Parser::ReductionRuleHandler0063 ()
{
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(4 < m_reduction_rule_token_count);
    Expression * element_index_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);

#line 406 "barf_preprocessor_parser.trison"
 return new IsDefined(id, element_index_expression); 
#line 1231 "barf_preprocessor_parser.cpp"
}

// rule 64: expression <- ID:id    
Ast::Base * Parser::ReductionRuleHandler0064 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 409 "barf_preprocessor_parser.trison"
 return new Dereference(id, NULL, DEREFERENCE_ALWAYS); 
#line 1242 "barf_preprocessor_parser.cpp"
}

// rule 65: expression <- ID:id '[' expression:element_index_expression ']'    
Ast::Base * Parser::ReductionRuleHandler0065 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * element_index_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 412 "barf_preprocessor_parser.trison"
 return new Dereference(id, element_index_expression, DEREFERENCE_ALWAYS); 
#line 1255 "barf_preprocessor_parser.cpp"
}

// rule 66: expression <- ID:id '?'    
Ast::Base * Parser::ReductionRuleHandler0066 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 415 "barf_preprocessor_parser.trison"
 return new Dereference(id, NULL, DEREFERENCE_IFF_DEFINED); 
#line 1266 "barf_preprocessor_parser.cpp"
}

// rule 67: expression <- ID:id '[' expression:element_index_expression ']' '?'    
Ast::Base * Parser::ReductionRuleHandler0067 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * element_index_expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 418 "barf_preprocessor_parser.trison"
 return new Dereference(id, element_index_expression, DEREFERENCE_IFF_DEFINED); 
#line 1279 "barf_preprocessor_parser.cpp"
}

// rule 68: expression <- expression:left '.' expression:right    %left %prec CONCATENATION
Ast::Base * Parser::ReductionRuleHandler0068 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 421 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::CONCATENATE, right); 
#line 1292 "barf_preprocessor_parser.cpp"
}

// rule 69: expression <- expression:left '|' '|' expression:right     %prec LOGICAL_OR
Ast::Base * Parser::ReductionRuleHandler0069 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 424 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LOGICAL_OR, right); 
#line 1305 "barf_preprocessor_parser.cpp"
}

// rule 70: expression <- expression:left '&' '&' expression:right     %prec LOGICAL_AND
Ast::Base * Parser::ReductionRuleHandler0070 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 427 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LOGICAL_AND, right); 
#line 1318 "barf_preprocessor_parser.cpp"
}

// rule 71: expression <- expression:left '=' '=' expression:right     %prec EQUALITY
Ast::Base * Parser::ReductionRuleHandler0071 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 430 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::EQUAL, right); 
#line 1331 "barf_preprocessor_parser.cpp"
}

// rule 72: expression <- expression:left '!' '=' expression:right     %prec EQUALITY
Ast::Base * Parser::ReductionRuleHandler0072 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 433 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::NOT_EQUAL, right); 
#line 1344 "barf_preprocessor_parser.cpp"
}

// rule 73: expression <- expression:left '<' expression:right    %left %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0073 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 436 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LESS_THAN, right); 
#line 1357 "barf_preprocessor_parser.cpp"
}

// rule 74: expression <- expression:left '<' '=' expression:right     %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0074 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 439 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::LESS_THAN_OR_EQUAL, right); 
#line 1370 "barf_preprocessor_parser.cpp"
}

// rule 75: expression <- expression:left '>' expression:right    %left %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0075 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 442 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::GREATER_THAN, right); 
#line 1383 "barf_preprocessor_parser.cpp"
}

// rule 76: expression <- expression:left '>' '=' expression:right     %prec COMPARISON
Ast::Base * Parser::ReductionRuleHandler0076 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 445 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::GREATER_THAN_OR_EQUAL, right); 
#line 1396 "barf_preprocessor_parser.cpp"
}

// rule 77: expression <- expression:left '+' expression:right    %left %prec ADDITION
Ast::Base * Parser::ReductionRuleHandler0077 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 448 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::PLUS, right); 
#line 1409 "barf_preprocessor_parser.cpp"
}

// rule 78: expression <- expression:left '-' expression:right    %left %prec ADDITION
Ast::Base * Parser::ReductionRuleHandler0078 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 451 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::MINUS, right); 
#line 1422 "barf_preprocessor_parser.cpp"
}

// rule 79: expression <- expression:left '*' expression:right    %left %prec MULTIPLICATION
Ast::Base * Parser::ReductionRuleHandler0079 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 454 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::MULTIPLY, right); 
#line 1435 "barf_preprocessor_parser.cpp"
}

// rule 80: expression <- expression:left '/' expression:right    %left %prec MULTIPLICATION
Ast::Base * Parser::ReductionRuleHandler0080 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 457 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::DIVIDE, right); 
#line 1448 "barf_preprocessor_parser.cpp"
}

// rule 81: expression <- expression:left '%' expression:right    %left %prec MULTIPLICATION
Ast::Base * Parser::ReductionRuleHandler0081 ()
{
    assert(0 < m_reduction_rule_token_count);
    Expression * left = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Expression * right = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 460 "barf_preprocessor_parser.trison"
 return new Operation(left, Operation::REMAINDER, right); 
#line 1461 "barf_preprocessor_parser.cpp"
}

// rule 82: expression <- '-' expression:expression     %prec UNARY
Ast::Base * Parser::ReductionRuleHandler0082 ()
{
    assert(1 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 463 "barf_preprocessor_parser.trison"
 return new Operation(Operation::MINUS, expression); 
#line 1472 "barf_preprocessor_parser.cpp"
}

// rule 83: expression <- '!' expression:expression     %prec UNARY
Ast::Base * Parser::ReductionRuleHandler0083 ()
{
    assert(1 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 466 "barf_preprocessor_parser.trison"
 return new Operation(Operation::LOGICAL_NOT, expression); 
#line 1483 "barf_preprocessor_parser.cpp"
}

// rule 84: expression <- '(' expression:expression ')'    
Ast::Base * Parser::ReductionRuleHandler0084 ()
{
    assert(1 < m_reduction_rule_token_count);
    Expression * expression = Dsc< Expression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 469 "barf_preprocessor_parser.trison"
 return expression; 
#line 1494 "barf_preprocessor_parser.cpp"
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
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0015, "rule 15: code_body <- TO_CHARACTER_LITERAL '(' expression ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0016, "rule 16: code_body <- TO_STRING_LITERAL '(' expression ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0017, "rule 17: code_body <- UNDEFINE '(' ID ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0018, "rule 18: code_body <- DECLARE_ARRAY '(' ID ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0019, "rule 19: code_body <- DECLARE_MAP '(' ID ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0020, "rule 20: code_body <- INCLUDE '(' expression ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0021, "rule 21: code_body <- SANDBOX_INCLUDE '(' expression ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0022, "rule 22: code_body <- WARNING '(' expression ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0023, "rule 23: code_body <- ERROR '(' expression ')'    "},
    {            Token::code_body__,  4, &Parser::ReductionRuleHandler0024, "rule 24: code_body <- FATAL_ERROR '(' expression ')'    "},
    {   Token::conditional_series__,  3, &Parser::ReductionRuleHandler0025, "rule 25: conditional_series <- if_statement body conditional_series_end    "},
    {Token::conditional_series_end__,  1, &Parser::ReductionRuleHandler0026, "rule 26: conditional_series_end <- end_if    "},
    {Token::conditional_series_end__,  3, &Parser::ReductionRuleHandler0027, "rule 27: conditional_series_end <- else_statement body end_if    "},
    {Token::conditional_series_end__,  3, &Parser::ReductionRuleHandler0028, "rule 28: conditional_series_end <- else_if_statement body conditional_series_end    "},
    {         Token::if_statement__,  6, &Parser::ReductionRuleHandler0029, "rule 29: if_statement <- START_CODE IF '(' expression ')' END_CODE    "},
    {         Token::if_statement__,  6, &Parser::ReductionRuleHandler0030, "rule 30: if_statement <- CODE_LINE IF '(' expression ')' CODE_NEWLINE    "},
    {       Token::else_statement__,  3, &Parser::ReductionRuleHandler0031, "rule 31: else_statement <- START_CODE ELSE END_CODE    "},
    {       Token::else_statement__,  3, &Parser::ReductionRuleHandler0032, "rule 32: else_statement <- CODE_LINE ELSE CODE_NEWLINE    "},
    {    Token::else_if_statement__,  6, &Parser::ReductionRuleHandler0033, "rule 33: else_if_statement <- START_CODE ELSE_IF '(' expression ')' END_CODE    "},
    {    Token::else_if_statement__,  6, &Parser::ReductionRuleHandler0034, "rule 34: else_if_statement <- CODE_LINE ELSE_IF '(' expression ')' CODE_NEWLINE    "},
    {               Token::end_if__,  3, &Parser::ReductionRuleHandler0035, "rule 35: end_if <- START_CODE END_IF END_CODE    "},
    {               Token::end_if__,  3, &Parser::ReductionRuleHandler0036, "rule 36: end_if <- CODE_LINE END_IF CODE_NEWLINE    "},
    {               Token::define__,  1, &Parser::ReductionRuleHandler0037, "rule 37: define <- define_scalar    "},
    {               Token::define__,  1, &Parser::ReductionRuleHandler0038, "rule 38: define <- define_array_element    "},
    {               Token::define__,  1, &Parser::ReductionRuleHandler0039, "rule 39: define <- define_map_element    "},
    {        Token::define_scalar__,  6, &Parser::ReductionRuleHandler0040, "rule 40: define_scalar <- START_CODE DEFINE '(' ID ')' END_CODE    "},
    {        Token::define_scalar__,  6, &Parser::ReductionRuleHandler0041, "rule 41: define_scalar <- CODE_LINE DEFINE '(' ID ')' CODE_NEWLINE    "},
    { Token::define_array_element__,  8, &Parser::ReductionRuleHandler0042, "rule 42: define_array_element <- START_CODE DEFINE '(' ID '[' ']' ')' END_CODE    "},
    { Token::define_array_element__,  8, &Parser::ReductionRuleHandler0043, "rule 43: define_array_element <- CODE_LINE DEFINE '(' ID '[' ']' ')' CODE_NEWLINE    "},
    {   Token::define_map_element__,  9, &Parser::ReductionRuleHandler0044, "rule 44: define_map_element <- START_CODE DEFINE '(' ID '[' STRING_LITERAL ']' ')' END_CODE    "},
    {   Token::define_map_element__,  9, &Parser::ReductionRuleHandler0045, "rule 45: define_map_element <- CODE_LINE DEFINE '(' ID '[' STRING_LITERAL ']' ')' CODE_NEWLINE    "},
    {           Token::end_define__,  3, &Parser::ReductionRuleHandler0046, "rule 46: end_define <- START_CODE END_DEFINE END_CODE    "},
    {           Token::end_define__,  3, &Parser::ReductionRuleHandler0047, "rule 47: end_define <- CODE_LINE END_DEFINE CODE_NEWLINE    "},
    {                 Token::loop__,  8, &Parser::ReductionRuleHandler0048, "rule 48: loop <- START_CODE LOOP '(' ID ',' expression ')' END_CODE    "},
    {                 Token::loop__,  8, &Parser::ReductionRuleHandler0049, "rule 49: loop <- CODE_LINE LOOP '(' ID ',' expression ')' CODE_NEWLINE    "},
    {             Token::end_loop__,  3, &Parser::ReductionRuleHandler0050, "rule 50: end_loop <- START_CODE END_LOOP END_CODE    "},
    {             Token::end_loop__,  3, &Parser::ReductionRuleHandler0051, "rule 51: end_loop <- CODE_LINE END_LOOP CODE_NEWLINE    "},
    {             Token::for_each__,  8, &Parser::ReductionRuleHandler0052, "rule 52: for_each <- START_CODE FOR_EACH '(' ID ',' ID ')' END_CODE    "},
    {             Token::for_each__,  8, &Parser::ReductionRuleHandler0053, "rule 53: for_each <- CODE_LINE FOR_EACH '(' ID ',' ID ')' CODE_NEWLINE    "},
    {         Token::end_for_each__,  3, &Parser::ReductionRuleHandler0054, "rule 54: end_for_each <- START_CODE END_FOR_EACH END_CODE    "},
    {         Token::end_for_each__,  3, &Parser::ReductionRuleHandler0055, "rule 55: end_for_each <- CODE_LINE END_FOR_EACH CODE_NEWLINE    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0056, "rule 56: expression <- STRING_LITERAL    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0057, "rule 57: expression <- INTEGER_LITERAL    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0058, "rule 58: expression <- SIZEOF '(' ID ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0059, "rule 59: expression <- KEYWORD_INT '(' expression ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0060, "rule 60: expression <- KEYWORD_STRING '(' expression ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0061, "rule 61: expression <- STRING_LENGTH '(' expression ')'    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0062, "rule 62: expression <- IS_DEFINED '(' ID ')'    "},
    {           Token::expression__,  7, &Parser::ReductionRuleHandler0063, "rule 63: expression <- IS_DEFINED '(' ID '[' expression ']' ')'    "},
    {           Token::expression__,  1, &Parser::ReductionRuleHandler0064, "rule 64: expression <- ID    "},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0065, "rule 65: expression <- ID '[' expression ']'    "},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0066, "rule 66: expression <- ID '?'    "},
    {           Token::expression__,  5, &Parser::ReductionRuleHandler0067, "rule 67: expression <- ID '[' expression ']' '?'    "},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0068, "rule 68: expression <- expression '.' expression    %left %prec CONCATENATION"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0069, "rule 69: expression <- expression '|' '|' expression     %prec LOGICAL_OR"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0070, "rule 70: expression <- expression '&' '&' expression     %prec LOGICAL_AND"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0071, "rule 71: expression <- expression '=' '=' expression     %prec EQUALITY"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0072, "rule 72: expression <- expression '!' '=' expression     %prec EQUALITY"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0073, "rule 73: expression <- expression '<' expression    %left %prec COMPARISON"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0074, "rule 74: expression <- expression '<' '=' expression     %prec COMPARISON"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0075, "rule 75: expression <- expression '>' expression    %left %prec COMPARISON"},
    {           Token::expression__,  4, &Parser::ReductionRuleHandler0076, "rule 76: expression <- expression '>' '=' expression     %prec COMPARISON"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0077, "rule 77: expression <- expression '+' expression    %left %prec ADDITION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0078, "rule 78: expression <- expression '-' expression    %left %prec ADDITION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0079, "rule 79: expression <- expression '*' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0080, "rule 80: expression <- expression '/' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0081, "rule 81: expression <- expression '%' expression    %left %prec MULTIPLICATION"},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0082, "rule 82: expression <- '-' expression     %prec UNARY"},
    {           Token::expression__,  2, &Parser::ReductionRuleHandler0083, "rule 83: expression <- '!' expression     %prec UNARY"},
    {           Token::expression__,  3, &Parser::ReductionRuleHandler0084, "rule 84: expression <- '(' expression ')'    "},

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
    {  19,   26,   45,   46,    2}, // state    4
    {  48,   26,   74,   75,    2}, // state    5
    {  77,    1,   78,    0,    0}, // state    6
    {   0,    0,   79,    0,    0}, // state    7
    {   0,    0,   80,    0,    0}, // state    8
    {  81,    1,   82,   83,    1}, // state    9
    {  84,    1,   85,   86,    1}, // state   10
    {   0,    0,   87,    0,    0}, // state   11
    {   0,    0,   88,    0,    0}, // state   12
    {   0,    0,   89,    0,    0}, // state   13
    {  90,    1,   91,   92,    1}, // state   14
    {  93,    1,   94,   95,    1}, // state   15
    {  96,    2,   98,    0,    0}, // state   16
    {  99,    1,    0,    0,    0}, // state   17
    { 100,    1,    0,    0,    0}, // state   18
    { 101,    1,    0,    0,    0}, // state   19
    { 102,    1,    0,    0,    0}, // state   20
    { 103,    1,    0,    0,    0}, // state   21
    { 104,    1,    0,    0,    0}, // state   22
    { 105,    1,    0,    0,    0}, // state   23
    { 106,    1,    0,    0,    0}, // state   24
    { 107,    1,    0,    0,    0}, // state   25
    { 108,    1,    0,    0,    0}, // state   26
    { 109,    1,    0,    0,    0}, // state   27
    { 110,    1,    0,    0,    0}, // state   28
    { 111,    1,    0,    0,    0}, // state   29
    { 112,    1,    0,    0,    0}, // state   30
    { 113,    1,    0,    0,    0}, // state   31
    {   0,    0,  114,    0,    0}, // state   32
    {   0,    0,  115,    0,    0}, // state   33
    { 116,    1,    0,    0,    0}, // state   34
    { 117,    1,    0,    0,    0}, // state   35
    { 118,    1,    0,    0,    0}, // state   36
    { 119,    1,    0,    0,    0}, // state   37
    { 120,    1,    0,    0,    0}, // state   38
    { 121,   11,    0,  132,    1}, // state   39
    { 133,   11,    0,  144,    1}, // state   40
    { 145,   11,    0,  156,    1}, // state   41
    { 157,    1,    0,    0,    0}, // state   42
    { 158,   12,  170,    0,    0}, // state   43
    { 171,    1,    0,    0,    0}, // state   44
    { 172,    1,    0,    0,    0}, // state   45
    { 173,    1,    0,    0,    0}, // state   46
    { 174,    1,    0,    0,    0}, // state   47
    { 175,    1,    0,    0,    0}, // state   48
    {   0,    0,  176,    0,    0}, // state   49
    { 177,    2,    0,  179,   14}, // state   50
    { 193,    2,    0,  195,   11}, // state   51
    { 206,    2,    0,  208,   11}, // state   52
    { 219,    2,    0,  221,   11}, // state   53
    { 232,   11,    0,  243,    1}, // state   54
    {   0,    0,  244,    0,    0}, // state   55
    { 245,    1,    0,    0,    0}, // state   56
    { 246,   11,    0,  257,    1}, // state   57
    { 258,    1,    0,    0,    0}, // state   58
    { 259,    1,    0,    0,    0}, // state   59
    { 260,    1,    0,    0,    0}, // state   60
    { 261,    1,    0,    0,    0}, // state   61
    { 262,    1,    0,    0,    0}, // state   62
    { 263,    1,    0,    0,    0}, // state   63
    { 264,   11,    0,  275,    1}, // state   64
    { 276,   11,    0,  287,    1}, // state   65
    { 288,   11,    0,  299,    1}, // state   66
    { 300,   11,    0,  311,    1}, // state   67
    { 312,   11,    0,  323,    1}, // state   68
    { 324,    1,    0,    0,    0}, // state   69
    { 325,    1,    0,    0,    0}, // state   70
    { 326,   11,    0,  337,    1}, // state   71
    { 338,   11,    0,  349,    1}, // state   72
    { 350,   11,    0,  361,    1}, // state   73
    { 362,   11,    0,  373,    1}, // state   74
    { 374,   11,    0,  385,    1}, // state   75
    { 386,   13,    0,    0,    0}, // state   76
    {   0,    0,  399,    0,    0}, // state   77
    {   0,    0,  400,    0,    0}, // state   78
    {   0,    0,  401,    0,    0}, // state   79
    { 402,   11,    0,  413,    1}, // state   80
    { 414,   11,    0,  425,    1}, // state   81
    { 426,   11,    0,  437,    1}, // state   82
    { 438,   11,    0,  449,    1}, // state   83
    { 450,   11,    0,  461,    1}, // state   84
    { 462,   11,    0,  473,    1}, // state   85
    { 474,    1,    0,    0,    0}, // state   86
    { 475,    1,    0,    0,    0}, // state   87
    { 476,    1,    0,    0,    0}, // state   88
    { 477,    1,    0,    0,    0}, // state   89
    { 478,   12,    0,  490,    1}, // state   90
    { 491,   12,    0,  503,    1}, // state   91
    { 504,   11,    0,  515,    1}, // state   92
    { 516,    1,    0,    0,    0}, // state   93
    { 517,    1,    0,    0,    0}, // state   94
    { 518,    1,    0,    0,    0}, // state   95
    {   0,    0,  519,    0,    0}, // state   96
    { 520,   29,  549,  550,    2}, // state   97
    { 552,   29,  581,  582,    2}, // state   98
    {   0,    0,  584,    0,    0}, // state   99
    { 585,    1,  586,  587,    1}, // state  100
    { 588,    1,  589,  590,    1}, // state  101
    {   0,    0,  591,    0,    0}, // state  102
    { 592,   27,  619,  620,    2}, // state  103
    { 622,   27,  649,  650,    2}, // state  104
    {   0,    0,  652,    0,    0}, // state  105
    { 653,   27,  680,  681,    2}, // state  106
    { 683,   27,  710,  711,    2}, // state  107
    {   0,    0,  713,    0,    0}, // state  108
    { 714,   27,  741,  742,    2}, // state  109
    { 744,   27,  771,  772,    2}, // state  110
    {   0,    0,  774,    0,    0}, // state  111
    { 775,   13,    0,    0,    0}, // state  112
    {   0,    0,  788,    0,    0}, // state  113
    { 789,   13,    0,    0,    0}, // state  114
    { 802,    1,    0,    0,    0}, // state  115
    { 803,    1,    0,    0,    0}, // state  116
    { 804,    1,    0,    0,    0}, // state  117
    { 805,    2,    0,    0,    0}, // state  118
    { 807,    1,    0,    0,    0}, // state  119
    { 808,    1,    0,    0,    0}, // state  120
    { 809,   13,    0,    0,    0}, // state  121
    { 822,   13,    0,    0,    0}, // state  122
    { 835,   13,    0,    0,    0}, // state  123
    { 848,   13,    0,    0,    0}, // state  124
    { 861,   13,    0,    0,    0}, // state  125
    { 874,    1,    0,    0,    0}, // state  126
    { 875,    2,    0,    0,    0}, // state  127
    { 877,   13,    0,    0,    0}, // state  128
    { 890,   13,    0,    0,    0}, // state  129
    { 903,   13,    0,    0,    0}, // state  130
    { 916,   13,    0,    0,    0}, // state  131
    { 929,   13,    0,    0,    0}, // state  132
    {   0,    0,  942,    0,    0}, // state  133
    { 943,    5,  948,    0,    0}, // state  134
    { 949,    3,  952,    0,    0}, // state  135
    { 953,    3,  956,    0,    0}, // state  136
    {   0,    0,  957,    0,    0}, // state  137
    {   0,    0,  958,    0,    0}, // state  138
    {   0,    0,  959,    0,    0}, // state  139
    { 960,   11,    0,  971,    1}, // state  140
    { 972,   11,    0,  983,    1}, // state  141
    { 984,   11,    0,  995,    1}, // state  142
    { 996,   11,    0, 1007,    1}, // state  143
    {1008,   11,    0, 1019,    1}, // state  144
    {1020,    6, 1026,    0,    0}, // state  145
    {1027,   11,    0, 1038,    1}, // state  146
    {1039,    6, 1045,    0,    0}, // state  147
    {1046,   13,    0,    0,    0}, // state  148
    {1059,    2,    0,    0,    0}, // state  149
    {1061,    1,    0,    0,    0}, // state  150
    {1062,    1,    0,    0,    0}, // state  151
    {1063,    1,    0,    0,    0}, // state  152
    {1064,    1,    0,    0,    0}, // state  153
    {1065,    1,    0,    0,    0}, // state  154
    {1066,    1,    0,    0,    0}, // state  155
    {1067,    1,    0,    0,    0}, // state  156
    {1068,    1,    0,    0,    0}, // state  157
    {1069,    2,    0, 1071,   11}, // state  158
    {1082,    2,    0, 1084,   14}, // state  159
    {1098,    1,    0,    0,    0}, // state  160
    {1099,    1,    0,    0,    0}, // state  161
    {1100,    1,    0,    0,    0}, // state  162
    {1101,    1,    0,    0,    0}, // state  163
    {1102,    1,    0,    0,    0}, // state  164
    {1103,    1,    0,    0,    0}, // state  165
    {1104,    1, 1105,    0,    0}, // state  166
    {1106,    1,    0,    0,    0}, // state  167
    {   0,    0, 1107,    0,    0}, // state  168
    {   0,    0, 1108,    0,    0}, // state  169
    {   0,    0, 1109,    0,    0}, // state  170
    {1110,    1,    0,    0,    0}, // state  171
    {1111,    2,    0,    0,    0}, // state  172
    {1113,   11,    0, 1124,    1}, // state  173
    {1125,    1,    0,    0,    0}, // state  174
    {   0,    0, 1126,    0,    0}, // state  175
    {   0,    0, 1127,    0,    0}, // state  176
    {   0,    0, 1128,    0,    0}, // state  177
    {   0,    0, 1129,    0,    0}, // state  178
    {   0,    0, 1130,    0,    0}, // state  179
    {   0,    0, 1131,    0,    0}, // state  180
    {   0,    0, 1132,    0,    0}, // state  181
    {1133,   11,    0, 1144,    1}, // state  182
    {   0,    0, 1145,    0,    0}, // state  183
    {   0,    0, 1146,    0,    0}, // state  184
    {   0,    0, 1147,    0,    0}, // state  185
    {   0,    0, 1148,    0,    0}, // state  186
    {   0,    0, 1149,    0,    0}, // state  187
    {1150,   10, 1160,    0,    0}, // state  188
    {1161,   11, 1172,    0,    0}, // state  189
    {1173,   12, 1185,    0,    0}, // state  190
    {1186,   10, 1196,    0,    0}, // state  191
    {1197,    8, 1205,    0,    0}, // state  192
    {1206,    8, 1214,    0,    0}, // state  193
    {1215,    1,    0,    0,    0}, // state  194
    {1216,    1,    0,    0,    0}, // state  195
    {1217,    2,    0,    0,    0}, // state  196
    {1219,   11,    0, 1230,    1}, // state  197
    {1231,    1,    0,    0,    0}, // state  198
    {   0,    0, 1232,    0,    0}, // state  199
    {1233,   11,    0, 1244,    1}, // state  200
    {   0,    0, 1245,    0,    0}, // state  201
    {   0,    0, 1246,    0,    0}, // state  202
    {1247,   11,    0, 1258,    1}, // state  203
    {   0,    0, 1259,    0,    0}, // state  204
    {1260,   27, 1287, 1288,    2}, // state  205
    {1290,   27, 1317, 1318,    2}, // state  206
    {   0,    0, 1320,    0,    0}, // state  207
    {   0,    0, 1321,    0,    0}, // state  208
    {   0,    0, 1322,    0,    0}, // state  209
    {   0,    0, 1323,    0,    0}, // state  210
    {   0,    0, 1324,    0,    0}, // state  211
    {   0,    0, 1325,    0,    0}, // state  212
    {   0,    0, 1326,    0,    0}, // state  213
    {   0,    0, 1327,    0,    0}, // state  214
    {   0,    0, 1328,    0,    0}, // state  215
    {   0,    0, 1329,    0,    0}, // state  216
    {   0,    0, 1330,    0,    0}, // state  217
    {1331,    1,    0,    0,    0}, // state  218
    {1332,    1,    0,    0,    0}, // state  219
    {1333,   13,    0,    0,    0}, // state  220
    {1346,    1,    0,    0,    0}, // state  221
    {1347,   13,    0,    0,    0}, // state  222
    {   0,    0, 1360,    0,    0}, // state  223
    {   0,    0, 1361,    0,    0}, // state  224
    {1362,    1,    0,    0,    0}, // state  225
    {1363,    1,    0,    0,    0}, // state  226
    {1364,   13,    0,    0,    0}, // state  227
    {1377,    1,    0,    0,    0}, // state  228
    {1378,   13,    0,    0,    0}, // state  229
    {1391,   13,    0,    0,    0}, // state  230
    {1404,    1,    0,    0,    0}, // state  231
    {1405,    1,    0,    0,    0}, // state  232
    {1406,    1,    0,    0,    0}, // state  233
    {1407,    1,    0,    0,    0}, // state  234
    {1408,    1,    0,    0,    0}, // state  235
    {1409,    1,    0,    0,    0}, // state  236
    {1410,    1,    0,    0,    0}, // state  237
    {1411,    1,    0,    0,    0}, // state  238
    {1412,    1,    0,    0,    0}, // state  239
    {1413,    1,    0,    0,    0}, // state  240
    {1414,    1,    0,    0,    0}, // state  241
    {1415,    1,    0,    0,    0}, // state  242
    {   0,    0, 1416,    0,    0}, // state  243
    {   0,    0, 1417,    0,    0}, // state  244
    {   0,    0, 1418,    0,    0}, // state  245
    {   0,    0, 1419,    0,    0}, // state  246
    {1420,    1,    0,    0,    0}, // state  247
    {   0,    0, 1421,    0,    0}, // state  248
    {   0,    0, 1422,    0,    0}, // state  249
    {   0,    0, 1423,    0,    0}, // state  250
    {   0,    0, 1424,    0,    0}, // state  251
    {   0,    0, 1425,    0,    0}, // state  252
    {   0,    0, 1426,    0,    0}, // state  253
    {   0,    0, 1427,    0,    0}  // state  254

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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   42}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state    5
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   46}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   47}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   48}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state    6
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,   49}},
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
    {                 Token::body__, {                  TA_PUSH_STATE,   50}},

// ///////////////////////////////////////////////////////////////////////////
// state   10
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state   11
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state   12
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   38}},

// ///////////////////////////////////////////////////////////////////////////
// state   13
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state   14
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,   52}},

// ///////////////////////////////////////////////////////////////////////////
// state   15
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state   16
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   54}},
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,   55}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   64}},

// ///////////////////////////////////////////////////////////////////////////
// state   17
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   56}},

// ///////////////////////////////////////////////////////////////////////////
// state   18
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   57}},

// ///////////////////////////////////////////////////////////////////////////
// state   19
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   58}},

// ///////////////////////////////////////////////////////////////////////////
// state   20
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state   21
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   60}},

// ///////////////////////////////////////////////////////////////////////////
// state   22
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state   23
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   62}},

// ///////////////////////////////////////////////////////////////////////////
// state   24
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   63}},

// ///////////////////////////////////////////////////////////////////////////
// state   25
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   64}},

// ///////////////////////////////////////////////////////////////////////////
// state   26
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   65}},

// ///////////////////////////////////////////////////////////////////////////
// state   27
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   66}},

// ///////////////////////////////////////////////////////////////////////////
// state   28
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   67}},

// ///////////////////////////////////////////////////////////////////////////
// state   29
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   68}},

// ///////////////////////////////////////////////////////////////////////////
// state   30
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   69}},

// ///////////////////////////////////////////////////////////////////////////
// state   31
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   70}},

// ///////////////////////////////////////////////////////////////////////////
// state   32
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   57}},

// ///////////////////////////////////////////////////////////////////////////
// state   33
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   56}},

// ///////////////////////////////////////////////////////////////////////////
// state   34
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   71}},

// ///////////////////////////////////////////////////////////////////////////
// state   35
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   72}},

// ///////////////////////////////////////////////////////////////////////////
// state   36
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   73}},

// ///////////////////////////////////////////////////////////////////////////
// state   37
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   74}},

// ///////////////////////////////////////////////////////////////////////////
// state   38
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   75}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   76}},

// ///////////////////////////////////////////////////////////////////////////
// state   40
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   77}},

// ///////////////////////////////////////////////////////////////////////////
// state   41
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state   42
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,   79}},

// ///////////////////////////////////////////////////////////////////////////
// state   43
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state   44
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   92}},

// ///////////////////////////////////////////////////////////////////////////
// state   45
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   93}},

// ///////////////////////////////////////////////////////////////////////////
// state   46
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   94}},

// ///////////////////////////////////////////////////////////////////////////
// state   47
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   95}},

// ///////////////////////////////////////////////////////////////////////////
// state   48
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   96}},

// ///////////////////////////////////////////////////////////////////////////
// state   49
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    4}},

// ///////////////////////////////////////////////////////////////////////////
// state   50
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,   97}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,   98}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {Token::conditional_series_end__, {                  TA_PUSH_STATE,   99}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {       Token::else_statement__, {                  TA_PUSH_STATE,  100}},
    {    Token::else_if_statement__, {                  TA_PUSH_STATE,  101}},
    {               Token::end_if__, {                  TA_PUSH_STATE,  102}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   51
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,  103}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  104}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {           Token::end_define__, {                  TA_PUSH_STATE,  105}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   52
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,  106}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  107}},
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
    {             Token::end_loop__, {                  TA_PUSH_STATE,  108}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   53
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,  109}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  110}},
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
    {         Token::end_for_each__, {                  TA_PUSH_STATE,  111}},

// ///////////////////////////////////////////////////////////////////////////
// state   54
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  112}},

// ///////////////////////////////////////////////////////////////////////////
// state   55
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   66}},

// ///////////////////////////////////////////////////////////////////////////
// state   56
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  113}},

// ///////////////////////////////////////////////////////////////////////////
// state   57
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  114}},

// ///////////////////////////////////////////////////////////////////////////
// state   58
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  115}},

// ///////////////////////////////////////////////////////////////////////////
// state   59
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  116}},

// ///////////////////////////////////////////////////////////////////////////
// state   60
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  117}},

// ///////////////////////////////////////////////////////////////////////////
// state   61
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  118}},

// ///////////////////////////////////////////////////////////////////////////
// state   62
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  119}},

// ///////////////////////////////////////////////////////////////////////////
// state   63
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  120}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  121}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  122}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  123}},

// ///////////////////////////////////////////////////////////////////////////
// state   67
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  124}},

// ///////////////////////////////////////////////////////////////////////////
// state   68
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  125}},

// ///////////////////////////////////////////////////////////////////////////
// state   69
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  126}},

// ///////////////////////////////////////////////////////////////////////////
// state   70
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  127}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  128}},

// ///////////////////////////////////////////////////////////////////////////
// state   72
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  129}},

// ///////////////////////////////////////////////////////////////////////////
// state   73
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  130}},

// ///////////////////////////////////////////////////////////////////////////
// state   74
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  131}},

// ///////////////////////////////////////////////////////////////////////////
// state   75
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  132}},

// ///////////////////////////////////////////////////////////////////////////
// state   76
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  133}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state   77
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   82}},

// ///////////////////////////////////////////////////////////////////////////
// state   78
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   83}},

// ///////////////////////////////////////////////////////////////////////////
// state   79
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  134}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  135}},

// ///////////////////////////////////////////////////////////////////////////
// state   82
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  136}},

// ///////////////////////////////////////////////////////////////////////////
// state   83
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  137}},

// ///////////////////////////////////////////////////////////////////////////
// state   84
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  138}},

// ///////////////////////////////////////////////////////////////////////////
// state   85
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  139}},

// ///////////////////////////////////////////////////////////////////////////
// state   86
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  140}},

// ///////////////////////////////////////////////////////////////////////////
// state   87
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,  141}},

// ///////////////////////////////////////////////////////////////////////////
// state   88
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,  142}},

// ///////////////////////////////////////////////////////////////////////////
// state   89
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  143}},

// ///////////////////////////////////////////////////////////////////////////
// state   90
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  144}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  145}},

// ///////////////////////////////////////////////////////////////////////////
// state   91
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,  146}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  147}},

// ///////////////////////////////////////////////////////////////////////////
// state   92
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  148}},

// ///////////////////////////////////////////////////////////////////////////
// state   93
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  149}},

// ///////////////////////////////////////////////////////////////////////////
// state   94
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  150}},

// ///////////////////////////////////////////////////////////////////////////
// state   95
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  151}},

// ///////////////////////////////////////////////////////////////////////////
// state   96
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state   97
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {                   Token::ELSE, {        TA_SHIFT_AND_PUSH_STATE,  152}},
    {                Token::ELSE_IF, {        TA_SHIFT_AND_PUSH_STATE,  153}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  154}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   42}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state   98
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {                   Token::ELSE, {        TA_SHIFT_AND_PUSH_STATE,  155}},
    {                Token::ELSE_IF, {        TA_SHIFT_AND_PUSH_STATE,  156}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  157}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   46}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   47}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   48}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state   99
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   25}},

// ///////////////////////////////////////////////////////////////////////////
// state  100
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,  158}},

// ///////////////////////////////////////////////////////////////////////////
// state  101
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::TEXT, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {                 Token::body__, {                  TA_PUSH_STATE,  159}},

// ///////////////////////////////////////////////////////////////////////////
// state  102
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   26}},

// ///////////////////////////////////////////////////////////////////////////
// state  103
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   22}},
    {             Token::END_DEFINE, {        TA_SHIFT_AND_PUSH_STATE,  160}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   42}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state  104
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {             Token::END_DEFINE, {        TA_SHIFT_AND_PUSH_STATE,  161}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   46}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   47}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   48}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state  105
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    7}},

// ///////////////////////////////////////////////////////////////////////////
// state  106
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
    {               Token::END_LOOP, {        TA_SHIFT_AND_PUSH_STATE,  162}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   42}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state  107
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   46}},
    {               Token::END_LOOP, {        TA_SHIFT_AND_PUSH_STATE,  163}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   47}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   48}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state  108
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    8}},

// ///////////////////////////////////////////////////////////////////////////
// state  109
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
    {           Token::END_FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,  164}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   42}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state  110
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   46}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   47}},
    {           Token::END_FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,  165}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   48}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state  111
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state  112
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  166}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  113
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state  114
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  167}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  115
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  168}},

// ///////////////////////////////////////////////////////////////////////////
// state  116
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  169}},

// ///////////////////////////////////////////////////////////////////////////
// state  117
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  170}},

// ///////////////////////////////////////////////////////////////////////////
// state  118
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  171}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,  172}},

// ///////////////////////////////////////////////////////////////////////////
// state  119
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  173}},

// ///////////////////////////////////////////////////////////////////////////
// state  120
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  174}},

// ///////////////////////////////////////////////////////////////////////////
// state  121
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  175}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  122
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  176}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  123
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  177}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  124
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  178}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  125
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  179}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  126
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  180}},

// ///////////////////////////////////////////////////////////////////////////
// state  127
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  181}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,  182}},

// ///////////////////////////////////////////////////////////////////////////
// state  128
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  183}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  129
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  184}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  130
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  185}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  131
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  186}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  132
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  187}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  133
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   84}},

// ///////////////////////////////////////////////////////////////////////////
// state  134
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   68}},

// ///////////////////////////////////////////////////////////////////////////
// state  135
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   77}},

// ///////////////////////////////////////////////////////////////////////////
// state  136
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state  137
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   79}},

// ///////////////////////////////////////////////////////////////////////////
// state  138
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   80}},

// ///////////////////////////////////////////////////////////////////////////
// state  139
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   81}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  188}},

// ///////////////////////////////////////////////////////////////////////////
// state  141
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  189}},

// ///////////////////////////////////////////////////////////////////////////
// state  142
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  190}},

// ///////////////////////////////////////////////////////////////////////////
// state  143
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  191}},

// ///////////////////////////////////////////////////////////////////////////
// state  144
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  192}},

// ///////////////////////////////////////////////////////////////////////////
// state  145
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   73}},

// ///////////////////////////////////////////////////////////////////////////
// state  146
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  193}},

// ///////////////////////////////////////////////////////////////////////////
// state  147
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   75}},

// ///////////////////////////////////////////////////////////////////////////
// state  148
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  194}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  149
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  195}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,  196}},

// ///////////////////////////////////////////////////////////////////////////
// state  150
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  197}},

// ///////////////////////////////////////////////////////////////////////////
// state  151
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,  198}},

// ///////////////////////////////////////////////////////////////////////////
// state  152
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  199}},

// ///////////////////////////////////////////////////////////////////////////
// state  153
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,  200}},

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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,  203}},

// ///////////////////////////////////////////////////////////////////////////
// state  157
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  204}},

// ///////////////////////////////////////////////////////////////////////////
// state  158
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,  205}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,  206}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {               Token::end_if__, {                  TA_PUSH_STATE,  207}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state  159
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {             Token::START_CODE, {        TA_SHIFT_AND_PUSH_STATE,   97}},
    {              Token::CODE_LINE, {        TA_SHIFT_AND_PUSH_STATE,   98}},
    // nonterminal transitions
    {           Token::executable__, {                  TA_PUSH_STATE,    6}},
    {                 Token::code__, {                  TA_PUSH_STATE,    7}},
    {   Token::conditional_series__, {                  TA_PUSH_STATE,    8}},
    {Token::conditional_series_end__, {                  TA_PUSH_STATE,  208}},
    {         Token::if_statement__, {                  TA_PUSH_STATE,    9}},
    {       Token::else_statement__, {                  TA_PUSH_STATE,  100}},
    {    Token::else_if_statement__, {                  TA_PUSH_STATE,  101}},
    {               Token::end_if__, {                  TA_PUSH_STATE,  102}},
    {               Token::define__, {                  TA_PUSH_STATE,   10}},
    {        Token::define_scalar__, {                  TA_PUSH_STATE,   11}},
    { Token::define_array_element__, {                  TA_PUSH_STATE,   12}},
    {   Token::define_map_element__, {                  TA_PUSH_STATE,   13}},
    {                 Token::loop__, {                  TA_PUSH_STATE,   14}},
    {             Token::for_each__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state  160
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  209}},

// ///////////////////////////////////////////////////////////////////////////
// state  161
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  210}},

// ///////////////////////////////////////////////////////////////////////////
// state  162
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  211}},

// ///////////////////////////////////////////////////////////////////////////
// state  163
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  212}},

// ///////////////////////////////////////////////////////////////////////////
// state  164
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  213}},

// ///////////////////////////////////////////////////////////////////////////
// state  165
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  214}},

// ///////////////////////////////////////////////////////////////////////////
// state  166
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,  215}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   65}},

// ///////////////////////////////////////////////////////////////////////////
// state  167
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  216}},

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
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  217}},

// ///////////////////////////////////////////////////////////////////////////
// state  172
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,  218}},
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  219}},

// ///////////////////////////////////////////////////////////////////////////
// state  173
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  220}},

// ///////////////////////////////////////////////////////////////////////////
// state  174
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  221}},

// ///////////////////////////////////////////////////////////////////////////
// state  175
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state  176
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   21}},

// ///////////////////////////////////////////////////////////////////////////
// state  177
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state  178
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   23}},

// ///////////////////////////////////////////////////////////////////////////
// state  179
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   24}},

// ///////////////////////////////////////////////////////////////////////////
// state  180
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   58}},

// ///////////////////////////////////////////////////////////////////////////
// state  181
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   62}},

// ///////////////////////////////////////////////////////////////////////////
// state  182
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  222}},

// ///////////////////////////////////////////////////////////////////////////
// state  183
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state  184
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   60}},

// ///////////////////////////////////////////////////////////////////////////
// state  185
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state  186
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state  187
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state  188
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   72}},

// ///////////////////////////////////////////////////////////////////////////
// state  189
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   70}},

// ///////////////////////////////////////////////////////////////////////////
// state  190
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   69}},

// ///////////////////////////////////////////////////////////////////////////
// state  191
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   71}},

// ///////////////////////////////////////////////////////////////////////////
// state  192
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   74}},

// ///////////////////////////////////////////////////////////////////////////
// state  193
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   76}},

// ///////////////////////////////////////////////////////////////////////////
// state  194
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  223}},

// ///////////////////////////////////////////////////////////////////////////
// state  195
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  224}},

// ///////////////////////////////////////////////////////////////////////////
// state  196
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,  225}},
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  226}},

// ///////////////////////////////////////////////////////////////////////////
// state  197
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  227}},

// ///////////////////////////////////////////////////////////////////////////
// state  198
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,  228}},

// ///////////////////////////////////////////////////////////////////////////
// state  199
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   31}},

// ///////////////////////////////////////////////////////////////////////////
// state  200
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  229}},

// ///////////////////////////////////////////////////////////////////////////
// state  201
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   35}},

// ///////////////////////////////////////////////////////////////////////////
// state  202
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   32}},

// ///////////////////////////////////////////////////////////////////////////
// state  203
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
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // nonterminal transitions
    {           Token::expression__, {                  TA_PUSH_STATE,  230}},

// ///////////////////////////////////////////////////////////////////////////
// state  204
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   36}},

// ///////////////////////////////////////////////////////////////////////////
// state  205
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  154}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   42}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state  206
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {      Token::DUMP_SYMBOL_TABLE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    {                     Token::IF, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {                 Token::END_IF, {        TA_SHIFT_AND_PUSH_STATE,  157}},
    {               Token::UNDEFINE, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {          Token::DECLARE_ARRAY, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {            Token::DECLARE_MAP, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {                 Token::DEFINE, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                   Token::LOOP, {        TA_SHIFT_AND_PUSH_STATE,   46}},
    {               Token::FOR_EACH, {        TA_SHIFT_AND_PUSH_STATE,   47}},
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
    {   Token::TO_CHARACTER_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {      Token::TO_STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   41}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},
    // nonterminal transitions
    {            Token::code_body__, {                  TA_PUSH_STATE,   48}},
    {           Token::expression__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state  207
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   27}},

// ///////////////////////////////////////////////////////////////////////////
// state  208
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state  209
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   46}},

// ///////////////////////////////////////////////////////////////////////////
// state  210
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   47}},

// ///////////////////////////////////////////////////////////////////////////
// state  211
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   50}},

// ///////////////////////////////////////////////////////////////////////////
// state  212
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state  213
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   54}},

// ///////////////////////////////////////////////////////////////////////////
// state  214
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   55}},

// ///////////////////////////////////////////////////////////////////////////
// state  215
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   67}},

// ///////////////////////////////////////////////////////////////////////////
// state  216
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   29}},

// ///////////////////////////////////////////////////////////////////////////
// state  217
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state  218
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  231}},

// ///////////////////////////////////////////////////////////////////////////
// state  219
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  232}},

// ///////////////////////////////////////////////////////////////////////////
// state  220
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  233}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  221
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  234}},

// ///////////////////////////////////////////////////////////////////////////
// state  222
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  235}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  223
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   30}},

// ///////////////////////////////////////////////////////////////////////////
// state  224
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state  225
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  236}},

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
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  228
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  239}},

// ///////////////////////////////////////////////////////////////////////////
// state  229
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  240}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  230
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  241}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   81}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('/'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('%'), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type('!'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('&'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type('='), {        TA_SHIFT_AND_PUSH_STATE,   89}},
    {              Token::Type('<'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    {              Token::Type('>'), {        TA_SHIFT_AND_PUSH_STATE,   91}},

// ///////////////////////////////////////////////////////////////////////////
// state  231
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  242}},

// ///////////////////////////////////////////////////////////////////////////
// state  232
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  243}},

// ///////////////////////////////////////////////////////////////////////////
// state  233
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  244}},

// ///////////////////////////////////////////////////////////////////////////
// state  234
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  245}},

// ///////////////////////////////////////////////////////////////////////////
// state  235
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  246}},

// ///////////////////////////////////////////////////////////////////////////
// state  236
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,  247}},

// ///////////////////////////////////////////////////////////////////////////
// state  237
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  248}},

// ///////////////////////////////////////////////////////////////////////////
// state  238
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  249}},

// ///////////////////////////////////////////////////////////////////////////
// state  239
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  250}},

// ///////////////////////////////////////////////////////////////////////////
// state  240
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  251}},

// ///////////////////////////////////////////////////////////////////////////
// state  241
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  252}},

// ///////////////////////////////////////////////////////////////////////////
// state  242
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {               Token::END_CODE, {        TA_SHIFT_AND_PUSH_STATE,  253}},

// ///////////////////////////////////////////////////////////////////////////
// state  243
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   42}},

// ///////////////////////////////////////////////////////////////////////////
// state  244
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   48}},

// ///////////////////////////////////////////////////////////////////////////
// state  245
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},

// ///////////////////////////////////////////////////////////////////////////
// state  246
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   63}},

// ///////////////////////////////////////////////////////////////////////////
// state  247
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CODE_NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,  254}},

// ///////////////////////////////////////////////////////////////////////////
// state  248
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state  249
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   49}},

// ///////////////////////////////////////////////////////////////////////////
// state  250
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state  251
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   33}},

// ///////////////////////////////////////////////////////////////////////////
// state  252
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state  253
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state  254
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   45}}

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

#line 4709 "barf_preprocessor_parser.cpp"

