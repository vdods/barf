#include "trison_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x


#line 73 "trison_parser.trison"

#include <sstream>

#include "trison_ast.hpp"

namespace Trison {

#line 21 "trison_parser.cpp"

Parser::Parser ()

{

#line 81 "trison_parser.trison"


#line 30 "trison_parser.cpp"
    m_debug_spew_level = 0;
    DEBUG_SPEW_2("### number of state transitions = " << ms_state_transition_count << std::endl);
}

Parser::~Parser ()
{

#line 84 "trison_parser.trison"


#line 41 "trison_parser.cpp"
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

#line 91 "trison_parser.trison"

    m_target_map = NULL;
    m_terminal_map = NULL;
    m_precedence_list = NULL;
    m_precedence_map = NULL;
    m_nonterminal_list = NULL;
    m_rule_count = 0;

#line 87 "trison_parser.cpp"

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


#line 87 "trison_parser.trison"

    delete token;

#line 399 "trison_parser.cpp"
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
        "CHAR_LITERAL",
        "DIRECTIVE_DEFAULT_PARSE_NONTERMINAL",
        "DIRECTIVE_ERROR",
        "DIRECTIVE_NONTERMINAL",
        "DIRECTIVE_PREC",
        "DIRECTIVE_TARGET",
        "DIRECTIVE_TARGETS",
        "DIRECTIVE_TERMINAL",
        "DIRECTIVE_TYPE",
        "DUMB_CODE_BLOCK",
        "END_PREAMBLE",
        "ID",
        "NEWLINE",
        "STRICT_CODE_BLOCK",
        "STRING_LITERAL",
        "END_",

        "any_type_of_code_block",
        "at_least_one_newline",
        "at_least_zero_newlines",
        "nonterminal",
        "nonterminal_specification",
        "nonterminals",
        "precedence_directive",
        "precedence_directives",
        "root",
        "rule",
        "rule_handler",
        "rule_handlers",
        "rule_precedence_directive",
        "rule_specification",
        "rule_token",
        "rule_token_list",
        "rules",
        "start_directive",
        "target_directive",
        "target_directive_param",
        "target_directives",
        "target_ids",
        "targets_directive",
        "terminal",
        "terminal_directive",
        "terminal_directives",
        "terminals",
        "token_id",
        "type_spec",
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

// rule 1: root <- at_least_zero_newlines targets_directive:target_map target_directives terminal_directives:terminal_map precedence_directives:precedence_map start_directive:default_parse_nonterminal_id END_PREAMBLE:throwaway nonterminals:nonterminal_map    
Ast::Base * Parser::ReductionRuleHandler0001 ()
{
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    TerminalMap * terminal_map = Dsc< TerminalMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(4 < m_reduction_rule_token_count);
    PrecedenceMap * precedence_map = Dsc< PrecedenceMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);
    assert(5 < m_reduction_rule_token_count);
    Ast::Id * default_parse_nonterminal_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);
    assert(6 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 6]);
    assert(7 < m_reduction_rule_token_count);
    NonterminalMap * nonterminal_map = Dsc< NonterminalMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 7]);

#line 199 "trison_parser.trison"

        assert(m_target_map != NULL);
        assert(target_map == m_target_map);
        assert(m_terminal_map != NULL);
        assert(terminal_map == m_terminal_map);
        assert(m_precedence_list != NULL);
        assert(precedence_map != NULL);
        assert(m_precedence_map == precedence_map);
        assert(m_nonterminal_list != NULL);

        // set the TargetMap's primary source path
        target_map->SetSourcePath(m_scanner.GetInputName());
        // make sure the %default_parse_nonterminal directive value specifies a real nonterminal
        if (default_parse_nonterminal_id != NULL &&
            nonterminal_map->GetElement(default_parse_nonterminal_id->GetText()) == NULL)
        {
            EmitError(
                default_parse_nonterminal_id->GetFiLoc(),
                "undeclared nonterminal \"" + default_parse_nonterminal_id->GetText() + "\"");
        }

        PrimarySource *primary_source =
            new PrimarySource(
                target_map,
                terminal_map,
                precedence_map,
                m_precedence_list,
                default_parse_nonterminal_id->GetText(),
                throwaway->GetFiLoc(),
                nonterminal_map,
                m_nonterminal_list);
        delete throwaway;
        delete default_parse_nonterminal_id;
        return primary_source;
    
#line 551 "trison_parser.cpp"
}

// rule 2: targets_directive <- DIRECTIVE_TARGETS:throwaway target_ids:target_map at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 243 "trison_parser.trison"

        assert(m_target_map == NULL);
        m_target_map = target_map;
        delete throwaway;
        return target_map;
    
#line 569 "trison_parser.cpp"
}

// rule 3: targets_directive <-     
Ast::Base * Parser::ReductionRuleHandler0003 ()
{

#line 251 "trison_parser.trison"

        assert(m_target_map == NULL);
        m_target_map = new CommonLang::TargetMap();
        return m_target_map;
    
#line 582 "trison_parser.cpp"
}

// rule 4: targets_directive <- DIRECTIVE_TARGETS:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0004 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 258 "trison_parser.trison"

        assert(m_target_map == NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in directive %targets");
        m_target_map = new CommonLang::TargetMap();
        return m_target_map;
    
#line 598 "trison_parser.cpp"
}

// rule 5: target_ids <- target_ids:target_map ID:target_id    
Ast::Base * Parser::ReductionRuleHandler0005 ()
{
    assert(0 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 269 "trison_parser.trison"

        CommonLang::Target *target = new CommonLang::Target(target_id->GetText());
        target->EnableCodeGeneration();
        target_map->Add(target_id->GetText(), target);
        return target_map;
    
#line 616 "trison_parser.cpp"
}

// rule 6: target_ids <-     
Ast::Base * Parser::ReductionRuleHandler0006 ()
{

#line 277 "trison_parser.trison"

        assert(m_target_map == NULL);
        return new CommonLang::TargetMap();
    
#line 628 "trison_parser.cpp"
}

// rule 7: target_directives <- target_directives target_directive:target_directive    
Ast::Base * Parser::ReductionRuleHandler0007 ()
{
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetDirective * target_directive = Dsc< CommonLang::TargetDirective * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 290 "trison_parser.trison"

        assert(m_target_map != NULL);
        if (target_directive != NULL)
            m_target_map->AddTargetDirective(target_directive);
        return NULL;
    
#line 644 "trison_parser.cpp"
}

// rule 8: target_directives <-     
Ast::Base * Parser::ReductionRuleHandler0008 ()
{

#line 298 "trison_parser.trison"

        if (m_target_map == NULL)
            m_target_map = new CommonLang::TargetMap();
        return NULL;
    
#line 657 "trison_parser.cpp"
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

#line 308 "trison_parser.trison"

        delete throwaway;
        return new CommonLang::TargetDirective(target_id, target_directive, param);
    
#line 677 "trison_parser.cpp"
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

#line 314 "trison_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in parameter for directive %target." + target_id->GetText() + "." + target_directive->GetText());
        delete throwaway;
        delete target_id;
        delete target_directive;
        return NULL;
    
#line 698 "trison_parser.cpp"
}

// rule 11: target_directive <- DIRECTIVE_TARGET:throwaway '.' ID:target_id %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0011 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 323 "trison_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in directive name for directive %target." + target_id->GetText());
        delete throwaway;
        delete target_id;
        return NULL;
    
#line 716 "trison_parser.cpp"
}

// rule 12: target_directive <- DIRECTIVE_TARGET:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0012 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 331 "trison_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in target name for directive %target");
        delete throwaway;
        return NULL;
    
#line 731 "trison_parser.cpp"
}

// rule 13: target_directive_param <- ID:value    
Ast::Base * Parser::ReductionRuleHandler0013 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * value = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 340 "trison_parser.trison"
 return value; 
#line 742 "trison_parser.cpp"
}

// rule 14: target_directive_param <- STRING_LITERAL:value    
Ast::Base * Parser::ReductionRuleHandler0014 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::String * value = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 341 "trison_parser.trison"
 return value; 
#line 753 "trison_parser.cpp"
}

// rule 15: target_directive_param <- STRICT_CODE_BLOCK:value    
Ast::Base * Parser::ReductionRuleHandler0015 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * value = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 342 "trison_parser.trison"
 return value; 
#line 764 "trison_parser.cpp"
}

// rule 16: target_directive_param <- DUMB_CODE_BLOCK:value    
Ast::Base * Parser::ReductionRuleHandler0016 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * value = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 343 "trison_parser.trison"
 return value; 
#line 775 "trison_parser.cpp"
}

// rule 17: target_directive_param <-     
Ast::Base * Parser::ReductionRuleHandler0017 ()
{

#line 344 "trison_parser.trison"
 return NULL; 
#line 784 "trison_parser.cpp"
}

// rule 18: terminal_directives <- terminal_directives:terminal_map terminal_directive    
Ast::Base * Parser::ReductionRuleHandler0018 ()
{
    assert(0 < m_reduction_rule_token_count);
    TerminalMap * terminal_map = Dsc< TerminalMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 354 "trison_parser.trison"

        assert(terminal_map == m_terminal_map);
        return terminal_map;
    
#line 798 "trison_parser.cpp"
}

// rule 19: terminal_directives <-     
Ast::Base * Parser::ReductionRuleHandler0019 ()
{

#line 360 "trison_parser.trison"

        m_terminal_map = new TerminalMap();
        return m_terminal_map;
    
#line 810 "trison_parser.cpp"
}

// rule 20: terminal_directive <- DIRECTIVE_TERMINAL:throwaway terminals:terminal_list type_spec:assigned_type_map at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0020 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    TerminalList * terminal_list = Dsc< TerminalList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    TypeMap * assigned_type_map = Dsc< TypeMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 369 "trison_parser.trison"

        assert(m_terminal_map != NULL);
        assert(terminal_list != NULL);
        assert(assigned_type_map != NULL);
        for (TerminalList::iterator it = terminal_list->begin(),
                                    it_end = terminal_list->end();
             it != it_end;
             ++it)
        {
            Trison::Terminal *terminal = *it;
            assert(terminal != NULL);
            terminal->SetAssignedTypeMap(assigned_type_map);
            m_terminal_map->Add(terminal->GetText(), terminal);
        }
        terminal_list->clear();
        delete throwaway;
        delete terminal_list;
        return NULL;
    
#line 843 "trison_parser.cpp"
}

// rule 21: terminals <- terminals:terminal_list terminal:terminal    
Ast::Base * Parser::ReductionRuleHandler0021 ()
{
    assert(0 < m_reduction_rule_token_count);
    TerminalList * terminal_list = Dsc< TerminalList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Terminal * terminal = Dsc< Terminal * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 393 "trison_parser.trison"

        if (terminal != NULL)
            terminal_list->Append(terminal);
        return terminal_list;
    
#line 860 "trison_parser.cpp"
}

// rule 22: terminals <- terminal:terminal    
Ast::Base * Parser::ReductionRuleHandler0022 ()
{
    assert(0 < m_reduction_rule_token_count);
    Terminal * terminal = Dsc< Terminal * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 400 "trison_parser.trison"

        TerminalList *terminal_list = new TerminalList();
        if (terminal != NULL)
            terminal_list->Append(terminal);
        return terminal_list;
    
#line 876 "trison_parser.cpp"
}

// rule 23: precedence_directives <- precedence_directives:precedence_map precedence_directive    
Ast::Base * Parser::ReductionRuleHandler0023 ()
{
    assert(0 < m_reduction_rule_token_count);
    PrecedenceMap * precedence_map = Dsc< PrecedenceMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 415 "trison_parser.trison"

        assert(precedence_map != NULL);
        assert(m_precedence_map == precedence_map);
        assert(m_precedence_list != NULL);
        return precedence_map;
    
#line 892 "trison_parser.cpp"
}

// rule 24: precedence_directives <-     
Ast::Base * Parser::ReductionRuleHandler0024 ()
{

#line 423 "trison_parser.trison"

        assert(m_precedence_list == NULL);
        assert(m_precedence_map == NULL);
        Precedence *precedence = new Precedence("DEFAULT_", A_LEFT, FiLoc::ms_invalid, 0);
        m_precedence_list = new PrecedenceList();
        m_precedence_list->Append(precedence);
        m_precedence_map = new PrecedenceMap();
        m_precedence_map->Add("DEFAULT_", precedence);
        return m_precedence_map;
    
#line 910 "trison_parser.cpp"
}

// rule 25: precedence_directive <- DIRECTIVE_PREC:throwaway ID:id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0025 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 438 "trison_parser.trison"

        assert(m_precedence_list != NULL);
        assert(m_precedence_map != NULL);
        Precedence *precedence =
            new Precedence(
                id->GetText(),
                A_NONASSOC,
                id->GetFiLoc(),
                m_precedence_map->size());
        m_precedence_list->Append(precedence);
        m_precedence_map->Add(precedence->m_precedence_id, precedence);
        delete throwaway;
        delete id;
        return m_precedence_map;
    
#line 937 "trison_parser.cpp"
}

// rule 26: precedence_directive <- DIRECTIVE_PREC:throwaway '.' ID:associativity_id ID:id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0026 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * associativity_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 455 "trison_parser.trison"

        assert(m_precedence_list != NULL);
        assert(m_precedence_map != NULL);
        assert(associativity_id != NULL);

        Associativity associativity = A_LEFT;
        if (associativity_id->GetText() == "left")
            associativity = A_LEFT;
        else if (associativity_id->GetText() == "nonassoc")
            associativity = A_NONASSOC;
        else if (associativity_id->GetText() == "right")
            associativity = A_RIGHT;
        else
            EmitError(throwaway->GetFiLoc(), "invalid associativity specifier \"" + associativity_id->GetText() + "\"");

        Precedence *precedence =
            new Precedence(
                id->GetText(),
                associativity,
                id->GetFiLoc(),
                m_precedence_map->size());
        m_precedence_list->Append(precedence);
        m_precedence_map->Add(precedence->m_precedence_id, precedence);
        delete throwaway;
        delete id;
        return m_precedence_map;
    
#line 978 "trison_parser.cpp"
}

// rule 27: start_directive <- DIRECTIVE_DEFAULT_PARSE_NONTERMINAL:throwaway ID:id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0027 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 491 "trison_parser.trison"

        delete throwaway;
        return id;
    
#line 994 "trison_parser.cpp"
}

// rule 28: nonterminals <- nonterminals:nonterminal_map nonterminal:nonterminal    
Ast::Base * Parser::ReductionRuleHandler0028 ()
{
    assert(0 < m_reduction_rule_token_count);
    NonterminalMap * nonterminal_map = Dsc< NonterminalMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Nonterminal * nonterminal = Dsc< Nonterminal * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 504 "trison_parser.trison"

        assert(m_terminal_map != NULL);
        assert(m_nonterminal_list != NULL);
        if (nonterminal != NULL)
        {
            nonterminal_map->Add(nonterminal->GetText(), nonterminal);
            m_nonterminal_list->Append(nonterminal);
        }
        return nonterminal_map;
    
#line 1016 "trison_parser.cpp"
}

// rule 29: nonterminals <-     
Ast::Base * Parser::ReductionRuleHandler0029 ()
{

#line 516 "trison_parser.trison"

        assert(m_nonterminal_list == NULL);
        m_nonterminal_list = new NonterminalList();
        return new NonterminalMap();
    
#line 1029 "trison_parser.cpp"
}

// rule 30: nonterminal <- nonterminal_specification:nonterminal ':' rules:rule_list ';'    
Ast::Base * Parser::ReductionRuleHandler0030 ()
{
    assert(0 < m_reduction_rule_token_count);
    Nonterminal * nonterminal = Dsc< Nonterminal * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 526 "trison_parser.trison"

        if (nonterminal != NULL)
            nonterminal->SetRuleList(rule_list);
        else
            delete rule_list;
        return nonterminal;
    
#line 1048 "trison_parser.cpp"
}

// rule 31: nonterminal <- %error ';'    
Ast::Base * Parser::ReductionRuleHandler0031 ()
{

#line 535 "trison_parser.trison"

        EmitError(GetFiLoc(), "syntax error in nonterminal definition");
        return NULL;
    
#line 1060 "trison_parser.cpp"
}

// rule 32: nonterminal_specification <- DIRECTIVE_NONTERMINAL:throwaway ID:id type_spec:assigned_type_map    
Ast::Base * Parser::ReductionRuleHandler0032 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    TypeMap * assigned_type_map = Dsc< TypeMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 544 "trison_parser.trison"

        assert(m_terminal_map != NULL);
        assert(id != NULL);
        assert(assigned_type_map != NULL);
        if (m_terminal_map->GetElement(id->GetText()) != NULL)
            EmitError(id->GetFiLoc(), "id collision with terminal " + id->GetText());
        Nonterminal *nonterminal =
            new Nonterminal(
                id->GetText(),
                id->GetFiLoc(),
                assigned_type_map);
        delete throwaway;
        delete id;
        return nonterminal;
    
#line 1089 "trison_parser.cpp"
}

// rule 33: nonterminal_specification <- DIRECTIVE_NONTERMINAL:throwaway %error    
Ast::Base * Parser::ReductionRuleHandler0033 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 561 "trison_parser.trison"

        assert(throwaway != NULL);
        EmitError(throwaway->GetFiLoc(), "syntax error while parsing nonterminal specification");
        delete throwaway;
        return NULL;
    
#line 1105 "trison_parser.cpp"
}

// rule 34: nonterminal_specification <- DIRECTIVE_NONTERMINAL:throwaway ID:id %error    
Ast::Base * Parser::ReductionRuleHandler0034 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 569 "trison_parser.trison"

        assert(id != NULL);
        EmitError(id->GetFiLoc(), "syntax error in %nonterminal directive");
        delete throwaway;
        delete id;
        return NULL;
    
#line 1124 "trison_parser.cpp"
}

// rule 35: rules <- rules:rule_list '|' rule:rule    
Ast::Base * Parser::ReductionRuleHandler0035 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 585 "trison_parser.trison"

        rule_list->Append(rule);
        return rule_list;
    
#line 1140 "trison_parser.cpp"
}

// rule 36: rules <- rule:rule    
Ast::Base * Parser::ReductionRuleHandler0036 ()
{
    assert(0 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 591 "trison_parser.trison"

        RuleList *rule_list = new RuleList();
        rule_list->Append(rule);
        return rule_list;
    
#line 1155 "trison_parser.cpp"
}

// rule 37: rule <- rule_specification:rule rule_handlers:rule_handler_map    
Ast::Base * Parser::ReductionRuleHandler0037 ()
{
    assert(0 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::RuleHandlerMap * rule_handler_map = Dsc< CommonLang::RuleHandlerMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 601 "trison_parser.trison"

        rule->m_rule_handler_map = rule_handler_map;
        return rule;
    
#line 1171 "trison_parser.cpp"
}

// rule 38: rule_specification <- rule_token_list:rule_token_list rule_precedence_directive:rule_precedence_directive    
Ast::Base * Parser::ReductionRuleHandler0038 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleTokenList * rule_token_list = Dsc< RuleTokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * rule_precedence_directive = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 610 "trison_parser.trison"

        string const &rule_precedence_id =
            rule_precedence_directive != NULL ?
            rule_precedence_directive->GetText() :
            "DEFAULT_";
        Rule *rule = new Rule(rule_token_list, rule_precedence_id, m_rule_count++);
        delete rule_precedence_directive;
        return rule;
    
#line 1192 "trison_parser.cpp"
}

// rule 39: rule_handlers <- rule_handlers:rule_handler_map rule_handler:rule_handler    
Ast::Base * Parser::ReductionRuleHandler0039 ()
{
    assert(0 < m_reduction_rule_token_count);
    CommonLang::RuleHandlerMap * rule_handler_map = Dsc< CommonLang::RuleHandlerMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::RuleHandler * rule_handler = Dsc< CommonLang::RuleHandler * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 624 "trison_parser.trison"

        if (rule_handler != NULL)
            rule_handler_map->Add(rule_handler->m_target_id->GetText(), rule_handler);
        return rule_handler_map;
    
#line 1209 "trison_parser.cpp"
}

// rule 40: rule_handlers <-     
Ast::Base * Parser::ReductionRuleHandler0040 ()
{

#line 631 "trison_parser.trison"

        return new CommonLang::RuleHandlerMap();
    
#line 1220 "trison_parser.cpp"
}

// rule 41: rule_handler <- DIRECTIVE_TARGET:throwaway '.' ID:target_id any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0041 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(3 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 639 "trison_parser.trison"

        delete throwaway;
        assert(m_target_map != NULL);
        if (m_target_map->GetElement(target_id->GetText()) == NULL)
            EmitWarning(
                target_id->GetFiLoc(),
                "undeclared target \"" + target_id->GetText() + "\"");
        return new CommonLang::RuleHandler(target_id, code_block);
    
#line 1243 "trison_parser.cpp"
}

// rule 42: rule_handler <- DIRECTIVE_TARGET:throwaway %error any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0042 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 650 "trison_parser.trison"

        assert(m_target_map != NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in target id after directive %target");
        delete throwaway;
        delete code_block;
        return NULL;
    
#line 1262 "trison_parser.cpp"
}

// rule 43: rule_handler <- DIRECTIVE_TARGET:throwaway %error    
Ast::Base * Parser::ReductionRuleHandler0043 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 659 "trison_parser.trison"

        assert(m_target_map != NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in directive %target");
        delete throwaway;
        return NULL;
    
#line 1278 "trison_parser.cpp"
}

// rule 44: rule_handler <- %error any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0044 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 667 "trison_parser.trison"

        assert(m_target_map != NULL);
        EmitError(code_block->GetFiLoc(), "missing directive %target before rule handler code block");
        delete code_block;
        return NULL;
    
#line 1294 "trison_parser.cpp"
}

// rule 45: rule_token_list <- rule_token_list:rule_token_list rule_token:rule_token    
Ast::Base * Parser::ReductionRuleHandler0045 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleTokenList * rule_token_list = Dsc< RuleTokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    RuleToken * rule_token = Dsc< RuleToken * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 678 "trison_parser.trison"

        rule_token_list->Append(rule_token);
        return rule_token_list;
    
#line 1310 "trison_parser.cpp"
}

// rule 46: rule_token_list <-     
Ast::Base * Parser::ReductionRuleHandler0046 ()
{

#line 684 "trison_parser.trison"

        return new RuleTokenList();
    
#line 1321 "trison_parser.cpp"
}

// rule 47: rule_token <- token_id:token_id ':' ID:assigned_id    
Ast::Base * Parser::ReductionRuleHandler0047 ()
{
    assert(0 < m_reduction_rule_token_count);
    TokenId * token_id = Dsc< TokenId * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * assigned_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 692 "trison_parser.trison"

        RuleToken *rule_token =
            token_id != NULL ?
            new RuleToken(token_id->GetText(), token_id->GetFiLoc(), assigned_id->GetText()) :
            NULL;
        delete token_id;
        delete assigned_id;
        return rule_token;
    
#line 1342 "trison_parser.cpp"
}

// rule 48: rule_token <- token_id:token_id    
Ast::Base * Parser::ReductionRuleHandler0048 ()
{
    assert(0 < m_reduction_rule_token_count);
    TokenId * token_id = Dsc< TokenId * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 703 "trison_parser.trison"

        RuleToken *rule_token =
            token_id != NULL ?
            new RuleToken(token_id->GetText(), token_id->GetFiLoc()) :
            NULL;
        delete token_id;
        return rule_token;
    
#line 1360 "trison_parser.cpp"
}

// rule 49: rule_precedence_directive <- DIRECTIVE_PREC:throwaway ID:id    
Ast::Base * Parser::ReductionRuleHandler0049 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 716 "trison_parser.trison"

        delete throwaway;
        return id;
    
#line 1376 "trison_parser.cpp"
}

// rule 50: rule_precedence_directive <-     
Ast::Base * Parser::ReductionRuleHandler0050 ()
{

#line 722 "trison_parser.trison"

        return NULL;
    
#line 1387 "trison_parser.cpp"
}

// rule 51: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0051 ()
{

#line 733 "trison_parser.trison"
 return NULL; 
#line 1396 "trison_parser.cpp"
}

// rule 52: at_least_zero_newlines <-     
Ast::Base * Parser::ReductionRuleHandler0052 ()
{

#line 735 "trison_parser.trison"
 return NULL; 
#line 1405 "trison_parser.cpp"
}

// rule 53: at_least_one_newline <- at_least_one_newline NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0053 ()
{

#line 740 "trison_parser.trison"
 return NULL; 
#line 1414 "trison_parser.cpp"
}

// rule 54: at_least_one_newline <- NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0054 ()
{

#line 742 "trison_parser.trison"
 return NULL; 
#line 1423 "trison_parser.cpp"
}

// rule 55: token_id <- ID:id    
Ast::Base * Parser::ReductionRuleHandler0055 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 748 "trison_parser.trison"

        TokenId *token_id = new TokenId(id->GetText(), id->GetFiLoc());
        delete id;
        return token_id;
    
#line 1438 "trison_parser.cpp"
}

// rule 56: token_id <- CHAR_LITERAL:ch    
Ast::Base * Parser::ReductionRuleHandler0056 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Char * ch = Dsc< Ast::Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 755 "trison_parser.trison"

        TokenId *token_id = new TokenId(GetCharLiteral(ch->GetChar()), ch->GetFiLoc());
        delete ch;
        return token_id;
    
#line 1453 "trison_parser.cpp"
}

// rule 57: terminal <- ID:id    
Ast::Base * Parser::ReductionRuleHandler0057 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 764 "trison_parser.trison"
 return new Terminal(id); 
#line 1464 "trison_parser.cpp"
}

// rule 58: terminal <- CHAR_LITERAL:ch    
Ast::Base * Parser::ReductionRuleHandler0058 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Char * ch = Dsc< Ast::Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 766 "trison_parser.trison"
 return new Terminal(ch); 
#line 1475 "trison_parser.cpp"
}

// rule 59: any_type_of_code_block <- DUMB_CODE_BLOCK:dumb_code_block    
Ast::Base * Parser::ReductionRuleHandler0059 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * dumb_code_block = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 771 "trison_parser.trison"
 return dumb_code_block; 
#line 1486 "trison_parser.cpp"
}

// rule 60: any_type_of_code_block <- STRICT_CODE_BLOCK:strict_code_block    
Ast::Base * Parser::ReductionRuleHandler0060 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * strict_code_block = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 773 "trison_parser.trison"
 return strict_code_block; 
#line 1497 "trison_parser.cpp"
}

// rule 61: type_spec <- type_spec:type_map DIRECTIVE_TYPE:throwaway '.' ID:target_id STRING_LITERAL:assigned_type    
Ast::Base * Parser::ReductionRuleHandler0061 ()
{
    assert(0 < m_reduction_rule_token_count);
    TypeMap * type_map = Dsc< TypeMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(4 < m_reduction_rule_token_count);
    Ast::String * assigned_type = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);

#line 779 "trison_parser.trison"

        assert(type_map != NULL);
        assert(target_id != NULL);
        assert(assigned_type != NULL);
        type_map->Add(target_id->GetText(), assigned_type);
        delete throwaway;
        delete target_id;
        return type_map;
    
#line 1522 "trison_parser.cpp"
}

// rule 62: type_spec <-     
Ast::Base * Parser::ReductionRuleHandler0062 ()
{

#line 790 "trison_parser.trison"

        return new TypeMap();
    
#line 1533 "trison_parser.cpp"
}



// ///////////////////////////////////////////////////////////////////////////
// reduction rule lookup table
// ///////////////////////////////////////////////////////////////////////////

Parser::ReductionRule const Parser::ms_reduction_rule[] =
{
    {                 Token::START_,  2, &Parser::ReductionRuleHandler0000, "rule 0: %start <- root END_    "},
    {                 Token::root__,  8, &Parser::ReductionRuleHandler0001, "rule 1: root <- at_least_zero_newlines targets_directive target_directives terminal_directives precedence_directives start_directive END_PREAMBLE nonterminals    "},
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
    {  Token::terminal_directives__,  2, &Parser::ReductionRuleHandler0018, "rule 18: terminal_directives <- terminal_directives terminal_directive    "},
    {  Token::terminal_directives__,  0, &Parser::ReductionRuleHandler0019, "rule 19: terminal_directives <-     "},
    {   Token::terminal_directive__,  4, &Parser::ReductionRuleHandler0020, "rule 20: terminal_directive <- DIRECTIVE_TERMINAL terminals type_spec at_least_one_newline    "},
    {            Token::terminals__,  2, &Parser::ReductionRuleHandler0021, "rule 21: terminals <- terminals terminal    "},
    {            Token::terminals__,  1, &Parser::ReductionRuleHandler0022, "rule 22: terminals <- terminal    "},
    {Token::precedence_directives__,  2, &Parser::ReductionRuleHandler0023, "rule 23: precedence_directives <- precedence_directives precedence_directive    "},
    {Token::precedence_directives__,  0, &Parser::ReductionRuleHandler0024, "rule 24: precedence_directives <-     "},
    { Token::precedence_directive__,  3, &Parser::ReductionRuleHandler0025, "rule 25: precedence_directive <- DIRECTIVE_PREC ID at_least_one_newline    "},
    { Token::precedence_directive__,  5, &Parser::ReductionRuleHandler0026, "rule 26: precedence_directive <- DIRECTIVE_PREC '.' ID ID at_least_one_newline    "},
    {      Token::start_directive__,  3, &Parser::ReductionRuleHandler0027, "rule 27: start_directive <- DIRECTIVE_DEFAULT_PARSE_NONTERMINAL ID at_least_one_newline    "},
    {         Token::nonterminals__,  2, &Parser::ReductionRuleHandler0028, "rule 28: nonterminals <- nonterminals nonterminal    "},
    {         Token::nonterminals__,  0, &Parser::ReductionRuleHandler0029, "rule 29: nonterminals <-     "},
    {          Token::nonterminal__,  4, &Parser::ReductionRuleHandler0030, "rule 30: nonterminal <- nonterminal_specification ':' rules ';'    "},
    {          Token::nonterminal__,  2, &Parser::ReductionRuleHandler0031, "rule 31: nonterminal <- %error ';'    "},
    {Token::nonterminal_specification__,  3, &Parser::ReductionRuleHandler0032, "rule 32: nonterminal_specification <- DIRECTIVE_NONTERMINAL ID type_spec    "},
    {Token::nonterminal_specification__,  2, &Parser::ReductionRuleHandler0033, "rule 33: nonterminal_specification <- DIRECTIVE_NONTERMINAL %error    "},
    {Token::nonterminal_specification__,  3, &Parser::ReductionRuleHandler0034, "rule 34: nonterminal_specification <- DIRECTIVE_NONTERMINAL ID %error    "},
    {                Token::rules__,  3, &Parser::ReductionRuleHandler0035, "rule 35: rules <- rules '|' rule    "},
    {                Token::rules__,  1, &Parser::ReductionRuleHandler0036, "rule 36: rules <- rule    "},
    {                 Token::rule__,  2, &Parser::ReductionRuleHandler0037, "rule 37: rule <- rule_specification rule_handlers    "},
    {   Token::rule_specification__,  2, &Parser::ReductionRuleHandler0038, "rule 38: rule_specification <- rule_token_list rule_precedence_directive    "},
    {        Token::rule_handlers__,  2, &Parser::ReductionRuleHandler0039, "rule 39: rule_handlers <- rule_handlers rule_handler    "},
    {        Token::rule_handlers__,  0, &Parser::ReductionRuleHandler0040, "rule 40: rule_handlers <-     "},
    {         Token::rule_handler__,  4, &Parser::ReductionRuleHandler0041, "rule 41: rule_handler <- DIRECTIVE_TARGET '.' ID any_type_of_code_block    "},
    {         Token::rule_handler__,  3, &Parser::ReductionRuleHandler0042, "rule 42: rule_handler <- DIRECTIVE_TARGET %error any_type_of_code_block    "},
    {         Token::rule_handler__,  2, &Parser::ReductionRuleHandler0043, "rule 43: rule_handler <- DIRECTIVE_TARGET %error    "},
    {         Token::rule_handler__,  2, &Parser::ReductionRuleHandler0044, "rule 44: rule_handler <- %error any_type_of_code_block    "},
    {      Token::rule_token_list__,  2, &Parser::ReductionRuleHandler0045, "rule 45: rule_token_list <- rule_token_list rule_token    "},
    {      Token::rule_token_list__,  0, &Parser::ReductionRuleHandler0046, "rule 46: rule_token_list <-     "},
    {           Token::rule_token__,  3, &Parser::ReductionRuleHandler0047, "rule 47: rule_token <- token_id ':' ID    "},
    {           Token::rule_token__,  1, &Parser::ReductionRuleHandler0048, "rule 48: rule_token <- token_id    "},
    {Token::rule_precedence_directive__,  2, &Parser::ReductionRuleHandler0049, "rule 49: rule_precedence_directive <- DIRECTIVE_PREC ID    "},
    {Token::rule_precedence_directive__,  0, &Parser::ReductionRuleHandler0050, "rule 50: rule_precedence_directive <-     "},
    {Token::at_least_zero_newlines__,  2, &Parser::ReductionRuleHandler0051, "rule 51: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    "},
    {Token::at_least_zero_newlines__,  0, &Parser::ReductionRuleHandler0052, "rule 52: at_least_zero_newlines <-     "},
    { Token::at_least_one_newline__,  2, &Parser::ReductionRuleHandler0053, "rule 53: at_least_one_newline <- at_least_one_newline NEWLINE    "},
    { Token::at_least_one_newline__,  1, &Parser::ReductionRuleHandler0054, "rule 54: at_least_one_newline <- NEWLINE    "},
    {             Token::token_id__,  1, &Parser::ReductionRuleHandler0055, "rule 55: token_id <- ID    "},
    {             Token::token_id__,  1, &Parser::ReductionRuleHandler0056, "rule 56: token_id <- CHAR_LITERAL    "},
    {             Token::terminal__,  1, &Parser::ReductionRuleHandler0057, "rule 57: terminal <- ID    "},
    {             Token::terminal__,  1, &Parser::ReductionRuleHandler0058, "rule 58: terminal <- CHAR_LITERAL    "},
    {Token::any_type_of_code_block__,  1, &Parser::ReductionRuleHandler0059, "rule 59: any_type_of_code_block <- DUMB_CODE_BLOCK    "},
    {Token::any_type_of_code_block__,  1, &Parser::ReductionRuleHandler0060, "rule 60: any_type_of_code_block <- STRICT_CODE_BLOCK    "},
    {            Token::type_spec__,  5, &Parser::ReductionRuleHandler0061, "rule 61: type_spec <- type_spec DIRECTIVE_TYPE '.' ID STRING_LITERAL    "},
    {            Token::type_spec__,  0, &Parser::ReductionRuleHandler0062, "rule 62: type_spec <-     "},

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
    {  35,    1,   36,   37,    2}, // state   16
    {   0,    0,   39,    0,    0}, // state   17
    {  40,    1,    0,   41,    1}, // state   18
    {  42,    1,    0,    0,    0}, // state   19
    {  43,    2,    0,   45,    2}, // state   20
    {   0,    0,   47,    0,    0}, // state   21
    {  48,    2,    0,   50,    2}, // state   22
    {  52,    1,   53,    0,    0}, // state   23
    {  54,    2,    0,    0,    0}, // state   24
    {   0,    0,   56,    0,    0}, // state   25
    {   0,    0,   57,    0,    0}, // state   26
    {  58,    2,   60,   61,    2}, // state   27
    {   0,    0,   63,    0,    0}, // state   28
    {  64,    1,    0,    0,    0}, // state   29
    {  65,    2,    0,    0,    0}, // state   30
    {   0,    0,   67,    0,    0}, // state   31
    {  68,    1,    0,    0,    0}, // state   32
    {  69,    1,    0,   70,    1}, // state   33
    {  71,    1,    0,    0,    0}, // state   34
    {   0,    0,   72,    0,    0}, // state   35
    {  73,    2,    0,   75,    1}, // state   36
    {  76,    1,    0,   77,    1}, // state   37
    {  78,    1,    0,   79,    1}, // state   38
    {  80,    1,    0,    0,    0}, // state   39
    {   0,    0,   81,   82,    1}, // state   40
    {  83,    1,   84,    0,    0}, // state   41
    {  85,    6,    0,   91,    1}, // state   42
    {  92,    1,    0,    0,    0}, // state   43
    {  93,    1,   94,    0,    0}, // state   44
    {  95,    1,   96,    0,    0}, // state   45
    {  97,    1,   98,    0,    0}, // state   46
    {  99,    1,    0,    0,    0}, // state   47
    { 100,    3,    0,  103,    2}, // state   48
    { 105,    1,    0,  106,    1}, // state   49
    {   0,    0,  107,    0,    0}, // state   50
    {   0,    0,  108,    0,    0}, // state   51
    {   0,    0,  109,    0,    0}, // state   52
    {   0,    0,  110,    0,    0}, // state   53
    { 111,    1,    0,  112,    1}, // state   54
    { 113,    1,    0,    0,    0}, // state   55
    { 114,    1,    0,  115,    1}, // state   56
    { 116,    1,    0,    0,    0}, // state   57
    { 117,    2,    0,    0,    0}, // state   58
    {   0,    0,  119,    0,    0}, // state   59
    { 120,    1,    0,    0,    0}, // state   60
    { 121,    1,  122,    0,    0}, // state   61
    { 123,    1,  124,    0,    0}, // state   62
    { 125,    1,    0,    0,    0}, // state   63
    { 126,    1,  127,    0,    0}, // state   64
    {   0,    0,  128,    0,    0}, // state   65
    {   0,    0,  129,    0,    0}, // state   66
    { 130,    4,    0,  134,    1}, // state   67
    {   0,    0,  135,  136,    4}, // state   68
    {   0,    0,  140,    0,    0}, // state   69
    {   0,    0,  141,    0,    0}, // state   70
    { 142,    1,  143,    0,    0}, // state   71
    { 144,    2,    0,    0,    0}, // state   72
    {   0,    0,  146,    0,    0}, // state   73
    {   0,    0,  147,  148,    1}, // state   74
    { 149,    3,  152,  153,    3}, // state   75
    {   0,    0,  156,    0,    0}, // state   76
    {   0,    0,  157,  158,    3}, // state   77
    { 161,    4,    0,  165,    1}, // state   78
    {   0,    0,  166,    0,    0}, // state   79
    { 167,    1,    0,    0,    0}, // state   80
    {   0,    0,  168,    0,    0}, // state   81
    {   0,    0,  169,    0,    0}, // state   82
    {   0,    0,  170,    0,    0}, // state   83
    { 171,    1,  172,    0,    0}, // state   84
    {   0,    0,  173,    0,    0}, // state   85
    { 174,    2,    0,  176,    1}, // state   86
    { 177,    2,    0,    0,    0}, // state   87
    {   0,    0,  179,    0,    0}, // state   88
    {   0,    0,  180,    0,    0}, // state   89
    { 181,    1,    0,    0,    0}, // state   90
    {   0,    0,  182,    0,    0}, // state   91
    {   0,    0,  183,    0,    0}, // state   92
    {   0,    0,  184,    0,    0}, // state   93
    { 185,    2,  187,  188,    1}, // state   94
    { 189,    1,    0,    0,    0}, // state   95
    {   0,    0,  190,    0,    0}, // state   96
    {   0,    0,  191,    0,    0}, // state   97
    { 192,    2,    0,  194,    1}, // state   98
    {   0,    0,  195,    0,    0}  // state   99

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
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},
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
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   51}},

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
    {  Token::terminal_directives__, {                  TA_PUSH_STATE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state   10
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   54}},

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
    {     Token::DIRECTIVE_TERMINAL, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   24}},
    // nonterminal transitions
    {   Token::terminal_directive__, {                  TA_PUSH_STATE,   21}},
    {Token::precedence_directives__, {                  TA_PUSH_STATE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   17
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   53}},

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
    {           Token::CHAR_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    // nonterminal transitions
    {            Token::terminals__, {                  TA_PUSH_STATE,   27}},
    {             Token::terminal__, {                  TA_PUSH_STATE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state   21
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   18}},

// ///////////////////////////////////////////////////////////////////////////
// state   22
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {Token::DIRECTIVE_DEFAULT_PARSE_NONTERMINAL, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {         Token::DIRECTIVE_PREC, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    // nonterminal transitions
    { Token::precedence_directive__, {                  TA_PUSH_STATE,   31}},
    {      Token::start_directive__, {                  TA_PUSH_STATE,   32}},

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
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state   25
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   58}},

// ///////////////////////////////////////////////////////////////////////////
// state   26
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   57}},

// ///////////////////////////////////////////////////////////////////////////
// state   27
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CHAR_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   62}},
    // nonterminal transitions
    {             Token::terminal__, {                  TA_PUSH_STATE,   35}},
    {            Token::type_spec__, {                  TA_PUSH_STATE,   36}},

// ///////////////////////////////////////////////////////////////////////////
// state   28
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   29
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state   30
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state   31
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   23}},

// ///////////////////////////////////////////////////////////////////////////
// state   32
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::END_PREAMBLE, {        TA_SHIFT_AND_PUSH_STATE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state   33
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state   34
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   42}},

// ///////////////////////////////////////////////////////////////////////////
// state   35
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   21}},

// ///////////////////////////////////////////////////////////////////////////
// state   36
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {         Token::DIRECTIVE_TYPE, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state   37
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   45}},

// ///////////////////////////////////////////////////////////////////////////
// state   38
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   46}},

// ///////////////////////////////////////////////////////////////////////////
// state   39
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   47}},

// ///////////////////////////////////////////////////////////////////////////
// state   40
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   29}},
    // nonterminal transitions
    {         Token::nonterminals__, {                  TA_PUSH_STATE,   48}},

// ///////////////////////////////////////////////////////////////////////////
// state   41
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state   42
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   49}},
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   50}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   51}},
    {                Token::NEWLINE, {           TA_REDUCE_USING_RULE,   17}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   52}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   53}},
    // nonterminal transitions
    {Token::target_directive_param__, {                  TA_PUSH_STATE,   54}},

// ///////////////////////////////////////////////////////////////////////////
// state   43
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   55}},

// ///////////////////////////////////////////////////////////////////////////
// state   44
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state   45
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   27}},

// ///////////////////////////////////////////////////////////////////////////
// state   46
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   25}},

// ///////////////////////////////////////////////////////////////////////////
// state   47
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   56}},

// ///////////////////////////////////////////////////////////////////////////
// state   48
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::END_, {           TA_REDUCE_USING_RULE,    1}},
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   57}},
    {  Token::DIRECTIVE_NONTERMINAL, {        TA_SHIFT_AND_PUSH_STATE,   58}},
    // nonterminal transitions
    {          Token::nonterminal__, {                  TA_PUSH_STATE,   59}},
    {Token::nonterminal_specification__, {                  TA_PUSH_STATE,   60}},

// ///////////////////////////////////////////////////////////////////////////
// state   49
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state   50
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state   51
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state   52
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   53
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state   54
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   62}},

// ///////////////////////////////////////////////////////////////////////////
// state   55
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   63}},

// ///////////////////////////////////////////////////////////////////////////
// state   56
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   64}},

// ///////////////////////////////////////////////////////////////////////////
// state   57
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(';'), {        TA_SHIFT_AND_PUSH_STATE,   65}},

// ///////////////////////////////////////////////////////////////////////////
// state   58
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   66}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   67}},

// ///////////////////////////////////////////////////////////////////////////
// state   59
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state   60
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   68}},

// ///////////////////////////////////////////////////////////////////////////
// state   61
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}},

// ///////////////////////////////////////////////////////////////////////////
// state   62
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state   63
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   69}},

// ///////////////////////////////////////////////////////////////////////////
// state   64
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   26}},

// ///////////////////////////////////////////////////////////////////////////
// state   65
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   31}},

// ///////////////////////////////////////////////////////////////////////////
// state   66
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   33}},

// ///////////////////////////////////////////////////////////////////////////
// state   67
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   70}},
    {         Token::DIRECTIVE_TYPE, {           TA_REDUCE_USING_RULE,   62}},
    {                Token::NEWLINE, {           TA_REDUCE_USING_RULE,   62}},
    {              Token::Type(':'), {           TA_REDUCE_USING_RULE,   62}},
    // nonterminal transitions
    {            Token::type_spec__, {                  TA_PUSH_STATE,   71}},

// ///////////////////////////////////////////////////////////////////////////
// state   68
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   46}},
    // nonterminal transitions
    {                Token::rules__, {                  TA_PUSH_STATE,   72}},
    {                 Token::rule__, {                  TA_PUSH_STATE,   73}},
    {   Token::rule_specification__, {                  TA_PUSH_STATE,   74}},
    {      Token::rule_token_list__, {                  TA_PUSH_STATE,   75}},

// ///////////////////////////////////////////////////////////////////////////
// state   69
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state   70
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state   71
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {         Token::DIRECTIVE_TYPE, {        TA_SHIFT_AND_PUSH_STATE,   43}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   32}},

// ///////////////////////////////////////////////////////////////////////////
// state   72
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(';'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   77}},

// ///////////////////////////////////////////////////////////////////////////
// state   73
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   36}},

// ///////////////////////////////////////////////////////////////////////////
// state   74
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   40}},
    // nonterminal transitions
    {        Token::rule_handlers__, {                  TA_PUSH_STATE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state   75
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CHAR_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {         Token::DIRECTIVE_PREC, {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   81}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   50}},
    // nonterminal transitions
    {           Token::rule_token__, {                  TA_PUSH_STATE,   82}},
    {Token::rule_precedence_directive__, {                  TA_PUSH_STATE,   83}},
    {             Token::token_id__, {                  TA_PUSH_STATE,   84}},

// ///////////////////////////////////////////////////////////////////////////
// state   76
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   30}},

// ///////////////////////////////////////////////////////////////////////////
// state   77
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   46}},
    // nonterminal transitions
    {                 Token::rule__, {                  TA_PUSH_STATE,   85}},
    {   Token::rule_specification__, {                  TA_PUSH_STATE,   74}},
    {      Token::rule_token_list__, {                  TA_PUSH_STATE,   75}},

// ///////////////////////////////////////////////////////////////////////////
// state   78
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {       Token::DIRECTIVE_TARGET, {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type(';'), {           TA_REDUCE_USING_RULE,   37}},
    {              Token::Type('|'), {           TA_REDUCE_USING_RULE,   37}},
    // nonterminal transitions
    {         Token::rule_handler__, {                  TA_PUSH_STATE,   88}},

// ///////////////////////////////////////////////////////////////////////////
// state   79
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   56}},

// ///////////////////////////////////////////////////////////////////////////
// state   80
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   89}},

// ///////////////////////////////////////////////////////////////////////////
// state   81
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   55}},

// ///////////////////////////////////////////////////////////////////////////
// state   82
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   45}},

// ///////////////////////////////////////////////////////////////////////////
// state   83
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   38}},

// ///////////////////////////////////////////////////////////////////////////
// state   84
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   90}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   48}},

// ///////////////////////////////////////////////////////////////////////////
// state   85
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   35}},

// ///////////////////////////////////////////////////////////////////////////
// state   86
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   91}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   92}},
    // nonterminal transitions
    {Token::any_type_of_code_block__, {                  TA_PUSH_STATE,   93}},

// ///////////////////////////////////////////////////////////////////////////
// state   87
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   94}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   95}},

// ///////////////////////////////////////////////////////////////////////////
// state   88
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state   89
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   49}},

// ///////////////////////////////////////////////////////////////////////////
// state   90
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   96}},

// ///////////////////////////////////////////////////////////////////////////
// state   91
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state   92
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   60}},

// ///////////////////////////////////////////////////////////////////////////
// state   93
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state   94
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   91}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   92}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   43}},
    // nonterminal transitions
    {Token::any_type_of_code_block__, {                  TA_PUSH_STATE,   97}},

// ///////////////////////////////////////////////////////////////////////////
// state   95
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   98}},

// ///////////////////////////////////////////////////////////////////////////
// state   96
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   47}},

// ///////////////////////////////////////////////////////////////////////////
// state   97
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   42}},

// ///////////////////////////////////////////////////////////////////////////
// state   98
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   91}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   92}},
    // nonterminal transitions
    {Token::any_type_of_code_block__, {                  TA_PUSH_STATE,   99}},

// ///////////////////////////////////////////////////////////////////////////
// state   99
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   41}}

};

unsigned int const Parser::ms_state_transition_count =
    sizeof(Parser::ms_state_transition) /
    sizeof(Parser::StateTransition);


#line 100 "trison_parser.trison"

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
        case CommonLang::Scanner::Token::BAD_END_OF_FILE:                     return Parser::Token::END_;
        case CommonLang::Scanner::Token::BAD_TOKEN:                           return Parser::Token::BAD_TOKEN;
        case CommonLang::Scanner::Token::CHAR_LITERAL:                        return Parser::Token::CHAR_LITERAL;
        case CommonLang::Scanner::Token::DIRECTIVE_DEFAULT_PARSE_NONTERMINAL: return Parser::Token::DIRECTIVE_DEFAULT_PARSE_NONTERMINAL;
        case CommonLang::Scanner::Token::DIRECTIVE_ERROR:                     return Parser::Token::DIRECTIVE_ERROR;
        case CommonLang::Scanner::Token::DIRECTIVE_NONTERMINAL:               return Parser::Token::DIRECTIVE_NONTERMINAL;
        case CommonLang::Scanner::Token::DIRECTIVE_PREC:                      return Parser::Token::DIRECTIVE_PREC;
        case CommonLang::Scanner::Token::DIRECTIVE_TARGET:                    return Parser::Token::DIRECTIVE_TARGET;
        case CommonLang::Scanner::Token::DIRECTIVE_TARGETS:                   return Parser::Token::DIRECTIVE_TARGETS;
        case CommonLang::Scanner::Token::DIRECTIVE_TERMINAL:                  return Parser::Token::DIRECTIVE_TERMINAL;
        case CommonLang::Scanner::Token::DIRECTIVE_TYPE:                      return Parser::Token::DIRECTIVE_TYPE;
        case CommonLang::Scanner::Token::DUMB_CODE_BLOCK:                     return Parser::Token::DUMB_CODE_BLOCK;
        case CommonLang::Scanner::Token::END_OF_FILE:                         return Parser::Token::END_;
        case CommonLang::Scanner::Token::END_PREAMBLE:                        return Parser::Token::END_PREAMBLE;
        case CommonLang::Scanner::Token::ID:                                  return Parser::Token::ID;
        case CommonLang::Scanner::Token::NEWLINE:                             return Parser::Token::NEWLINE;
        case CommonLang::Scanner::Token::STRICT_CODE_BLOCK:                   return Parser::Token::STRICT_CODE_BLOCK;
        case CommonLang::Scanner::Token::STRING_LITERAL:                      return Parser::Token::STRING_LITERAL;

        case CommonLang::Scanner::Token::DIRECTIVE_ADD_CODESPEC:
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_OPTIONAL_DIRECTIVE:
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_REQUIRED_DIRECTIVE:
        case CommonLang::Scanner::Token::DIRECTIVE_DEFAULT:
        case CommonLang::Scanner::Token::DIRECTIVE_DUMB_CODE_BLOCK:
        case CommonLang::Scanner::Token::DIRECTIVE_ID:
        case CommonLang::Scanner::Token::DIRECTIVE_MACRO:
        case CommonLang::Scanner::Token::DIRECTIVE_START_IN_SCANNER_MODE:
        case CommonLang::Scanner::Token::DIRECTIVE_STRICT_CODE_BLOCK:
        case CommonLang::Scanner::Token::DIRECTIVE_STRING:
        case CommonLang::Scanner::Token::REGEX:
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

} // end of namespace Trison

#line 2548 "trison_parser.cpp"

