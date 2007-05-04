#include "barf_langspec_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x


#line 55 "barf_langspec_parser.trison"

#include "barf_ast.hpp"
#include "barf_langspec_ast.hpp"

namespace Barf {
namespace TargetSpec {

#line 21 "barf_langspec_parser.cpp"

Parser::Parser ()

{

#line 63 "barf_langspec_parser.trison"


#line 30 "barf_langspec_parser.cpp"
    m_debug_spew_level = 0;
    DEBUG_SPEW_2("### number of state transitions = " << ms_state_transition_count << std::endl);
    m_reduction_token = NULL;
}

Parser::~Parser ()
{

#line 66 "barf_langspec_parser.trison"


#line 42 "barf_langspec_parser.cpp"
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

#line 73 "barf_langspec_parser.trison"

    m_add_codespec_list = new AddCodeSpecList();
    m_add_directive_map = new AddDirectiveMap();

#line 84 "barf_langspec_parser.cpp"

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

#line 69 "barf_langspec_parser.trison"

    delete token;

#line 410 "barf_langspec_parser.cpp"
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
        "DIRECTIVE_ADD_CODESPEC",
        "DIRECTIVE_ADD_OPTIONAL_DIRECTIVE",
        "DIRECTIVE_ADD_REQUIRED_DIRECTIVE",
        "DIRECTIVE_DEFAULT",
        "DIRECTIVE_DUMB_CODE_BLOCK",
        "DIRECTIVE_ID",
        "DIRECTIVE_STRICT_CODE_BLOCK",
        "DIRECTIVE_STRING",
        "DIRECTIVE_TARGET",
        "DUMB_CODE_BLOCK",
        "ID",
        "NEWLINE",
        "STRICT_CODE_BLOCK",
        "STRING_LITERAL",
        "END_",

        "add_codespec",
        "add_directive",
        "at_least_one_newline",
        "at_least_zero_newlines",
        "default_value",
        "directives",
        "param_spec",
        "root",
        "target",
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

// rule 1: root <- at_least_zero_newlines target:target directives    
Ast::Base * Parser::ReductionRuleHandler0001 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * target = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 174 "barf_langspec_parser.trison"

        assert(m_add_codespec_list != NULL);
        assert(m_add_directive_map != NULL);
        return new Specification(
            target,
            m_add_codespec_list,
            m_add_directive_map);
    
#line 505 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 2: target <- DIRECTIVE_TARGET:throwaway ID:target_id at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * target_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 191 "barf_langspec_parser.trison"

        delete throwaway;
        return target_id;
    
#line 522 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 3: directives <- directives add_codespec at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0003 ()
{
    return NULL;
}

// rule 4: directives <- directives add_directive at_least_one_newline    
Ast::Base * Parser::ReductionRuleHandler0004 ()
{
    return NULL;
}

// rule 5: directives <-     
Ast::Base * Parser::ReductionRuleHandler0005 ()
{
    return NULL;
}

// rule 6: add_codespec <- DIRECTIVE_ADD_CODESPEC:throwaway STRING_LITERAL:filename ID:filename_directive_id    
Ast::Base * Parser::ReductionRuleHandler0006 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::String * filename = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    Ast::Id * filename_directive_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 213 "barf_langspec_parser.trison"

        assert(m_add_codespec_list != NULL);
        assert(m_add_directive_map != NULL);
        AddDirective *add_directive = m_add_directive_map->GetElement(filename_directive_id->GetText());
        if (add_directive == NULL)
            EmitError(throwaway->GetFiLoc(), "undeclared directive id \"" + filename_directive_id->GetText() + "\" in add_codespec directive");
        if (add_directive == NULL || !add_directive->GetIsRequired() || add_directive->m_param_type != Ast::AT_STRING)
            EmitError(throwaway->GetFiLoc(), "directive id \"" + filename_directive_id->GetText() + "\" in add_codespec directive must refer to a required directive accepting param type %string");
        if (filename->GetText().find_first_of(DIRECTORY_SLASH_STRING) != string::npos)
            EmitError(throwaway->GetFiLoc(), "filename portion \"" + filename->GetText() + "\" of %add_codespec directive may not contain slash (directory-delimiting) characters");
        m_add_codespec_list->Append(new AddCodeSpec(filename, filename_directive_id));
        delete throwaway;
        return NULL;
    
#line 569 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 7: add_directive <- DIRECTIVE_ADD_OPTIONAL_DIRECTIVE:throwaway ID:directive_to_add_id param_spec:param_type    
Ast::Base * Parser::ReductionRuleHandler0007 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * directive_to_add_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    ParamType * param_type = Dsc< ParamType * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 232 "barf_langspec_parser.trison"

        assert(m_add_directive_map != NULL);
        m_add_directive_map->Add(
            directive_to_add_id->GetText(),
            new AddOptionalDirective(directive_to_add_id, param_type->m_param_type, NULL));
        delete throwaway;
        delete param_type;
        return NULL;
    
#line 593 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 8: add_directive <- DIRECTIVE_ADD_OPTIONAL_DIRECTIVE:throwaway1 ID:directive_to_add_id param_spec:param_type DIRECTIVE_DEFAULT:throwaway2 default_value:default_value    
Ast::Base * Parser::ReductionRuleHandler0008 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway1 = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * directive_to_add_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    ParamType * param_type = Dsc< ParamType * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);
    assert(3 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway2 = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);
    assert(4 < m_reduction_rule_token_count);
    Ast::TextBase * default_value = Dsc< Ast::TextBase * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 4]);

#line 243 "barf_langspec_parser.trison"

        assert(m_add_directive_map != NULL);
        AddDirective *directive =
            new AddOptionalDirective(
                directive_to_add_id,
                param_type->m_param_type, default_value);
        m_add_directive_map->Add(
            directive_to_add_id->GetText(),
            directive);
        if (param_type->m_param_type != default_value->GetAstType())
            EmitError(
                throwaway1->GetFiLoc(),
                "type mismatch for default value for directive " + directive->GetDirectiveString() +
                "; was expecting type " + Ast::TextBase::GetDirectiveTypeString(param_type->m_param_type) +
                " but got type " + Ast::TextBase::GetDirectiveTypeString(default_value->GetAstType()));
        delete throwaway1;
        delete param_type;
        delete throwaway2;
        return NULL;
    
#line 632 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 9: add_directive <- DIRECTIVE_ADD_REQUIRED_DIRECTIVE:throwaway ID:directive_to_add_id param_spec:param_type    
Ast::Base * Parser::ReductionRuleHandler0009 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::ThrowAway * throwaway = Dsc< Ast::ThrowAway * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * directive_to_add_id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(2 < m_reduction_rule_token_count);
    ParamType * param_type = Dsc< ParamType * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 265 "barf_langspec_parser.trison"

        assert(m_add_directive_map != NULL);
        m_add_directive_map->Add(
            directive_to_add_id->GetText(),
            new AddRequiredDirective(directive_to_add_id, param_type->m_param_type));
        delete throwaway;
        delete param_type;
        return NULL;
    
#line 656 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 10: param_spec <-     
Ast::Base * Parser::ReductionRuleHandler0010 ()
{

#line 279 "barf_langspec_parser.trison"

        return new ParamType(Ast::AT_NONE);
    
#line 668 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 11: param_spec <- DIRECTIVE_ID:throwaway    
Ast::Base * Parser::ReductionRuleHandler0011 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Base * throwaway = Dsc< Ast::Base * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 284 "barf_langspec_parser.trison"

        delete throwaway;
        return new ParamType(Ast::AT_ID);
    
#line 683 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 12: param_spec <- DIRECTIVE_STRING:throwaway    
Ast::Base * Parser::ReductionRuleHandler0012 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Base * throwaway = Dsc< Ast::Base * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 290 "barf_langspec_parser.trison"

        delete throwaway;
        return new ParamType(Ast::AT_STRING);
    
#line 698 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 13: param_spec <- DIRECTIVE_DUMB_CODE_BLOCK:throwaway    
Ast::Base * Parser::ReductionRuleHandler0013 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Base * throwaway = Dsc< Ast::Base * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 296 "barf_langspec_parser.trison"

        delete throwaway;
        return new ParamType(Ast::AT_DUMB_CODE_BLOCK);
    
#line 713 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 14: param_spec <- DIRECTIVE_STRICT_CODE_BLOCK:throwaway    
Ast::Base * Parser::ReductionRuleHandler0014 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Base * throwaway = Dsc< Ast::Base * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 302 "barf_langspec_parser.trison"

        delete throwaway;
        return new ParamType(Ast::AT_STRICT_CODE_BLOCK);
    
#line 728 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 15: default_value <- ID:value    
Ast::Base * Parser::ReductionRuleHandler0015 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * value = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 310 "barf_langspec_parser.trison"
 return value; 
#line 740 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 16: default_value <- STRING_LITERAL:value    
Ast::Base * Parser::ReductionRuleHandler0016 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::String * value = Dsc< Ast::String * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 311 "barf_langspec_parser.trison"
 return value; 
#line 752 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 17: default_value <- DUMB_CODE_BLOCK:value    
Ast::Base * Parser::ReductionRuleHandler0017 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::DumbCodeBlock * value = Dsc< Ast::DumbCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 312 "barf_langspec_parser.trison"
 return value; 
#line 764 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 18: default_value <- STRICT_CODE_BLOCK:value    
Ast::Base * Parser::ReductionRuleHandler0018 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::StrictCodeBlock * value = Dsc< Ast::StrictCodeBlock * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 313 "barf_langspec_parser.trison"
 return value; 
#line 776 "barf_langspec_parser.cpp"
    return NULL;
}

// rule 19: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0019 ()
{
    return NULL;
}

// rule 20: at_least_zero_newlines <-     
Ast::Base * Parser::ReductionRuleHandler0020 ()
{
    return NULL;
}

// rule 21: at_least_one_newline <- at_least_one_newline NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0021 ()
{
    return NULL;
}

// rule 22: at_least_one_newline <- NEWLINE    
Ast::Base * Parser::ReductionRuleHandler0022 ()
{
    return NULL;
}



// ///////////////////////////////////////////////////////////////////////////
// reduction rule lookup table
// ///////////////////////////////////////////////////////////////////////////

Parser::ReductionRule const Parser::ms_reduction_rule[] =
{
    {                 Token::START_,  2, &Parser::ReductionRuleHandler0000, "rule 0: %start <- root END_    "},
    {                 Token::root__,  3, &Parser::ReductionRuleHandler0001, "rule 1: root <- at_least_zero_newlines target directives    "},
    {               Token::target__,  3, &Parser::ReductionRuleHandler0002, "rule 2: target <- DIRECTIVE_TARGET ID at_least_one_newline    "},
    {           Token::directives__,  3, &Parser::ReductionRuleHandler0003, "rule 3: directives <- directives add_codespec at_least_one_newline    "},
    {           Token::directives__,  3, &Parser::ReductionRuleHandler0004, "rule 4: directives <- directives add_directive at_least_one_newline    "},
    {           Token::directives__,  0, &Parser::ReductionRuleHandler0005, "rule 5: directives <-     "},
    {         Token::add_codespec__,  3, &Parser::ReductionRuleHandler0006, "rule 6: add_codespec <- DIRECTIVE_ADD_CODESPEC STRING_LITERAL ID    "},
    {        Token::add_directive__,  3, &Parser::ReductionRuleHandler0007, "rule 7: add_directive <- DIRECTIVE_ADD_OPTIONAL_DIRECTIVE ID param_spec    "},
    {        Token::add_directive__,  5, &Parser::ReductionRuleHandler0008, "rule 8: add_directive <- DIRECTIVE_ADD_OPTIONAL_DIRECTIVE ID param_spec DIRECTIVE_DEFAULT default_value    "},
    {        Token::add_directive__,  3, &Parser::ReductionRuleHandler0009, "rule 9: add_directive <- DIRECTIVE_ADD_REQUIRED_DIRECTIVE ID param_spec    "},
    {           Token::param_spec__,  0, &Parser::ReductionRuleHandler0010, "rule 10: param_spec <-     "},
    {           Token::param_spec__,  1, &Parser::ReductionRuleHandler0011, "rule 11: param_spec <- DIRECTIVE_ID    "},
    {           Token::param_spec__,  1, &Parser::ReductionRuleHandler0012, "rule 12: param_spec <- DIRECTIVE_STRING    "},
    {           Token::param_spec__,  1, &Parser::ReductionRuleHandler0013, "rule 13: param_spec <- DIRECTIVE_DUMB_CODE_BLOCK    "},
    {           Token::param_spec__,  1, &Parser::ReductionRuleHandler0014, "rule 14: param_spec <- DIRECTIVE_STRICT_CODE_BLOCK    "},
    {        Token::default_value__,  1, &Parser::ReductionRuleHandler0015, "rule 15: default_value <- ID    "},
    {        Token::default_value__,  1, &Parser::ReductionRuleHandler0016, "rule 16: default_value <- STRING_LITERAL    "},
    {        Token::default_value__,  1, &Parser::ReductionRuleHandler0017, "rule 17: default_value <- DUMB_CODE_BLOCK    "},
    {        Token::default_value__,  1, &Parser::ReductionRuleHandler0018, "rule 18: default_value <- STRICT_CODE_BLOCK    "},
    {Token::at_least_zero_newlines__,  2, &Parser::ReductionRuleHandler0019, "rule 19: at_least_zero_newlines <- at_least_zero_newlines NEWLINE    "},
    {Token::at_least_zero_newlines__,  0, &Parser::ReductionRuleHandler0020, "rule 20: at_least_zero_newlines <-     "},
    { Token::at_least_one_newline__,  2, &Parser::ReductionRuleHandler0021, "rule 21: at_least_one_newline <- at_least_one_newline NEWLINE    "},
    { Token::at_least_one_newline__,  1, &Parser::ReductionRuleHandler0022, "rule 22: at_least_one_newline <- NEWLINE    "},

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
    {   5,    2,    0,    7,    1}, // state    2
    {   0,    0,    8,    0,    0}, // state    3
    {   9,    1,    0,    0,    0}, // state    4
    {   0,    0,   10,    0,    0}, // state    5
    {   0,    0,   11,   12,    1}, // state    6
    {  13,    1,    0,   14,    1}, // state    7
    {  15,    3,   18,   19,    2}, // state    8
    {   0,    0,   21,    0,    0}, // state    9
    {  22,    1,   23,    0,    0}, // state   10
    {  24,    1,    0,    0,    0}, // state   11
    {  25,    1,    0,    0,    0}, // state   12
    {  26,    1,    0,    0,    0}, // state   13
    {  27,    1,    0,   28,    1}, // state   14
    {  29,    1,    0,   30,    1}, // state   15
    {   0,    0,   31,    0,    0}, // state   16
    {  32,    1,    0,    0,    0}, // state   17
    {  33,    4,   37,   38,    1}, // state   18
    {  39,    4,   43,   44,    1}, // state   19
    {  45,    1,   46,    0,    0}, // state   20
    {  47,    1,   48,    0,    0}, // state   21
    {   0,    0,   49,    0,    0}, // state   22
    {   0,    0,   50,    0,    0}, // state   23
    {   0,    0,   51,    0,    0}, // state   24
    {   0,    0,   52,    0,    0}, // state   25
    {   0,    0,   53,    0,    0}, // state   26
    {  54,    1,   55,    0,    0}, // state   27
    {   0,    0,   56,    0,    0}, // state   28
    {  57,    4,    0,   61,    1}, // state   29
    {   0,    0,   62,    0,    0}, // state   30
    {   0,    0,   63,    0,    0}, // state   31
    {   0,    0,   64,    0,    0}, // state   32
    {   0,    0,   65,    0,    0}, // state   33
    {   0,    0,   66,    0,    0}  // state   34

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
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   20}},
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
    {       Token::DIRECTIVE_TARGET, {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,    5}},
    // nonterminal transitions
    {               Token::target__, {                  TA_PUSH_STATE,    6}},

// ///////////////////////////////////////////////////////////////////////////
// state    3
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {TA_REDUCE_AND_ACCEPT_USING_RULE,    0}},

// ///////////////////////////////////////////////////////////////////////////
// state    4
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,    7}},

// ///////////////////////////////////////////////////////////////////////////
// state    5
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   19}},

// ///////////////////////////////////////////////////////////////////////////
// state    6
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    5}},
    // nonterminal transitions
    {           Token::directives__, {                  TA_PUSH_STATE,    8}},

// ///////////////////////////////////////////////////////////////////////////
// state    7
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,    9}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   10}},

// ///////////////////////////////////////////////////////////////////////////
// state    8
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    { Token::DIRECTIVE_ADD_CODESPEC, {        TA_SHIFT_AND_PUSH_STATE,   11}},
    {Token::DIRECTIVE_ADD_OPTIONAL_DIRECTIVE, {        TA_SHIFT_AND_PUSH_STATE,   12}},
    {Token::DIRECTIVE_ADD_REQUIRED_DIRECTIVE, {        TA_SHIFT_AND_PUSH_STATE,   13}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},
    // nonterminal transitions
    {        Token::add_directive__, {                  TA_PUSH_STATE,   14}},
    {         Token::add_codespec__, {                  TA_PUSH_STATE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state    9
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   10
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    2}},

// ///////////////////////////////////////////////////////////////////////////
// state   11
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   17}},

// ///////////////////////////////////////////////////////////////////////////
// state   12
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   18}},

// ///////////////////////////////////////////////////////////////////////////
// state   13
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   19}},

// ///////////////////////////////////////////////////////////////////////////
// state   14
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,    9}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state   15
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,    9}},
    // nonterminal transitions
    { Token::at_least_one_newline__, {                  TA_PUSH_STATE,   21}},

// ///////////////////////////////////////////////////////////////////////////
// state   16
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   21}},

// ///////////////////////////////////////////////////////////////////////////
// state   17
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   18
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {Token::DIRECTIVE_DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {           Token::DIRECTIVE_ID, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {Token::DIRECTIVE_STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {       Token::DIRECTIVE_STRING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}},
    // nonterminal transitions
    {           Token::param_spec__, {                  TA_PUSH_STATE,   27}},

// ///////////////////////////////////////////////////////////////////////////
// state   19
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {Token::DIRECTIVE_DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {           Token::DIRECTIVE_ID, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {Token::DIRECTIVE_STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   25}},
    {       Token::DIRECTIVE_STRING, {        TA_SHIFT_AND_PUSH_STATE,   26}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}},
    // nonterminal transitions
    {           Token::param_spec__, {                  TA_PUSH_STATE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state   20
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    4}},

// ///////////////////////////////////////////////////////////////////////////
// state   21
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                Token::NEWLINE, {        TA_SHIFT_AND_PUSH_STATE,   16}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    3}},

// ///////////////////////////////////////////////////////////////////////////
// state   22
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    6}},

// ///////////////////////////////////////////////////////////////////////////
// state   23
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state   24
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state   25
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state   26
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},

// ///////////////////////////////////////////////////////////////////////////
// state   27
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {      Token::DIRECTIVE_DEFAULT, {        TA_SHIFT_AND_PUSH_STATE,   29}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    7}},

// ///////////////////////////////////////////////////////////////////////////
// state   28
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state   29
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {        Token::DUMB_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {                     Token::ID, {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {      Token::STRICT_CODE_BLOCK, {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {         Token::STRING_LITERAL, {        TA_SHIFT_AND_PUSH_STATE,   33}},
    // nonterminal transitions
    {        Token::default_value__, {                  TA_PUSH_STATE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state   30
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   17}},

// ///////////////////////////////////////////////////////////////////////////
// state   31
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   32
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   18}},

// ///////////////////////////////////////////////////////////////////////////
// state   33
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state   34
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    8}}

};

unsigned int const Parser::ms_state_transition_count =
    sizeof(Parser::ms_state_transition) /
    sizeof(Parser::StateTransition);


#line 78 "barf_langspec_parser.trison"

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
        case CommonLang::Scanner::Token::BAD_END_OF_FILE:                  return Parser::Token::END_;
        case CommonLang::Scanner::Token::BAD_TOKEN:                        return Parser::Token::BAD_TOKEN;
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_CODESPEC:           return Parser::Token::DIRECTIVE_ADD_CODESPEC;
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_OPTIONAL_DIRECTIVE: return Parser::Token::DIRECTIVE_ADD_OPTIONAL_DIRECTIVE;
        case CommonLang::Scanner::Token::DIRECTIVE_ADD_REQUIRED_DIRECTIVE: return Parser::Token::DIRECTIVE_ADD_REQUIRED_DIRECTIVE;
        case CommonLang::Scanner::Token::DIRECTIVE_DEFAULT:                return Parser::Token::DIRECTIVE_DEFAULT;
        case CommonLang::Scanner::Token::DIRECTIVE_DUMB_CODE_BLOCK:        return Parser::Token::DIRECTIVE_DUMB_CODE_BLOCK;
        case CommonLang::Scanner::Token::DIRECTIVE_ID:                     return Parser::Token::DIRECTIVE_ID;
        case CommonLang::Scanner::Token::DIRECTIVE_STRICT_CODE_BLOCK:      return Parser::Token::DIRECTIVE_STRICT_CODE_BLOCK;
        case CommonLang::Scanner::Token::DIRECTIVE_STRING:                 return Parser::Token::DIRECTIVE_STRING;
        case CommonLang::Scanner::Token::DIRECTIVE_TARGET:                 return Parser::Token::DIRECTIVE_TARGET;
        case CommonLang::Scanner::Token::DUMB_CODE_BLOCK:                  return Parser::Token::DUMB_CODE_BLOCK;
        case CommonLang::Scanner::Token::END_OF_FILE:                      return Parser::Token::END_;
        case CommonLang::Scanner::Token::ID:                               return Parser::Token::ID;
        case CommonLang::Scanner::Token::NEWLINE:                          return Parser::Token::NEWLINE;
        case CommonLang::Scanner::Token::STRICT_CODE_BLOCK:                return Parser::Token::STRICT_CODE_BLOCK;
        case CommonLang::Scanner::Token::STRING_LITERAL:                   return Parser::Token::STRING_LITERAL;

        case CommonLang::Scanner::Token::CHAR_LITERAL:
        case CommonLang::Scanner::Token::DIRECTIVE_DEFAULT_PARSE_NONTERMINAL:
        case CommonLang::Scanner::Token::DIRECTIVE_ERROR:
        case CommonLang::Scanner::Token::DIRECTIVE_LEFT:
        case CommonLang::Scanner::Token::DIRECTIVE_MACRO:
        case CommonLang::Scanner::Token::DIRECTIVE_NONASSOC:
        case CommonLang::Scanner::Token::DIRECTIVE_PREC:
        case CommonLang::Scanner::Token::DIRECTIVE_RIGHT:
        case CommonLang::Scanner::Token::DIRECTIVE_START_IN_SCANNER_MODE:
        case CommonLang::Scanner::Token::DIRECTIVE_STATE:
        case CommonLang::Scanner::Token::DIRECTIVE_TARGETS:
        case CommonLang::Scanner::Token::DIRECTIVE_TOKEN:
        case CommonLang::Scanner::Token::DIRECTIVE_TYPE:
        case CommonLang::Scanner::Token::END_PREAMBLE:
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

} // end of namespace TargetSpec
} // end of namespace Barf

#line 1229 "barf_langspec_parser.cpp"

