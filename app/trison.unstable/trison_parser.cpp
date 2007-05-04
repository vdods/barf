#include "trison_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x


#line 72 "trison_parser.trison"

#include <sstream>

#include "trison_ast.hpp"

namespace Trison {

#line 21 "trison_parser.cpp"

Parser::Parser ()

{

#line 80 "trison_parser.trison"


#line 30 "trison_parser.cpp"
    m_debug_spew_level = 0;
    DEBUG_SPEW_2("### number of state transitions = " << ms_state_transition_count << std::endl);
    m_reduction_token = NULL;
}

Parser::~Parser ()
{

#line 83 "trison_parser.trison"


#line 42 "trison_parser.cpp"
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

#line 90 "trison_parser.trison"

    m_target_map = NULL;
    m_token_map = NULL;
    m_precedence_list = NULL;
    m_precedence_map = NULL;
    m_nonterminal_list = NULL;

#line 87 "trison_parser.cpp"

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

#line 86 "trison_parser.trison"

    delete token;

#line 413 "trison_parser.cpp"
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
        "DIRECTIVE_LEFT",
        "DIRECTIVE_NONASSOC",
        "DIRECTIVE_PREC",
        "DIRECTIVE_RIGHT",
        "DIRECTIVE_TARGET",
        "DIRECTIVE_TARGETS",
        "DIRECTIVE_TOKEN",
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
        "token",
        "token_directive",
        "token_directives",
        "token_type_spec",
        "tokens",
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

// rule 1: root <- at_least_zero_newlines targets_directive:target_map target_directives token_directives:token_map precedence_directives:precedence_map start_directive:default_parse_nonterminal_id END_PREAMBLE:throwaway nonterminals:nonterminal_map    
Ast::Base * Parser::ReductionRuleHandler0001 ()
{
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    TokenMap * token_map = Dsc< TokenMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(4 < m_reduction_rule_token_count);
    PrecedenceMap * precedence_map = Dsc< PrecedenceMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);
    assert(5 < m_reduction_rule_token_count);
    Ast::Id * default_parse_nonterminal_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 5]);
    assert(6 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 6]);
    assert(7 < m_reduction_rule_token_count);
    NonterminalMap * nonterminal_map = Dsc< NonterminalMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 7]);

#line 201 "trison_parser.trison"

        assert(m_target_map != NULL);
        assert(target_map == m_target_map);
        assert(m_token_map != NULL);
        assert(token_map == m_token_map);
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

        Representation *representation =
            new Representation(
                target_map,
                token_map,
                precedence_map,
                m_precedence_list,
                default_parse_nonterminal_id->GetText(),
                throwaway->GetFiLoc(),
                nonterminal_map,
                m_nonterminal_list);
        delete throwaway;
        delete default_parse_nonterminal_id;
        return representation;
    
#line 567 "trison_parser.cpp"
    return NULL;
}

// rule 2: targets_directive <- DIRECTIVE_TARGETS:throwaway target_ids:target_map at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 245 "trison_parser.trison"

        assert(m_target_map == NULL);
        m_target_map = target_map;
        delete throwaway;
        return target_map;
    
#line 586 "trison_parser.cpp"
    return NULL;
}

// rule 3: targets_directive <-     
Ast::Base * Parser::ReductionRuleHandler0003 ()
{

#line 253 "trison_parser.trison"

        assert(m_target_map == NULL);
        m_target_map = new CommonLang::TargetMap();
        return m_target_map;
    
#line 600 "trison_parser.cpp"
    return NULL;
}

// rule 4: targets_directive <- DIRECTIVE_TARGETS:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0004 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 260 "trison_parser.trison"

        assert(m_target_map == NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in directive %targets");
        m_target_map = new CommonLang::TargetMap();
        return m_target_map;
    
#line 617 "trison_parser.cpp"
    return NULL;
}

// rule 5: target_ids <- target_ids:target_map ID:target_id    
Ast::Base * Parser::ReductionRuleHandler0005 ()
{
    assert(0 < m_reduction_rule_token_count);
    CommonLang::TargetMap * target_map = Dsc< CommonLang::TargetMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 271 "trison_parser.trison"

        CommonLang::Target *target = new CommonLang::Target(target_id->GetText());
        target->EnableCodeGeneration();
        target_map->Add(target_id->GetText(), target);
        return target_map;
    
#line 636 "trison_parser.cpp"
    return NULL;
}

// rule 6: target_ids <-     
Ast::Base * Parser::ReductionRuleHandler0006 ()
{

#line 279 "trison_parser.trison"

        assert(m_target_map == NULL);
        return new CommonLang::TargetMap();
    
#line 649 "trison_parser.cpp"
    return NULL;
}

// rule 7: target_directives <- target_directives target_directive:target_directive    
Ast::Base * Parser::ReductionRuleHandler0007 ()
{
    assert(1 < m_reduction_rule_token_count);
    CommonLang::TargetDirective * target_directive = Dsc< CommonLang::TargetDirective * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 292 "trison_parser.trison"

        assert(m_target_map != NULL);
        if (target_directive != NULL)
            m_target_map->AddTargetDirective(target_directive);
        return NULL;
    
#line 666 "trison_parser.cpp"
    return NULL;
}

// rule 8: target_directives <-     
Ast::Base * Parser::ReductionRuleHandler0008 ()
{

#line 300 "trison_parser.trison"

        if (m_target_map == NULL)
            m_target_map = new CommonLang::TargetMap();
        return NULL;
    
#line 680 "trison_parser.cpp"
    return NULL;
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

#line 310 "trison_parser.trison"

        delete throwaway;
        return new CommonLang::TargetDirective(target_id, target_directive, param);
    
#line 701 "trison_parser.cpp"
    return NULL;
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

#line 316 "trison_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in parameter for directive %target." + target_id->GetText() + "." + target_directive->GetText());
        delete throwaway;
        delete target_id;
        delete target_directive;
        return NULL;
    
#line 723 "trison_parser.cpp"
    return NULL;
}

// rule 11: target_directive <- DIRECTIVE_TARGET:throwaway '.' ID:target_id %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0011 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 325 "trison_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in directive name for directive %target." + target_id->GetText());
        delete throwaway;
        delete target_id;
        return NULL;
    
#line 742 "trison_parser.cpp"
    return NULL;
}

// rule 12: target_directive <- DIRECTIVE_TARGET:throwaway %error at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0012 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 333 "trison_parser.trison"

        EmitError(throwaway->GetFiLoc(), "parse error in target name for directive %target");
        delete throwaway;
        return NULL;
    
#line 758 "trison_parser.cpp"
    return NULL;
}

// rule 13: target_directive_param <- ID:value    
Ast::Base * Parser::ReductionRuleHandler0013 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * value = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 342 "trison_parser.trison"
 return value; 
#line 770 "trison_parser.cpp"
    return NULL;
}

// rule 14: target_directive_param <- STRING_LITERAL:value    
Ast::Base * Parser::ReductionRuleHandler0014 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::String * value = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 343 "trison_parser.trison"
 return value; 
#line 782 "trison_parser.cpp"
    return NULL;
}

// rule 15: target_directive_param <- STRICT_CODE_BLOCK:value    
Ast::Base * Parser::ReductionRuleHandler0015 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * value = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 344 "trison_parser.trison"
 return value; 
#line 794 "trison_parser.cpp"
    return NULL;
}

// rule 16: target_directive_param <- DUMB_CODE_BLOCK:value    
Ast::Base * Parser::ReductionRuleHandler0016 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * value = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 345 "trison_parser.trison"
 return value; 
#line 806 "trison_parser.cpp"
    return NULL;
}

// rule 17: target_directive_param <-     
Ast::Base * Parser::ReductionRuleHandler0017 ()
{

#line 346 "trison_parser.trison"
 return NULL; 
#line 816 "trison_parser.cpp"
    return NULL;
}

// rule 18: token_directives <- token_directives:token_map token_directive    
Ast::Base * Parser::ReductionRuleHandler0018 ()
{
    assert(0 < m_reduction_rule_token_count);
    TokenMap * token_map = Dsc< TokenMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 356 "trison_parser.trison"

        assert(token_map == m_token_map);
        return token_map;
    
#line 831 "trison_parser.cpp"
    return NULL;
}

// rule 19: token_directives <-     
Ast::Base * Parser::ReductionRuleHandler0019 ()
{

#line 362 "trison_parser.trison"

        m_token_map = new TokenMap();
        return m_token_map;
    
#line 844 "trison_parser.cpp"
    return NULL;
}

// rule 20: token_directive <- DIRECTIVE_TOKEN:throwaway tokens:token_list token_type_spec:assigned_type at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0020 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    TokenList * token_list = Dsc< TokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::String * assigned_type = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 371 "trison_parser.trison"

        assert(m_token_map != NULL);
        assert(token_list != NULL);
        for (TokenList::iterator it = token_list->begin(),
                                 it_end = token_list->end();
             it != it_end;
             ++it)
        {
            Trison::Token *token = *it;
            assert(token != NULL);
            if (assigned_type != NULL)
                token->SetAssignedType(assigned_type->GetText());
            m_token_map->Add(token->GetSourceText(), token);
        }
        delete throwaway;
        delete assigned_type;
        return NULL;
    
#line 877 "trison_parser.cpp"
    return NULL;
}

// rule 21: tokens <- tokens:token_list token:token    
Ast::Base * Parser::ReductionRuleHandler0021 ()
{
    assert(0 < m_reduction_rule_token_count);
    TokenList * token_list = Dsc< TokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Trison::Token * token = Dsc< Trison::Token * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 394 "trison_parser.trison"

        if (token != NULL)
            token_list->Append(token);
        return token_list;
    
#line 895 "trison_parser.cpp"
    return NULL;
}

// rule 22: tokens <- token:token    
Ast::Base * Parser::ReductionRuleHandler0022 ()
{
    assert(0 < m_reduction_rule_token_count);
    Trison::Token * token = Dsc< Trison::Token * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 401 "trison_parser.trison"

        TokenList *token_list = new TokenList();
        if (token != NULL)
            token_list->Append(token);
        return token_list;
    
#line 912 "trison_parser.cpp"
    return NULL;
}

// rule 23: token_type_spec <- DIRECTIVE_TYPE:throwaway STRING_LITERAL:assigned_type    
Ast::Base * Parser::ReductionRuleHandler0023 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::String * assigned_type = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 412 "trison_parser.trison"

        delete throwaway;
        return assigned_type;
    
#line 929 "trison_parser.cpp"
    return NULL;
}

// rule 24: token_type_spec <-     
Ast::Base * Parser::ReductionRuleHandler0024 ()
{

#line 418 "trison_parser.trison"

        return NULL;
    
#line 941 "trison_parser.cpp"
    return NULL;
}

// rule 25: precedence_directives <- precedence_directives:precedence_map precedence_directive    
Ast::Base * Parser::ReductionRuleHandler0025 ()
{
    assert(0 < m_reduction_rule_token_count);
    PrecedenceMap * precedence_map = Dsc< PrecedenceMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 430 "trison_parser.trison"

        assert(precedence_map != NULL);
        assert(m_precedence_map == precedence_map);
        assert(m_precedence_list != NULL);
        return precedence_map;
    
#line 958 "trison_parser.cpp"
    return NULL;
}

// rule 26: precedence_directives <-     
Ast::Base * Parser::ReductionRuleHandler0026 ()
{

#line 438 "trison_parser.trison"

        assert(m_precedence_list == NULL);
        assert(m_precedence_map == NULL);
        Precedence *precedence = new Precedence("DEFAULT_", A_LEFT, FiLoc::ms_invalid, 0);
        m_precedence_list = new PrecedenceList();
        m_precedence_list->Append(precedence);
        m_precedence_map = new PrecedenceMap();
        m_precedence_map->Add("DEFAULT_", precedence);
        return m_precedence_map;
    
#line 977 "trison_parser.cpp"
    return NULL;
}

// rule 27: precedence_directive <- DIRECTIVE_PREC:throwaway ID:id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0027 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 453 "trison_parser.trison"

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
    
#line 1005 "trison_parser.cpp"
    return NULL;
}

// rule 28: precedence_directive <- DIRECTIVE_PREC:throwaway DIRECTIVE_LEFT:throwaway2 ID:id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0028 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway2 = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 470 "trison_parser.trison"

        assert(m_precedence_list != NULL);
        assert(m_precedence_map != NULL);
        Precedence *precedence =
            new Precedence(
                id->GetText(),
                A_LEFT,
                id->GetFiLoc(),
                m_precedence_map->size());
        m_precedence_list->Append(precedence);
        m_precedence_map->Add(precedence->m_precedence_id, precedence);
        delete throwaway;
        delete throwaway2;
        delete id;
        return m_precedence_map;
    
#line 1036 "trison_parser.cpp"
    return NULL;
}

// rule 29: precedence_directive <- DIRECTIVE_PREC:throwaway DIRECTIVE_NONASSOC:throwaway2 ID:id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0029 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway2 = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 488 "trison_parser.trison"

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
        delete throwaway2;
        delete id;
        return m_precedence_map;
    
#line 1067 "trison_parser.cpp"
    return NULL;
}

// rule 30: precedence_directive <- DIRECTIVE_PREC:throwaway DIRECTIVE_RIGHT:throwaway2 ID:id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0030 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway2 = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 506 "trison_parser.trison"

        assert(m_precedence_list != NULL);
        assert(m_precedence_map != NULL);
        Precedence *precedence =
            new Precedence(
                id->GetText(),
                A_RIGHT,
                id->GetFiLoc(),
                m_precedence_map->size());
        m_precedence_list->Append(precedence);
        m_precedence_map->Add(precedence->m_precedence_id, precedence);
        delete throwaway;
        delete throwaway2;
        delete id;
        return m_precedence_map;
    
#line 1098 "trison_parser.cpp"
    return NULL;
}

// rule 31: start_directive <- DIRECTIVE_DEFAULT_PARSE_NONTERMINAL:throwaway ID:id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0031 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 531 "trison_parser.trison"

        delete throwaway;
        return id;
    
#line 1115 "trison_parser.cpp"
    return NULL;
}

// rule 32: nonterminals <- nonterminals:nonterminal_map nonterminal:nonterminal    
Ast::Base * Parser::ReductionRuleHandler0032 ()
{
    assert(0 < m_reduction_rule_token_count);
    NonterminalMap * nonterminal_map = Dsc< NonterminalMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Nonterminal * nonterminal = Dsc< Nonterminal * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 544 "trison_parser.trison"

        assert(m_token_map != NULL);
        assert(m_nonterminal_list != NULL);
        if (nonterminal != NULL)
        {
            nonterminal_map->Add(nonterminal->m_id, nonterminal);
            m_nonterminal_list->Append(nonterminal);
        }
        return nonterminal_map;
    
#line 1138 "trison_parser.cpp"
    return NULL;
}

// rule 33: nonterminals <-     
Ast::Base * Parser::ReductionRuleHandler0033 ()
{

#line 556 "trison_parser.trison"

        assert(m_nonterminal_list == NULL);
        m_nonterminal_list = new NonterminalList();
        return new NonterminalMap();
    
#line 1152 "trison_parser.cpp"
    return NULL;
}

// rule 34: nonterminal <- nonterminal_specification:nonterminal ':' rules:rule_list ';'    
Ast::Base * Parser::ReductionRuleHandler0034 ()
{
    assert(0 < m_reduction_rule_token_count);
    Nonterminal * nonterminal = Dsc< Nonterminal * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 566 "trison_parser.trison"

        nonterminal->SetRuleList(rule_list);
        return nonterminal;
    
#line 1169 "trison_parser.cpp"
    return NULL;
}

// rule 35: nonterminal <- %error ';'    
Ast::Base * Parser::ReductionRuleHandler0035 ()
{

#line 572 "trison_parser.trison"

        EmitError(GetFiLoc(), "syntax error in nonterminal definition");
        return NULL;
    
#line 1182 "trison_parser.cpp"
    return NULL;
}

// rule 36: nonterminal_specification <- ID:id DIRECTIVE_TYPE:throwaway STRING_LITERAL:assigned_type    
Ast::Base * Parser::ReductionRuleHandler0036 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::String * assigned_type = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 581 "trison_parser.trison"

        assert(m_token_map != NULL);
        if (m_token_map->GetElement(id->GetText()) != NULL)
            EmitError(id->GetFiLoc(), "id collision with %token " + id->GetText());
        Nonterminal *nonterminal =
            new Nonterminal(
                id->GetText(),
                id->GetFiLoc(),
                assigned_type->GetText());
        delete throwaway;
        delete id;
        delete assigned_type;
        return nonterminal;
    
#line 1211 "trison_parser.cpp"
    return NULL;
}

// rule 37: nonterminal_specification <- ID:id    
Ast::Base * Parser::ReductionRuleHandler0037 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 597 "trison_parser.trison"

        assert(m_token_map != NULL);
        if (m_token_map->GetElement(id->GetText()) != NULL)
            EmitError(id->GetFiLoc(), "id collision with %token " + id->GetText());
        Nonterminal *nonterminal =
            new Nonterminal(
                id->GetText(),
                id->GetFiLoc());
        delete id;
        return nonterminal;
    
#line 1233 "trison_parser.cpp"
    return NULL;
}

// rule 38: nonterminal_specification <- %error    
Ast::Base * Parser::ReductionRuleHandler0038 ()
{

#line 610 "trison_parser.trison"

        EmitError(GetFiLoc(), "syntax error while parsing nonterminal specification");
        return NULL;
    
#line 1246 "trison_parser.cpp"
    return NULL;
}

// rule 39: nonterminal_specification <- ID:id %error    
Ast::Base * Parser::ReductionRuleHandler0039 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 616 "trison_parser.trison"

        EmitError(id->GetFiLoc(), "syntax error in nonterminal directive");
        delete id;
        return NULL;
    
#line 1262 "trison_parser.cpp"
    return NULL;
}

// rule 40: nonterminal_specification <- ID:id DIRECTIVE_TYPE:throwaway %error    
Ast::Base * Parser::ReductionRuleHandler0040 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 623 "trison_parser.trison"

        EmitError(id->GetFiLoc(), "syntax error in nonterminal %type directive; was expecting a string");
        delete id;
        delete throwaway;
        return NULL;
    
#line 1281 "trison_parser.cpp"
    return NULL;
}

// rule 41: rules <- rules:rule_list '|' rule:rule    
Ast::Base * Parser::ReductionRuleHandler0041 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleList * rule_list = Dsc< RuleList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 638 "trison_parser.trison"

        rule_list->Append(rule);
        return rule_list;
    
#line 1298 "trison_parser.cpp"
    return NULL;
}

// rule 42: rules <- rule:rule    
Ast::Base * Parser::ReductionRuleHandler0042 ()
{
    assert(0 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 644 "trison_parser.trison"

        RuleList *rule_list = new RuleList();
        rule_list->Append(rule);
        return rule_list;
    
#line 1314 "trison_parser.cpp"
    return NULL;
}

// rule 43: rule <- rule_specification:rule rule_handlers:rule_handler_map    
Ast::Base * Parser::ReductionRuleHandler0043 ()
{
    assert(0 < m_reduction_rule_token_count);
    Rule * rule = Dsc< Rule * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::RuleHandlerMap * rule_handler_map = Dsc< CommonLang::RuleHandlerMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 654 "trison_parser.trison"

        rule->m_rule_handler_map = rule_handler_map;
        return rule;
    
#line 1331 "trison_parser.cpp"
    return NULL;
}

// rule 44: rule_specification <- rule_token_list:rule_token_list rule_precedence_directive:rule_precedence_directive    
Ast::Base * Parser::ReductionRuleHandler0044 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleTokenList * rule_token_list = Dsc< RuleTokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * rule_precedence_directive = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 663 "trison_parser.trison"

        string const &rule_precedence_id =
            rule_precedence_directive != NULL ?
            rule_precedence_directive->GetText() :
            "DEFAULT_";
        Rule *rule = new Rule(rule_token_list, rule_precedence_id);
        delete rule_precedence_directive;
        return rule;
    
#line 1353 "trison_parser.cpp"
    return NULL;
}

// rule 45: rule_handlers <- rule_handlers:rule_handler_map rule_handler:rule_handler    
Ast::Base * Parser::ReductionRuleHandler0045 ()
{
    assert(0 < m_reduction_rule_token_count);
    CommonLang::RuleHandlerMap * rule_handler_map = Dsc< CommonLang::RuleHandlerMap * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    CommonLang::RuleHandler * rule_handler = Dsc< CommonLang::RuleHandler * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 677 "trison_parser.trison"

        if (rule_handler != NULL)
            rule_handler_map->Add(rule_handler->m_target_id->GetText(), rule_handler);
        return rule_handler_map;
    
#line 1371 "trison_parser.cpp"
    return NULL;
}

// rule 46: rule_handlers <-     
Ast::Base * Parser::ReductionRuleHandler0046 ()
{

#line 684 "trison_parser.trison"

        return new CommonLang::RuleHandlerMap();
    
#line 1383 "trison_parser.cpp"
    return NULL;
}

// rule 47: rule_handler <- DIRECTIVE_TARGET:throwaway '.' ID:target_id any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0047 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(3 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 692 "trison_parser.trison"

        delete throwaway;
        assert(m_target_map != NULL);
        if (m_target_map->GetElement(target_id->GetText()) == NULL)
            EmitWarning(
                target_id->GetFiLoc(),
                "undeclared target \"" + target_id->GetText() + "\"");
        return new CommonLang::RuleHandler(target_id, code_block);
    
#line 1407 "trison_parser.cpp"
    return NULL;
}

// rule 48: rule_handler <- DIRECTIVE_TARGET:throwaway %error any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0048 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 703 "trison_parser.trison"

        assert(m_target_map != NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in target id after directive %target");
        delete throwaway;
        delete code_block;
        return NULL;
    
#line 1427 "trison_parser.cpp"
    return NULL;
}

// rule 49: rule_handler <- DIRECTIVE_TARGET:throwaway %error    
Ast::Base * Parser::ReductionRuleHandler0049 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 712 "trison_parser.trison"

        assert(m_target_map != NULL);
        EmitError(throwaway->GetFiLoc(), "parse error in directive %target");
        delete throwaway;
        return NULL;
    
#line 1444 "trison_parser.cpp"
    return NULL;
}

// rule 50: rule_handler <- %error any_type_of_code_block:code_block    
Ast::Base * Parser::ReductionRuleHandler0050 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::CodeBlock * code_block = Dsc< Ast::CodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 720 "trison_parser.trison"

        assert(m_target_map != NULL);
        EmitError(code_block->GetFiLoc(), "missing directive %target before rule handler code block");
        delete code_block;
        return NULL;
    
#line 1461 "trison_parser.cpp"
    return NULL;
}

// rule 51: rule_token_list <- rule_token_list:rule_token_list rule_token:rule_token    
Ast::Base * Parser::ReductionRuleHandler0051 ()
{
    assert(0 < m_reduction_rule_token_count);
    RuleTokenList * rule_token_list = Dsc< RuleTokenList * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    RuleToken * rule_token = Dsc< RuleToken * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 731 "trison_parser.trison"

        rule_token_list->Append(rule_token);
        return rule_token_list;
    
#line 1478 "trison_parser.cpp"
    return NULL;
}

// rule 52: rule_token_list <-     
Ast::Base * Parser::ReductionRuleHandler0052 ()
{

#line 737 "trison_parser.trison"

        return new RuleTokenList();
    
#line 1490 "trison_parser.cpp"
    return NULL;
}

// rule 53: rule_token <- token:token ':' ID:assigned_id    
Ast::Base * Parser::ReductionRuleHandler0053 ()
{
    assert(0 < m_reduction_rule_token_count);
    Trison::Token * token = Dsc< Trison::Token * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * assigned_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 745 "trison_parser.trison"

        RuleToken *rule_token =
            token != NULL ?
            new RuleToken(token->GetSourceText(), token->GetFiLoc(), assigned_id->GetText()) :
            NULL;
        delete token;
        delete assigned_id;
        return rule_token;
    
#line 1512 "trison_parser.cpp"
    return NULL;
}

// rule 54: rule_token <- token:token    
Ast::Base * Parser::ReductionRuleHandler0054 ()
{
    assert(0 < m_reduction_rule_token_count);
    Trison::Token * token = Dsc< Trison::Token * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 756 "trison_parser.trison"

        RuleToken *rule_token =
            token != NULL ?
            new RuleToken(token->GetSourceText(), token->GetFiLoc()) :
            NULL;
        delete token;
        return rule_token;
    
#line 1531 "trison_parser.cpp"
    return NULL;
}

// rule 55: rule_precedence_directive <- DIRECTIVE_PREC:throwaway ID:id    
Ast::Base * Parser::ReductionRuleHandler0055 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 769 "trison_parser.trison"

        delete throwaway;
        return id;
    
#line 1548 "trison_parser.cpp"
    return NULL;
}

// rule 56: rule_precedence_directive <-     
Ast::Base * Parser::ReductionRuleHandler0056 ()
{

#line 775 "trison_parser.trison"

        return NULL;
    
#line 1560 "trison_parser.cpp"
    return NULL;
}

// rule 57: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0057 ()
{
    return NULL;
}

// rule 58: at_least_zero_newlines <-     
Ast::Base * Parser::ReductionRuleHandler0058 ()
{
    return NULL;
}

// rule 59: at_least_one_newline <- at_least_one_newline NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0059 ()
{
    return NULL;
}

// rule 60: at_least_one_newline <- NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0060 ()
{
    return NULL;
}

// rule 61: token <- ID:id    
Ast::Base * Parser::ReductionRuleHandler0061 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 800 "trison_parser.trison"
 return new Trison::Token(id); 
#line 1596 "trison_parser.cpp"
    return NULL;
}

// rule 62: token <- CHAR_LITERAL:ch    
Ast::Base * Parser::ReductionRuleHandler0062 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Char * ch = Dsc< Ast::Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 802 "trison_parser.trison"
 return new Trison::Token(ch); 
#line 1608 "trison_parser.cpp"
    return NULL;
}

// rule 63: any_type_of_code_block <- DUMB_CODE_BLOCK:dumb_code_block    
Ast::Base * Parser::ReductionRuleHandler0063 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * dumb_code_block = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 807 "trison_parser.trison"
 return dumb_code_block; 
#line 1620 "trison_parser.cpp"
    return NULL;
}

// rule 64: any_type_of_code_block <- STRICT_CODE_BLOCK:strict_code_block    
Ast::Base * Parser::ReductionRuleHandler0064 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * strict_code_block = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 809 "trison_parser.trison"
 return strict_code_block; 
#line 1632 "trison_parser.cpp"
    return NULL;
}



// ///////////////////////////////////////////////////////////////////////////
// reduction rule lookup table
// ///////////////////////////////////////////////////////////////////////////

Parser::ReductionRule const Parser::ms_reduction_rule[] =
{
    {                 Token::START_,  2, &Parser::ReductionRuleHandler0000, "rule 0: %start <- root END_    "},
    {                 Token::root__,  8, &Parser::ReductionRuleHandler0001, "rule 1: root <- at_least_zero_newlines targets_directive target_directives token_directives precedence_directives start_directive END_PREAMBLE nonterminals    "},
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
    {     Token::token_directives__,  2, &Parser::ReductionRuleHandler0018, "rule 18: token_directives <- token_directives token_directive    "},
    {     Token::token_directives__,  0, &Parser::ReductionRuleHandler0019, "rule 19: token_directives <-     "},
    {      Token::token_directive__,  4, &Parser::ReductionRuleHandler0020, "rule 20: token_directive <- DIRECTIVE_TOKEN tokens token_type_spec at_least_one_newline    "},
    {               Token::tokens__,  2, &Parser::ReductionRuleHandler0021, "rule 21: tokens <- tokens token    "},
    {               Token::tokens__,  1, &Parser::ReductionRuleHandler0022, "rule 22: tokens <- token    "},
    {      Token::token_type_spec__,  2, &Parser::ReductionRuleHandler0023, "rule 23: token_type_spec <- DIRECTIVE_TYPE STRING_LITERAL    "},
    {      Token::token_type_spec__,  0, &Parser::ReductionRuleHandler0024, "rule 24: token_type_spec <-     "},
    {Token::precedence_directives__,  2, &Parser::ReductionRuleHandler0025, "rule 25: precedence_directives <- precedence_directives precedence_directive    "},
    {Token::precedence_directives__,  0, &Parser::ReductionRuleHandler0026, "rule 26: precedence_directives <-     "},
    { Token::precedence_directive__,  3, &Parser::ReductionRuleHandler0027, "rule 27: precedence_directive <- DIRECTIVE_PREC ID at_least_one_newline    "},
    { Token::precedence_directive__,  4, &Parser::ReductionRuleHandler0028, "rule 28: precedence_directive <- DIRECTIVE_PREC DIRECTIVE_LEFT ID at_least_one_newline    "},
    { Token::precedence_directive__,  4, &Parser::ReductionRuleHandler0029, "rule 29: precedence_directive <- DIRECTIVE_PREC DIRECTIVE_NONASSOC ID at_least_one_newline    "},
    { Token::precedence_directive__,  4, &Parser::ReductionRuleHandler0030, "rule 30: precedence_directive <- DIRECTIVE_PREC DIRECTIVE_RIGHT ID at_least_one_newline    "},
    {      Token::start_directive__,  3, &Parser::ReductionRuleHandler0031, "rule 31: start_directive <- DIRECTIVE_DEFAULT_PARSE_NONTERMINAL ID at_least_one_newline    "},
    {         Token::nonterminals__,  2, &Parser::ReductionRuleHandler0032, "rule 32: nonterminals <- nonterminals nonterminal    "},
    {         Token::nonterminals__,  0, &Parser::ReductionRuleHandler0033, "rule 33: nonterminals <-     "},
    {          Token::nonterminal__,  4, &Parser::ReductionRuleHandler0034, "rule 34: nonterminal <- nonterminal_specification ':' rules ';'    "},
    {          Token::nonterminal__,  2, &Parser::ReductionRuleHandler0035, "rule 35: nonterminal <- %error ';'    "},
    {Token::nonterminal_specification__,  3, &Parser::ReductionRuleHandler0036, "rule 36: nonterminal_specification <- ID DIRECTIVE_TYPE STRING_LITERAL    "},
    {Token::nonterminal_specification__,  1, &Parser::ReductionRuleHandler0037, "rule 37: nonterminal_specification <- ID    "},
    {Token::nonterminal_specification__,  1, &Parser::ReductionRuleHandler0038, "rule 38: nonterminal_specification <- %error    "},
    {Token::nonterminal_specification__,  2, &Parser::ReductionRuleHandler0039, "rule 39: nonterminal_specification <- ID %error    "},
    {Token::nonterminal_specification__,  3, &Parser::ReductionRuleHandler0040, "rule 40: nonterminal_specification <- ID DIRECTIVE_TYPE %error    "},
    {                Token::rules__,  3, &Parser::ReductionRuleHandler0041, "rule 41: rules <- rules '|' rule    "},
    {                Token::rules__,  1, &Parser::ReductionRuleHandler0042, "rule 42: rules <- rule    "},
    {                 Token::rule__,  2, &Parser::ReductionRuleHandler0043, "rule 43: rule <- rule_specification rule_handlers    "},
    {   Token::rule_specification__,  2, &Parser::ReductionRuleHandler0044, "rule 44: rule_specification <- rule_token_list rule_precedence_directive    "},
    {        Token::rule_handlers__,  2, &Parser::ReductionRuleHandler0045, "rule 45: rule_handlers <- rule_handlers rule_handler    "},
    {        Token::rule_handlers__,  0, &Parser::ReductionRuleHandler0046, "rule 46: rule_handlers <-     "},
    {         Token::rule_handler__,  4, &Parser::ReductionRuleHandler0047, "rule 47: rule_handler <- DIRECTIVE_TARGET '.' ID any_type_of_code_block    "},
    {         Token::rule_handler__,  3, &Parser::ReductionRuleHandler0048, "rule 48: rule_handler <- DIRECTIVE_TARGET %error any_type_of_code_block    "},
    {         Token::rule_handler__,  2, &Parser::ReductionRuleHandler0049, "rule 49: rule_handler <- DIRECTIVE_TARGET %error    "},
    {         Token::rule_handler__,  2, &Parser::ReductionRuleHandler0050, "rule 50: rule_handler <- %error any_type_of_code_block    "},
    {      Token::rule_token_list__,  2, &Parser::ReductionRuleHandler0051, "rule 51: rule_token_list <- rule_token_list rule_token    "},
    {      Token::rule_token_list__,  0, &Parser::ReductionRuleHandler0052, "rule 52: rule_token_list <-     "},
    {           Token::rule_token__,  3, &Parser::ReductionRuleHandler0053, "rule 53: rule_token <- token ':' ID    "},
    {           Token::rule_token__,  1, &Parser::ReductionRuleHandler0054, "rule 54: rule_token <- token    "},
    {Token::rule_precedence_directive__,  2, &Parser::ReductionRuleHandler0055, "rule 55: rule_precedence_directive <- DIRECTIVE_PREC ID    "},
    {Token::rule_precedence_directive__,  0, &Parser::ReductionRuleHandler0056, "rule 56: rule_precedence_directive <-     "},
    {Token::at_least_zero_newlines__,  2, &Parser::ReductionRuleHandler0057, "rule 57: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    "},
    {Token::at_least_zero_newlines__,  0, &Parser::ReductionRuleHandler0058, "rule 58: at_least_zero_newlines <-     "},
    { Token::at_least_one_newline__,  2, &Parser::ReductionRuleHandler0059, "rule 59: at_least_one_newline <- at_least_one_newline NEWLINE    "},
    { Token::at_least_one_newline__,  1, &Parser::ReductionRuleHandler0060, "rule 60: at_least_one_newline <- NEWLINE    "},
    {                Token::token__,  1, &Parser::ReductionRuleHandler0061, "rule 61: token <- ID    "},
    {                Token::token__,  1, &Parser::ReductionRuleHandler0062, "rule 62: token <- CHAR_LITERAL    "},
    {Token::any_type_of_code_block__,  1, &Parser::ReductionRuleHandler0063, "rule 63: any_type_of_code_block <- DUMB_CODE_BLOCK    "},
    {Token::any_type_of_code_block__,  1, &Parser::ReductionRuleHandler0064, "rule 64: any_type_of_code_block <- STRICT_CODE_BLOCK    "},

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
    {  36,    1,   37,   38,    2}, // state   16
    {   0,    0,   40,    0,    0}, // state   17
    {  41,    2,    0,   43,    1}, // state   18
    {  44,    1,    0,    0,    0}, // state   19
    {  45,    2,    0,   47,    2}, // state   20
    {   0,    0,   49,    0,    0}, // state   21
    {  50,    2,    0,   52,    2}, // state   22
    {  54,    1,   55,    0,    0}, // state   23
    {  56,    2,    0,    0,    0}, // state   24
    {   0,    0,   58,    0,    0}, // state   25
    {   0,    0,   59,    0,    0}, // state   26
    {  60,    3,   63,   64,    2}, // state   27
    {   0,    0,   66,    0,    0}, // state   28
    {  67,    1,    0,    0,    0}, // state   29
    {  68,    4,    0,    0,    0}, // state   30
    {   0,    0,   72,    0,    0}, // state   31
    {  73,    1,    0,    0,    0}, // state   32
    {  74,    2,    0,   76,    1}, // state   33
    {  77,    1,    0,    0,    0}, // state   34
    {  78,    1,    0,    0,    0}, // state   35
    {  79,    1,    0,   80,    1}, // state   36
    {   0,    0,   81,    0,    0}, // state   37
    {  82,    1,    0,   83,    1}, // state   38
    {  84,    1,    0,    0,    0}, // state   39
    {  85,    1,    0,    0,    0}, // state   40
    {  86,    1,    0,    0,    0}, // state   41
    {  87,    1,    0,   88,    1}, // state   42
    {   0,    0,   89,   90,    1}, // state   43
    {  91,    1,   92,    0,    0}, // state   44
    {  93,    6,    0,   99,    1}, // state   45
    {   0,    0,  100,    0,    0}, // state   46
    { 101,    1,  102,    0,    0}, // state   47
    { 103,    1,  104,    0,    0}, // state   48
    { 105,    1,    0,  106,    1}, // state   49
    { 107,    1,    0,  108,    1}, // state   50
    { 109,    1,    0,  110,    1}, // state   51
    { 111,    1,  112,    0,    0}, // state   52
    { 113,    3,    0,  116,    2}, // state   53
    { 118,    2,    0,  120,    1}, // state   54
    {   0,    0,  121,    0,    0}, // state   55
    {   0,    0,  122,    0,    0}, // state   56
    {   0,    0,  123,    0,    0}, // state   57
    {   0,    0,  124,    0,    0}, // state   58
    { 125,    1,    0,  126,    1}, // state   59
    { 127,    1,  128,    0,    0}, // state   60
    { 129,    1,  130,    0,    0}, // state   61
    { 131,    1,  132,    0,    0}, // state   62
    { 133,    3,    0,    0,    0}, // state   63
    { 136,    3,    0,    0,    0}, // state   64
    {   0,    0,  139,    0,    0}, // state   65
    { 140,    1,    0,    0,    0}, // state   66
    { 141,    1,  142,    0,    0}, // state   67
    { 143,    1,  144,    0,    0}, // state   68
    {   0,    0,  145,    0,    0}, // state   69
    { 146,    2,    0,    0,    0}, // state   70
    { 148,    2,    0,    0,    0}, // state   71
    {   0,    0,  150,  151,    4}, // state   72
    { 155,    2,    0,    0,    0}, // state   73
    {   0,    0,  157,    0,    0}, // state   74
    { 158,    2,    0,    0,    0}, // state   75
    {   0,    0,  160,    0,    0}, // state   76
    {   0,    0,  161,  162,    1}, // state   77
    { 163,    3,  166,  167,    3}, // state   78
    {   0,    0,  170,    0,    0}, // state   79
    {   0,    0,  171,  172,    3}, // state   80
    { 175,    4,    0,  179,    1}, // state   81
    { 180,    1,    0,    0,    0}, // state   82
    {   0,    0,  181,    0,    0}, // state   83
    {   0,    0,  182,    0,    0}, // state   84
    { 183,    1,  184,    0,    0}, // state   85
    {   0,    0,  185,    0,    0}, // state   86
    { 186,    3,    0,  189,    1}, // state   87
    { 190,    2,    0,    0,    0}, // state   88
    {   0,    0,  192,    0,    0}, // state   89
    {   0,    0,  193,    0,    0}, // state   90
    { 194,    1,    0,    0,    0}, // state   91
    {   0,    0,  195,    0,    0}, // state   92
    {   0,    0,  196,    0,    0}, // state   93
    {   0,    0,  197,    0,    0}, // state   94
    { 198,    6,    0,  204,    1}, // state   95
    { 205,    1,    0,    0,    0}, // state   96
    {   0,    0,  206,    0,    0}, // state   97
    {   0,    0,  207,    0,    0}, // state   98
    { 208,    2,    0,  210,    1}, // state   99
    {   0,    0,  211,    0,    0}  // state  100

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
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   58}},
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
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   57}},

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
    {       Token::DIRECTIVE_TARGET, {        TA_SHIFT_AND_PUSH_STATE,   14}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   19}},
    // nonterminal transitions
    {     Token::target_directive__, {                  TA_PUSH_STATE,   15}},
    {     Token::token_directives__, {                  TA_PUSH_STATE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state   10
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   60}},

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
    {        Token::DIRECTIVE_TOKEN, {        TA_SHIFT_AND_PUSH_STATE,   20}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   26}},
    // nonterminal transitions
    {      Token::token_directive__, {                  TA_PUSH_STATE,   21}},
    {Token::precedence_directives__, {                  TA_PUSH_STATE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   17
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   59}},

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
    {           Token::CHAR_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    // nonterminal transitions
    {               Token::tokens__, {                  TA_PUSH_STATE,   27}},
    {                Token::token__, {                  TA_PUSH_STATE,   28}},

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
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   62}},

// ///////////////////////////////////////////////////////////////////////////
// state   26
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state   27
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CHAR_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {         Token::DIRECTIVE_TYPE, {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   24}},
    // nonterminal transitions
    {      Token::token_type_spec__, {                  TA_PUSH_STATE,   36}},
    {                Token::token__, {                  TA_PUSH_STATE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state   28
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   29
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   38}},

// ///////////////////////////////////////////////////////////////////////////
// state   30
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {         Token::DIRECTIVE_LEFT, {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {     Token::DIRECTIVE_NONASSOC, {        TA_SHIFT_AND_PUSH_STATE,   40}},
    {        Token::DIRECTIVE_RIGHT, {        TA_SHIFT_AND_PUSH_STATE,   41}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   42}},

// ///////////////////////////////////////////////////////////////////////////
// state   31
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   25}},

// ///////////////////////////////////////////////////////////////////////////
// state   32
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::END_PREAMBLE, {        TA_SHIFT_AND_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state   33
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state   34
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   45}},

// ///////////////////////////////////////////////////////////////////////////
// state   35
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   46}},

// ///////////////////////////////////////////////////////////////////////////
// state   36
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   47}},

// ///////////////////////////////////////////////////////////////////////////
// state   37
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   21}},

// ///////////////////////////////////////////////////////////////////////////
// state   38
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   48}},

// ///////////////////////////////////////////////////////////////////////////
// state   39
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   49}},

// ///////////////////////////////////////////////////////////////////////////
// state   40
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   50}},

// ///////////////////////////////////////////////////////////////////////////
// state   41
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state   42
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   52}},

// ///////////////////////////////////////////////////////////////////////////
// state   43
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   33}},
    // nonterminal transitions
    {         Token::nonterminals__, {                  TA_PUSH_STATE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state   44
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state   45
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   54}},
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   55}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   56}},
    {                Token::NEWLINE, {           TA_REDUCE_USING_RULE,   17}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   57}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   58}},
    // nonterminal transitions
    {Token::target_directive_param__, {                  TA_PUSH_STATE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state   46
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   23}},

// ///////////////////////////////////////////////////////////////////////////
// state   47
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state   48
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   31}},

// ///////////////////////////////////////////////////////////////////////////
// state   49
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   60}},

// ///////////////////////////////////////////////////////////////////////////
// state   50
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state   51
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   62}},

// ///////////////////////////////////////////////////////////////////////////
// state   52
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   27}},

// ///////////////////////////////////////////////////////////////////////////
// state   53
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::END_, {           TA_REDUCE_USING_RULE,    1}},
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   63}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   64}},
    // nonterminal transitions
    {          Token::nonterminal__, {                  TA_PUSH_STATE,   65}},
    {Token::nonterminal_specification__, {                  TA_PUSH_STATE,   66}},

// ///////////////////////////////////////////////////////////////////////////
// state   54
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   67}},

// ///////////////////////////////////////////////////////////////////////////
// state   55
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state   56
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state   57
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   58
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state   59
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   10}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   68}},

// ///////////////////////////////////////////////////////////////////////////
// state   60
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state   61
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   29}},

// ///////////////////////////////////////////////////////////////////////////
// state   62
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   30}},

// ///////////////////////////////////////////////////////////////////////////
// state   63
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {              Token::Type(':'), {           TA_REDUCE_USING_RULE,   38}},
    {              Token::Type(';'), {        TA_SHIFT_AND_PUSH_STATE,   69}},

// ///////////////////////////////////////////////////////////////////////////
// state   64
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   70}},
    {         Token::DIRECTIVE_TYPE, {        TA_SHIFT_AND_PUSH_STATE,   71}},
    {              Token::Type(':'), {           TA_REDUCE_USING_RULE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state   65
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   32}},

// ///////////////////////////////////////////////////////////////////////////
// state   66
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   72}},

// ///////////////////////////////////////////////////////////////////////////
// state   67
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}},

// ///////////////////////////////////////////////////////////////////////////
// state   68
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   17}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state   69
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   35}},

// ///////////////////////////////////////////////////////////////////////////
// state   70
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {              Token::Type(':'), {           TA_REDUCE_USING_RULE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state   71
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   73}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   74}},

// ///////////////////////////////////////////////////////////////////////////
// state   72
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},
    // nonterminal transitions
    {                Token::rules__, {                  TA_PUSH_STATE,   75}},
    {                 Token::rule__, {                  TA_PUSH_STATE,   76}},
    {   Token::rule_specification__, {                  TA_PUSH_STATE,   77}},
    {      Token::rule_token_list__, {                  TA_PUSH_STATE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state   73
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {              Token::Type(':'), {           TA_REDUCE_USING_RULE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state   74
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   36}},

// ///////////////////////////////////////////////////////////////////////////
// state   75
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(';'), {        TA_SHIFT_AND_PUSH_STATE,   79}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   80}},

// ///////////////////////////////////////////////////////////////////////////
// state   76
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   42}},

// ///////////////////////////////////////////////////////////////////////////
// state   77
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   46}},
    // nonterminal transitions
    {        Token::rule_handlers__, {                  TA_PUSH_STATE,   81}},

// ///////////////////////////////////////////////////////////////////////////
// state   78
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {           Token::CHAR_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {         Token::DIRECTIVE_PREC, {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   56}},
    // nonterminal transitions
    {           Token::rule_token__, {                  TA_PUSH_STATE,   83}},
    {Token::rule_precedence_directive__, {                  TA_PUSH_STATE,   84}},
    {                Token::token__, {                  TA_PUSH_STATE,   85}},

// ///////////////////////////////////////////////////////////////////////////
// state   79
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state   80
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},
    // nonterminal transitions
    {                 Token::rule__, {                  TA_PUSH_STATE,   86}},
    {   Token::rule_specification__, {                  TA_PUSH_STATE,   77}},
    {      Token::rule_token_list__, {                  TA_PUSH_STATE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state   81
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {       Token::DIRECTIVE_TARGET, {        TA_SHIFT_AND_PUSH_STATE,   88}},
    {              Token::Type(';'), {           TA_REDUCE_USING_RULE,   43}},
    {              Token::Type('|'), {           TA_REDUCE_USING_RULE,   43}},
    // nonterminal transitions
    {         Token::rule_handler__, {                  TA_PUSH_STATE,   89}},

// ///////////////////////////////////////////////////////////////////////////
// state   82
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   90}},

// ///////////////////////////////////////////////////////////////////////////
// state   83
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state   84
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state   85
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   91}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   54}},

// ///////////////////////////////////////////////////////////////////////////
// state   86
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state   87
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   92}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   93}},
    // nonterminal transitions
    {Token::any_type_of_code_block__, {                  TA_PUSH_STATE,   94}},

// ///////////////////////////////////////////////////////////////////////////
// state   88
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {        TA_SHIFT_AND_PUSH_STATE,   95}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   96}},

// ///////////////////////////////////////////////////////////////////////////
// state   89
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   45}},

// ///////////////////////////////////////////////////////////////////////////
// state   90
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   55}},

// ///////////////////////////////////////////////////////////////////////////
// state   91
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   97}},

// ///////////////////////////////////////////////////////////////////////////
// state   92
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   63}},

// ///////////////////////////////////////////////////////////////////////////
// state   93
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   64}},

// ///////////////////////////////////////////////////////////////////////////
// state   94
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   50}},

// ///////////////////////////////////////////////////////////////////////////
// state   95
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                 Token::ERROR_, {  TA_THROW_AWAY_LOOKAHEAD_TOKEN,    0}},
    {       Token::DIRECTIVE_TARGET, {           TA_REDUCE_USING_RULE,   49}},
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   92}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   93}},
    {              Token::Type(';'), {           TA_REDUCE_USING_RULE,   49}},
    {              Token::Type('|'), {           TA_REDUCE_USING_RULE,   49}},
    // nonterminal transitions
    {Token::any_type_of_code_block__, {                  TA_PUSH_STATE,   98}},

// ///////////////////////////////////////////////////////////////////////////
// state   96
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   99}},

// ///////////////////////////////////////////////////////////////////////////
// state   97
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state   98
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   48}},

// ///////////////////////////////////////////////////////////////////////////
// state   99
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   92}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   93}},
    // nonterminal transitions
    {Token::any_type_of_code_block__, {                  TA_PUSH_STATE,  100}},

// ///////////////////////////////////////////////////////////////////////////
// state  100
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   47}}

};

unsigned int const Parser::ms_state_transition_count =
    sizeof(Parser::ms_state_transition) /
    sizeof(Parser::StateTransition);


#line 98 "trison_parser.trison"

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
        case CommonLang::Scanner::Token::BAD_END_OF_FILE:    return Parser::Token::END_;
        case CommonLang::Scanner::Token::BAD_TOKEN:          return Parser::Token::BAD_TOKEN;
        case CommonLang::Scanner::Token::CHAR_LITERAL:       return Parser::Token::CHAR_LITERAL;
        case CommonLang::Scanner::Token::DIRECTIVE_DEFAULT_PARSE_NONTERMINAL: return Parser::Token::DIRECTIVE_DEFAULT_PARSE_NONTERMINAL;
        case CommonLang::Scanner::Token::DIRECTIVE_ERROR:    return Parser::Token::DIRECTIVE_ERROR;
        case CommonLang::Scanner::Token::DIRECTIVE_LEFT:     return Parser::Token::DIRECTIVE_LEFT;
        case CommonLang::Scanner::Token::DIRECTIVE_NONASSOC: return Parser::Token::DIRECTIVE_NONASSOC;
        case CommonLang::Scanner::Token::DIRECTIVE_PREC:     return Parser::Token::DIRECTIVE_PREC;
        case CommonLang::Scanner::Token::DIRECTIVE_RIGHT:    return Parser::Token::DIRECTIVE_RIGHT;
        case CommonLang::Scanner::Token::DIRECTIVE_TARGET:   return Parser::Token::DIRECTIVE_TARGET;
        case CommonLang::Scanner::Token::DIRECTIVE_TARGETS:  return Parser::Token::DIRECTIVE_TARGETS;
        case CommonLang::Scanner::Token::DIRECTIVE_TOKEN:    return Parser::Token::DIRECTIVE_TOKEN;
        case CommonLang::Scanner::Token::DIRECTIVE_TYPE:     return Parser::Token::DIRECTIVE_TYPE;
        case CommonLang::Scanner::Token::DUMB_CODE_BLOCK:    return Parser::Token::DUMB_CODE_BLOCK;
        case CommonLang::Scanner::Token::END_OF_FILE:        return Parser::Token::END_;
        case CommonLang::Scanner::Token::END_PREAMBLE:       return Parser::Token::END_PREAMBLE;
        case CommonLang::Scanner::Token::ID:                 return Parser::Token::ID;
        case CommonLang::Scanner::Token::NEWLINE:            return Parser::Token::NEWLINE;
        case CommonLang::Scanner::Token::STRICT_CODE_BLOCK:  return Parser::Token::STRICT_CODE_BLOCK;
        case CommonLang::Scanner::Token::STRING_LITERAL:     return Parser::Token::STRING_LITERAL;

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

#line 2675 "trison_parser.cpp"

