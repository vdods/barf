#include "barf_regex_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x


#line 62 "barf_regex_parser.trison"

#include <sstream>

namespace Barf {
namespace Regex {

#line 20 "barf_regex_parser.cpp"

Parser::Parser ()
    : InputBase()
{

#line 69 "barf_regex_parser.trison"

    m_macro_map = NULL;

#line 30 "barf_regex_parser.cpp"
    m_debug_spew_level = 0;
    DEBUG_SPEW_2("### number of state transitions = " << ms_state_transition_count << std::endl);
    m_reduction_token = NULL;
}

Parser::~Parser ()
{

#line 73 "barf_regex_parser.trison"

    assert(m_macro_map == NULL);

#line 43 "barf_regex_parser.cpp"
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

#line 77 "barf_regex_parser.trison"


#line 83 "barf_regex_parser.cpp"

    ParserReturnCode return_code = PrivateParse();


#line 80 "barf_regex_parser.trison"


#line 91 "barf_regex_parser.cpp"

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

void Parser::ThrowAwayToken (AstCommon::Ast * token)
{

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
        "ALPHA",
        "BAD_TOKEN",
        "CHAR",
        "DIGIT",
        "END_",

        "atom",
        "atom_control_character",
        "atom_normal_character",
        "bound",
        "bracket_character_set",
        "bracket_expression",
        "bracket_expression_character",
        "bracket_expression_control_character",
        "bracket_expression_normal_character",
        "branch",
        "branch_which_didnt_just_accept_an_atom",
        "branch_which_just_accepted_an_atom",
        "identifier",
        "integer",
        "regex",
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

// rule 0: %start <- regex END_    
AstCommon::Ast * Parser::ReductionRuleHandler0000 ()
{
    assert(0 < m_reduction_rule_token_count);
    return m_token_stack[m_token_stack.size() - m_reduction_rule_token_count];

    return NULL;
}

// rule 1: regex <- regex:regex '|' branch:branch    
AstCommon::Ast * Parser::ReductionRuleHandler0001 ()
{
    assert(0 < m_reduction_rule_token_count);
    RegularExpression * regex = Dsc< RegularExpression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 310 "barf_regex_parser.trison"

        regex->Append(branch);
        return regex;
    
#line 496 "barf_regex_parser.cpp"
    return NULL;
}

// rule 2: regex <- branch:branch    
AstCommon::Ast * Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 316 "barf_regex_parser.trison"

        RegularExpression *regex = new RegularExpression();
        regex->Append(branch);
        return regex;
    
#line 512 "barf_regex_parser.cpp"
    return NULL;
}

// rule 3: branch <- branch_which_didnt_just_accept_an_atom:branch    
AstCommon::Ast * Parser::ReductionRuleHandler0003 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 325 "barf_regex_parser.trison"
 return branch; 
#line 524 "barf_regex_parser.cpp"
    return NULL;
}

// rule 4: branch <- branch_which_just_accepted_an_atom:branch    
AstCommon::Ast * Parser::ReductionRuleHandler0004 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 326 "barf_regex_parser.trison"
 return branch; 
#line 536 "barf_regex_parser.cpp"
    return NULL;
}

// rule 5: branch <-     
AstCommon::Ast * Parser::ReductionRuleHandler0005 ()
{

#line 327 "barf_regex_parser.trison"
 return new Branch(); 
#line 546 "barf_regex_parser.cpp"
    return NULL;
}

// rule 6: branch_which_didnt_just_accept_an_atom <- branch_which_just_accepted_an_atom:branch bound:bound    
AstCommon::Ast * Parser::ReductionRuleHandler0006 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Bound * bound = Dsc< Bound * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 333 "barf_regex_parser.trison"

        branch->AddBound(bound);
        return branch;
    
#line 563 "barf_regex_parser.cpp"
    return NULL;
}

// rule 7: branch_which_just_accepted_an_atom <- branch_which_just_accepted_an_atom:branch atom:atom    
AstCommon::Ast * Parser::ReductionRuleHandler0007 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Atom * atom = Dsc< Atom * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 342 "barf_regex_parser.trison"

        branch->AddAtom(atom);
        return branch;
    
#line 580 "barf_regex_parser.cpp"
    return NULL;
}

// rule 8: branch_which_just_accepted_an_atom <- branch_which_didnt_just_accept_an_atom:branch atom:atom    
AstCommon::Ast * Parser::ReductionRuleHandler0008 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Atom * atom = Dsc< Atom * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 348 "barf_regex_parser.trison"

        branch->AddAtom(atom);
        return branch;
    
#line 597 "barf_regex_parser.cpp"
    return NULL;
}

// rule 9: branch_which_just_accepted_an_atom <- atom:atom    
AstCommon::Ast * Parser::ReductionRuleHandler0009 ()
{
    assert(0 < m_reduction_rule_token_count);
    Atom * atom = Dsc< Atom * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 354 "barf_regex_parser.trison"

        Branch *branch = new Branch();
        branch->AddAtom(atom);
        return branch;
    
#line 613 "barf_regex_parser.cpp"
    return NULL;
}

// rule 10: atom <- '{' identifier:macro_name '}'    
AstCommon::Ast * Parser::ReductionRuleHandler0010 ()
{
    assert(1 < m_reduction_rule_token_count);
    AstCommon::Identifier * macro_name = Dsc< AstCommon::Identifier * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 368 "barf_regex_parser.trison"

        assert(macro_name != NULL);
        if (m_macro_map == NULL)
            THROW_STRING("can not use regex macros if no macro map was provided");
        RegularExpression *macro_regex = m_macro_map->GetElement(macro_name->GetText());
        if (macro_regex == NULL)
            THROW_STRING("undefined macro \"" + macro_name->GetText() + "\"");
        return macro_regex;
    
#line 633 "barf_regex_parser.cpp"
    return NULL;
}

// rule 11: atom <- '(' regex:regex ')'    
AstCommon::Ast * Parser::ReductionRuleHandler0011 ()
{
    assert(1 < m_reduction_rule_token_count);
    RegularExpression * regex = Dsc< RegularExpression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 378 "barf_regex_parser.trison"
 return regex; 
#line 645 "barf_regex_parser.cpp"
    return NULL;
}

// rule 12: atom <- '(' ')'    
AstCommon::Ast * Parser::ReductionRuleHandler0012 ()
{

#line 379 "barf_regex_parser.trison"
 return new RegularExpression(); 
#line 655 "barf_regex_parser.cpp"
    return NULL;
}

// rule 13: atom <- '^'    
AstCommon::Ast * Parser::ReductionRuleHandler0013 ()
{

#line 380 "barf_regex_parser.trison"
 return new Character('\0', CT_BEGINNING_OF_LINE); 
#line 665 "barf_regex_parser.cpp"
    return NULL;
}

// rule 14: atom <- '$'    
AstCommon::Ast * Parser::ReductionRuleHandler0014 ()
{

#line 381 "barf_regex_parser.trison"
 return new Character('\0', CT_END_OF_LINE); 
#line 675 "barf_regex_parser.cpp"
    return NULL;
}

// rule 15: atom <- '.'    
AstCommon::Ast * Parser::ReductionRuleHandler0015 ()
{

#line 382 "barf_regex_parser.trison"
 return new BracketCharacterSet('\n', true); 
#line 685 "barf_regex_parser.cpp"
    return NULL;
}

// rule 16: atom <- atom_normal_character:ch    
AstCommon::Ast * Parser::ReductionRuleHandler0016 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * ch = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 383 "barf_regex_parser.trison"
 return ch; 
#line 697 "barf_regex_parser.cpp"
    return NULL;
}

// rule 17: atom <- '\\' atom_normal_character:ch    
AstCommon::Ast * Parser::ReductionRuleHandler0017 ()
{
    assert(1 < m_reduction_rule_token_count);
    Character * ch = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 384 "barf_regex_parser.trison"
 ch->Escape(); return ch; 
#line 709 "barf_regex_parser.cpp"
    return NULL;
}

// rule 18: atom <- '\\' atom_control_character:ch    
AstCommon::Ast * Parser::ReductionRuleHandler0018 ()
{
    assert(1 < m_reduction_rule_token_count);
    Character * ch = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 385 "barf_regex_parser.trison"
 return ch; 
#line 721 "barf_regex_parser.cpp"
    return NULL;
}

// rule 19: atom <- bracket_expression:exp    
AstCommon::Ast * Parser::ReductionRuleHandler0019 ()
{
    assert(0 < m_reduction_rule_token_count);
    Atom * exp = Dsc< Atom * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 386 "barf_regex_parser.trison"
 return exp; 
#line 733 "barf_regex_parser.cpp"
    return NULL;
}

// rule 20: bound <- '*'    
AstCommon::Ast * Parser::ReductionRuleHandler0020 ()
{

#line 391 "barf_regex_parser.trison"
 return new Bound(0, Bound::NO_UPPER_BOUND); 
#line 743 "barf_regex_parser.cpp"
    return NULL;
}

// rule 21: bound <- '+'    
AstCommon::Ast * Parser::ReductionRuleHandler0021 ()
{

#line 393 "barf_regex_parser.trison"
 return new Bound(1, Bound::NO_UPPER_BOUND); 
#line 753 "barf_regex_parser.cpp"
    return NULL;
}

// rule 22: bound <- '?'    
AstCommon::Ast * Parser::ReductionRuleHandler0022 ()
{

#line 395 "barf_regex_parser.trison"
 return new Bound(0, 1); 
#line 763 "barf_regex_parser.cpp"
    return NULL;
}

// rule 23: bound <- '{' integer:exact_bound '}'    
AstCommon::Ast * Parser::ReductionRuleHandler0023 ()
{
    assert(1 < m_reduction_rule_token_count);
    AstCommon::SignedInteger * exact_bound = Dsc< AstCommon::SignedInteger * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 398 "barf_regex_parser.trison"

        assert(exact_bound->GetValue() >= 0);
        Bound *bound = new Bound(exact_bound->GetValue(), exact_bound->GetValue());
        delete exact_bound;
        return bound;
    
#line 780 "barf_regex_parser.cpp"
    return NULL;
}

// rule 24: bound <- '{' integer:lower_bound ',' '}'    
AstCommon::Ast * Parser::ReductionRuleHandler0024 ()
{
    assert(1 < m_reduction_rule_token_count);
    AstCommon::SignedInteger * lower_bound = Dsc< AstCommon::SignedInteger * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 406 "barf_regex_parser.trison"

        assert(lower_bound->GetValue() >= 0);
        return new Bound(lower_bound->GetValue(), Bound::NO_UPPER_BOUND);
    
#line 795 "barf_regex_parser.cpp"
    return NULL;
}

// rule 25: bound <- '{' integer:lower_bound ',' integer:upper_bound '}'    
AstCommon::Ast * Parser::ReductionRuleHandler0025 ()
{
    assert(1 < m_reduction_rule_token_count);
    AstCommon::SignedInteger * lower_bound = Dsc< AstCommon::SignedInteger * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    AstCommon::SignedInteger * upper_bound = Dsc< AstCommon::SignedInteger * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 412 "barf_regex_parser.trison"

        assert(lower_bound->GetValue() >= 0);
        assert(upper_bound->GetValue() >= 0);
        if (lower_bound->GetValue() > upper_bound->GetValue() ||
            lower_bound->GetValue() == 0 && upper_bound->GetValue() == 0)
        {
            ostringstream out;
            out << "invalid bound {" << lower_bound->GetValue() << ',' << upper_bound->GetValue() << '}';
            throw out.str();
        }
        else if (lower_bound->GetValue() > Bound::GetMaximumBoundValue())
        {
            ostringstream out;
            out << "bound value " << lower_bound->GetValue() << " is out of range (max 255)";
            throw out.str();
        }
        else if (upper_bound->GetValue() > Bound::GetMaximumBoundValue())
        {
            ostringstream out;
            out << "bound value " << upper_bound->GetValue() << " is out of range (max 255)";
            throw out.str();
        }
        Bound *bound = new Bound(lower_bound->GetValue(), upper_bound->GetValue());
        delete lower_bound;
        delete upper_bound;
        return bound;
    
#line 835 "barf_regex_parser.cpp"
    return NULL;
}

// rule 26: bracket_expression <- '[' bracket_character_set:bracket_character_set ']'    
AstCommon::Ast * Parser::ReductionRuleHandler0026 ()
{
    assert(1 < m_reduction_rule_token_count);
    BracketCharacterSet * bracket_character_set = Dsc< BracketCharacterSet * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 448 "barf_regex_parser.trison"

        if (bracket_character_set->GetIsEmpty())
            THROW_STRING("invalid empty bracket expression");
        return bracket_character_set;
    
#line 851 "barf_regex_parser.cpp"
    return NULL;
}

// rule 27: bracket_expression <- '[' '^' bracket_character_set:bracket_character_set ']'    
AstCommon::Ast * Parser::ReductionRuleHandler0027 ()
{
    assert(2 < m_reduction_rule_token_count);
    BracketCharacterSet * bracket_character_set = Dsc< BracketCharacterSet * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 455 "barf_regex_parser.trison"

        if (bracket_character_set->GetIsEmpty())
            THROW_STRING("invalid empty bracket expression");
        bracket_character_set->Negate();
        return bracket_character_set;
    
#line 868 "barf_regex_parser.cpp"
    return NULL;
}

// rule 28: bracket_character_set <- bracket_character_set:bracket_character_set bracket_expression_character:ch    
AstCommon::Ast * Parser::ReductionRuleHandler0028 ()
{
    assert(0 < m_reduction_rule_token_count);
    BracketCharacterSet * bracket_character_set = Dsc< BracketCharacterSet * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Character * ch = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 466 "barf_regex_parser.trison"

        bracket_character_set->AddCharacter(ch->GetCharacter());
        delete ch;
        return bracket_character_set;
    
#line 886 "barf_regex_parser.cpp"
    return NULL;
}

// rule 29: bracket_character_set <- bracket_character_set:bracket_character_set bracket_expression_character:begin_range '-' bracket_expression_character:end_range    
AstCommon::Ast * Parser::ReductionRuleHandler0029 ()
{
    assert(0 < m_reduction_rule_token_count);
    BracketCharacterSet * bracket_character_set = Dsc< BracketCharacterSet * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Character * begin_range = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    Character * end_range = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 473 "barf_regex_parser.trison"

        if (end_range->GetCharacter() < begin_range->GetCharacter())
            THROW_STRING("invalid bracketed range [" << GetCharacterLiteral(begin_range->GetCharacter(), false) << '-' << GetCharacterLiteral(end_range->GetCharacter(), false) << ']');
        bracket_character_set->AddCharacterRange(
            begin_range->GetCharacter(),
            end_range->GetCharacter());
        delete begin_range;
        delete end_range;
        return bracket_character_set;
    
#line 911 "barf_regex_parser.cpp"
    return NULL;
}

// rule 30: bracket_character_set <- bracket_character_set:bracket_character_set '[' ':' identifier:identifier ':' ']'    
AstCommon::Ast * Parser::ReductionRuleHandler0030 ()
{
    assert(0 < m_reduction_rule_token_count);
    BracketCharacterSet * bracket_character_set = Dsc< BracketCharacterSet * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    AstCommon::Identifier * identifier = Dsc< AstCommon::Identifier * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 485 "barf_regex_parser.trison"

        bracket_character_set->AddCharacterClass(identifier->GetText());
        delete identifier;
        return bracket_character_set;
    
#line 929 "barf_regex_parser.cpp"
    return NULL;
}

// rule 31: bracket_character_set <-     
AstCommon::Ast * Parser::ReductionRuleHandler0031 ()
{

#line 492 "barf_regex_parser.trison"

        BracketCharacterSet *bracket_character_set = new BracketCharacterSet();
        return bracket_character_set;
    
#line 942 "barf_regex_parser.cpp"
    return NULL;
}

// rule 32: bracket_expression_character <- bracket_expression_normal_character:normal_char    
AstCommon::Ast * Parser::ReductionRuleHandler0032 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * normal_char = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 500 "barf_regex_parser.trison"
 return normal_char; 
#line 954 "barf_regex_parser.cpp"
    return NULL;
}

// rule 33: bracket_expression_character <- '\\' bracket_expression_normal_character:normal_char    
AstCommon::Ast * Parser::ReductionRuleHandler0033 ()
{
    assert(1 < m_reduction_rule_token_count);
    Character * normal_char = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 502 "barf_regex_parser.trison"
 normal_char->Escape(); return normal_char; 
#line 966 "barf_regex_parser.cpp"
    return NULL;
}

// rule 34: bracket_expression_character <- '\\' bracket_expression_control_character:control_char    
AstCommon::Ast * Parser::ReductionRuleHandler0034 ()
{
    assert(1 < m_reduction_rule_token_count);
    Character * control_char = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 504 "barf_regex_parser.trison"
 return control_char; 
#line 978 "barf_regex_parser.cpp"
    return NULL;
}

// rule 35: atom_control_character <- '|'    
AstCommon::Ast * Parser::ReductionRuleHandler0035 ()
{

#line 520 "barf_regex_parser.trison"
 return new Character('|'); 
#line 988 "barf_regex_parser.cpp"
    return NULL;
}

// rule 36: atom_control_character <- '('    
AstCommon::Ast * Parser::ReductionRuleHandler0036 ()
{

#line 522 "barf_regex_parser.trison"
 return new Character('('); 
#line 998 "barf_regex_parser.cpp"
    return NULL;
}

// rule 37: atom_control_character <- ')'    
AstCommon::Ast * Parser::ReductionRuleHandler0037 ()
{

#line 524 "barf_regex_parser.trison"
 return new Character(')'); 
#line 1008 "barf_regex_parser.cpp"
    return NULL;
}

// rule 38: atom_control_character <- '{'    
AstCommon::Ast * Parser::ReductionRuleHandler0038 ()
{

#line 526 "barf_regex_parser.trison"
 return new Character('{'); 
#line 1018 "barf_regex_parser.cpp"
    return NULL;
}

// rule 39: atom_control_character <- '}'    
AstCommon::Ast * Parser::ReductionRuleHandler0039 ()
{

#line 528 "barf_regex_parser.trison"
 return new Character('}'); 
#line 1028 "barf_regex_parser.cpp"
    return NULL;
}

// rule 40: atom_control_character <- '['    
AstCommon::Ast * Parser::ReductionRuleHandler0040 ()
{

#line 530 "barf_regex_parser.trison"
 return new Character('['); 
#line 1038 "barf_regex_parser.cpp"
    return NULL;
}

// rule 41: atom_control_character <- ']'    
AstCommon::Ast * Parser::ReductionRuleHandler0041 ()
{

#line 532 "barf_regex_parser.trison"
 return new Character(']'); 
#line 1048 "barf_regex_parser.cpp"
    return NULL;
}

// rule 42: atom_control_character <- '?'    
AstCommon::Ast * Parser::ReductionRuleHandler0042 ()
{

#line 534 "barf_regex_parser.trison"
 return new Character('?'); 
#line 1058 "barf_regex_parser.cpp"
    return NULL;
}

// rule 43: atom_control_character <- '*'    
AstCommon::Ast * Parser::ReductionRuleHandler0043 ()
{

#line 536 "barf_regex_parser.trison"
 return new Character('*'); 
#line 1068 "barf_regex_parser.cpp"
    return NULL;
}

// rule 44: atom_control_character <- '+'    
AstCommon::Ast * Parser::ReductionRuleHandler0044 ()
{

#line 538 "barf_regex_parser.trison"
 return new Character('+'); 
#line 1078 "barf_regex_parser.cpp"
    return NULL;
}

// rule 45: atom_control_character <- '.'    
AstCommon::Ast * Parser::ReductionRuleHandler0045 ()
{

#line 540 "barf_regex_parser.trison"
 return new Character('.'); 
#line 1088 "barf_regex_parser.cpp"
    return NULL;
}

// rule 46: atom_control_character <- '^'    
AstCommon::Ast * Parser::ReductionRuleHandler0046 ()
{

#line 542 "barf_regex_parser.trison"
 return new Character('^'); 
#line 1098 "barf_regex_parser.cpp"
    return NULL;
}

// rule 47: atom_control_character <- '$'    
AstCommon::Ast * Parser::ReductionRuleHandler0047 ()
{

#line 544 "barf_regex_parser.trison"
 return new Character('$'); 
#line 1108 "barf_regex_parser.cpp"
    return NULL;
}

// rule 48: atom_control_character <- '\\'    
AstCommon::Ast * Parser::ReductionRuleHandler0048 ()
{

#line 546 "barf_regex_parser.trison"
 return new Character('\\'); 
#line 1118 "barf_regex_parser.cpp"
    return NULL;
}

// rule 49: atom_normal_character <- ALPHA:alpha    
AstCommon::Ast * Parser::ReductionRuleHandler0049 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * alpha = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 557 "barf_regex_parser.trison"
 return alpha; 
#line 1130 "barf_regex_parser.cpp"
    return NULL;
}

// rule 50: atom_normal_character <- DIGIT:digit    
AstCommon::Ast * Parser::ReductionRuleHandler0050 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * digit = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 558 "barf_regex_parser.trison"
 return digit; 
#line 1142 "barf_regex_parser.cpp"
    return NULL;
}

// rule 51: atom_normal_character <- CHAR:ch    
AstCommon::Ast * Parser::ReductionRuleHandler0051 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * ch = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 559 "barf_regex_parser.trison"
 return ch; 
#line 1154 "barf_regex_parser.cpp"
    return NULL;
}

// rule 52: atom_normal_character <- ','    
AstCommon::Ast * Parser::ReductionRuleHandler0052 ()
{

#line 560 "barf_regex_parser.trison"
 return new Character(','); 
#line 1164 "barf_regex_parser.cpp"
    return NULL;
}

// rule 53: atom_normal_character <- '-'    
AstCommon::Ast * Parser::ReductionRuleHandler0053 ()
{

#line 561 "barf_regex_parser.trison"
 return new Character('-'); 
#line 1174 "barf_regex_parser.cpp"
    return NULL;
}

// rule 54: atom_normal_character <- ':'    
AstCommon::Ast * Parser::ReductionRuleHandler0054 ()
{

#line 562 "barf_regex_parser.trison"
 return new Character(':'); 
#line 1184 "barf_regex_parser.cpp"
    return NULL;
}

// rule 55: bracket_expression_control_character <- '-'    
AstCommon::Ast * Parser::ReductionRuleHandler0055 ()
{

#line 574 "barf_regex_parser.trison"
 return new Character('-'); 
#line 1194 "barf_regex_parser.cpp"
    return NULL;
}

// rule 56: bracket_expression_control_character <- '^'    
AstCommon::Ast * Parser::ReductionRuleHandler0056 ()
{

#line 576 "barf_regex_parser.trison"
 return new Character('^'); 
#line 1204 "barf_regex_parser.cpp"
    return NULL;
}

// rule 57: bracket_expression_control_character <- '['    
AstCommon::Ast * Parser::ReductionRuleHandler0057 ()
{

#line 578 "barf_regex_parser.trison"
 return new Character('['); 
#line 1214 "barf_regex_parser.cpp"
    return NULL;
}

// rule 58: bracket_expression_control_character <- ']'    
AstCommon::Ast * Parser::ReductionRuleHandler0058 ()
{

#line 580 "barf_regex_parser.trison"
 return new Character(']'); 
#line 1224 "barf_regex_parser.cpp"
    return NULL;
}

// rule 59: bracket_expression_control_character <- '\\'    
AstCommon::Ast * Parser::ReductionRuleHandler0059 ()
{

#line 582 "barf_regex_parser.trison"
 return new Character('\\'); 
#line 1234 "barf_regex_parser.cpp"
    return NULL;
}

// rule 60: bracket_expression_normal_character <- ALPHA:alpha    
AstCommon::Ast * Parser::ReductionRuleHandler0060 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * alpha = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 593 "barf_regex_parser.trison"
 return alpha; 
#line 1246 "barf_regex_parser.cpp"
    return NULL;
}

// rule 61: bracket_expression_normal_character <- DIGIT:digit    
AstCommon::Ast * Parser::ReductionRuleHandler0061 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * digit = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 594 "barf_regex_parser.trison"
 return digit; 
#line 1258 "barf_regex_parser.cpp"
    return NULL;
}

// rule 62: bracket_expression_normal_character <- CHAR:ch    
AstCommon::Ast * Parser::ReductionRuleHandler0062 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * ch = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 595 "barf_regex_parser.trison"
 return ch; 
#line 1270 "barf_regex_parser.cpp"
    return NULL;
}

// rule 63: bracket_expression_normal_character <- '|'    
AstCommon::Ast * Parser::ReductionRuleHandler0063 ()
{

#line 596 "barf_regex_parser.trison"
 return new Character('|'); 
#line 1280 "barf_regex_parser.cpp"
    return NULL;
}

// rule 64: bracket_expression_normal_character <- ':'    
AstCommon::Ast * Parser::ReductionRuleHandler0064 ()
{

#line 597 "barf_regex_parser.trison"
 return new Character(':'); 
#line 1290 "barf_regex_parser.cpp"
    return NULL;
}

// rule 65: bracket_expression_normal_character <- '?'    
AstCommon::Ast * Parser::ReductionRuleHandler0065 ()
{

#line 598 "barf_regex_parser.trison"
 return new Character('?'); 
#line 1300 "barf_regex_parser.cpp"
    return NULL;
}

// rule 66: bracket_expression_normal_character <- '*'    
AstCommon::Ast * Parser::ReductionRuleHandler0066 ()
{

#line 599 "barf_regex_parser.trison"
 return new Character('*'); 
#line 1310 "barf_regex_parser.cpp"
    return NULL;
}

// rule 67: bracket_expression_normal_character <- '+'    
AstCommon::Ast * Parser::ReductionRuleHandler0067 ()
{

#line 600 "barf_regex_parser.trison"
 return new Character('+'); 
#line 1320 "barf_regex_parser.cpp"
    return NULL;
}

// rule 68: bracket_expression_normal_character <- '.'    
AstCommon::Ast * Parser::ReductionRuleHandler0068 ()
{

#line 601 "barf_regex_parser.trison"
 return new Character('.'); 
#line 1330 "barf_regex_parser.cpp"
    return NULL;
}

// rule 69: bracket_expression_normal_character <- '$'    
AstCommon::Ast * Parser::ReductionRuleHandler0069 ()
{

#line 602 "barf_regex_parser.trison"
 return new Character('$'); 
#line 1340 "barf_regex_parser.cpp"
    return NULL;
}

// rule 70: bracket_expression_normal_character <- ','    
AstCommon::Ast * Parser::ReductionRuleHandler0070 ()
{

#line 603 "barf_regex_parser.trison"
 return new Character(','); 
#line 1350 "barf_regex_parser.cpp"
    return NULL;
}

// rule 71: bracket_expression_normal_character <- '('    
AstCommon::Ast * Parser::ReductionRuleHandler0071 ()
{

#line 604 "barf_regex_parser.trison"
 return new Character('('); 
#line 1360 "barf_regex_parser.cpp"
    return NULL;
}

// rule 72: bracket_expression_normal_character <- ')'    
AstCommon::Ast * Parser::ReductionRuleHandler0072 ()
{

#line 605 "barf_regex_parser.trison"
 return new Character(')'); 
#line 1370 "barf_regex_parser.cpp"
    return NULL;
}

// rule 73: bracket_expression_normal_character <- '{'    
AstCommon::Ast * Parser::ReductionRuleHandler0073 ()
{

#line 606 "barf_regex_parser.trison"
 return new Character('{'); 
#line 1380 "barf_regex_parser.cpp"
    return NULL;
}

// rule 74: bracket_expression_normal_character <- '}'    
AstCommon::Ast * Parser::ReductionRuleHandler0074 ()
{

#line 607 "barf_regex_parser.trison"
 return new Character('}'); 
#line 1390 "barf_regex_parser.cpp"
    return NULL;
}

// rule 75: identifier <- identifier:identifier ALPHA:alpha    
AstCommon::Ast * Parser::ReductionRuleHandler0075 ()
{
    assert(0 < m_reduction_rule_token_count);
    AstCommon::Identifier * identifier = Dsc< AstCommon::Identifier * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Character * alpha = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 613 "barf_regex_parser.trison"

        assert(identifier != NULL);
        identifier->AppendCharacter(alpha->GetCharacter());
        delete alpha;
        return identifier;
    
#line 1409 "barf_regex_parser.cpp"
    return NULL;
}

// rule 76: identifier <- identifier:identifier CHAR:ch    
AstCommon::Ast * Parser::ReductionRuleHandler0076 ()
{
    assert(0 < m_reduction_rule_token_count);
    AstCommon::Identifier * identifier = Dsc< AstCommon::Identifier * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Character * ch = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 621 "barf_regex_parser.trison"

        assert(identifier != NULL);
        identifier->AppendCharacter(ch->GetCharacter());
        delete ch;
        return identifier;
    
#line 1428 "barf_regex_parser.cpp"
    return NULL;
}

// rule 77: identifier <- identifier:identifier DIGIT:digit    
AstCommon::Ast * Parser::ReductionRuleHandler0077 ()
{
    assert(0 < m_reduction_rule_token_count);
    AstCommon::Identifier * identifier = Dsc< AstCommon::Identifier * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Character * digit = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 629 "barf_regex_parser.trison"

        assert(identifier != NULL);
        identifier->AppendCharacter(digit->GetCharacter());
        delete digit;
        return identifier;
    
#line 1447 "barf_regex_parser.cpp"
    return NULL;
}

// rule 78: identifier <- ALPHA:alpha    
AstCommon::Ast * Parser::ReductionRuleHandler0078 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * alpha = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 637 "barf_regex_parser.trison"

        string temp;
        temp += alpha->GetCharacter();
        AstCommon::Identifier *identifier = new AstCommon::Identifier(temp, FiLoc::ms_invalid);
        delete alpha;
        return identifier;
    
#line 1465 "barf_regex_parser.cpp"
    return NULL;
}

// rule 79: identifier <- CHAR:ch    
AstCommon::Ast * Parser::ReductionRuleHandler0079 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * ch = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 646 "barf_regex_parser.trison"

        string temp;
        temp += ch->GetCharacter();
        AstCommon::Identifier *identifier = new AstCommon::Identifier(temp, FiLoc::ms_invalid);
        delete ch;
        return identifier;
    
#line 1483 "barf_regex_parser.cpp"
    return NULL;
}

// rule 80: integer <- integer:integer DIGIT:digit    
AstCommon::Ast * Parser::ReductionRuleHandler0080 ()
{
    assert(0 < m_reduction_rule_token_count);
    AstCommon::SignedInteger * integer = Dsc< AstCommon::SignedInteger * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Character * digit = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 658 "barf_regex_parser.trison"

        integer->ShiftAndAdd(digit->GetCharacter() - '0');
        if (integer->GetValue() > 255)
            integer->SetValue(255);
        delete digit;
        return integer;
    
#line 1503 "barf_regex_parser.cpp"
    return NULL;
}

// rule 81: integer <- DIGIT:digit    
AstCommon::Ast * Parser::ReductionRuleHandler0081 ()
{
    assert(0 < m_reduction_rule_token_count);
    Character * digit = Dsc< Character * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 667 "barf_regex_parser.trison"

        AstCommon::SignedInteger *integer = new AstCommon::SignedInteger(digit->GetCharacter() - '0', FiLoc::ms_invalid);
        delete digit;
        return integer;
    
#line 1519 "barf_regex_parser.cpp"
    return NULL;
}



// ///////////////////////////////////////////////////////////////////////////
// reduction rule lookup table
// ///////////////////////////////////////////////////////////////////////////

Parser::ReductionRule const Parser::ms_reduction_rule[] =
{
    {                 Token::START_,  2, &Parser::ReductionRuleHandler0000, "rule 0: %start <- regex END_    "},
    {                Token::regex__,  3, &Parser::ReductionRuleHandler0001, "rule 1: regex <- regex '|' branch    "},
    {                Token::regex__,  1, &Parser::ReductionRuleHandler0002, "rule 2: regex <- branch    "},
    {               Token::branch__,  1, &Parser::ReductionRuleHandler0003, "rule 3: branch <- branch_which_didnt_just_accept_an_atom    "},
    {               Token::branch__,  1, &Parser::ReductionRuleHandler0004, "rule 4: branch <- branch_which_just_accepted_an_atom    "},
    {               Token::branch__,  0, &Parser::ReductionRuleHandler0005, "rule 5: branch <-     "},
    {Token::branch_which_didnt_just_accept_an_atom__,  2, &Parser::ReductionRuleHandler0006, "rule 6: branch_which_didnt_just_accept_an_atom <- branch_which_just_accepted_an_atom bound    "},
    {Token::branch_which_just_accepted_an_atom__,  2, &Parser::ReductionRuleHandler0007, "rule 7: branch_which_just_accepted_an_atom <- branch_which_just_accepted_an_atom atom    "},
    {Token::branch_which_just_accepted_an_atom__,  2, &Parser::ReductionRuleHandler0008, "rule 8: branch_which_just_accepted_an_atom <- branch_which_didnt_just_accept_an_atom atom    "},
    {Token::branch_which_just_accepted_an_atom__,  1, &Parser::ReductionRuleHandler0009, "rule 9: branch_which_just_accepted_an_atom <- atom    "},
    {                 Token::atom__,  3, &Parser::ReductionRuleHandler0010, "rule 10: atom <- '{' identifier '}'    "},
    {                 Token::atom__,  3, &Parser::ReductionRuleHandler0011, "rule 11: atom <- '(' regex ')'    "},
    {                 Token::atom__,  2, &Parser::ReductionRuleHandler0012, "rule 12: atom <- '(' ')'    "},
    {                 Token::atom__,  1, &Parser::ReductionRuleHandler0013, "rule 13: atom <- '^'    "},
    {                 Token::atom__,  1, &Parser::ReductionRuleHandler0014, "rule 14: atom <- '$'    "},
    {                 Token::atom__,  1, &Parser::ReductionRuleHandler0015, "rule 15: atom <- '.'    "},
    {                 Token::atom__,  1, &Parser::ReductionRuleHandler0016, "rule 16: atom <- atom_normal_character    "},
    {                 Token::atom__,  2, &Parser::ReductionRuleHandler0017, "rule 17: atom <- '\\' atom_normal_character    "},
    {                 Token::atom__,  2, &Parser::ReductionRuleHandler0018, "rule 18: atom <- '\\' atom_control_character    "},
    {                 Token::atom__,  1, &Parser::ReductionRuleHandler0019, "rule 19: atom <- bracket_expression    "},
    {                Token::bound__,  1, &Parser::ReductionRuleHandler0020, "rule 20: bound <- '*'    "},
    {                Token::bound__,  1, &Parser::ReductionRuleHandler0021, "rule 21: bound <- '+'    "},
    {                Token::bound__,  1, &Parser::ReductionRuleHandler0022, "rule 22: bound <- '?'    "},
    {                Token::bound__,  3, &Parser::ReductionRuleHandler0023, "rule 23: bound <- '{' integer '}'    "},
    {                Token::bound__,  4, &Parser::ReductionRuleHandler0024, "rule 24: bound <- '{' integer ',' '}'    "},
    {                Token::bound__,  5, &Parser::ReductionRuleHandler0025, "rule 25: bound <- '{' integer ',' integer '}'    "},
    {   Token::bracket_expression__,  3, &Parser::ReductionRuleHandler0026, "rule 26: bracket_expression <- '[' bracket_character_set ']'    "},
    {   Token::bracket_expression__,  4, &Parser::ReductionRuleHandler0027, "rule 27: bracket_expression <- '[' '^' bracket_character_set ']'    "},
    {Token::bracket_character_set__,  2, &Parser::ReductionRuleHandler0028, "rule 28: bracket_character_set <- bracket_character_set bracket_expression_character    "},
    {Token::bracket_character_set__,  4, &Parser::ReductionRuleHandler0029, "rule 29: bracket_character_set <- bracket_character_set bracket_expression_character '-' bracket_expression_character    "},
    {Token::bracket_character_set__,  6, &Parser::ReductionRuleHandler0030, "rule 30: bracket_character_set <- bracket_character_set '[' ':' identifier ':' ']'    "},
    {Token::bracket_character_set__,  0, &Parser::ReductionRuleHandler0031, "rule 31: bracket_character_set <-     "},
    {Token::bracket_expression_character__,  1, &Parser::ReductionRuleHandler0032, "rule 32: bracket_expression_character <- bracket_expression_normal_character    "},
    {Token::bracket_expression_character__,  2, &Parser::ReductionRuleHandler0033, "rule 33: bracket_expression_character <- '\\' bracket_expression_normal_character    "},
    {Token::bracket_expression_character__,  2, &Parser::ReductionRuleHandler0034, "rule 34: bracket_expression_character <- '\\' bracket_expression_control_character    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0035, "rule 35: atom_control_character <- '|'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0036, "rule 36: atom_control_character <- '('    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0037, "rule 37: atom_control_character <- ')'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0038, "rule 38: atom_control_character <- '{'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0039, "rule 39: atom_control_character <- '}'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0040, "rule 40: atom_control_character <- '['    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0041, "rule 41: atom_control_character <- ']'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0042, "rule 42: atom_control_character <- '?'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0043, "rule 43: atom_control_character <- '*'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0044, "rule 44: atom_control_character <- '+'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0045, "rule 45: atom_control_character <- '.'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0046, "rule 46: atom_control_character <- '^'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0047, "rule 47: atom_control_character <- '$'    "},
    {Token::atom_control_character__,  1, &Parser::ReductionRuleHandler0048, "rule 48: atom_control_character <- '\\'    "},
    {Token::atom_normal_character__,  1, &Parser::ReductionRuleHandler0049, "rule 49: atom_normal_character <- ALPHA    "},
    {Token::atom_normal_character__,  1, &Parser::ReductionRuleHandler0050, "rule 50: atom_normal_character <- DIGIT    "},
    {Token::atom_normal_character__,  1, &Parser::ReductionRuleHandler0051, "rule 51: atom_normal_character <- CHAR    "},
    {Token::atom_normal_character__,  1, &Parser::ReductionRuleHandler0052, "rule 52: atom_normal_character <- ','    "},
    {Token::atom_normal_character__,  1, &Parser::ReductionRuleHandler0053, "rule 53: atom_normal_character <- '-'    "},
    {Token::atom_normal_character__,  1, &Parser::ReductionRuleHandler0054, "rule 54: atom_normal_character <- ':'    "},
    {Token::bracket_expression_control_character__,  1, &Parser::ReductionRuleHandler0055, "rule 55: bracket_expression_control_character <- '-'    "},
    {Token::bracket_expression_control_character__,  1, &Parser::ReductionRuleHandler0056, "rule 56: bracket_expression_control_character <- '^'    "},
    {Token::bracket_expression_control_character__,  1, &Parser::ReductionRuleHandler0057, "rule 57: bracket_expression_control_character <- '['    "},
    {Token::bracket_expression_control_character__,  1, &Parser::ReductionRuleHandler0058, "rule 58: bracket_expression_control_character <- ']'    "},
    {Token::bracket_expression_control_character__,  1, &Parser::ReductionRuleHandler0059, "rule 59: bracket_expression_control_character <- '\\'    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0060, "rule 60: bracket_expression_normal_character <- ALPHA    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0061, "rule 61: bracket_expression_normal_character <- DIGIT    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0062, "rule 62: bracket_expression_normal_character <- CHAR    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0063, "rule 63: bracket_expression_normal_character <- '|'    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0064, "rule 64: bracket_expression_normal_character <- ':'    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0065, "rule 65: bracket_expression_normal_character <- '?'    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0066, "rule 66: bracket_expression_normal_character <- '*'    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0067, "rule 67: bracket_expression_normal_character <- '+'    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0068, "rule 68: bracket_expression_normal_character <- '.'    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0069, "rule 69: bracket_expression_normal_character <- '$'    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0070, "rule 70: bracket_expression_normal_character <- ','    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0071, "rule 71: bracket_expression_normal_character <- '('    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0072, "rule 72: bracket_expression_normal_character <- ')'    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0073, "rule 73: bracket_expression_normal_character <- '{'    "},
    {Token::bracket_expression_normal_character__,  1, &Parser::ReductionRuleHandler0074, "rule 74: bracket_expression_normal_character <- '}'    "},
    {           Token::identifier__,  2, &Parser::ReductionRuleHandler0075, "rule 75: identifier <- identifier ALPHA    "},
    {           Token::identifier__,  2, &Parser::ReductionRuleHandler0076, "rule 76: identifier <- identifier CHAR    "},
    {           Token::identifier__,  2, &Parser::ReductionRuleHandler0077, "rule 77: identifier <- identifier DIGIT    "},
    {           Token::identifier__,  1, &Parser::ReductionRuleHandler0078, "rule 78: identifier <- ALPHA    "},
    {           Token::identifier__,  1, &Parser::ReductionRuleHandler0079, "rule 79: identifier <- CHAR    "},
    {              Token::integer__,  2, &Parser::ReductionRuleHandler0080, "rule 80: integer <- integer DIGIT    "},
    {              Token::integer__,  1, &Parser::ReductionRuleHandler0081, "rule 81: integer <- DIGIT    "},

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
    {   1,   13,   14,   15,    7}, // state    0
    {   0,    0,   22,    0,    0}, // state    1
    {   0,    0,   23,    0,    0}, // state    2
    {   0,    0,   24,    0,    0}, // state    3
    {  25,   14,   39,   40,    7}, // state    4
    {  47,    2,    0,   49,    1}, // state    5
    {   0,    0,   50,    0,    0}, // state    6
    {   0,    0,   51,    0,    0}, // state    7
    {   0,    0,   52,    0,    0}, // state    8
    {   0,    0,   53,    0,    0}, // state    9
    {  54,   20,    0,   74,    2}, // state   10
    {  76,    1,   77,   78,    1}, // state   11
    {   0,    0,   79,    0,    0}, // state   12
    {   0,    0,   80,    0,    0}, // state   13
    {  81,    2,    0,    0,    0}, // state   14
    {   0,    0,   83,    0,    0}, // state   15
    {  84,   13,   97,   98,    3}, // state   16
    { 101,   16,  117,  118,    4}, // state   17
    {   0,    0,  122,    0,    0}, // state   18
    {   0,    0,  123,    0,    0}, // state   19
    {   0,    0,  124,    0,    0}, // state   20
    {   0,    0,  125,    0,    0}, // state   21
    { 126,    2,    0,    0,    0}, // state   22
    {   0,    0,  128,    0,    0}, // state   23
    {   0,    0,  129,    0,    0}, // state   24
    { 130,    4,    0,    0,    0}, // state   25
    {   0,    0,  134,    0,    0}, // state   26
    {   0,    0,  135,    0,    0}, // state   27
    {   0,    0,  136,    0,    0}, // state   28
    {   0,    0,  137,    0,    0}, // state   29
    {   0,    0,  138,    0,    0}, // state   30
    {   0,    0,  139,    0,    0}, // state   31
    {   0,    0,  140,    0,    0}, // state   32
    {   0,    0,  141,    0,    0}, // state   33
    {   0,    0,  142,    0,    0}, // state   34
    {   0,    0,  143,    0,    0}, // state   35
    {   0,    0,  144,    0,    0}, // state   36
    {   0,    0,  145,    0,    0}, // state   37
    {   0,    0,  146,    0,    0}, // state   38
    {   0,    0,  147,    0,    0}, // state   39
    {   0,    0,  148,    0,    0}, // state   40
    {   0,    0,  149,    0,    0}, // state   41
    {   0,    0,  150,  151,    1}, // state   42
    { 152,   18,    0,  170,    2}, // state   43
    {   0,    0,  172,    0,    0}, // state   44
    { 173,   13,  186,  187,    6}, // state   45
    {   0,    0,  193,    0,    0}, // state   46
    {   0,    0,  194,    0,    0}, // state   47
    {   0,    0,  195,    0,    0}, // state   48
    {   0,    0,  196,    0,    0}, // state   49
    { 197,    3,    0,  200,    2}, // state   50
    {   0,    0,  202,    0,    0}, // state   51
    {   0,    0,  203,    0,    0}, // state   52
    {   0,    0,  204,    0,    0}, // state   53
    {   0,    0,  205,    0,    0}, // state   54
    {   0,    0,  206,    0,    0}, // state   55
    {   0,    0,  207,    0,    0}, // state   56
    {   0,    0,  208,    0,    0}, // state   57
    { 209,   18,    0,  227,    2}, // state   58
    {   0,    0,  229,    0,    0}, // state   59
    {   0,    0,  230,    0,    0}, // state   60
    {   0,    0,  231,    0,    0}, // state   61
    {   0,    0,  232,    0,    0}, // state   62
    {   0,    0,  233,    0,    0}, // state   63
    {   0,    0,  234,    0,    0}, // state   64
    {   0,    0,  235,    0,    0}, // state   65
    {   0,    0,  236,    0,    0}, // state   66
    {   0,    0,  237,    0,    0}, // state   67
    {   0,    0,  238,    0,    0}, // state   68
    {   0,    0,  239,    0,    0}, // state   69
    {   0,    0,  240,    0,    0}, // state   70
    {   0,    0,  241,    0,    0}, // state   71
    {   0,    0,  242,    0,    0}, // state   72
    { 243,   20,    0,  263,    2}, // state   73
    { 265,    1,    0,    0,    0}, // state   74
    {   0,    0,  266,    0,    0}, // state   75
    {   0,    0,  267,    0,    0}, // state   76
    { 268,    1,  269,    0,    0}, // state   77
    {   0,    0,  270,    0,    0}, // state   78
    {   0,    0,  271,    0,    0}, // state   79
    {   0,    0,  272,    0,    0}, // state   80
    { 273,    3,    0,    0,    0}, // state   81
    {   0,    0,  276,    0,    0}, // state   82
    {   0,    0,  277,    0,    0}, // state   83
    {   0,    0,  278,    0,    0}, // state   84
    {   0,    0,  279,    0,    0}, // state   85
    {   0,    0,  280,    0,    0}, // state   86
    {   0,    0,  281,    0,    0}, // state   87
    {   0,    0,  282,    0,    0}, // state   88
    {   0,    0,  283,    0,    0}, // state   89
    { 284,    2,    0,  286,    1}, // state   90
    { 287,   16,    0,  303,    2}, // state   91
    {   0,    0,  305,    0,    0}, // state   92
    { 306,    2,    0,  308,    1}, // state   93
    {   0,    0,  309,    0,    0}, // state   94
    { 310,    4,    0,    0,    0}, // state   95
    {   0,    0,  314,    0,    0}, // state   96
    {   0,    0,  315,    0,    0}, // state   97
    { 316,    2,    0,    0,    0}, // state   98
    { 318,    1,    0,    0,    0}, // state   99
    {   0,    0,  319,    0,    0}, // state  100
    {   0,    0,  320,    0,    0}  // state  101

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
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('{'), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,    6}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,    7}},
    {              Token::Type('$'), {        TA_SHIFT_AND_PUSH_STATE,    8}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,    9}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,   10}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   11}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   12}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   13}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    5}},
    // nonterminal transitions
    {                Token::regex__, {                  TA_PUSH_STATE,   14}},
    {               Token::branch__, {                  TA_PUSH_STATE,   15}},
    {Token::branch_which_didnt_just_accept_an_atom__, {                  TA_PUSH_STATE,   16}},
    {Token::branch_which_just_accepted_an_atom__, {                  TA_PUSH_STATE,   17}},
    {                 Token::atom__, {                  TA_PUSH_STATE,   18}},
    {   Token::bracket_expression__, {                  TA_PUSH_STATE,   19}},
    {Token::atom_normal_character__, {                  TA_PUSH_STATE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state    1
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   49}},

// ///////////////////////////////////////////////////////////////////////////
// state    2
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   51}},

// ///////////////////////////////////////////////////////////////////////////
// state    3
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   50}},

// ///////////////////////////////////////////////////////////////////////////
// state    4
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,   21}},
    {              Token::Type('{'), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,    6}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,    7}},
    {              Token::Type('$'), {        TA_SHIFT_AND_PUSH_STATE,    8}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,    9}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,   10}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   11}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   12}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   13}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    5}},
    // nonterminal transitions
    {                Token::regex__, {                  TA_PUSH_STATE,   22}},
    {               Token::branch__, {                  TA_PUSH_STATE,   15}},
    {Token::branch_which_didnt_just_accept_an_atom__, {                  TA_PUSH_STATE,   16}},
    {Token::branch_which_just_accepted_an_atom__, {                  TA_PUSH_STATE,   17}},
    {                 Token::atom__, {                  TA_PUSH_STATE,   18}},
    {   Token::bracket_expression__, {                  TA_PUSH_STATE,   19}},
    {Token::atom_normal_character__, {                  TA_PUSH_STATE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state    5
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    // nonterminal transitions
    {           Token::identifier__, {                  TA_PUSH_STATE,   25}},

// ///////////////////////////////////////////////////////////////////////////
// state    6
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   52}},

// ///////////////////////////////////////////////////////////////////////////
// state    7
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   13}},

// ///////////////////////////////////////////////////////////////////////////
// state    8
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   14}},

// ///////////////////////////////////////////////////////////////////////////
// state    9
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   15}},

// ///////////////////////////////////////////////////////////////////////////
// state   10
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   26}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   27}},
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,   28}},
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,   29}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   30}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   31}},
    {              Token::Type('{'), {        TA_SHIFT_AND_PUSH_STATE,   32}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,    6}},
    {              Token::Type('}'), {        TA_SHIFT_AND_PUSH_STATE,   33}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   34}},
    {              Token::Type('$'), {        TA_SHIFT_AND_PUSH_STATE,   35}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   36}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,   37}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   38}},
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,   39}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   12}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   13}},
    // nonterminal transitions
    {Token::atom_control_character__, {                  TA_PUSH_STATE,   40}},
    {Token::atom_normal_character__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state   11
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   42}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   31}},
    // nonterminal transitions
    {Token::bracket_character_set__, {                  TA_PUSH_STATE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state   12
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state   13
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   54}},

// ///////////////////////////////////////////////////////////////////////////
// state   14
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                   Token::END_, {        TA_SHIFT_AND_PUSH_STATE,   44}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   45}},

// ///////////////////////////////////////////////////////////////////////////
// state   15
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    2}},

// ///////////////////////////////////////////////////////////////////////////
// state   16
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('{'), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,    6}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,    7}},
    {              Token::Type('$'), {        TA_SHIFT_AND_PUSH_STATE,    8}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,    9}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,   10}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   11}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   12}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   13}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    3}},
    // nonterminal transitions
    {                 Token::atom__, {                  TA_PUSH_STATE,   46}},
    {   Token::bracket_expression__, {                  TA_PUSH_STATE,   19}},
    {Token::atom_normal_character__, {                  TA_PUSH_STATE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state   17
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,   47}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   48}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   49}},
    {              Token::Type('{'), {        TA_SHIFT_AND_PUSH_STATE,   50}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,    6}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,    7}},
    {              Token::Type('$'), {        TA_SHIFT_AND_PUSH_STATE,    8}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,    9}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,   10}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   11}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   12}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   13}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    4}},
    // nonterminal transitions
    {                 Token::atom__, {                  TA_PUSH_STATE,   51}},
    {   Token::bracket_expression__, {                  TA_PUSH_STATE,   19}},
    {                Token::bound__, {                  TA_PUSH_STATE,   52}},
    {Token::atom_normal_character__, {                  TA_PUSH_STATE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state   18
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    9}},

// ///////////////////////////////////////////////////////////////////////////
// state   19
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   19}},

// ///////////////////////////////////////////////////////////////////////////
// state   20
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   16}},

// ///////////////////////////////////////////////////////////////////////////
// state   21
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   12}},

// ///////////////////////////////////////////////////////////////////////////
// state   22
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   45}},
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,   53}},

// ///////////////////////////////////////////////////////////////////////////
// state   23
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state   24
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   79}},

// ///////////////////////////////////////////////////////////////////////////
// state   25
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,   54}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,   55}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,   56}},
    {              Token::Type('}'), {        TA_SHIFT_AND_PUSH_STATE,   57}},

// ///////////////////////////////////////////////////////////////////////////
// state   26
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   35}},

// ///////////////////////////////////////////////////////////////////////////
// state   27
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   36}},

// ///////////////////////////////////////////////////////////////////////////
// state   28
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   37}},

// ///////////////////////////////////////////////////////////////////////////
// state   29
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   42}},

// ///////////////////////////////////////////////////////////////////////////
// state   30
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   43}},

// ///////////////////////////////////////////////////////////////////////////
// state   31
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   44}},

// ///////////////////////////////////////////////////////////////////////////
// state   32
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   38}},

// ///////////////////////////////////////////////////////////////////////////
// state   33
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   39}},

// ///////////////////////////////////////////////////////////////////////////
// state   34
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   46}},

// ///////////////////////////////////////////////////////////////////////////
// state   35
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   47}},

// ///////////////////////////////////////////////////////////////////////////
// state   36
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   45}},

// ///////////////////////////////////////////////////////////////////////////
// state   37
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   48}},

// ///////////////////////////////////////////////////////////////////////////
// state   38
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   40}},

// ///////////////////////////////////////////////////////////////////////////
// state   39
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state   40
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   18}},

// ///////////////////////////////////////////////////////////////////////////
// state   41
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   17}},

// ///////////////////////////////////////////////////////////////////////////
// state   42
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   31}},
    // nonterminal transitions
    {Token::bracket_character_set__, {                  TA_PUSH_STATE,   58}},

// ///////////////////////////////////////////////////////////////////////////
// state   43
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,   59}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,   60}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,   61}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   62}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   63}},
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,   64}},
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,   65}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   66}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   67}},
    {              Token::Type('{'), {        TA_SHIFT_AND_PUSH_STATE,   68}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,   69}},
    {              Token::Type('}'), {        TA_SHIFT_AND_PUSH_STATE,   70}},
    {              Token::Type('$'), {        TA_SHIFT_AND_PUSH_STATE,   71}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   72}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,   73}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   74}},
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,   75}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    // nonterminal transitions
    {Token::bracket_expression_character__, {                  TA_PUSH_STATE,   77}},
    {Token::bracket_expression_normal_character__, {                  TA_PUSH_STATE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state   44
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {TA_REDUCE_AND_ACCEPT_USING_RULE,    0}},

// ///////////////////////////////////////////////////////////////////////////
// state   45
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,    1}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,    2}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,    3}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,    4}},
    {              Token::Type('{'), {        TA_SHIFT_AND_PUSH_STATE,    5}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,    6}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,    7}},
    {              Token::Type('$'), {        TA_SHIFT_AND_PUSH_STATE,    8}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,    9}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,   10}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   11}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   12}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   13}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    5}},
    // nonterminal transitions
    {               Token::branch__, {                  TA_PUSH_STATE,   79}},
    {Token::branch_which_didnt_just_accept_an_atom__, {                  TA_PUSH_STATE,   16}},
    {Token::branch_which_just_accepted_an_atom__, {                  TA_PUSH_STATE,   17}},
    {                 Token::atom__, {                  TA_PUSH_STATE,   18}},
    {   Token::bracket_expression__, {                  TA_PUSH_STATE,   19}},
    {Token::atom_normal_character__, {                  TA_PUSH_STATE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state   46
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    8}},

// ///////////////////////////////////////////////////////////////////////////
// state   47
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   22}},

// ///////////////////////////////////////////////////////////////////////////
// state   48
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state   49
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   21}},

// ///////////////////////////////////////////////////////////////////////////
// state   50
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,   80}},
    // nonterminal transitions
    {           Token::identifier__, {                  TA_PUSH_STATE,   25}},
    {              Token::integer__, {                  TA_PUSH_STATE,   81}},

// ///////////////////////////////////////////////////////////////////////////
// state   51
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    7}},

// ///////////////////////////////////////////////////////////////////////////
// state   52
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    6}},

// ///////////////////////////////////////////////////////////////////////////
// state   53
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   11}},

// ///////////////////////////////////////////////////////////////////////////
// state   54
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   75}},

// ///////////////////////////////////////////////////////////////////////////
// state   55
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   76}},

// ///////////////////////////////////////////////////////////////////////////
// state   56
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   77}},

// ///////////////////////////////////////////////////////////////////////////
// state   57
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   10}},

// ///////////////////////////////////////////////////////////////////////////
// state   58
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,   59}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,   60}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,   61}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   62}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   63}},
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,   64}},
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,   65}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   66}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   67}},
    {              Token::Type('{'), {        TA_SHIFT_AND_PUSH_STATE,   68}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,   69}},
    {              Token::Type('}'), {        TA_SHIFT_AND_PUSH_STATE,   70}},
    {              Token::Type('$'), {        TA_SHIFT_AND_PUSH_STATE,   71}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   72}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,   73}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   74}},
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,   82}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    // nonterminal transitions
    {Token::bracket_expression_character__, {                  TA_PUSH_STATE,   77}},
    {Token::bracket_expression_normal_character__, {                  TA_PUSH_STATE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state   59
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   60}},

// ///////////////////////////////////////////////////////////////////////////
// state   60
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   62}},

// ///////////////////////////////////////////////////////////////////////////
// state   61
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   61}},

// ///////////////////////////////////////////////////////////////////////////
// state   62
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   63}},

// ///////////////////////////////////////////////////////////////////////////
// state   63
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   71}},

// ///////////////////////////////////////////////////////////////////////////
// state   64
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   72}},

// ///////////////////////////////////////////////////////////////////////////
// state   65
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   65}},

// ///////////////////////////////////////////////////////////////////////////
// state   66
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   66}},

// ///////////////////////////////////////////////////////////////////////////
// state   67
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   67}},

// ///////////////////////////////////////////////////////////////////////////
// state   68
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   73}},

// ///////////////////////////////////////////////////////////////////////////
// state   69
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   70}},

// ///////////////////////////////////////////////////////////////////////////
// state   70
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   74}},

// ///////////////////////////////////////////////////////////////////////////
// state   71
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   69}},

// ///////////////////////////////////////////////////////////////////////////
// state   72
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   68}},

// ///////////////////////////////////////////////////////////////////////////
// state   73
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,   59}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,   60}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,   61}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   62}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   63}},
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,   64}},
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,   65}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   66}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   67}},
    {              Token::Type('{'), {        TA_SHIFT_AND_PUSH_STATE,   68}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,   69}},
    {              Token::Type('}'), {        TA_SHIFT_AND_PUSH_STATE,   70}},
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   83}},
    {              Token::Type('$'), {        TA_SHIFT_AND_PUSH_STATE,   71}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   72}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,   84}},
    {              Token::Type('['), {        TA_SHIFT_AND_PUSH_STATE,   85}},
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,   86}},
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   87}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    // nonterminal transitions
    {Token::bracket_expression_control_character__, {                  TA_PUSH_STATE,   88}},
    {Token::bracket_expression_normal_character__, {                  TA_PUSH_STATE,   89}},

// ///////////////////////////////////////////////////////////////////////////
// state   74
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   90}},

// ///////////////////////////////////////////////////////////////////////////
// state   75
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   26}},

// ///////////////////////////////////////////////////////////////////////////
// state   76
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   64}},

// ///////////////////////////////////////////////////////////////////////////
// state   77
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('-'), {        TA_SHIFT_AND_PUSH_STATE,   91}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   28}},

// ///////////////////////////////////////////////////////////////////////////
// state   78
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   32}},

// ///////////////////////////////////////////////////////////////////////////
// state   79
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,    1}},

// ///////////////////////////////////////////////////////////////////////////
// state   80
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   81}},

// ///////////////////////////////////////////////////////////////////////////
// state   81
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,   92}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,   93}},
    {              Token::Type('}'), {        TA_SHIFT_AND_PUSH_STATE,   94}},

// ///////////////////////////////////////////////////////////////////////////
// state   82
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   27}},

// ///////////////////////////////////////////////////////////////////////////
// state   83
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   56}},

// ///////////////////////////////////////////////////////////////////////////
// state   84
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   59}},

// ///////////////////////////////////////////////////////////////////////////
// state   85
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   57}},

// ///////////////////////////////////////////////////////////////////////////
// state   86
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   58}},

// ///////////////////////////////////////////////////////////////////////////
// state   87
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   55}},

// ///////////////////////////////////////////////////////////////////////////
// state   88
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   34}},

// ///////////////////////////////////////////////////////////////////////////
// state   89
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   33}},

// ///////////////////////////////////////////////////////////////////////////
// state   90
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    // nonterminal transitions
    {           Token::identifier__, {                  TA_PUSH_STATE,   95}},

// ///////////////////////////////////////////////////////////////////////////
// state   91
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,   59}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,   60}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,   61}},
    {              Token::Type('|'), {        TA_SHIFT_AND_PUSH_STATE,   62}},
    {              Token::Type('('), {        TA_SHIFT_AND_PUSH_STATE,   63}},
    {              Token::Type(')'), {        TA_SHIFT_AND_PUSH_STATE,   64}},
    {              Token::Type('?'), {        TA_SHIFT_AND_PUSH_STATE,   65}},
    {              Token::Type('*'), {        TA_SHIFT_AND_PUSH_STATE,   66}},
    {              Token::Type('+'), {        TA_SHIFT_AND_PUSH_STATE,   67}},
    {              Token::Type('{'), {        TA_SHIFT_AND_PUSH_STATE,   68}},
    {              Token::Type(','), {        TA_SHIFT_AND_PUSH_STATE,   69}},
    {              Token::Type('}'), {        TA_SHIFT_AND_PUSH_STATE,   70}},
    {              Token::Type('$'), {        TA_SHIFT_AND_PUSH_STATE,   71}},
    {              Token::Type('.'), {        TA_SHIFT_AND_PUSH_STATE,   72}},
    {             Token::Type('\\'), {        TA_SHIFT_AND_PUSH_STATE,   73}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   76}},
    // nonterminal transitions
    {Token::bracket_expression_character__, {                  TA_PUSH_STATE,   96}},
    {Token::bracket_expression_normal_character__, {                  TA_PUSH_STATE,   78}},

// ///////////////////////////////////////////////////////////////////////////
// state   92
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   80}},

// ///////////////////////////////////////////////////////////////////////////
// state   93
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,   80}},
    {              Token::Type('}'), {        TA_SHIFT_AND_PUSH_STATE,   97}},
    // nonterminal transitions
    {              Token::integer__, {                  TA_PUSH_STATE,   98}},

// ///////////////////////////////////////////////////////////////////////////
// state   94
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   23}},

// ///////////////////////////////////////////////////////////////////////////
// state   95
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,   54}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,   55}},
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,   56}},
    {              Token::Type(':'), {        TA_SHIFT_AND_PUSH_STATE,   99}},

// ///////////////////////////////////////////////////////////////////////////
// state   96
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   29}},

// ///////////////////////////////////////////////////////////////////////////
// state   97
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   24}},

// ///////////////////////////////////////////////////////////////////////////
// state   98
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::DIGIT, {        TA_SHIFT_AND_PUSH_STATE,   92}},
    {              Token::Type('}'), {        TA_SHIFT_AND_PUSH_STATE,  100}},

// ///////////////////////////////////////////////////////////////////////////
// state   99
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type(']'), {        TA_SHIFT_AND_PUSH_STATE,  101}},

// ///////////////////////////////////////////////////////////////////////////
// state  100
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   25}},

// ///////////////////////////////////////////////////////////////////////////
// state  101
// ///////////////////////////////////////////////////////////////////////////
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   30}}

};

unsigned int const Parser::ms_state_transition_count =
    sizeof(Parser::ms_state_transition) /
    sizeof(Parser::StateTransition);


#line 83 "barf_regex_parser.trison"

inline bool IsHexDigit (Uint8 const c)
{
    return c >= '0' && c <= '9' || c >= 'A' && c <= 'F' || c >= 'a' && c <= 'f';
}

inline Uint8 GetHexDigitValue (Uint8 c)
{
    assert(IsHexDigit(c));
    if (c >= '0' && c <= '9')
        c -= '0';
    else if (c >= 'A' && c <= 'F')
        c += 0xA - 'A';
    else if (c >= 'a' && c <= 'f')
        c += 0xA - 'a';
    return c;
}

Parser::ParserReturnCode Parser::Parse (RegularExpressionMap *macro_map)
{
    assert(m_macro_map == NULL);
    m_macro_map = macro_map;
    try {
        ParserReturnCode retval = Parse();
        m_macro_map = NULL;
        return retval;
    } catch (string const &exception) {
        m_macro_map = NULL;
        throw exception;
    }
}

Parser::Token::Type Parser::Scan ()
{
    while (true)
    {
        int c;

        c = In().get();
        if (c == EOF)
            return Token::END_;

        if (c >= '0' && c <= '9')
        {
//             fprintf(stderr, "\n######## scanning DIGIT '%c'\n", static_cast<char>(c));
            m_lookahead_token = new Character(c);
            return Token::DIGIT;
        }
        else if (c >= 'A' && c <= 'Z'
                 ||
                 c >= 'a' && c <= 'z')
        {
//             fprintf(stderr, "\n######## scanning CHAR '%c'\n", static_cast<char>(c));
            m_lookahead_token = new Character(c);
            return Token::ALPHA;
        }
        else if (c >= 128 && c < 256)
        {
//             fprintf(stderr, "\n######## scanning extended ascii CHAR \\0x%02X\n", c);
            m_lookahead_token = new Character(c);
            return Token::ALPHA;
        }

        switch (c)
        {
            // special case for backslash, because it can be followed by a hex char
            case '\\':
                // fuck it for now, just return the backslash
                /*
                if (In().peek() == EOF)
                {
                    In().get();
                    assert(In().eof());
                    return static_cast<Token::Type>('\\');
                }

                c = In().peek();
                if (c == 'x')
                {
                    In().get();
                    assert(!In().eof());

                    c = In().peek();
                    if (c == EOF)
                    {
                        cerr << "\n\nBLAH BLAH\n\n" << endl;
                        assert(In().eof());
                    }
                    if (c != EOF && IsHexDigit(c))
                    {
                        In().get();
                        assert(!In().eof());
                        Uint8 hex_value = GetHexDigitValue(c);

                        c = In().peek();
                        if (c != EOF && IsHexDigit(c))
                        {
                            In().get();
                            assert(!In().eof());
                            hex_value = (hex_value << 4) + GetHexDigitValue(c);
                        }

                        m_lookahead_token = new Character(hex_value, true);
                        return Token::ALPHA;
                    }
                    else
                    {
                        // unget the 'x' char
//                         In().putback('x');
                        In().unget();
//                         assert(!In().eof());
//                         assert((c = In().peek()) == 'x');
                        return static_cast<Token::Type>('\\');
                    }
                }
                // if it didn't work, push the char back in the stream and return '\\'
                else
                {
                    return static_cast<Token::Type>('\\');
                }
                */
            case '^':
            case '$':
            case '*':
            case '+':
            case '?':
            case '{':
            case '}':
            case '[':
            case ']':
            case '(':
            case ')':
            case '.':
            case ',':
            case '|':
            case '-':
            case ':':
//                 fprintf(stderr, "\n######## scanning control char '%c'\n", static_cast<char>(c));
                return static_cast<Token::Type>(c);

            case '\0':
            case '\a':
            case '\b':
            case '\t':
            case '\n':
            case '\v':
            case '\f':
            case '\r':
//                 fprintf(stderr, "\n######## skipping escaped character\n");
                break;

            case ' ':
            case '!':
            case '"':
            case '#':
            case '%':
            case '&':
            case '\'':
            case '/':
            case ';':
            case '<':
            case '=':
            case '>':
            case '@':
            case '_':
            case '`':
            case '~':
//                 fprintf(stderr, "\n######## scanning CHAR '%c'\n", static_cast<char>(c));
                m_lookahead_token = new Character(c);
                return Token::CHAR;

            default:
//                 fprintf(stderr, "\n######## skipping invalid character\n");
                break;
        }
    }
}

} // end of namespace Regex
} // end of namespace Barf

#line 2788 "barf_regex_parser.cpp"

