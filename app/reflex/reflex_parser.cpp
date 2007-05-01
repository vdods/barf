#include "reflex_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x


#line 68 "reflex_parser.trison"

#include <sstream>

#include "barf_regex_ast.hpp"
#include "barf_regex_parser.hpp"
#include "reflex_ast.hpp"

namespace Reflex {

#line 23 "reflex_parser.cpp"

Parser::Parser ()

{

#line 78 "reflex_parser.trison"

    m_target_map = NULL;
    m_regex_macro_map = NULL;

#line 34 "reflex_parser.cpp"
    m_debug_spew_level = 0;
    DEBUG_SPEW_2("### number of state transitions = " << ms_state_transition_count << std::endl);
    m_reduction_token = NULL;
}

Parser::~Parser ()
{

#line 83 "reflex_parser.trison"


#line 46 "reflex_parser.cpp"
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

#line 86 "reflex_parser.trison"

    delete token;

#line 408 "reflex_parser.cpp"
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
        "DIRECTIVE_LANGUAGE",
        "DIRECTIVE_MACRO",
        "DIRECTIVE_START",
        "DIRECTIVE_STATE",
        "DIRECTIVE_TARGETS",
        "DUMB_CODE_BLOCK",
        "END_PREAMBLE",
        "ID",
        "NEWLINE",
        "REGEX",
        "STRICT_CODE_BLOCK",
        "STRING_LITERAL",
        "END_",

        "any_type_of_code_block",
        "at_least_one_newline",
        "at_least_zero_newlines",
        "language_directive",
        "language_directive_param",
        "language_directives",
        "macro_directives",
        "root",
        "rule",
        "rule_handler",
        "rule_handlers",
        "rule_list",
        "scanner_state",
        "scanner_state_rules",
        "scanner_states",
        "start_directive",
        "target_ids",
        "targets_directive",
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

// rule 1: root <- at_least_zero_newlines targets_directive:target_map language_directives macro_directives:regular_expression_map start_directive:start_directive END_PREAMBLE:throwaway scanner_states:scanner_state_map    
Ast::Base * Parser::ReductionRuleHandler0001 ()
{
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    Regex::RegularExpressionMap * regular_expression_map = Dsc< Regex::RegularExpressionMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(4 < m_reduction_rule_token_count);
    StartDirective * start_directive = Dsc< StartDirective * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);
    assert(5 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);
    assert(6 < m_reduction_rule_token_count);
    ScannerStateMap * scanner_state_map = Dsc< ScannerStateMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 6]);

#line 188 "reflex_parser.trison"

        assert(m_target_map != NULL);
        assert(target_map == m_target_map);

        // set the TargetMap's primary source path
        target_map->SetSourcePath(m_scanner.GetInputName());
        // make sure the %start directive value specifies a real scanner state
        if (start_directive != NULL &&
            scanner_state_map->GetElement(start_directive->m_start_state_id->GetText()) == NULL)
        {
            EmitError(
                start_directive->GetFiLoc(),
                "undeclared state \"" + start_directive->m_start_state_id->GetText() + "\"");
        }

        Representation *representation =
            new Representation(
                target_map,
                regular_expression_map,
                start_directive,
                throwaway->GetFiLoc(),
                scanner_state_map);
        delete throwaway;
        return representation;
    
#line 535 "reflex_parser.cpp"
    return NULL;
}

// rule 2: targets_directive <- DIRECTIVE_TARGETS:throwaway target_ids:target_map at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 222 "reflex_parser.trison"

        assert(m_target_map == NULL);
        m_target_map = target_map;
        delete throwaway;
        return target_map;
    
#line 554 "reflex_parser.cpp"
    return NULL;
}

// rule 3: targets_directive <-     
Ast::Base * Parser::ReductionRuleHandler0003 ()
{

#line 230 "reflex_parser.trison"

        assert(m_target_map == NULL);
        m_target_map = new CommonLang::TargetMap();
        return m_target_map;
    
#line 568 "reflex_parser.cpp"
    return NULL;
}

// rule 4: targets_directive <- DIRECTIVE_TARGETS:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0004 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 237 "reflex_parser.trison"

        assert(m_target_map == NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in directive %targets");
        m_target_map = new CommonLang::TargetMap();
        return m_target_map;
    
#line 585 "reflex_parser.cpp"
    return NULL;
}

// rule 5: target_ids <- target_ids:target_map ID:target_id    
Ast::Base * Parser::ReductionRuleHandler0005 ()
{
    assert(0 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 248 "reflex_parser.trison"

        CommonLang::Target *target = new CommonLang::Target(target_id->GetText());
        target->EnableCodeGeneration();
        target_map->Add(target_id->GetText(), target);
        return target_map;
    
#line 604 "reflex_parser.cpp"
    return NULL;
}

// rule 6: target_ids <-     
Ast::Base * Parser::ReductionRuleHandler0006 ()
{

#line 256 "reflex_parser.trison"

        assert(m_target_map == NULL);
        return new CommonLang::TargetMap();
    
#line 617 "reflex_parser.cpp"
    return NULL;
}

// rule 7: language_directives <- language_directives language_directive:language_directive    
Ast::Base * Parser::ReductionRuleHandler0007 ()
{
    assert(1 < m_reduction_rule_token_count);
    CommonLang::LanguageDirective * language_directive = Dsc< CommonLang::LanguageDirective * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 269 "reflex_parser.trison"

        assert(m_target_map != NULL);
        if (language_directive != NULL)
            m_target_map->AddLanguageDirective(language_directive);
        return NULL;
    
#line 634 "reflex_parser.cpp"
    return NULL;
}

// rule 8: language_directives <-     
Ast::Base * Parser::ReductionRuleHandler0008 ()
{

#line 277 "reflex_parser.trison"

        if (m_target_map == NULL)
            m_target_map = new CommonLang::TargetMap();
        return NULL;
    
#line 648 "reflex_parser.cpp"
    return NULL;
}

// rule 9: language_directive <- DIRECTIVE_LANGUAGE:throwaway '.' ID:language_id '.' ID:language_directive language_directive_param:param at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0009 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * language_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(4 < m_reduction_rule_token_count);
    Ast::Id * language_directive = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);
    assert(5 < m_reduction_rule_token_count);
    Ast::TextBase * param = Dsc< Ast::TextBase * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 287 "reflex_parser.trison"

        delete throwaway;
        return new CommonLang::LanguageDirective(language_id, language_directive, param);
    
#line 669 "reflex_parser.cpp"
    return NULL;
}

// rule 10: language_directive <- DIRECTIVE_LANGUAGE:throwaway '.' ID:language_id '.' ID:language_directive %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0010 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * language_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(4 < m_reduction_rule_token_count);
    Ast::Id * language_directive = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);

#line 293 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in parameter for directive %language." + language_id->GetText() + "." + language_directive->GetText());
        delete throwaway;
        delete language_id;
        delete language_directive;
        return NULL;
    
#line 691 "reflex_parser.cpp"
    return NULL;
}

// rule 11: language_directive <- DIRECTIVE_LANGUAGE:throwaway '.' ID:language_id %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0011 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * language_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 302 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in directive name for directive %language." + language_id->GetText());
        delete throwaway;
        delete language_id;
        return NULL;
    
#line 710 "reflex_parser.cpp"
    return NULL;
}

// rule 12: language_directive <- DIRECTIVE_LANGUAGE:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0012 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 310 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in language name for directive %language");
        delete throwaway;
        return NULL;
    
#line 726 "reflex_parser.cpp"
    return NULL;
}

// rule 13: language_directive_param <- ID:value    
Ast::Base * Parser::ReductionRuleHandler0013 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * value = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 319 "reflex_parser.trison"
 return value; 
#line 738 "reflex_parser.cpp"
    return NULL;
}

// rule 14: language_directive_param <- STRING_LITERAL:value    
Ast::Base * Parser::ReductionRuleHandler0014 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::String * value = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 320 "reflex_parser.trison"
 return value; 
#line 750 "reflex_parser.cpp"
    return NULL;
}

// rule 15: language_directive_param <- STRICT_CODE_BLOCK:value    
Ast::Base * Parser::ReductionRuleHandler0015 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * value = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 321 "reflex_parser.trison"
 return value; 
#line 762 "reflex_parser.cpp"
    return NULL;
}

// rule 16: language_directive_param <- DUMB_CODE_BLOCK:value    
Ast::Base * Parser::ReductionRuleHandler0016 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * value = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 322 "reflex_parser.trison"
 return value; 
#line 774 "reflex_parser.cpp"
    return NULL;
}

// rule 17: language_directive_param <-     
Ast::Base * Parser::ReductionRuleHandler0017 ()
{

#line 323 "reflex_parser.trison"
 return NULL; 
#line 784 "reflex_parser.cpp"
    return NULL;
}

// rule 18: macro_directives <- macro_directives:regular_expression_map DIRECTIVE_MACRO:throwaway ID:macro_id REGEX:macro_regex_string at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0018 ()
{
    assert(0 < m_reduction_rule_token_count);
    Regex::RegularExpressionMap * regular_expression_map = Dsc< Regex::RegularExpressionMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * macro_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(3 < m_reduction_rule_token_count);
    Ast::String * macro_regex_string = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 333 "reflex_parser.trison"

        Regex::Parser parser;
        istringstream in(macro_regex_string->GetText());
        parser.OpenUsingStream(&in, "%macro " + macro_regex_string->GetText(), false);
        try {
            if (parser.Parse(regular_expression_map) == Regex::Parser::PRC_SUCCESS)
                regular_expression_map->Add(
                    macro_id->GetText(),
                    Dsc<Regex::RegularExpression *>(parser.GetAcceptedToken()));
            else
                EmitError(throwaway->GetFiLoc(), "parse error in regular expression (" + macro_regex_string->GetText() + ")");
        } catch (string const &exception) {
            EmitError(throwaway->GetFiLoc(), exception + " in regular expression (" + macro_regex_string->GetText() + ")");
        }
        delete throwaway;
        delete macro_id;
        delete macro_regex_string;
        return regular_expression_map;
    
#line 820 "reflex_parser.cpp"
    return NULL;
}

// rule 19: macro_directives <-     
Ast::Base * Parser::ReductionRuleHandler0019 ()
{

#line 354 "reflex_parser.trison"

        // we save the regex macro map in a member var, so that the reduction
        // rule handler for the scanner state rules can use it.
        m_regex_macro_map = new Regex::RegularExpressionMap();
        return m_regex_macro_map;
    
#line 835 "reflex_parser.cpp"
    return NULL;
}

// rule 20: macro_directives <- macro_directives:regular_expression_map DIRECTIVE_MACRO:throwaway ID:macro_id %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0020 ()
{
    assert(0 < m_reduction_rule_token_count);
    Regex::RegularExpressionMap * regular_expression_map = Dsc< Regex::RegularExpressionMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * macro_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 362 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in directive %macro " + macro_id->GetText());
        delete throwaway;
        delete macro_id;
        return regular_expression_map;
    
#line 856 "reflex_parser.cpp"
    return NULL;
}

// rule 21: macro_directives <- macro_directives:regular_expression_map DIRECTIVE_MACRO:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0021 ()
{
    assert(0 < m_reduction_rule_token_count);
    Regex::RegularExpressionMap * regular_expression_map = Dsc< Regex::RegularExpressionMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 370 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in directive %macro");
        delete throwaway;
        return regular_expression_map;
    
#line 874 "reflex_parser.cpp"
    return NULL;
}

// rule 22: start_directive <- DIRECTIVE_START:throwaway ID:start_state_id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0022 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * start_state_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 384 "reflex_parser.trison"

        delete throwaway;
        return new StartDirective(start_state_id);
    
#line 891 "reflex_parser.cpp"
    return NULL;
}

// rule 23: start_directive <- DIRECTIVE_START:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0023 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 390 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in directive %start");
        delete throwaway;
        return NULL;
    
#line 907 "reflex_parser.cpp"
    return NULL;
}

// rule 24: scanner_states <- scanner_states:scanner_state_map scanner_state:scanner_state    
Ast::Base * Parser::ReductionRuleHandler0024 ()
{
    assert(0 < m_reduction_rule_token_count);
    ScannerStateMap * scanner_state_map = Dsc< ScannerStateMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    ScannerState * scanner_state = Dsc< ScannerState * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 404 "reflex_parser.trison"

        if (scanner_state != NULL)
            scanner_state_map->Add(scanner_state->m_scanner_state_id->GetText(), scanner_state);
        return scanner_state_map;
    
#line 925 "reflex_parser.cpp"
    return NULL;
}

// rule 25: scanner_states <-     
Ast::Base * Parser::ReductionRuleHandler0025 ()
{

#line 411 "reflex_parser.trison"

        return new ScannerStateMap();
    
#line 937 "reflex_parser.cpp"
    return NULL;
}

// rule 26: scanner_state <- DIRECTIVE_STATE:throwaway ID:scanner_state_id ':' scanner_state_rules:rule_list ';'    
Ast::Base * Parser::ReductionRuleHandler0026 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * scanner_state_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 419 "reflex_parser.trison"

        delete throwaway;
        return new ScannerState(scanner_state_id, rule_list);
    
#line 956 "reflex_parser.cpp"
    return NULL;
}

// rule 27: scanner_state <- DIRECTIVE_STATE:throwaway ID:scanner_state_id ':' %error ';'    
Ast::Base * Parser::ReductionRuleHandler0027 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * scanner_state_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 425 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in scanner state rule list");
        delete throwaway;
        return new ScannerState(scanner_state_id, new RuleList());
    
#line 974 "reflex_parser.cpp"
    return NULL;
}

// rule 28: scanner_state <- DIRECTIVE_STATE:throwaway %error ':' scanner_state_rules:rule_list ';'    
Ast::Base * Parser::ReductionRuleHandler0028 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 432 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in scanner state id");
        delete throwaway;
        delete rule_list;
        return NULL;
    
#line 993 "reflex_parser.cpp"
    return NULL;
}

// rule 29: scanner_state_rules <- rule_list:rule_list    
Ast::Base * Parser::ReductionRuleHandler0029 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 443 "reflex_parser.trison"

        return rule_list;
    
#line 1007 "reflex_parser.cpp"
    return NULL;
}

// rule 30: scanner_state_rules <-     
Ast::Base * Parser::ReductionRuleHandler0030 ()
{

#line 448 "reflex_parser.trison"

        return new RuleList();
    
#line 1019 "reflex_parser.cpp"
    return NULL;
}

// rule 31: rule_list <- rule_list:rule_list '|' rule:rule    
Ast::Base * Parser::ReductionRuleHandler0031 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 456 "reflex_parser.trison"

        rule_list->Append(rule);
        return rule_list;
    
#line 1036 "reflex_parser.cpp"
    return NULL;
}

// rule 32: rule_list <- rule:rule    
Ast::Base * Parser::ReductionRuleHandler0032 ()
{
    assert(0 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 462 "reflex_parser.trison"

        RuleList *rule_list = new RuleList();
        rule_list->Append(rule);
        return rule_list;
    
#line 1052 "reflex_parser.cpp"
    return NULL;
}

// rule 33: rule <- REGEX:regex_string rule_handlers:rule_handler_map    
Ast::Base * Parser::ReductionRuleHandler0033 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::String * regex_string = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::RuleHandlerMap * rule_handler_map = Dsc< CommonLang::RuleHandlerMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 472 "reflex_parser.trison"

        assert(m_regex_macro_map != NULL);

        // parse the rule regex
        Regex::RegularExpression *regex = NULL;
        {
            Regex::Parser parser;
            istringstream in(regex_string->GetText());
            parser.OpenUsingStream(&in, regex_string->GetText(), false);
            try {
                if (parser.Parse(m_regex_macro_map) == Regex::Parser::PRC_SUCCESS)
                    regex = Dsc<Regex::RegularExpression *>(parser.GetAcceptedToken());
                else
                {
                    EmitError(
                        regex_string->GetFiLoc(),
                        "parse error in regular expression (" + regex_string->GetText() + ")");
                    regex = new Regex::RegularExpression();
                }
            } catch (string const &exception) {
                EmitError(
                    regex_string->GetFiLoc(),
                    exception + " in regular expression (" + regex_string->GetText() + ")");
                regex = new Regex::RegularExpression();
            }
            assert(regex != NULL);
        }

        // enforcement of presence of rule handlers for all targets
        assert(m_target_map != NULL);
        for (CommonLang::TargetMap::const_iterator it = m_target_map->begin(),
                                                           it_end = m_target_map->end();
             it != it_end;
             ++it)
        {
            string const &target_id = it->first;
            if (rule_handler_map->GetElement(target_id) == NULL)
            {
                EmitWarning(
                    regex_string->GetFiLoc(),
                    "missing rule handler for target \"" + target_id + "\"");
                // add a blank code block for the rule handler's missing target
                rule_handler_map->Add(
                    target_id,
                    new CommonLang::RuleHandler(
                        new Ast::Id(target_id, FiLoc::ms_invalid),
                        new Ast::StrictCodeBlock(FiLoc::ms_invalid)));
                assert(rule_handler_map->GetElement(target_id) != NULL);
            }
        }

        Rule *rule = new Rule(regex_string->GetText(), regex, rule_handler_map);
        delete regex_string;
        return rule;
    
#line 1120 "reflex_parser.cpp"
    return NULL;
}

// rule 34: rule_handlers <- rule_handlers:rule_handler_map rule_handler:rule_handler    
Ast::Base * Parser::ReductionRuleHandler0034 ()
{
    assert(0 < m_reduction_rule_token_count);
    CommonLang::RuleHandlerMap * rule_handler_map = Dsc< CommonLang::RuleHandlerMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::RuleHandler * rule_handler = Dsc< CommonLang::RuleHandler * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 532 "reflex_parser.trison"

        if (rule_handler != NULL)
            rule_handler_map->Add(rule_handler->m_language_id->GetText(), rule_handler);
        return rule_handler_map;
    
#line 1138 "reflex_parser.cpp"
    return NULL;
}

// rule 35: rule_handlers <-     
Ast::Base * Parser::ReductionRuleHandler0035 ()
{

#line 539 "reflex_parser.trison"

        return new CommonLang::RuleHandlerMap();
    
#line 1150 "reflex_parser.cpp"
    return NULL;
}

// rule 36: rule_handler <- DIRECTIVE_LANGUAGE:throwaway '.' ID:language_id any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0036 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * language_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(3 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 547 "reflex_parser.trison"

        delete throwaway;
        assert(m_target_map != NULL);
        if (m_target_map->GetElement(language_id->GetText()) == NULL)
            EmitWarning(
                language_id->GetFiLoc(),
                "undeclared target \"" + language_id->GetText() + "\"");
        return new CommonLang::RuleHandler(language_id, code_block);
    
#line 1174 "reflex_parser.cpp"
    return NULL;
}

// rule 37: rule_handler <- DIRECTIVE_LANGUAGE:throwaway %error any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0037 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 558 "reflex_parser.trison"

        assert(m_target_map != NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in language id after directive %language");
        delete throwaway;
        delete code_block;
        return NULL;
    
#line 1194 "reflex_parser.cpp"
    return NULL;
}

// rule 38: rule_handler <- DIRECTIVE_LANGUAGE:throwaway %error    
Ast::Base * Parser::ReductionRuleHandler0038 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 567 "reflex_parser.trison"

        assert(m_target_map != NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in directive %language");
        delete throwaway;
        return NULL;
    
#line 1211 "reflex_parser.cpp"
    return NULL;
}

// rule 39: rule_handler <- %error any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0039 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 575 "reflex_parser.trison"

        assert(m_target_map != NULL);
        EmitError(code_block->GetFiLoc(), "missing directive %language before rule handler code block");
        delete code_block;
        return NULL;
    
#line 1228 "reflex_parser.cpp"
    return NULL;
}

// rule 40: any_type_of_code_block <- DUMB_CODE_BLOCK:dumb_code_block    
Ast::Base * Parser::ReductionRuleHandler0040 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * dumb_code_block = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 589 "reflex_parser.trison"
 return dumb_code_block; 
#line 1240 "reflex_parser.cpp"
    return NULL;
}

// rule 41: any_type_of_code_block <- STRICT_CODE_BLOCK:strict_code_block    
Ast::Base * Parser::ReductionRuleHandler0041 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * strict_code_block = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 591 "reflex_parser.trison"
 return strict_code_block; 
#line 1252 "reflex_parser.cpp"
    return NULL;
}

// rule 42: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0042 ()
{
    return NULL;
}

// rule 43: at_least_zero_newlines <-     
Ast::Base * Parser::ReductionRuleHandler0043 ()
{
    return NULL;
}

// rule 44: at_least_one_newline <- at_least_one_newline NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0044 ()
{
    return NULL;
}

// rule 45: at_least_one_newline <- NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0045 ()
{
    return NULL;
}



// ///////////////////////////////////////////////////////////////////////////
// reduction rule lookup table
// ///////////////////////////////////////////////////////////////////////////

Parser::ReductionRule const Parser::ms_reduction_rule[] =
{
    {                 Token::START_,  2, &Parser::ReductionRuleHandler0000, "rule 0: %start <- root END_    "},
    {                 Token::root__,  7, &Parser::ReductionRuleHandler0001, "rule 1: root <- at_least_zero_newlines targets_directive language_directives macro_directives start_directive END_PREAMBLE scanner_states    "},
    {    Token::targets_directive__,  3, &Parser::ReductionRuleHandler0002, "rule 2: targets_directive <- DIRECTIVE_TARGETS target_ids at_least_one_newline    "},
    {    Token::targets_directive__,  0, &Parser::ReductionRuleHandler0003, "rule 3: targets_directive <-     "},
    {    Token::targets_directive__,  3, &Parser::ReductionRuleHandler0004, "rule 4: targets_directive <- DIRECTIVE_TARGETS %error at_least_one_newline    "},
    {           Token::target_ids__,  2, &Parser::ReductionRuleHandler0005, "rule 5: target_ids <- target_ids ID    "},
    {           Token::target_ids__,  0, &Parser::ReductionRuleHandler0006, "rule 6: target_ids <-     "},
    {  Token::language_directives__,  2, &Parser::ReductionRuleHandler0007, "rule 7: language_directives <- language_directives language_directive    "},
    {  Token::language_directives__,  0, &Parser::ReductionRuleHandler0008, "rule 8: language_directives <-     "},
    {   Token::language_directive__,  7, &Parser::ReductionRuleHandler0009, "rule 9: language_directive <- DIRECTIVE_LANGUAGE '.' ID '.' ID language_directive_param at_least_one_newline    "},
    {   Token::language_directive__,  7, &Parser::ReductionRuleHandler0010, "rule 10: language_directive <- DIRECTIVE_LANGUAGE '.' ID '.' ID %error at_least_one_newline    "},
    {   Token::language_directive__,  5, &Parser::ReductionRuleHandler0011, "rule 11: language_directive <- DIRECTIVE_LANGUAGE '.' ID %error at_least_one_newline    "},
    {   Token::language_directive__,  3, &Parser::ReductionRuleHandler0012, "rule 12: language_directive <- DIRECTIVE_LANGUAGE %error at_least_one_newline    "},
    {Token::language_directive_param__,  1, &Parser::ReductionRuleHandler0013, "rule 13: language_directive_param <- ID    "},
    {Token::language_directive_param__,  1, &Parser::ReductionRuleHandler0014, "rule 14: language_directive_param <- STRING_LITERAL    "},
    {Token::language_directive_param__,  1, &Parser::ReductionRuleHandler0015, "rule 15: language_directive_param <- STRICT_CODE_BLOCK    "},
    {Token::language_directive_param__,  1, &Parser::ReductionRuleHandler0016, "rule 16: language_directive_param <- DUMB_CODE_BLOCK    "},
    {Token::language_directive_param__,  0, &Parser::ReductionRuleHandler0017, "rule 17: language_directive_param <-     "},
    {     Token::macro_directives__,  5, &Parser::ReductionRuleHandler0018, "rule 18: macro_directives <- macro_directives DIRECTIVE_MACRO ID REGEX at_least_one_newline    "},
    {     Token::macro_directives__,  0, &Parser::ReductionRuleHandler0019, "rule 19: macro_directives <-     "},
    {     Token::macro_directives__,  5, &Parser::ReductionRuleHandler0020, "rule 20: macro_directives <- macro_directives DIRECTIVE_MACRO ID %error at_least_one_newline    "},
    {     Token::macro_directives__,  4, &Parser::ReductionRuleHandler0021, "rule 21: macro_directives <- macro_directives DIRECTIVE_MACRO %error at_least_one_newline    "},
    {      Token::start_directive__,  3, &Parser::ReductionRuleHandler0022, "rule 22: start_directive <- DIRECTIVE_START ID at_least_one_newline    "},
    {      Token::start_directive__,  3, &Parser::ReductionRuleHandler0023, "rule 23: start_directive <- DIRECTIVE_START %error at_least_one_newline    "},
    {       Token::scanner_states__,  2, &Parser::ReductionRuleHandler0024, "rule 24: scanner_states <- scanner_states scanner_state    "},
    {       Token::scanner_states__,  0, &Parser::ReductionRuleHandler0025, "rule 25: scanner_states <-     "},
    {        Token::scanner_state__,  5, &Parser::ReductionRuleHandler0026, "rule 26: scanner_state <- DIRECTIVE_STATE ID ':' scanner_state_rules ';'    "},
    {        Token::scanner_state__,  5, &Parser::ReductionRuleHandler0027, "rule 27: scanner_state <- DIRECTIVE_STATE ID ':' %error ';'    "},
    {        Token::scanner_state__,  5, &Parser::ReductionRuleHandler0028, "rule 28: scanner_state <- DIRECTIVE_STATE %error ':' scanner_state_rules ';'    "},
    {  Token::scanner_state_rules__,  1, &Parser::ReductionRuleHandler0029, "rule 29: scanner_state_rules <- rule_list    "},
    {  Token::scanner_state_rules__,  0, &Parser::ReductionRuleHandler0030, "rule 30: scanner_state_rules <-     "},
    {            Token::rule_list__,  3, &Parser::ReductionRuleHandler0031, "rule 31: rule_list <- rule_list '|' rule    "},
    {            Token::rule_list__,  1, &Parser::ReductionRuleHandler0032, "rule 32: rule_list <- rule    "},
    {                 Token::rule__,  2, &Parser::ReductionRuleHandler0033, "rule 33: rule <- REGEX rule_handlers    "},
    {        Token::rule_handlers__,  2, &Parser::ReductionRuleHandler0034, "rule 34: rule_handlers <- rule_handlers rule_handler    "},
    {        Token::rule_handlers__,  0, &Parser::ReductionRuleHandler0035, "rule 35: rule_handlers <-     "},
    {         Token::rule_handler__,  4, &Parser::ReductionRuleHandler0036, "rule 36: rule_handler <- DIRECTIVE_LANGUAGE '.' ID any_type_of_code_block    "},
    {         Token::rule_handler__,  3, &Parser::ReductionRuleHandler0037, "rule 37: rule_handler <- DIRECTIVE_LANGUAGE %error any_type_of_code_block    "},
    {         Token::rule_handler__,  2, &Parser::ReductionRuleHandler0038, "rule 38: rule_handler <- DIRECTIVE_LANGUAGE %error    "},
    {         Token::rule_handler__,  2, &Parser::ReductionRuleHandler0039, "rule 39: rule_handler <- %error any_type_of_code_block    "},
    {Token::any_type_of_code_block__,  1, &Parser::ReductionRuleHandler0040, "rule 40: any_type_of_code_block <- DUMB_CODE_BLOCK    "},
    {Token::any_type_of_code_block__,  1, &Parser::ReductionRuleHandler0041, "rule 41: any_type_of_code_block <- STRICT_CODE_BLOCK    "},
    {Token::at_least_zero_newlines__,  2, &Parser::ReductionRuleHandler0042, "rule 42: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    "},
    {Token::at_least_zero_newlines__,  0, &Parser::ReductionRuleHandler0043, "rule 43: at_least_zero_newlines <-     "},
    { Token::at_least_one_newline__,  2, &Parser::ReductionRuleHandler0044, "rule 44: at_least_one_newline <- at_least_one_newline NEWLINE    "},
    { Token::at_least_one_newline__,  1, &Parser::ReductionRuleHandler0045, "rule 45: at_least_one_newline <- NEWLINE    "},

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
    {   0,    0,    1,    2,    2}, // state    0
    {   4,    1,    0,    0,    0}, // state    1
    {   5,    2,    7,    8,    1}, // state    2
    {   0,    0,    9,    0,    0}, // state    3
    {  10,    3,    0,   13,    1}, // state    4
    {   0,    0,   14,    0,    0}, // state    5
    {   0,    0,   15,   16,    1}, // state    6
    {  17,    2,    0,   19,    1}, // state    7
    {  20,    2,    0,   22,    1}, // state    8
    {  23,    1,   24,   25,    2}, // state    9
    {   0,    0,   27,    0,    0}, // state   10
    {  28,    1,   29,    0,    0}, // state   11
    {   0,    0,   30,    0,    0}, // state   12
    {  31,    1,   32,    0,    0}, // state   13
    {  33,    2,    0,    0,    0}, // state   14
    {   0,    0,   35,    0,    0}, // state   15
    {  36,    2,    0,   38,    1}, // state   16
    {   0,    0,   39,    0,    0}, // state   17
    {  40,    2,    0,   42,    1}, // state   18
    {  43,    1,    0,    0,    0}, // state   19
    {  44,    2,    0,    0,    0}, // state   20
    {  46,    2,    0,    0,    0}, // state   21
    {  48,    1,    0,    0,    0}, // state   22
    {  49,    1,   50,    0,    0}, // state   23
    {  51,    2,    0,    0,    0}, // state   24
    {  53,    2,    0,   55,    1}, // state   25
    {  56,    2,    0,    0,    0}, // state   26
    {  58,    2,    0,   60,    1}, // state   27
    {  61,    1,    0,   62,    1}, // state   28
    {   0,    0,   63,   64,    1}, // state   29
    {  65,    2,    0,   67,    1}, // state   30
    {  68,    1,    0,    0,    0}, // state   31
    {  69,    1,   70,    0,    0}, // state   32
    {  71,    2,    0,   73,    1}, // state   33
    {  74,    1,    0,   75,    1}, // state   34
    {  76,    1,   77,    0,    0}, // state   35
    {  78,    1,   79,    0,    0}, // state   36
    {  80,    1,   81,   82,    1}, // state   37
    {  83,    1,   84,    0,    0}, // state   38
    {  85,    6,    0,   91,    1}, // state   39
    {  92,    1,   93,    0,    0}, // state   40
    {  94,    1,   95,    0,    0}, // state   41
    {  96,    2,    0,    0,    0}, // state   42
    {   0,    0,   98,    0,    0}, // state   43
    {  99,    2,    0,  101,    1}, // state   44
    {   0,    0,  102,    0,    0}, // state   45
    {   0,    0,  103,    0,    0}, // state   46
    {   0,    0,  104,    0,    0}, // state   47
    {   0,    0,  105,    0,    0}, // state   48
    { 106,    1,    0,  107,    1}, // state   49
    { 108,    2,    0,    0,    0}, // state   50
    { 110,    1,    0,    0,    0}, // state   51
    { 111,    1,  112,    0,    0}, // state   52
    { 113,    1,  114,    0,    0}, // state   53
    { 115,    1,  116,  117,    3}, // state   54
    { 120,    3,    0,  123,    3}, // state   55
    {   0,    0,  126,  127,    1}, // state   56
    { 128,    1,    0,    0,    0}, // state   57
    { 129,    1,  130,    0,    0}, // state   58
    {   0,    0,  131,    0,    0}, // state   59
    { 132,    2,    0,    0,    0}, // state   60
    { 134,    1,    0,    0,    0}, // state   61
    { 135,    4,    0,  139,    1}, // state   62
    {   0,    0,  140,    0,    0}, // state   63
    { 141,    1,    0,  142,    1}, // state   64
    {   0,    0,  143,    0,    0}, // state   65
    {   0,    0,  144,    0,    0}, // state   66
    { 145,    3,    0,  148,    1}, // state   67
    { 149,    2,    0,    0,    0}, // state   68
    {   0,    0,  151,    0,    0}, // state   69
    {   0,    0,  152,    0,    0}, // state   70
    {   0,    0,  153,    0,    0}, // state   71
    {   0,    0,  154,    0,    0}, // state   72
    {   0,    0,  155,    0,    0}, // state   73
    { 156,    6,    0,  162,    1}, // state   74
    { 163,    1,    0,    0,    0}, // state   75
    {   0,    0,  164,    0,    0}, // state   76
    { 165,    2,    0,  167,    1}, // state   77
    {   0,    0,  168,    0,    0}  // state   78

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
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   43}},
    // nonterminal transitions
    {                 Token::root__, {                  TA_PUSH_STATE,    1}},
    {Token::at_least_zero_newlines__, {                  TA_PUSH_STATE,    2}},

// ///////////////////////////////////////////////////////////////////////////
// state    1
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::END_, {        TA_SHIFT_AND_PUSH_STATE,    3}},

// ///////////////////////////////////////////////////////////////////////////
// state    2
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {      Token::DIRECTIVE_TARGETS, {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    3}},
    // nonterminal transitions
    {    Token::targets_directive__, {                  TA_PUSH_STATE,    6}},

// ///////////////////////////////////////////////////////////////////////////
// state    3
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {TA_REDUCE_AND_ACCEPT_USING_RULE,    0}},

// ///////////////////////////////////////////////////////////////////////////
// state    4
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,    7}},
    {                     Token::ID, {           TA_REDUCE_USING_RULE,    6}},
    {                Token::NEWLINE, {           TA_REDUCE_USING_RULE,    6}},
    // nonterminal transitions
    {           Token::target_ids__, {                  TA_PUSH_STATE,    8}},

// ///////////////////////////////////////////////////////////////////////////
// state    5
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   42}},

// ///////////////////////////////////////////////////////////////////////////
// state    6
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    8}},
    // nonterminal transitions
    {  Token::language_directives__, {                  TA_PUSH_STATE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state    7
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state    8
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   12}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state    9
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {     Token::DIRECTIVE_LANGUAGE, {        TA_SHIFT_AND_PUSH_STATE,   14}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   19}},
    // nonterminal transitions
    {   Token::language_directive__, {                  TA_PUSH_STATE,   15}},
    {     Token::macro_directives__, {                  TA_PUSH_STATE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state   10
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   45}},

// ///////////////////////////////////////////////////////////////////////////
// state   11
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    4}},

// ///////////////////////////////////////////////////////////////////////////
// state   12
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    5}},

// ///////////////////////////////////////////////////////////////////////////
// state   13
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    2}},

// ///////////////////////////////////////////////////////////////////////////
// state   14
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   18}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   19}},

// ///////////////////////////////////////////////////////////////////////////
// state   15
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    7}},

// ///////////////////////////////////////////////////////////////////////////
// state   16
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {        Token::DIRECTIVE_MACRO, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    {        Token::DIRECTIVE_START, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    // nonterminal transitions
    {      Token::start_directive__, {                  TA_PUSH_STATE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   17
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state   18
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   23}},

// ///////////////////////////////////////////////////////////////////////////
// state   19
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   24}},

// ///////////////////////////////////////////////////////////////////////////
// state   20
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   26}},

// ///////////////////////////////////////////////////////////////////////////
// state   21
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state   22
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::END_PREAMBLE, {        TA_SHIFT_AND_PUSH_STATE,   29}},

// ///////////////////////////////////////////////////////////////////////////
// state   23
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},

// ///////////////////////////////////////////////////////////////////////////
// state   24
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   31}},

// ///////////////////////////////////////////////////////////////////////////
// state   25
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   32}},

// ///////////////////////////////////////////////////////////////////////////
// state   26
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {                  Token::REGEX, {        TA_SHIFT_AND_PUSH_STATE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state   27
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   35}},

// ///////////////////////////////////////////////////////////////////////////
// state   28
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   36}},

// ///////////////////////////////////////////////////////////////////////////
// state   29
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   25}},
    // nonterminal transitions
    {       Token::scanner_states__, {                  TA_PUSH_STATE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state   30
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   38}},

// ///////////////////////////////////////////////////////////////////////////
// state   31
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state   32
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   21}},

// ///////////////////////////////////////////////////////////////////////////
// state   33
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state   34
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state   35
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   23}},

// ///////////////////////////////////////////////////////////////////////////
// state   36
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   37
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {        Token::DIRECTIVE_STATE, {        TA_SHIFT_AND_PUSH_STATE,   42}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {        Token::scanner_state__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state   38
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state   39
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   46}},
    {                Token::NEWLINE, {           TA_REDUCE_USING_RULE,   17}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   47}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   48}},
    // nonterminal transitions
    {Token::language_directive_param__, {                  TA_PUSH_STATE,   49}},

// ///////////////////////////////////////////////////////////////////////////
// state   40
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state   41
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   18}},

// ///////////////////////////////////////////////////////////////////////////
// state   42
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   50}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state   43
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   24}},

// ///////////////////////////////////////////////////////////////////////////
// state   44
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   52}},

// ///////////////////////////////////////////////////////////////////////////
// state   45
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state   46
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state   47
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   48
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state   49
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state   50
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   54}},

// ///////////////////////////////////////////////////////////////////////////
// state   51
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   55}},

// ///////////////////////////////////////////////////////////////////////////
// state   52
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}},

// ///////////////////////////////////////////////////////////////////////////
// state   53
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state   54
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::REGEX, {        TA_SHIFT_AND_PUSH_STATE,   56}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   30}},
    // nonterminal transitions
    {  Token::scanner_state_rules__, {                  TA_PUSH_STATE,   57}},
    {            Token::rule_list__, {                  TA_PUSH_STATE,   58}},
    {                 Token::rule__, {                  TA_PUSH_STATE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state   55
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   60}},
    {                  Token::REGEX, {        TA_SHIFT_AND_PUSH_STATE,   56}},
    {              Token::Type(';'), {           TA_REDUCE_USING_RULE,   30}},
    // nonterminal transitions
    {  Token::scanner_state_rules__, {                  TA_PUSH_STATE,   61}},
    {            Token::rule_list__, {                  TA_PUSH_STATE,   58}},
    {                 Token::rule__, {                  TA_PUSH_STATE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state   56
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   35}},
    // nonterminal transitions
    {        Token::rule_handlers__, {                  TA_PUSH_STATE,   62}},

// ///////////////////////////////////////////////////////////////////////////
// state   57
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(';'), {        TA_SHIFT_AND_PUSH_STATE,   63}},

// ///////////////////////////////////////////////////////////////////////////
// state   58
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   64}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   29}},

// ///////////////////////////////////////////////////////////////////////////
// state   59
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   32}},

// ///////////////////////////////////////////////////////////////////////////
// state   60
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {              Token::Type(';'), {        TA_SHIFT_AND_PUSH_STATE,   65}},

// ///////////////////////////////////////////////////////////////////////////
// state   61
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(';'), {        TA_SHIFT_AND_PUSH_STATE,   66}},

// ///////////////////////////////////////////////////////////////////////////
// state   62
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   67}},
    {     Token::DIRECTIVE_LANGUAGE, {        TA_SHIFT_AND_PUSH_STATE,   68}},
    {              Token::Type('|'), {           TA_REDUCE_USING_RULE,   33}},
    {              Token::Type(';'), {           TA_REDUCE_USING_RULE,   33}},
    // nonterminal transitions
    {         Token::rule_handler__, {                  TA_PUSH_STATE,   69}},

// ///////////////////////////////////////////////////////////////////////////
// state   63
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state   64
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::REGEX, {        TA_SHIFT_AND_PUSH_STATE,   56}},
    // nonterminal transitions
    {                 Token::rule__, {                  TA_PUSH_STATE,   70}},

// ///////////////////////////////////////////////////////////////////////////
// state   65
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   27}},

// ///////////////////////////////////////////////////////////////////////////
// state   66
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   26}},

// ///////////////////////////////////////////////////////////////////////////
// state   67
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   71}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   72}},
    // nonterminal transitions
    {Token::any_type_of_code_block__, {                  TA_PUSH_STATE,   73}},

// ///////////////////////////////////////////////////////////////////////////
// state   68
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   74}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   75}},

// ///////////////////////////////////////////////////////////////////////////
// state   69
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state   70
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   31}},

// ///////////////////////////////////////////////////////////////////////////
// state   71
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state   72
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state   73
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state   74
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {     Token::DIRECTIVE_LANGUAGE, {           TA_REDUCE_USING_RULE,   38}},
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   71}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   72}},
    {              Token::Type('|'), {           TA_REDUCE_USING_RULE,   38}},
    {              Token::Type(';'), {           TA_REDUCE_USING_RULE,   38}},
    // nonterminal transitions
    {Token::any_type_of_code_block__, {                  TA_PUSH_STATE,   76}},

// ///////////////////////////////////////////////////////////////////////////
// state   75
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   77}},

// ///////////////////////////////////////////////////////////////////////////
// state   76
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state   77
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   71}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   72}},
    // nonterminal transitions
    {Token::any_type_of_code_block__, {                  TA_PUSH_STATE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state   78
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   36}}

};

unsigned int const Parser::ms_state_transition_count =
    sizeof(Parser::ms_state_transition) /
    sizeof(Parser::StateTransition);


#line 90 "reflex_parser.trison"

bool Parser::OpenFile (string const &input_filename)
{
    return m_scanner.OpenFile(input_filename);
}

Parser::Token::Type Parser::Scan ()
{
    CommonLang::Scanner::Token::Type scanner_token_type = m_scanner.Scan(&m_lookahead_token);
    assert(scanner_token_type >= 0);
    if (scanner_token_type < 0x100)
        return Parser::Token::Type(scanner_token_type);
    switch (scanner_token_type)
    {
        case CommonLang::Scanner::Token::BAD_END_OF_FILE:            return Parser::Token::END_;
        case CommonLang::Scanner::Token::BAD_TOKEN:                  return Parser::Token::BAD_TOKEN;
        case CommonLang::Scanner::Token::DIRECTIVE_LANGUAGE:         return Parser::Token::DIRECTIVE_LANGUAGE;
        case CommonLang::Scanner::Token::DIRECTIVE_MACRO:            return Parser::Token::DIRECTIVE_MACRO;
        case CommonLang::Scanner::Token::DIRECTIVE_START:            return Parser::Token::DIRECTIVE_START;
        case CommonLang::Scanner::Token::DIRECTIVE_STATE:            return Parser::Token::DIRECTIVE_STATE;
        case CommonLang::Scanner::Token::DIRECTIVE_TARGETS: return Parser::Token::DIRECTIVE_TARGETS;
        case CommonLang::Scanner::Token::DUMB_CODE_BLOCK:            return Parser::Token::DUMB_CODE_BLOCK;
        case CommonLang::Scanner::Token::END_OF_FILE:                return Parser::Token::END_;
        case CommonLang::Scanner::Token::END_PREAMBLE:               return Parser::Token::END_PREAMBLE;
        case CommonLang::Scanner::Token::ID:                 return Parser::Token::ID;
        case CommonLang::Scanner::Token::NEWLINE:                    return Parser::Token::NEWLINE;
        case CommonLang::Scanner::Token::REGEX:                      return Parser::Token::REGEX;
        case CommonLang::Scanner::Token::STRICT_CODE_BLOCK:          return Parser::Token::STRICT_CODE_BLOCK;
        case CommonLang::Scanner::Token::STRING_LITERAL:             return Parser::Token::STRING_LITERAL;

        case CommonLang::Scanner::Token::CHAR_LITERAL:
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_CODESPEC:
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_OPTIONAL_DIRECTIVE:
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_REQUIRED_DIRECTIVE:
        case CommonLang::Scanner::Token::DIRECTIVE_DEFAULT:
        case CommonLang::Scanner::Token::DIRECTIVE_DUMB_CODE_BLOCK:
        case CommonLang::Scanner::Token::DIRECTIVE_ERROR:
        case CommonLang::Scanner::Token::DIRECTIVE_ID:
        case CommonLang::Scanner::Token::DIRECTIVE_LEFT:
        case CommonLang::Scanner::Token::DIRECTIVE_NONASSOC:
        case CommonLang::Scanner::Token::DIRECTIVE_PREC:
        case CommonLang::Scanner::Token::DIRECTIVE_RIGHT:
        case CommonLang::Scanner::Token::DIRECTIVE_STRICT_CODE_BLOCK:
        case CommonLang::Scanner::Token::DIRECTIVE_STRING:
        case CommonLang::Scanner::Token::DIRECTIVE_TARGET:
        case CommonLang::Scanner::Token::DIRECTIVE_TOKEN:
        case CommonLang::Scanner::Token::DIRECTIVE_TYPE:
            assert(m_lookahead_token != NULL);
            EmitError(m_lookahead_token->GetFiLoc(), "unrecognized token encountered in langspec");
            delete m_lookahead_token;
            m_lookahead_token = NULL;
            return Parser::Token::BAD_TOKEN;

        default:
            assert(false && "this should never happen");
            return Parser::Token::BAD_TOKEN;
    }
}

} // end of namespace Reflex

#line 2120 "reflex_parser.cpp"

