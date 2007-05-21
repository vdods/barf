#include "reflex_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x


#line 68 "reflex_parser.trison"

#include <sstream>

#include "barf_optionsbase.hpp"
#include "barf_regex_ast.hpp"
#include "barf_regex_parser.hpp"
#include "reflex_ast.hpp"

namespace Reflex {

#line 24 "reflex_parser.cpp"

Parser::Parser ()

{

#line 79 "reflex_parser.trison"

    m_target_map = NULL;
    m_regex_macro_map = NULL;

#line 35 "reflex_parser.cpp"
    m_debug_spew_level = 0;
    DEBUG_SPEW_2("### number of state transitions = " << ms_state_transition_count << std::endl);
}

Parser::~Parser ()
{

#line 84 "reflex_parser.trison"


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


#line 87 "reflex_parser.trison"

    delete token;

#line 394 "reflex_parser.cpp"
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
        "DIRECTIVE_MACRO",
        "DIRECTIVE_START_IN_SCANNER_MODE",
        "DIRECTIVE_STATE",
        "DIRECTIVE_TARGET",
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
        "macro_directives",
        "root",
        "rule",
        "rule_handler",
        "rule_handlers",
        "rule_list",
        "scanner_mode",
        "scanner_mode_rules",
        "scanner_modes",
        "start_in_scanner_mode_directive",
        "target_directive",
        "target_directive_param",
        "target_directives",
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

}

// rule 1: root <- at_least_zero_newlines targets_directive:target_map target_directives macro_directives:regular_expression_map start_in_scanner_mode_directive:start_in_scanner_mode_directive END_PREAMBLE:throwaway scanner_modes:scanner_mode_map    
Ast::Base * Parser::ReductionRuleHandler0001 ()
{
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    Regex::RegularExpressionMap * regular_expression_map = Dsc< Regex::RegularExpressionMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(4 < m_reduction_rule_token_count);
    StartInScannerModeDirective * start_in_scanner_mode_directive = Dsc< StartInScannerModeDirective * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);
    assert(5 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);
    assert(6 < m_reduction_rule_token_count);
    ScannerModeMap * scanner_mode_map = Dsc< ScannerModeMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 6]);

#line 187 "reflex_parser.trison"

        assert(m_target_map != NULL);
        assert(target_map == m_target_map);

        // set the TargetMap's primary source path
        target_map->SetSourcePath(m_scanner.GetInputName());
        // make sure the %start_in_scanner_mode directive value specifies a real scanner mode
        if (start_in_scanner_mode_directive != NULL &&
            scanner_mode_map->GetElement(start_in_scanner_mode_directive->m_scanner_mode_id->GetText()) == NULL)
        {
            EmitError(
                start_in_scanner_mode_directive->GetFiLoc(),
                "undeclared state \"" + start_in_scanner_mode_directive->m_scanner_mode_id->GetText() + "\"");
        }

        PrimarySource *primary_source =
            new PrimarySource(
                target_map,
                regular_expression_map,
                start_in_scanner_mode_directive,
                throwaway->GetFiLoc(),
                scanner_mode_map);
        delete throwaway;
        return primary_source;
    
#line 520 "reflex_parser.cpp"
}

// rule 2: targets_directive <- DIRECTIVE_TARGETS:throwaway target_ids:target_map at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 221 "reflex_parser.trison"

        assert(m_target_map == NULL);
        m_target_map = target_map;
        delete throwaway;
        return target_map;
    
#line 538 "reflex_parser.cpp"
}

// rule 3: targets_directive <-     
Ast::Base * Parser::ReductionRuleHandler0003 ()
{

#line 229 "reflex_parser.trison"

        assert(m_target_map == NULL);
        m_target_map = new CommonLang::TargetMap();
        return m_target_map;
    
#line 551 "reflex_parser.cpp"
}

// rule 4: targets_directive <- DIRECTIVE_TARGETS:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0004 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 236 "reflex_parser.trison"

        assert(m_target_map == NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in directive %targets");
        m_target_map = new CommonLang::TargetMap();
        return m_target_map;
    
#line 567 "reflex_parser.cpp"
}

// rule 5: target_ids <- target_ids:target_map ID:target_id    
Ast::Base * Parser::ReductionRuleHandler0005 ()
{
    assert(0 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 247 "reflex_parser.trison"

        CommonLang::Target *target = new CommonLang::Target(target_id->GetText());
        target->EnableCodeGeneration();
        target_map->Add(target_id->GetText(), target);
        return target_map;
    
#line 585 "reflex_parser.cpp"
}

// rule 6: target_ids <-     
Ast::Base * Parser::ReductionRuleHandler0006 ()
{

#line 255 "reflex_parser.trison"

        assert(m_target_map == NULL);
        return new CommonLang::TargetMap();
    
#line 597 "reflex_parser.cpp"
}

// rule 7: target_directives <- target_directives target_directive:target_directive    
Ast::Base * Parser::ReductionRuleHandler0007 ()
{
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetDirective * target_directive = Dsc< CommonLang::TargetDirective * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 268 "reflex_parser.trison"

        assert(m_target_map != NULL);
        if (target_directive != NULL)
            m_target_map->AddTargetDirective(target_directive);
        return NULL;
    
#line 613 "reflex_parser.cpp"
}

// rule 8: target_directives <-     
Ast::Base * Parser::ReductionRuleHandler0008 ()
{

#line 276 "reflex_parser.trison"

        if (m_target_map == NULL)
            m_target_map = new CommonLang::TargetMap();
        return NULL;
    
#line 626 "reflex_parser.cpp"
}

// rule 9: target_directive <- DIRECTIVE_TARGET:throwaway '.' ID:target_id '.' ID:target_directive target_directive_param:param at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0009 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(4 < m_reduction_rule_token_count);
    Ast::Id * target_directive = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);
    assert(5 < m_reduction_rule_token_count);
    Ast::TextBase * param = Dsc< Ast::TextBase * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);

#line 286 "reflex_parser.trison"

        delete throwaway;
        return new CommonLang::TargetDirective(target_id, target_directive, param);
    
#line 646 "reflex_parser.cpp"
}

// rule 10: target_directive <- DIRECTIVE_TARGET:throwaway '.' ID:target_id '.' ID:target_directive %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0010 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(4 < m_reduction_rule_token_count);
    Ast::Id * target_directive = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);

#line 292 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in parameter for directive %target." + target_id->GetText() + "." + target_directive->GetText());
        delete throwaway;
        delete target_id;
        delete target_directive;
        return NULL;
    
#line 667 "reflex_parser.cpp"
}

// rule 11: target_directive <- DIRECTIVE_TARGET:throwaway '.' ID:target_id %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0011 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 301 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in directive name for directive %target." + target_id->GetText());
        delete throwaway;
        delete target_id;
        return NULL;
    
#line 685 "reflex_parser.cpp"
}

// rule 12: target_directive <- DIRECTIVE_TARGET:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0012 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 309 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in target name for directive %target");
        delete throwaway;
        return NULL;
    
#line 700 "reflex_parser.cpp"
}

// rule 13: target_directive_param <- ID:value    
Ast::Base * Parser::ReductionRuleHandler0013 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * value = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 318 "reflex_parser.trison"
 return value; 
#line 711 "reflex_parser.cpp"
}

// rule 14: target_directive_param <- STRING_LITERAL:value    
Ast::Base * Parser::ReductionRuleHandler0014 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::String * value = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 319 "reflex_parser.trison"
 return value; 
#line 722 "reflex_parser.cpp"
}

// rule 15: target_directive_param <- STRICT_CODE_BLOCK:value    
Ast::Base * Parser::ReductionRuleHandler0015 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * value = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 320 "reflex_parser.trison"
 return value; 
#line 733 "reflex_parser.cpp"
}

// rule 16: target_directive_param <- DUMB_CODE_BLOCK:value    
Ast::Base * Parser::ReductionRuleHandler0016 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * value = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 321 "reflex_parser.trison"
 return value; 
#line 744 "reflex_parser.cpp"
}

// rule 17: target_directive_param <-     
Ast::Base * Parser::ReductionRuleHandler0017 ()
{

#line 322 "reflex_parser.trison"
 return NULL; 
#line 753 "reflex_parser.cpp"
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

#line 332 "reflex_parser.trison"

        Regex::Parser parser;
        parser.ScannerDebugSpew(g_options->GetIsVerbose(OptionsBase::V_REGEX_SCANNER));
        if (g_options->GetIsVerbose(OptionsBase::V_REGEX_PARSER))
            parser.SetDebugSpewLevel(2);
        istringstream in(macro_regex_string->GetText());
        parser.OpenUsingStream(&in, "%macro " + macro_regex_string->GetText(), false);
        try {
            Regex::RegularExpression *regex = NULL;
            if (parser.Parse(&regex, regular_expression_map) == Regex::Parser::PRC_SUCCESS)
            {
                if (g_options->GetIsVerbose(OptionsBase::V_REGEX_AST))
                    regex->Print(cerr);
                regular_expression_map->Add(macro_id->GetText(), regex);
            }
            else
                EmitError(throwaway->GetFiLoc(), "parse error in regular expression (" + macro_regex_string->GetText() + ")");
        } catch (string const &exception) {
            EmitError(throwaway->GetFiLoc(), exception + " in regular expression (" + macro_regex_string->GetText() + ")");
        }
        delete throwaway;
        delete macro_id;
        delete macro_regex_string;
        return regular_expression_map;
    
#line 794 "reflex_parser.cpp"
}

// rule 19: macro_directives <-     
Ast::Base * Parser::ReductionRuleHandler0019 ()
{

#line 359 "reflex_parser.trison"

        // we save the regex macro map in a member var, so that the reduction
        // rule handler for the scanner mode rules can use it.
        m_regex_macro_map = new Regex::RegularExpressionMap();
        return m_regex_macro_map;
    
#line 808 "reflex_parser.cpp"
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

#line 367 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in directive %macro " + macro_id->GetText());
        delete throwaway;
        delete macro_id;
        return regular_expression_map;
    
#line 828 "reflex_parser.cpp"
}

// rule 21: macro_directives <- macro_directives:regular_expression_map DIRECTIVE_MACRO:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0021 ()
{
    assert(0 < m_reduction_rule_token_count);
    Regex::RegularExpressionMap * regular_expression_map = Dsc< Regex::RegularExpressionMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 375 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in directive %macro");
        delete throwaway;
        return regular_expression_map;
    
#line 845 "reflex_parser.cpp"
}

// rule 22: start_in_scanner_mode_directive <- DIRECTIVE_START_IN_SCANNER_MODE:throwaway ID:scanner_mode_id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0022 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * scanner_mode_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 389 "reflex_parser.trison"

        delete throwaway;
        return new StartInScannerModeDirective(scanner_mode_id);
    
#line 861 "reflex_parser.cpp"
}

// rule 23: start_in_scanner_mode_directive <- DIRECTIVE_START_IN_SCANNER_MODE:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0023 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 395 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in directive %start_in_scanner_mode");
        delete throwaway;
        return NULL;
    
#line 876 "reflex_parser.cpp"
}

// rule 24: scanner_modes <- scanner_modes:scanner_mode_map scanner_mode:scanner_mode    
Ast::Base * Parser::ReductionRuleHandler0024 ()
{
    assert(0 < m_reduction_rule_token_count);
    ScannerModeMap * scanner_mode_map = Dsc< ScannerModeMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    ScannerMode * scanner_mode = Dsc< ScannerMode * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 409 "reflex_parser.trison"

        if (scanner_mode != NULL)
            scanner_mode_map->Add(scanner_mode->m_scanner_mode_id->GetText(), scanner_mode);
        return scanner_mode_map;
    
#line 893 "reflex_parser.cpp"
}

// rule 25: scanner_modes <-     
Ast::Base * Parser::ReductionRuleHandler0025 ()
{

#line 416 "reflex_parser.trison"

        return new ScannerModeMap();
    
#line 904 "reflex_parser.cpp"
}

// rule 26: scanner_mode <- DIRECTIVE_STATE:throwaway ID:scanner_mode_id ':' scanner_mode_rules:rule_list ';'    
Ast::Base * Parser::ReductionRuleHandler0026 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * scanner_mode_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 424 "reflex_parser.trison"

        delete throwaway;
        return new ScannerMode(scanner_mode_id, rule_list);
    
#line 922 "reflex_parser.cpp"
}

// rule 27: scanner_mode <- DIRECTIVE_STATE:throwaway ID:scanner_mode_id ':' %error ';'    
Ast::Base * Parser::ReductionRuleHandler0027 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * scanner_mode_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 430 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in scanner mode rule list");
        delete throwaway;
        return new ScannerMode(scanner_mode_id, new RuleList());
    
#line 939 "reflex_parser.cpp"
}

// rule 28: scanner_mode <- DIRECTIVE_STATE:throwaway %error ':' scanner_mode_rules:rule_list ';'    
Ast::Base * Parser::ReductionRuleHandler0028 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 437 "reflex_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in scanner mode id");
        delete throwaway;
        delete rule_list;
        return NULL;
    
#line 957 "reflex_parser.cpp"
}

// rule 29: scanner_mode_rules <- rule_list:rule_list    
Ast::Base * Parser::ReductionRuleHandler0029 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 448 "reflex_parser.trison"

        return rule_list;
    
#line 970 "reflex_parser.cpp"
}

// rule 30: scanner_mode_rules <-     
Ast::Base * Parser::ReductionRuleHandler0030 ()
{

#line 453 "reflex_parser.trison"

        return new RuleList();
    
#line 981 "reflex_parser.cpp"
}

// rule 31: rule_list <- rule_list:rule_list '|' rule:rule    
Ast::Base * Parser::ReductionRuleHandler0031 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 461 "reflex_parser.trison"

        rule_list->Append(rule);
        return rule_list;
    
#line 997 "reflex_parser.cpp"
}

// rule 32: rule_list <- rule:rule    
Ast::Base * Parser::ReductionRuleHandler0032 ()
{
    assert(0 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 467 "reflex_parser.trison"

        RuleList *rule_list = new RuleList();
        rule_list->Append(rule);
        return rule_list;
    
#line 1012 "reflex_parser.cpp"
}

// rule 33: rule <- REGEX:regex_string rule_handlers:rule_handler_map    
Ast::Base * Parser::ReductionRuleHandler0033 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::String * regex_string = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::RuleHandlerMap * rule_handler_map = Dsc< CommonLang::RuleHandlerMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 477 "reflex_parser.trison"

        assert(m_regex_macro_map != NULL);

        // parse the rule regex
        Regex::RegularExpression *regex = NULL;
        {
            Regex::Parser parser;
            parser.ScannerDebugSpew(g_options->GetIsVerbose(OptionsBase::V_REGEX_SCANNER));
            if (g_options->GetIsVerbose(OptionsBase::V_REGEX_PARSER))
                parser.SetDebugSpewLevel(2);
            istringstream in(regex_string->GetText());
            parser.OpenUsingStream(&in, regex_string->GetText(), false);
            try {
                if (parser.Parse(&regex, m_regex_macro_map) == Regex::Parser::PRC_SUCCESS)
                {
                    if (g_options->GetIsVerbose(OptionsBase::V_REGEX_AST))
                        regex->Print(cerr);
                }
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
    
#line 1085 "reflex_parser.cpp"
}

// rule 34: rule_handlers <- rule_handlers:rule_handler_map rule_handler:rule_handler    
Ast::Base * Parser::ReductionRuleHandler0034 ()
{
    assert(0 < m_reduction_rule_token_count);
    CommonLang::RuleHandlerMap * rule_handler_map = Dsc< CommonLang::RuleHandlerMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::RuleHandler * rule_handler = Dsc< CommonLang::RuleHandler * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 543 "reflex_parser.trison"

        if (rule_handler != NULL)
            rule_handler_map->Add(rule_handler->m_target_id->GetText(), rule_handler);
        return rule_handler_map;
    
#line 1102 "reflex_parser.cpp"
}

// rule 35: rule_handlers <-     
Ast::Base * Parser::ReductionRuleHandler0035 ()
{

#line 550 "reflex_parser.trison"

        return new CommonLang::RuleHandlerMap();
    
#line 1113 "reflex_parser.cpp"
}

// rule 36: rule_handler <- DIRECTIVE_TARGET:throwaway '.' ID:target_id any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0036 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(3 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 558 "reflex_parser.trison"

        delete throwaway;
        assert(m_target_map != NULL);
        if (m_target_map->GetElement(target_id->GetText()) == NULL)
            EmitWarning(
                target_id->GetFiLoc(),
                "undeclared target \"" + target_id->GetText() + "\"");
        return new CommonLang::RuleHandler(target_id, code_block);
    
#line 1136 "reflex_parser.cpp"
}

// rule 37: rule_handler <- DIRECTIVE_TARGET:throwaway %error any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0037 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 569 "reflex_parser.trison"

        assert(m_target_map != NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in target id after directive %target");
        delete throwaway;
        delete code_block;
        return NULL;
    
#line 1155 "reflex_parser.cpp"
}

// rule 38: rule_handler <- DIRECTIVE_TARGET:throwaway %error    
Ast::Base * Parser::ReductionRuleHandler0038 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 578 "reflex_parser.trison"

        assert(m_target_map != NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in directive %target");
        delete throwaway;
        return NULL;
    
#line 1171 "reflex_parser.cpp"
}

// rule 39: rule_handler <- %error any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0039 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 586 "reflex_parser.trison"

        assert(m_target_map != NULL);
        EmitError(code_block->GetFiLoc(), "missing directive %target before rule handler code block");
        delete code_block;
        return NULL;
    
#line 1187 "reflex_parser.cpp"
}

// rule 40: any_type_of_code_block <- DUMB_CODE_BLOCK:dumb_code_block    
Ast::Base * Parser::ReductionRuleHandler0040 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * dumb_code_block = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 600 "reflex_parser.trison"
 return dumb_code_block; 
#line 1198 "reflex_parser.cpp"
}

// rule 41: any_type_of_code_block <- STRICT_CODE_BLOCK:strict_code_block    
Ast::Base * Parser::ReductionRuleHandler0041 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * strict_code_block = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 602 "reflex_parser.trison"
 return strict_code_block; 
#line 1209 "reflex_parser.cpp"
}

// rule 42: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0042 ()
{

#line 607 "reflex_parser.trison"
 return NULL; 
#line 1218 "reflex_parser.cpp"
}

// rule 43: at_least_zero_newlines <-     
Ast::Base * Parser::ReductionRuleHandler0043 ()
{

#line 609 "reflex_parser.trison"
 return NULL; 
#line 1227 "reflex_parser.cpp"
}

// rule 44: at_least_one_newline <- at_least_one_newline NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0044 ()
{

#line 614 "reflex_parser.trison"
 return NULL; 
#line 1236 "reflex_parser.cpp"
}

// rule 45: at_least_one_newline <- NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0045 ()
{

#line 616 "reflex_parser.trison"
 return NULL; 
#line 1245 "reflex_parser.cpp"
}



// ///////////////////////////////////////////////////////////////////////////
// reduction rule lookup table
// ///////////////////////////////////////////////////////////////////////////

Parser::ReductionRule const Parser::ms_reduction_rule[] =
{
    {                 Token::START_,  2, &Parser::ReductionRuleHandler0000, "rule 0: %start <- root END_    "},
    {                 Token::root__,  7, &Parser::ReductionRuleHandler0001, "rule 1: root <- at_least_zero_newlines targets_directive target_directives macro_directives start_in_scanner_mode_directive END_PREAMBLE scanner_modes    "},
    {    Token::targets_directive__,  3, &Parser::ReductionRuleHandler0002, "rule 2: targets_directive <- DIRECTIVE_TARGETS target_ids at_least_one_newline    "},
    {    Token::targets_directive__,  0, &Parser::ReductionRuleHandler0003, "rule 3: targets_directive <-     "},
    {    Token::targets_directive__,  3, &Parser::ReductionRuleHandler0004, "rule 4: targets_directive <- DIRECTIVE_TARGETS %error at_least_one_newline    "},
    {           Token::target_ids__,  2, &Parser::ReductionRuleHandler0005, "rule 5: target_ids <- target_ids ID    "},
    {           Token::target_ids__,  0, &Parser::ReductionRuleHandler0006, "rule 6: target_ids <-     "},
    {    Token::target_directives__,  2, &Parser::ReductionRuleHandler0007, "rule 7: target_directives <- target_directives target_directive    "},
    {    Token::target_directives__,  0, &Parser::ReductionRuleHandler0008, "rule 8: target_directives <-     "},
    {     Token::target_directive__,  7, &Parser::ReductionRuleHandler0009, "rule 9: target_directive <- DIRECTIVE_TARGET '.' ID '.' ID target_directive_param at_least_one_newline    "},
    {     Token::target_directive__,  7, &Parser::ReductionRuleHandler0010, "rule 10: target_directive <- DIRECTIVE_TARGET '.' ID '.' ID %error at_least_one_newline    "},
    {     Token::target_directive__,  5, &Parser::ReductionRuleHandler0011, "rule 11: target_directive <- DIRECTIVE_TARGET '.' ID %error at_least_one_newline    "},
    {     Token::target_directive__,  3, &Parser::ReductionRuleHandler0012, "rule 12: target_directive <- DIRECTIVE_TARGET %error at_least_one_newline    "},
    {Token::target_directive_param__,  1, &Parser::ReductionRuleHandler0013, "rule 13: target_directive_param <- ID    "},
    {Token::target_directive_param__,  1, &Parser::ReductionRuleHandler0014, "rule 14: target_directive_param <- STRING_LITERAL    "},
    {Token::target_directive_param__,  1, &Parser::ReductionRuleHandler0015, "rule 15: target_directive_param <- STRICT_CODE_BLOCK    "},
    {Token::target_directive_param__,  1, &Parser::ReductionRuleHandler0016, "rule 16: target_directive_param <- DUMB_CODE_BLOCK    "},
    {Token::target_directive_param__,  0, &Parser::ReductionRuleHandler0017, "rule 17: target_directive_param <-     "},
    {     Token::macro_directives__,  5, &Parser::ReductionRuleHandler0018, "rule 18: macro_directives <- macro_directives DIRECTIVE_MACRO ID REGEX at_least_one_newline    "},
    {     Token::macro_directives__,  0, &Parser::ReductionRuleHandler0019, "rule 19: macro_directives <-     "},
    {     Token::macro_directives__,  5, &Parser::ReductionRuleHandler0020, "rule 20: macro_directives <- macro_directives DIRECTIVE_MACRO ID %error at_least_one_newline    "},
    {     Token::macro_directives__,  4, &Parser::ReductionRuleHandler0021, "rule 21: macro_directives <- macro_directives DIRECTIVE_MACRO %error at_least_one_newline    "},
    {Token::start_in_scanner_mode_directive__,  3, &Parser::ReductionRuleHandler0022, "rule 22: start_in_scanner_mode_directive <- DIRECTIVE_START_IN_SCANNER_MODE ID at_least_one_newline    "},
    {Token::start_in_scanner_mode_directive__,  3, &Parser::ReductionRuleHandler0023, "rule 23: start_in_scanner_mode_directive <- DIRECTIVE_START_IN_SCANNER_MODE %error at_least_one_newline    "},
    {        Token::scanner_modes__,  2, &Parser::ReductionRuleHandler0024, "rule 24: scanner_modes <- scanner_modes scanner_mode    "},
    {        Token::scanner_modes__,  0, &Parser::ReductionRuleHandler0025, "rule 25: scanner_modes <-     "},
    {         Token::scanner_mode__,  5, &Parser::ReductionRuleHandler0026, "rule 26: scanner_mode <- DIRECTIVE_STATE ID ':' scanner_mode_rules ';'    "},
    {         Token::scanner_mode__,  5, &Parser::ReductionRuleHandler0027, "rule 27: scanner_mode <- DIRECTIVE_STATE ID ':' %error ';'    "},
    {         Token::scanner_mode__,  5, &Parser::ReductionRuleHandler0028, "rule 28: scanner_mode <- DIRECTIVE_STATE %error ':' scanner_mode_rules ';'    "},
    {   Token::scanner_mode_rules__,  1, &Parser::ReductionRuleHandler0029, "rule 29: scanner_mode_rules <- rule_list    "},
    {   Token::scanner_mode_rules__,  0, &Parser::ReductionRuleHandler0030, "rule 30: scanner_mode_rules <-     "},
    {            Token::rule_list__,  3, &Parser::ReductionRuleHandler0031, "rule 31: rule_list <- rule_list '|' rule    "},
    {            Token::rule_list__,  1, &Parser::ReductionRuleHandler0032, "rule 32: rule_list <- rule    "},
    {                 Token::rule__,  2, &Parser::ReductionRuleHandler0033, "rule 33: rule <- REGEX rule_handlers    "},
    {        Token::rule_handlers__,  2, &Parser::ReductionRuleHandler0034, "rule 34: rule_handlers <- rule_handlers rule_handler    "},
    {        Token::rule_handlers__,  0, &Parser::ReductionRuleHandler0035, "rule 35: rule_handlers <-     "},
    {         Token::rule_handler__,  4, &Parser::ReductionRuleHandler0036, "rule 36: rule_handler <- DIRECTIVE_TARGET '.' ID any_type_of_code_block    "},
    {         Token::rule_handler__,  3, &Parser::ReductionRuleHandler0037, "rule 37: rule_handler <- DIRECTIVE_TARGET %error any_type_of_code_block    "},
    {         Token::rule_handler__,  2, &Parser::ReductionRuleHandler0038, "rule 38: rule_handler <- DIRECTIVE_TARGET %error    "},
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
    {  17,    1,    0,   18,    1}, // state    7
    {  19,    2,    0,   21,    1}, // state    8
    {  22,    1,   23,   24,    2}, // state    9
    {   0,    0,   26,    0,    0}, // state   10
    {  27,    1,   28,    0,    0}, // state   11
    {   0,    0,   29,    0,    0}, // state   12
    {  30,    1,   31,    0,    0}, // state   13
    {  32,    2,    0,    0,    0}, // state   14
    {   0,    0,   34,    0,    0}, // state   15
    {  35,    2,    0,   37,    1}, // state   16
    {   0,    0,   38,    0,    0}, // state   17
    {  39,    1,    0,   40,    1}, // state   18
    {  41,    1,    0,    0,    0}, // state   19
    {  42,    2,    0,    0,    0}, // state   20
    {  44,    2,    0,    0,    0}, // state   21
    {  46,    1,    0,    0,    0}, // state   22
    {  47,    1,   48,    0,    0}, // state   23
    {  49,    2,    0,    0,    0}, // state   24
    {  51,    1,    0,   52,    1}, // state   25
    {  53,    2,    0,    0,    0}, // state   26
    {  55,    1,    0,   56,    1}, // state   27
    {  57,    1,    0,   58,    1}, // state   28
    {   0,    0,   59,   60,    1}, // state   29
    {  61,    1,    0,   62,    1}, // state   30
    {  63,    1,    0,    0,    0}, // state   31
    {  64,    1,   65,    0,    0}, // state   32
    {  66,    1,    0,   67,    1}, // state   33
    {  68,    1,    0,   69,    1}, // state   34
    {  70,    1,   71,    0,    0}, // state   35
    {  72,    1,   73,    0,    0}, // state   36
    {  74,    1,   75,   76,    1}, // state   37
    {  77,    1,   78,    0,    0}, // state   38
    {  79,    6,    0,   85,    1}, // state   39
    {  86,    1,   87,    0,    0}, // state   40
    {  88,    1,   89,    0,    0}, // state   41
    {  90,    2,    0,    0,    0}, // state   42
    {   0,    0,   92,    0,    0}, // state   43
    {  93,    1,    0,   94,    1}, // state   44
    {   0,    0,   95,    0,    0}, // state   45
    {   0,    0,   96,    0,    0}, // state   46
    {   0,    0,   97,    0,    0}, // state   47
    {   0,    0,   98,    0,    0}, // state   48
    {  99,    1,    0,  100,    1}, // state   49
    { 101,    1,    0,    0,    0}, // state   50
    { 102,    1,    0,    0,    0}, // state   51
    { 103,    1,  104,    0,    0}, // state   52
    { 105,    1,  106,    0,    0}, // state   53
    { 107,    1,  108,  109,    3}, // state   54
    { 112,    3,    0,  115,    3}, // state   55
    {   0,    0,  118,  119,    1}, // state   56
    { 120,    1,    0,    0,    0}, // state   57
    { 121,    1,  122,    0,    0}, // state   58
    {   0,    0,  123,    0,    0}, // state   59
    { 124,    1,    0,    0,    0}, // state   60
    { 125,    1,    0,    0,    0}, // state   61
    { 126,    4,    0,  130,    1}, // state   62
    {   0,    0,  131,    0,    0}, // state   63
    { 132,    1,    0,  133,    1}, // state   64
    {   0,    0,  134,    0,    0}, // state   65
    {   0,    0,  135,    0,    0}, // state   66
    { 136,    2,    0,  138,    1}, // state   67
    { 139,    2,    0,    0,    0}, // state   68
    {   0,    0,  141,    0,    0}, // state   69
    {   0,    0,  142,    0,    0}, // state   70
    {   0,    0,  143,    0,    0}, // state   71
    {   0,    0,  144,    0,    0}, // state   72
    {   0,    0,  145,    0,    0}, // state   73
    { 146,    2,  148,  149,    1}, // state   74
    { 150,    1,    0,    0,    0}, // state   75
    {   0,    0,  151,    0,    0}, // state   76
    { 152,    2,    0,  154,    1}, // state   77
    {   0,    0,  155,    0,    0}  // state   78

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
    {    Token::target_directives__, {                  TA_PUSH_STATE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state    7
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
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
    {       Token::DIRECTIVE_TARGET, {        TA_SHIFT_AND_PUSH_STATE,   14}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   19}},
    // nonterminal transitions
    {     Token::target_directive__, {                  TA_PUSH_STATE,   15}},
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
    {Token::DIRECTIVE_START_IN_SCANNER_MODE, {        TA_SHIFT_AND_PUSH_STATE,   21}},
    // nonterminal transitions
    {Token::start_in_scanner_mode_directive__, {                  TA_PUSH_STATE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   17
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state   18
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
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
    {        Token::scanner_modes__, {                  TA_PUSH_STATE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state   30
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
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
    {         Token::scanner_mode__, {                  TA_PUSH_STATE,   43}},

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
    {Token::target_directive_param__, {                  TA_PUSH_STATE,   49}},

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
    {   Token::scanner_mode_rules__, {                  TA_PUSH_STATE,   57}},
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
    {   Token::scanner_mode_rules__, {                  TA_PUSH_STATE,   61}},
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
    {       Token::DIRECTIVE_TARGET, {        TA_SHIFT_AND_PUSH_STATE,   68}},
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
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   71}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   72}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   38}},
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


#line 91 "reflex_parser.trison"

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
        case CommonLang::Scanner::Token::BAD_END_OF_FILE:                 return Parser::Token::END_;
        case CommonLang::Scanner::Token::BAD_TOKEN:                       return Parser::Token::BAD_TOKEN;
        case CommonLang::Scanner::Token::DIRECTIVE_MACRO:                 return Parser::Token::DIRECTIVE_MACRO;
        case CommonLang::Scanner::Token::DIRECTIVE_START_IN_SCANNER_MODE: return Parser::Token::DIRECTIVE_START_IN_SCANNER_MODE;
        case CommonLang::Scanner::Token::DIRECTIVE_STATE:                 return Parser::Token::DIRECTIVE_STATE;
        case CommonLang::Scanner::Token::DIRECTIVE_TARGET:                return Parser::Token::DIRECTIVE_TARGET;
        case CommonLang::Scanner::Token::DIRECTIVE_TARGETS:               return Parser::Token::DIRECTIVE_TARGETS;
        case CommonLang::Scanner::Token::DUMB_CODE_BLOCK:                 return Parser::Token::DUMB_CODE_BLOCK;
        case CommonLang::Scanner::Token::END_OF_FILE:                     return Parser::Token::END_;
        case CommonLang::Scanner::Token::END_PREAMBLE:                    return Parser::Token::END_PREAMBLE;
        case CommonLang::Scanner::Token::ID:                              return Parser::Token::ID;
        case CommonLang::Scanner::Token::NEWLINE:                         return Parser::Token::NEWLINE;
        case CommonLang::Scanner::Token::REGEX:                           return Parser::Token::REGEX;
        case CommonLang::Scanner::Token::STRICT_CODE_BLOCK:               return Parser::Token::STRICT_CODE_BLOCK;
        case CommonLang::Scanner::Token::STRING_LITERAL:                  return Parser::Token::STRING_LITERAL;

        case CommonLang::Scanner::Token::CHAR_LITERAL:
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_CODESPEC:
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_OPTIONAL_DIRECTIVE:
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_REQUIRED_DIRECTIVE:
        case CommonLang::Scanner::Token::DIRECTIVE_DEFAULT:
        case CommonLang::Scanner::Token::DIRECTIVE_DEFAULT_PARSE_NONTERMINAL:
        case CommonLang::Scanner::Token::DIRECTIVE_DUMB_CODE_BLOCK:
        case CommonLang::Scanner::Token::DIRECTIVE_ERROR:
        case CommonLang::Scanner::Token::DIRECTIVE_ID:
        case CommonLang::Scanner::Token::DIRECTIVE_NONTERMINAL:
        case CommonLang::Scanner::Token::DIRECTIVE_PREC:
        case CommonLang::Scanner::Token::DIRECTIVE_STRICT_CODE_BLOCK:
        case CommonLang::Scanner::Token::DIRECTIVE_STRING:
        case CommonLang::Scanner::Token::DIRECTIVE_TERMINAL:
        case CommonLang::Scanner::Token::DIRECTIVE_TYPE:
            assert(m_lookahead_token != NULL);
            EmitError(m_lookahead_token->GetFiLoc(), "unrecognized token encountered in targetspec");
            delete m_lookahead_token;
            m_lookahead_token = NULL;
            return Parser::Token::BAD_TOKEN;

        default:
            assert(false && "this should never happen");
            return Parser::Token::BAD_TOKEN;
    }
}

} // end of namespace Reflex

#line 2074 "reflex_parser.cpp"

