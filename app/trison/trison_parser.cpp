#include "trison_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x


#line 50 "trison_parser.trison"

#include <sstream>

#include "trison_ast.hpp"
#include "trison_scanner.hpp"

namespace Trison {

#undef FL
#define FL FiLoc(m_scanner->GetInputFilename(), m_scanner->GetLineNumber())

#line 25 "trison_parser.cpp"

Parser::Parser ()

{

#line 62 "trison_parser.trison"

    m_scanner = new Scanner();

#line 35 "trison_parser.cpp"
    m_debug_spew_level = 0;
    DEBUG_SPEW_2("### number of state transitions = " << ms_state_transition_count << std::endl);
    m_reduction_token = NULL;
}

Parser::~Parser ()
{

#line 66 "trison_parser.trison"

    delete m_scanner;

#line 48 "trison_parser.cpp"
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

#line 70 "trison_parser.trison"

    delete token;

#line 410 "trison_parser.cpp"
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
        "DIRECTIVE_ERROR",
        "DIRECTIVE_PARSER_WITHOUT_PARAMETER",
        "DIRECTIVE_PARSER_WITH_PARAMETER",
        "DIRECTIVE_TOKEN",
        "DUMB_CODE_BLOCK",
        "END_PREAMBLE",
        "ID",
        "LEFT",
        "NEWLINE",
        "NONASSOC",
        "PREC",
        "RIGHT",
        "START",
        "STRICT_CODE_BLOCK",
        "STRING",
        "TOKEN_ID_CHAR",
        "TYPE",
        "END_",

        "at_least_one_newline",
        "at_least_zero_newlines",
        "directive",
        "directive_list",
        "nonterminal",
        "nonterminal_list",
        "nonterminal_specification",
        "root",
        "rule",
        "rule_list",
        "rule_precedence_directive",
        "rule_specification",
        "rule_token",
        "rule_token_list",
        "token_id",
        "token_id_list",
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
Ast::Base * Parser::ReductionRuleHandler0000 ()
{
    assert(0 < m_reduction_rule_token_count);
    return m_token_stack[m_token_stack.size() - m_reduction_rule_token_count];

    return NULL;
}

// rule 1: root <- directive_list:directive_list END_PREAMBLE:end_preamble nonterminal_list:nonterminal_list    
Ast::Base * Parser::ReductionRuleHandler0001 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DirectiveList * directive_list = Dsc< Ast::DirectiveList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * end_preamble = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    NonterminalList * nonterminal_list = Dsc< NonterminalList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 122 "trison_parser.trison"

        Grammar *grammar =
            new Grammar(
                directive_list,
                end_preamble->GetFiLoc(),
                nonterminal_list);
        delete end_preamble;
        return grammar;
    
#line 520 "trison_parser.cpp"
    return NULL;
}

// rule 2: directive_list <- directive_list:directive_list directive:directive    
Ast::Base * Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DirectiveList * directive_list = Dsc< Ast::DirectiveList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Directive * directive = Dsc< Ast::Directive * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 136 "trison_parser.trison"

        if (directive != NULL)
            directive_list->Append(directive);
        return directive_list;
    
#line 538 "trison_parser.cpp"
    return NULL;
}

// rule 3: directive_list <- at_least_zero_newlines    
Ast::Base * Parser::ReductionRuleHandler0003 ()
{

#line 143 "trison_parser.trison"

        return new Ast::DirectiveList();
    
#line 550 "trison_parser.cpp"
    return NULL;
}

// rule 4: directive <- DIRECTIVE_PARSER_WITH_PARAMETER:parser_directive STRICT_CODE_BLOCK:value at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0004 ()
{
    assert(0 < m_reduction_rule_token_count);
    ParserDirective * parser_directive = Dsc< ParserDirective * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * value = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 151 "trison_parser.trison"

        parser_directive->SetValue(value);
        return parser_directive;
    
#line 567 "trison_parser.cpp"
    return NULL;
}

// rule 5: directive <- DIRECTIVE_PARSER_WITH_PARAMETER:parser_directive DUMB_CODE_BLOCK:value at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0005 ()
{
    assert(0 < m_reduction_rule_token_count);
    ParserDirective * parser_directive = Dsc< ParserDirective * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * value = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 157 "trison_parser.trison"

        if (!ParserDirective::GetDoesParserDirectiveAcceptDumbCodeBlock(
                parser_directive->GetParserDirectiveType()))
        {
            ostringstream out;
            out << ParserDirective::GetString(parser_directive->GetParserDirectiveType())
                << " does not accept %{ %} code blocks -- use { }";
            EmitError(parser_directive->GetFiLoc(), out.str());
            return NULL;
        }

        parser_directive->SetValue(value);
        return parser_directive;
    
#line 594 "trison_parser.cpp"
    return NULL;
}

// rule 6: directive <- DIRECTIVE_PARSER_WITH_PARAMETER:parser_directive STRING:value at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0006 ()
{
    assert(0 < m_reduction_rule_token_count);
    ParserDirective * parser_directive = Dsc< ParserDirective * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::String * value = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 173 "trison_parser.trison"

        parser_directive->SetValue(value);
        return parser_directive;
    
#line 611 "trison_parser.cpp"
    return NULL;
}

// rule 7: directive <- DIRECTIVE_PARSER_WITHOUT_PARAMETER:parser_directive at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0007 ()
{
    assert(0 < m_reduction_rule_token_count);
    ParserDirective * parser_directive = Dsc< ParserDirective * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 179 "trison_parser.trison"

        return parser_directive;
    
#line 625 "trison_parser.cpp"
    return NULL;
}

// rule 8: directive <- DIRECTIVE_TOKEN:throwaway token_id_list:token_id_list at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0008 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    TokenIdList * token_id_list = Dsc< TokenIdList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 184 "trison_parser.trison"

        delete throwaway;
        return new TokenDirective(token_id_list, NULL);
    
#line 642 "trison_parser.cpp"
    return NULL;
}

// rule 9: directive <- DIRECTIVE_TOKEN:throwaway1 token_id_list:token_id_list TYPE:throwaway2 STRING:assigned_type at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0009 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway1 = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    TokenIdList * token_id_list = Dsc< TokenIdList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway2 = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(3 < m_reduction_rule_token_count);
    Ast::String * assigned_type = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 190 "trison_parser.trison"

        delete throwaway1;
        delete throwaway2;
        return new TokenDirective(token_id_list, assigned_type);
    
#line 664 "trison_parser.cpp"
    return NULL;
}

// rule 10: directive <- DIRECTIVE_TOKEN:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0010 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 197 "trison_parser.trison"

        EmitError(throwaway->GetFiLoc(), "syntax error in %token directive");

        delete throwaway;
        return new TokenDirective(new TokenIdList(), NULL);
    
#line 681 "trison_parser.cpp"
    return NULL;
}

// rule 11: directive <- DIRECTIVE_TOKEN:throwaway1 token_id_list:token_id_list TYPE:throwaway2 %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0011 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway1 = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    TokenIdList * token_id_list = Dsc< TokenIdList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway2 = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 205 "trison_parser.trison"

        EmitError(throwaway2->GetFiLoc(), "syntax error in %type directive");

        Ast::String *dummy_string = new Ast::String(throwaway2->GetFiLoc());
        delete throwaway1;
        delete throwaway2;
        return new TokenDirective(token_id_list, dummy_string);
    
#line 704 "trison_parser.cpp"
    return NULL;
}

// rule 12: directive <- PREC:throwaway ID:id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0012 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 215 "trison_parser.trison"

        delete throwaway;
        return new PrecedenceDirective(id);
    
#line 721 "trison_parser.cpp"
    return NULL;
}

// rule 13: directive <- PREC:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0013 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 221 "trison_parser.trison"

        EmitError(throwaway->GetFiLoc(), "syntax error in %prec directive");

        Ast::Id *dummy_id = new Ast::Id("ERROR", throwaway->GetFiLoc());
        delete throwaway;
        return new PrecedenceDirective(dummy_id);
    
#line 739 "trison_parser.cpp"
    return NULL;
}

// rule 14: directive <- START:throwaway ID:id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0014 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 230 "trison_parser.trison"

        delete throwaway;
        return new StartDirective(id);
    
#line 756 "trison_parser.cpp"
    return NULL;
}

// rule 15: directive <- START:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0015 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 236 "trison_parser.trison"

        EmitError(throwaway->GetFiLoc(), "syntax error in %start directive");

        Ast::Id *dummy_id = new Ast::Id("ERROR", throwaway->GetFiLoc());
        delete throwaway;
        return new StartDirective(dummy_id);
    
#line 774 "trison_parser.cpp"
    return NULL;
}

// rule 16: directive <- %error STRICT_CODE_BLOCK:value at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0016 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * value = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 245 "trison_parser.trison"

        EmitError(value->GetFiLoc(), "syntax error in directive");

        return NULL;
    
#line 790 "trison_parser.cpp"
    return NULL;
}

// rule 17: directive <- %error DUMB_CODE_BLOCK:value at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0017 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * value = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 252 "trison_parser.trison"

        EmitError(value->GetFiLoc(), "syntax error in directive");

        return NULL;
    
#line 806 "trison_parser.cpp"
    return NULL;
}

// rule 18: directive <- %error STRING:value at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0018 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::String * value = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 259 "trison_parser.trison"

        EmitError(value->GetFiLoc(), "syntax error in directive");

        return NULL;
    
#line 822 "trison_parser.cpp"
    return NULL;
}

// rule 19: directive <- %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0019 ()
{

#line 266 "trison_parser.trison"

        EmitError(FL, "syntax error in directive");

        return NULL;
    
#line 836 "trison_parser.cpp"
    return NULL;
}

// rule 20: token_id_list <- token_id_list:token_id_list token_id:token_id    
Ast::Base * Parser::ReductionRuleHandler0020 ()
{
    assert(0 < m_reduction_rule_token_count);
    TokenIdList * token_id_list = Dsc< TokenIdList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    TokenId * token_id = Dsc< TokenId * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 276 "trison_parser.trison"

        token_id_list->Append(token_id);
        return token_id_list;
    
#line 853 "trison_parser.cpp"
    return NULL;
}

// rule 21: token_id_list <- token_id:token_id    
Ast::Base * Parser::ReductionRuleHandler0021 ()
{
    assert(0 < m_reduction_rule_token_count);
    TokenId * token_id = Dsc< TokenId * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 282 "trison_parser.trison"

        TokenIdList *token_id_list = new TokenIdList();
        token_id_list->Append(token_id);
        return token_id_list;
    
#line 869 "trison_parser.cpp"
    return NULL;
}

// rule 22: token_id <- ID:id    
Ast::Base * Parser::ReductionRuleHandler0022 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 292 "trison_parser.trison"

        TokenIdId *token_id_id =
            new TokenIdId(id->GetText(), id->GetFiLoc());
        delete id;
        return token_id_id;
    
#line 886 "trison_parser.cpp"
    return NULL;
}

// rule 23: token_id <- TOKEN_ID_CHAR:token_id_char    
Ast::Base * Parser::ReductionRuleHandler0023 ()
{
    assert(0 < m_reduction_rule_token_count);
    TokenIdChar * token_id_char = Dsc< TokenIdChar * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 300 "trison_parser.trison"

        return token_id_char;
    
#line 900 "trison_parser.cpp"
    return NULL;
}

// rule 24: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0024 ()
{
    return NULL;
}

// rule 25: at_least_zero_newlines <-     
Ast::Base * Parser::ReductionRuleHandler0025 ()
{
    return NULL;
}

// rule 26: at_least_one_newline <- at_least_one_newline NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0026 ()
{
    return NULL;
}

// rule 27: at_least_one_newline <- NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0027 ()
{
    return NULL;
}

// rule 28: nonterminal_list <- nonterminal_list:nonterminal_list nonterminal:nonterminal    
Ast::Base * Parser::ReductionRuleHandler0028 ()
{
    assert(0 < m_reduction_rule_token_count);
    NonterminalList * nonterminal_list = Dsc< NonterminalList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Nonterminal * nonterminal = Dsc< Nonterminal * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 322 "trison_parser.trison"

        // the nonterminal could be null if an error was recovered from
        if (nonterminal != NULL)
            nonterminal_list->Append(nonterminal);
        return nonterminal_list;
    
#line 943 "trison_parser.cpp"
    return NULL;
}

// rule 29: nonterminal_list <-     
Ast::Base * Parser::ReductionRuleHandler0029 ()
{

#line 330 "trison_parser.trison"

        return new NonterminalList();
    
#line 955 "trison_parser.cpp"
    return NULL;
}

// rule 30: nonterminal <- nonterminal_specification:nonterminal_specification ':' rule_list:rule_list ';'    
Ast::Base * Parser::ReductionRuleHandler0030 ()
{
    assert(0 < m_reduction_rule_token_count);
    Nonterminal * nonterminal_specification = Dsc< Nonterminal * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 338 "trison_parser.trison"

        nonterminal_specification->SetRuleList(rule_list);
        return nonterminal_specification;
    
#line 972 "trison_parser.cpp"
    return NULL;
}

// rule 31: nonterminal <- %error ';'    
Ast::Base * Parser::ReductionRuleHandler0031 ()
{

#line 344 "trison_parser.trison"

        EmitError(FL, "syntax error in nonterminal definition");

        return NULL;
    
#line 986 "trison_parser.cpp"
    return NULL;
}

// rule 32: nonterminal_specification <- ID:id TYPE:throwaway STRING:assigned_type    
Ast::Base * Parser::ReductionRuleHandler0032 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::String * assigned_type = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 354 "trison_parser.trison"

        delete throwaway;
        return new Nonterminal(id, assigned_type);
    
#line 1005 "trison_parser.cpp"
    return NULL;
}

// rule 33: nonterminal_specification <- ID:id    
Ast::Base * Parser::ReductionRuleHandler0033 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 360 "trison_parser.trison"

        return new Nonterminal(id, NULL);
    
#line 1019 "trison_parser.cpp"
    return NULL;
}

// rule 34: nonterminal_specification <- %error    
Ast::Base * Parser::ReductionRuleHandler0034 ()
{

#line 365 "trison_parser.trison"

        EmitError(FL, "syntax error while parsing nonterminal specification");

        return NULL;
    
#line 1033 "trison_parser.cpp"
    return NULL;
}

// rule 35: nonterminal_specification <- ID:id %error    
Ast::Base * Parser::ReductionRuleHandler0035 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 372 "trison_parser.trison"

        EmitError(id->GetFiLoc(), "syntax error in nonterminal directive");

        return new Nonterminal(id, NULL);
    
#line 1049 "trison_parser.cpp"
    return NULL;
}

// rule 36: nonterminal_specification <- ID:id TYPE:throwaway %error    
Ast::Base * Parser::ReductionRuleHandler0036 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 379 "trison_parser.trison"

        EmitError(id->GetFiLoc(), "syntax error in nonterminal %type directive; was expecting a string");

        delete throwaway;
        return new Nonterminal(id, NULL);
    
#line 1068 "trison_parser.cpp"
    return NULL;
}

// rule 37: rule_list <- rule_list:rule_list '|' rule:rule    
Ast::Base * Parser::ReductionRuleHandler0037 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 390 "trison_parser.trison"

        rule_list->Append(rule);
        return rule_list;
    
#line 1085 "trison_parser.cpp"
    return NULL;
}

// rule 38: rule_list <- rule:rule    
Ast::Base * Parser::ReductionRuleHandler0038 ()
{
    assert(0 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 396 "trison_parser.trison"

        RuleList *rule_list = new RuleList();
        rule_list->Append(rule);
        return rule_list;
    
#line 1101 "trison_parser.cpp"
    return NULL;
}

// rule 39: rule <- rule_specification:rule_specification STRICT_CODE_BLOCK:code_block    
Ast::Base * Parser::ReductionRuleHandler0039 ()
{
    assert(0 < m_reduction_rule_token_count);
    Rule * rule_specification = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * code_block = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 406 "trison_parser.trison"

        rule_specification->SetCodeBlock(code_block);
        return rule_specification;
    
#line 1118 "trison_parser.cpp"
    return NULL;
}

// rule 40: rule <- rule_specification:rule_specification    
Ast::Base * Parser::ReductionRuleHandler0040 ()
{
    assert(0 < m_reduction_rule_token_count);
    Rule * rule_specification = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 412 "trison_parser.trison"

        return rule_specification;
    
#line 1132 "trison_parser.cpp"
    return NULL;
}

// rule 41: rule <- rule_specification:rule_specification DUMB_CODE_BLOCK:code_block    
Ast::Base * Parser::ReductionRuleHandler0041 ()
{
    assert(0 < m_reduction_rule_token_count);
    Rule * rule_specification = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * code_block = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 417 "trison_parser.trison"

        EmitError(
            code_block->GetFiLoc(),
            "rules do not accept %{ %} code blocks -- use { } instead");

        rule_specification->SetCodeBlock(code_block);
        return rule_specification;
    
#line 1153 "trison_parser.cpp"
    return NULL;
}

// rule 42: rule_specification <- rule_token_list:rule_token_list LEFT:throwaway rule_precedence_directive:rule_precedence_directive    
Ast::Base * Parser::ReductionRuleHandler0042 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleTokenList * rule_token_list = Dsc< RuleTokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * rule_precedence_directive = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 430 "trison_parser.trison"

        delete throwaway;
        return new Rule(rule_token_list, A_LEFT, rule_precedence_directive);
    
#line 1172 "trison_parser.cpp"
    return NULL;
}

// rule 43: rule_specification <- rule_token_list:rule_token_list RIGHT:throwaway rule_precedence_directive:rule_precedence_directive    
Ast::Base * Parser::ReductionRuleHandler0043 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleTokenList * rule_token_list = Dsc< RuleTokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * rule_precedence_directive = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 436 "trison_parser.trison"

        delete throwaway;
        return new Rule(rule_token_list, A_RIGHT, rule_precedence_directive);
    
#line 1191 "trison_parser.cpp"
    return NULL;
}

// rule 44: rule_specification <- rule_token_list:rule_token_list NONASSOC:throwaway rule_precedence_directive:rule_precedence_directive    
Ast::Base * Parser::ReductionRuleHandler0044 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleTokenList * rule_token_list = Dsc< RuleTokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * rule_precedence_directive = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 442 "trison_parser.trison"

        delete throwaway;
        return new Rule(rule_token_list, A_NONASSOC, rule_precedence_directive);
    
#line 1210 "trison_parser.cpp"
    return NULL;
}

// rule 45: rule_specification <- rule_token_list:rule_token_list rule_precedence_directive:rule_precedence_directive    
Ast::Base * Parser::ReductionRuleHandler0045 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleTokenList * rule_token_list = Dsc< RuleTokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * rule_precedence_directive = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 448 "trison_parser.trison"

        // the default associativity when none is specified is LEFT
        return new Rule(rule_token_list, A_LEFT, rule_precedence_directive);
    
#line 1227 "trison_parser.cpp"
    return NULL;
}

// rule 46: rule_token_list <- rule_token_list:rule_token_list rule_token:rule_token    
Ast::Base * Parser::ReductionRuleHandler0046 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleTokenList * rule_token_list = Dsc< RuleTokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    RuleToken * rule_token = Dsc< RuleToken * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 457 "trison_parser.trison"

        rule_token_list->Append(rule_token);
        return rule_token_list;
    
#line 1244 "trison_parser.cpp"
    return NULL;
}

// rule 47: rule_token_list <-     
Ast::Base * Parser::ReductionRuleHandler0047 ()
{

#line 463 "trison_parser.trison"

        // it is necessary to explicitly assign the file location here because
        // the rule token list may be empty (and thus wouldn't take its file
        // location from the first appended list element).
        return new RuleTokenList(FL);
    
#line 1259 "trison_parser.cpp"
    return NULL;
}

// rule 48: rule_token <- token_id:token_id ':' ID:assigned_id    
Ast::Base * Parser::ReductionRuleHandler0048 ()
{
    assert(0 < m_reduction_rule_token_count);
    TokenId * token_id = Dsc< TokenId * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * assigned_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 474 "trison_parser.trison"

        return new RuleToken(token_id, assigned_id);
    
#line 1275 "trison_parser.cpp"
    return NULL;
}

// rule 49: rule_token <- token_id:token_id    
Ast::Base * Parser::ReductionRuleHandler0049 ()
{
    assert(0 < m_reduction_rule_token_count);
    TokenId * token_id = Dsc< TokenId * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 479 "trison_parser.trison"

        return new RuleToken(token_id, NULL);
    
#line 1289 "trison_parser.cpp"
    return NULL;
}

// rule 50: rule_token <- DIRECTIVE_ERROR:throwaway    
Ast::Base * Parser::ReductionRuleHandler0050 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 484 "trison_parser.trison"

        delete throwaway;
        return new RuleToken(new TokenIdId("%error", FL), NULL);
    
#line 1304 "trison_parser.cpp"
    return NULL;
}

// rule 51: rule_precedence_directive <- PREC:throwaway ID:id    
Ast::Base * Parser::ReductionRuleHandler0051 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 493 "trison_parser.trison"

        delete throwaway;
        return id;
    
#line 1321 "trison_parser.cpp"
    return NULL;
}

// rule 52: rule_precedence_directive <-     
Ast::Base * Parser::ReductionRuleHandler0052 ()
{

#line 499 "trison_parser.trison"

        return NULL;
    
#line 1333 "trison_parser.cpp"
    return NULL;
}



// ///////////////////////////////////////////////////////////////////////////
// reduction rule lookup table
// ///////////////////////////////////////////////////////////////////////////

Parser::ReductionRule const Parser::ms_reduction_rule[] =
{
    {                 Token::START_,  2, &Parser::ReductionRuleHandler0000, "rule 0: %start <- root END_    "},
    {                 Token::root__,  3, &Parser::ReductionRuleHandler0001, "rule 1: root <- directive_list END_PREAMBLE nonterminal_list    "},
    {       Token::directive_list__,  2, &Parser::ReductionRuleHandler0002, "rule 2: directive_list <- directive_list directive    "},
    {       Token::directive_list__,  1, &Parser::ReductionRuleHandler0003, "rule 3: directive_list <- at_least_zero_newlines    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0004, "rule 4: directive <- DIRECTIVE_PARSER_WITH_PARAMETER STRICT_CODE_BLOCK at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0005, "rule 5: directive <- DIRECTIVE_PARSER_WITH_PARAMETER DUMB_CODE_BLOCK at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0006, "rule 6: directive <- DIRECTIVE_PARSER_WITH_PARAMETER STRING at_least_one_newline    "},
    {            Token::directive__,  2, &Parser::ReductionRuleHandler0007, "rule 7: directive <- DIRECTIVE_PARSER_WITHOUT_PARAMETER at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0008, "rule 8: directive <- DIRECTIVE_TOKEN token_id_list at_least_one_newline    "},
    {            Token::directive__,  5, &Parser::ReductionRuleHandler0009, "rule 9: directive <- DIRECTIVE_TOKEN token_id_list TYPE STRING at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0010, "rule 10: directive <- DIRECTIVE_TOKEN %error at_least_one_newline    "},
    {            Token::directive__,  5, &Parser::ReductionRuleHandler0011, "rule 11: directive <- DIRECTIVE_TOKEN token_id_list TYPE %error at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0012, "rule 12: directive <- PREC ID at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0013, "rule 13: directive <- PREC %error at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0014, "rule 14: directive <- START ID at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0015, "rule 15: directive <- START %error at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0016, "rule 16: directive <- %error STRICT_CODE_BLOCK at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0017, "rule 17: directive <- %error DUMB_CODE_BLOCK at_least_one_newline    "},
    {            Token::directive__,  3, &Parser::ReductionRuleHandler0018, "rule 18: directive <- %error STRING at_least_one_newline    "},
    {            Token::directive__,  2, &Parser::ReductionRuleHandler0019, "rule 19: directive <- %error at_least_one_newline    "},
    {        Token::token_id_list__,  2, &Parser::ReductionRuleHandler0020, "rule 20: token_id_list <- token_id_list token_id    "},
    {        Token::token_id_list__,  1, &Parser::ReductionRuleHandler0021, "rule 21: token_id_list <- token_id    "},
    {             Token::token_id__,  1, &Parser::ReductionRuleHandler0022, "rule 22: token_id <- ID    "},
    {             Token::token_id__,  1, &Parser::ReductionRuleHandler0023, "rule 23: token_id <- TOKEN_ID_CHAR    "},
    {Token::at_least_zero_newlines__,  2, &Parser::ReductionRuleHandler0024, "rule 24: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    "},
    {Token::at_least_zero_newlines__,  0, &Parser::ReductionRuleHandler0025, "rule 25: at_least_zero_newlines <-     "},
    { Token::at_least_one_newline__,  2, &Parser::ReductionRuleHandler0026, "rule 26: at_least_one_newline <- at_least_one_newline NEWLINE    "},
    { Token::at_least_one_newline__,  1, &Parser::ReductionRuleHandler0027, "rule 27: at_least_one_newline <- NEWLINE    "},
    {     Token::nonterminal_list__,  2, &Parser::ReductionRuleHandler0028, "rule 28: nonterminal_list <- nonterminal_list nonterminal    "},
    {     Token::nonterminal_list__,  0, &Parser::ReductionRuleHandler0029, "rule 29: nonterminal_list <-     "},
    {          Token::nonterminal__,  4, &Parser::ReductionRuleHandler0030, "rule 30: nonterminal <- nonterminal_specification ':' rule_list ';'    "},
    {          Token::nonterminal__,  2, &Parser::ReductionRuleHandler0031, "rule 31: nonterminal <- %error ';'    "},
    {Token::nonterminal_specification__,  3, &Parser::ReductionRuleHandler0032, "rule 32: nonterminal_specification <- ID TYPE STRING    "},
    {Token::nonterminal_specification__,  1, &Parser::ReductionRuleHandler0033, "rule 33: nonterminal_specification <- ID    "},
    {Token::nonterminal_specification__,  1, &Parser::ReductionRuleHandler0034, "rule 34: nonterminal_specification <- %error    "},
    {Token::nonterminal_specification__,  2, &Parser::ReductionRuleHandler0035, "rule 35: nonterminal_specification <- ID %error    "},
    {Token::nonterminal_specification__,  3, &Parser::ReductionRuleHandler0036, "rule 36: nonterminal_specification <- ID TYPE %error    "},
    {            Token::rule_list__,  3, &Parser::ReductionRuleHandler0037, "rule 37: rule_list <- rule_list '|' rule    "},
    {            Token::rule_list__,  1, &Parser::ReductionRuleHandler0038, "rule 38: rule_list <- rule    "},
    {                 Token::rule__,  2, &Parser::ReductionRuleHandler0039, "rule 39: rule <- rule_specification STRICT_CODE_BLOCK    "},
    {                 Token::rule__,  1, &Parser::ReductionRuleHandler0040, "rule 40: rule <- rule_specification    "},
    {                 Token::rule__,  2, &Parser::ReductionRuleHandler0041, "rule 41: rule <- rule_specification DUMB_CODE_BLOCK    "},
    {   Token::rule_specification__,  3, &Parser::ReductionRuleHandler0042, "rule 42: rule_specification <- rule_token_list LEFT rule_precedence_directive    "},
    {   Token::rule_specification__,  3, &Parser::ReductionRuleHandler0043, "rule 43: rule_specification <- rule_token_list RIGHT rule_precedence_directive    "},
    {   Token::rule_specification__,  3, &Parser::ReductionRuleHandler0044, "rule 44: rule_specification <- rule_token_list NONASSOC rule_precedence_directive    "},
    {   Token::rule_specification__,  2, &Parser::ReductionRuleHandler0045, "rule 45: rule_specification <- rule_token_list rule_precedence_directive    "},
    {      Token::rule_token_list__,  2, &Parser::ReductionRuleHandler0046, "rule 46: rule_token_list <- rule_token_list rule_token    "},
    {      Token::rule_token_list__,  0, &Parser::ReductionRuleHandler0047, "rule 47: rule_token_list <-     "},
    {           Token::rule_token__,  3, &Parser::ReductionRuleHandler0048, "rule 48: rule_token <- token_id ':' ID    "},
    {           Token::rule_token__,  1, &Parser::ReductionRuleHandler0049, "rule 49: rule_token <- token_id    "},
    {           Token::rule_token__,  1, &Parser::ReductionRuleHandler0050, "rule 50: rule_token <- DIRECTIVE_ERROR    "},
    {Token::rule_precedence_directive__,  2, &Parser::ReductionRuleHandler0051, "rule 51: rule_precedence_directive <- PREC ID    "},
    {Token::rule_precedence_directive__,  0, &Parser::ReductionRuleHandler0052, "rule 52: rule_precedence_directive <-     "},

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
    {   0,    0,    1,    2,    3}, // state    0
    {   5,    1,    0,    0,    0}, // state    1
    {   6,    7,    0,   13,    1}, // state    2
    {  14,    1,   15,    0,    0}, // state    3
    {   0,    0,   16,    0,    0}, // state    4
    {  17,    5,    0,   22,    1}, // state    5
    {  23,    3,    0,    0,    0}, // state    6
    {  26,    1,    0,   27,    1}, // state    7
    {  28,    3,    0,   31,    2}, // state    8
    {   0,    0,   33,   34,    1}, // state    9
    {  35,    2,    0,    0,    0}, // state   10
    {  37,    2,    0,    0,    0}, // state   11
    {   0,    0,   39,    0,    0}, // state   12
    {   0,    0,   40,    0,    0}, // state   13
    {  41,    1,    0,   42,    1}, // state   14
    {  43,    1,    0,   44,    1}, // state   15
    {  45,    1,    0,   46,    1}, // state   16
    {   0,    0,   47,    0,    0}, // state   17
    {  48,    1,   49,    0,    0}, // state   18
    {  50,    1,    0,   51,    1}, // state   19
    {  52,    1,    0,   53,    1}, // state   20
    {  54,    1,    0,   55,    1}, // state   21
    {  56,    1,   57,    0,    0}, // state   22
    {  58,    2,    0,   60,    1}, // state   23
    {   0,    0,   61,    0,    0}, // state   24
    {   0,    0,   62,    0,    0}, // state   25
    {  63,    4,    0,   67,    2}, // state   26
    {   0,    0,   69,    0,    0}, // state   27
    {  70,    3,    0,   73,    2}, // state   28
    {  75,    2,    0,   77,    1}, // state   29
    {  78,    1,    0,   79,    1}, // state   30
    {  80,    2,    0,   82,    1}, // state   31
    {  83,    1,    0,   84,    1}, // state   32
    {  85,    1,   86,    0,    0}, // state   33
    {  87,    1,   88,    0,    0}, // state   34
    {  89,    1,   90,    0,    0}, // state   35
    {   0,    0,   91,    0,    0}, // state   36
    {  92,    1,   93,    0,    0}, // state   37
    {  94,    1,   95,    0,    0}, // state   38
    {  96,    1,   97,    0,    0}, // state   39
    {  98,    1,   99,    0,    0}, // state   40
    { 100,    2,    0,    0,    0}, // state   41
    {   0,    0,  102,    0,    0}, // state   42
    { 103,    1,  104,    0,    0}, // state   43
    { 105,    3,    0,    0,    0}, // state   44
    { 108,    3,    0,    0,    0}, // state   45
    {   0,    0,  111,    0,    0}, // state   46
    { 112,    1,    0,    0,    0}, // state   47
    { 113,    1,  114,    0,    0}, // state   48
    { 115,    1,  116,    0,    0}, // state   49
    { 117,    1,  118,    0,    0}, // state   50
    { 119,    1,  120,    0,    0}, // state   51
    { 121,    2,    0,  123,    1}, // state   52
    { 124,    1,    0,  125,    1}, // state   53
    {   0,    0,  126,    0,    0}, // state   54
    { 127,    2,    0,    0,    0}, // state   55
    { 129,    2,    0,    0,    0}, // state   56
    {   0,    0,  131,  132,    4}, // state   57
    { 136,    1,  137,    0,    0}, // state   58
    { 138,    1,  139,    0,    0}, // state   59
    { 140,    2,    0,    0,    0}, // state   60
    {   0,    0,  142,    0,    0}, // state   61
    { 143,    2,    0,    0,    0}, // state   62
    {   0,    0,  145,    0,    0}, // state   63
    { 146,    2,  148,    0,    0}, // state   64
    { 149,    7,  156,  157,    3}, // state   65
    {   0,    0,  160,    0,    0}, // state   66
    {   0,    0,  161,  162,    3}, // state   67
    {   0,    0,  165,    0,    0}, // state   68
    {   0,    0,  166,    0,    0}, // state   69
    {   0,    0,  167,    0,    0}, // state   70
    { 168,    1,  169,  170,    1}, // state   71
    { 171,    1,  172,  173,    1}, // state   72
    { 174,    1,    0,    0,    0}, // state   73
    { 175,    1,  176,  177,    1}, // state   74
    { 178,    1,  179,    0,    0}, // state   75
    {   0,    0,  180,    0,    0}, // state   76
    {   0,    0,  181,    0,    0}, // state   77
    {   0,    0,  182,    0,    0}, // state   78
    {   0,    0,  183,    0,    0}, // state   79
    {   0,    0,  184,    0,    0}, // state   80
    {   0,    0,  185,    0,    0}, // state   81
    {   0,    0,  186,    0,    0}, // state   82
    { 187,    1,    0,    0,    0}, // state   83
    {   0,    0,  188,    0,    0}  // state   84

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
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   25}},
    // nonterminal transitions
    {                 Token::root__, {                  TA_PUSH_STATE,    1}},
    {       Token::directive_list__, {                  TA_PUSH_STATE,    2}},
    {Token::at_least_zero_newlines__, {                  TA_PUSH_STATE,    3}},

// ///////////////////////////////////////////////////////////////////////////
// state    1
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::END_, {        TA_SHIFT_AND_PUSH_STATE,    4}},

// ///////////////////////////////////////////////////////////////////////////
// state    2
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,    5}},
    {Token::DIRECTIVE_PARSER_WITH_PARAMETER, {        TA_SHIFT_AND_PUSH_STATE,    6}},
    {Token::DIRECTIVE_PARSER_WITHOUT_PARAMETER, {        TA_SHIFT_AND_PUSH_STATE,    7}},
    {        Token::DIRECTIVE_TOKEN, {        TA_SHIFT_AND_PUSH_STATE,    8}},
    {           Token::END_PREAMBLE, {        TA_SHIFT_AND_PUSH_STATE,    9}},
    {                   Token::PREC, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    {                  Token::START, {        TA_SHIFT_AND_PUSH_STATE,   11}},
    // nonterminal transitions
    {            Token::directive__, {                  TA_PUSH_STATE,   12}},

// ///////////////////////////////////////////////////////////////////////////
// state    3
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   13}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    3}},

// ///////////////////////////////////////////////////////////////////////////
// state    4
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {TA_REDUCE_AND_ACCEPT_USING_RULE,    0}},

// ///////////////////////////////////////////////////////////////////////////
// state    5
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   14}},
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   15}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   18}},

// ///////////////////////////////////////////////////////////////////////////
// state    6
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   19}},
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   21}},

// ///////////////////////////////////////////////////////////////////////////
// state    7
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state    8
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {          Token::TOKEN_ID_CHAR, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    // nonterminal transitions
    {        Token::token_id_list__, {                  TA_PUSH_STATE,   26}},
    {             Token::token_id__, {                  TA_PUSH_STATE,   27}},

// ///////////////////////////////////////////////////////////////////////////
// state    9
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   29}},
    // nonterminal transitions
    {     Token::nonterminal_list__, {                  TA_PUSH_STATE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state   10
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   30}},

// ///////////////////////////////////////////////////////////////////////////
// state   11
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   32}},

// ///////////////////////////////////////////////////////////////////////////
// state   12
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    2}},

// ///////////////////////////////////////////////////////////////////////////
// state   13
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   24}},

// ///////////////////////////////////////////////////////////////////////////
// state   14
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   33}},

// ///////////////////////////////////////////////////////////////////////////
// state   15
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state   16
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   35}},

// ///////////////////////////////////////////////////////////////////////////
// state   17
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   27}},

// ///////////////////////////////////////////////////////////////////////////
// state   18
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   19}},

// ///////////////////////////////////////////////////////////////////////////
// state   19
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state   20
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   38}},

// ///////////////////////////////////////////////////////////////////////////
// state   21
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state   22
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    7}},

// ///////////////////////////////////////////////////////////////////////////
// state   23
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state   24
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   25
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   23}},

// ///////////////////////////////////////////////////////////////////////////
// state   26
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {                   Token::TYPE, {        TA_SHIFT_AND_PUSH_STATE,   41}},
    {          Token::TOKEN_ID_CHAR, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    {             Token::token_id__, {                  TA_PUSH_STATE,   42}},
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state   27
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   21}},

// ///////////////////////////////////////////////////////////////////////////
// state   28
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::END_, {           TA_REDUCE_USING_RULE,    1}},
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    // nonterminal transitions
    {          Token::nonterminal__, {                  TA_PUSH_STATE,   46}},
    {Token::nonterminal_specification__, {                  TA_PUSH_STATE,   47}},

// ///////////////////////////////////////////////////////////////////////////
// state   29
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   48}},

// ///////////////////////////////////////////////////////////////////////////
// state   30
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   49}},

// ///////////////////////////////////////////////////////////////////////////
// state   31
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   50}},

// ///////////////////////////////////////////////////////////////////////////
// state   32
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state   33
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state   34
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   17}},

// ///////////////////////////////////////////////////////////////////////////
// state   35
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   18}},

// ///////////////////////////////////////////////////////////////////////////
// state   36
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   26}},

// ///////////////////////////////////////////////////////////////////////////
// state   37
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    4}},

// ///////////////////////////////////////////////////////////////////////////
// state   38
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    5}},

// ///////////////////////////////////////////////////////////////////////////
// state   39
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    6}},

// ///////////////////////////////////////////////////////////////////////////
// state   40
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}},

// ///////////////////////////////////////////////////////////////////////////
// state   41
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   52}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state   42
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state   43
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    8}},

// ///////////////////////////////////////////////////////////////////////////
// state   44
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {              Token::Type(':'), {           TA_REDUCE_USING_RULE,   34}},
    {              Token::Type(';'), {        TA_SHIFT_AND_PUSH_STATE,   54}},

// ///////////////////////////////////////////////////////////////////////////
// state   45
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   55}},
    {              Token::Type(':'), {           TA_REDUCE_USING_RULE,   33}},
    {                   Token::TYPE, {        TA_SHIFT_AND_PUSH_STATE,   56}},

// ///////////////////////////////////////////////////////////////////////////
// state   46
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state   47
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   57}},

// ///////////////////////////////////////////////////////////////////////////
// state   48
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state   49
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},

// ///////////////////////////////////////////////////////////////////////////
// state   50
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   51
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state   52
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   58}},

// ///////////////////////////////////////////////////////////////////////////
// state   53
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state   54
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   31}},

// ///////////////////////////////////////////////////////////////////////////
// state   55
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {              Token::Type(':'), {           TA_REDUCE_USING_RULE,   35}},

// ///////////////////////////////////////////////////////////////////////////
// state   56
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   60}},
    {                 Token::STRING, {        TA_SHIFT_AND_PUSH_STATE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state   57
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   47}},
    // nonterminal transitions
    {            Token::rule_list__, {                  TA_PUSH_STATE,   62}},
    {                 Token::rule__, {                  TA_PUSH_STATE,   63}},
    {   Token::rule_specification__, {                  TA_PUSH_STATE,   64}},
    {      Token::rule_token_list__, {                  TA_PUSH_STATE,   65}},

// ///////////////////////////////////////////////////////////////////////////
// state   58
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state   59
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   36}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state   60
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {              Token::Type(':'), {           TA_REDUCE_USING_RULE,   36}},

// ///////////////////////////////////////////////////////////////////////////
// state   61
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   32}},

// ///////////////////////////////////////////////////////////////////////////
// state   62
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(';'), {        TA_SHIFT_AND_PUSH_STATE,   66}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   67}},

// ///////////////////////////////////////////////////////////////////////////
// state   63
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   38}},

// ///////////////////////////////////////////////////////////////////////////
// state   64
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   68}},
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   69}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state   65
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {        Token::DIRECTIVE_ERROR, {        TA_SHIFT_AND_PUSH_STATE,   70}},
    {                   Token::LEFT, {        TA_SHIFT_AND_PUSH_STATE,   71}},
    {               Token::NONASSOC, {        TA_SHIFT_AND_PUSH_STATE,   72}},
    {                   Token::PREC, {        TA_SHIFT_AND_PUSH_STATE,   73}},
    {                  Token::RIGHT, {        TA_SHIFT_AND_PUSH_STATE,   74}},
    {          Token::TOKEN_ID_CHAR, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},
    // nonterminal transitions
    {             Token::token_id__, {                  TA_PUSH_STATE,   75}},
    {           Token::rule_token__, {                  TA_PUSH_STATE,   76}},
    {Token::rule_precedence_directive__, {                  TA_PUSH_STATE,   77}},

// ///////////////////////////////////////////////////////////////////////////
// state   66
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   30}},

// ///////////////////////////////////////////////////////////////////////////
// state   67
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   47}},
    // nonterminal transitions
    {                 Token::rule__, {                  TA_PUSH_STATE,   78}},
    {   Token::rule_specification__, {                  TA_PUSH_STATE,   64}},
    {      Token::rule_token_list__, {                  TA_PUSH_STATE,   65}},

// ///////////////////////////////////////////////////////////////////////////
// state   68
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state   69
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state   70
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   50}},

// ///////////////////////////////////////////////////////////////////////////
// state   71
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::PREC, {        TA_SHIFT_AND_PUSH_STATE,   73}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},
    // nonterminal transitions
    {Token::rule_precedence_directive__, {                  TA_PUSH_STATE,   79}},

// ///////////////////////////////////////////////////////////////////////////
// state   72
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::PREC, {        TA_SHIFT_AND_PUSH_STATE,   73}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},
    // nonterminal transitions
    {Token::rule_precedence_directive__, {                  TA_PUSH_STATE,   80}},

// ///////////////////////////////////////////////////////////////////////////
// state   73
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   81}},

// ///////////////////////////////////////////////////////////////////////////
// state   74
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::PREC, {        TA_SHIFT_AND_PUSH_STATE,   73}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},
    // nonterminal transitions
    {Token::rule_precedence_directive__, {                  TA_PUSH_STATE,   82}},

// ///////////////////////////////////////////////////////////////////////////
// state   75
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   49}},

// ///////////////////////////////////////////////////////////////////////////
// state   76
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   46}},

// ///////////////////////////////////////////////////////////////////////////
// state   77
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   45}},

// ///////////////////////////////////////////////////////////////////////////
// state   78
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state   79
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   42}},

// ///////////////////////////////////////////////////////////////////////////
// state   80
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state   81
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state   82
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state   83
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   84}},

// ///////////////////////////////////////////////////////////////////////////
// state   84
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   48}}

};

unsigned int const Parser::ms_state_transition_count =
    sizeof(Parser::ms_state_transition) /
    sizeof(Parser::StateTransition);


#line 74 "trison_parser.trison"

bool Parser::SetInputFilename (string const &input_filename)
{
    assert(m_scanner != NULL);
    m_scanner->Close();
    return m_scanner->Open(input_filename);
}

Parser::Token::Type Parser::Scan ()
{
    assert(m_scanner != NULL);
    return m_scanner->Scan(&m_lookahead_token);
}

} // end of namespace Trison

#line 2201 "trison_parser.cpp"

