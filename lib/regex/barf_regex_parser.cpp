#include "barf_regex_parser.hpp"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#define DEBUG_SPEW_1(x) if (m_debug_spew_level >= 1) std::cerr << x
#define DEBUG_SPEW_2(x) if (m_debug_spew_level >= 2) std::cerr << x


#line 65 "barf_regex_parser.trison"

#include <sstream>

namespace Barf {
namespace Regex {

#line 20 "barf_regex_parser.cpp"

Parser::Parser ()
    : InputBase()
{

#line 72 "barf_regex_parser.trison"

    m_macro_map = NULL;

#line 30 "barf_regex_parser.cpp"
    m_debug_spew_level = 0;
    DEBUG_SPEW_2("### number of state transitions = " << ms_state_transition_count << std::endl);
}

Parser::~Parser ()
{

#line 76 "barf_regex_parser.trison"

    assert(m_macro_map == NULL);

#line 42 "barf_regex_parser.cpp"
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

#line 80 "barf_regex_parser.trison"


#line 82 "barf_regex_parser.cpp"

    ParserReturnCode return_code = PrivateParse(parsed_tree_root);


#line 83 "barf_regex_parser.trison"


#line 90 "barf_regex_parser.cpp"

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
        "atom_control_char",
        "atom_normal_char",
        "bound",
        "bracket_char_set",
        "bracket_expression",
        "bracket_expression_char",
        "bracket_expression_control_char",
        "bracket_expression_normal_char",
        "branch",
        "branch_which_didnt_just_accept_an_atom",
        "branch_which_just_accepted_an_atom",
        "id",
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
Ast::Base * Parser::ReductionRuleHandler0000 ()
{
    assert(0 < m_reduction_rule_token_count);
    return m_token_stack[m_token_stack.size() - m_reduction_rule_token_count];

}

// rule 1: regex <- regex:regex '|' branch:branch    
Ast::Base * Parser::ReductionRuleHandler0001 ()
{
    assert(0 < m_reduction_rule_token_count);
    RegularExpression * regex = Dsc< RegularExpression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(2 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 313 "barf_regex_parser.trison"

        regex->Append(branch);
        return regex;
    
#line 480 "barf_regex_parser.cpp"
}

// rule 2: regex <- branch:branch    
Ast::Base * Parser::ReductionRuleHandler0002 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 319 "barf_regex_parser.trison"

        RegularExpression *regex = new RegularExpression();
        regex->Append(branch);
        return regex;
    
#line 495 "barf_regex_parser.cpp"
}

// rule 3: branch <- branch_which_didnt_just_accept_an_atom:branch    
Ast::Base * Parser::ReductionRuleHandler0003 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 328 "barf_regex_parser.trison"
 return branch; 
#line 506 "barf_regex_parser.cpp"
}

// rule 4: branch <- branch_which_just_accepted_an_atom:branch    
Ast::Base * Parser::ReductionRuleHandler0004 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 329 "barf_regex_parser.trison"
 return branch; 
#line 517 "barf_regex_parser.cpp"
}

// rule 5: branch <-     
Ast::Base * Parser::ReductionRuleHandler0005 ()
{

#line 330 "barf_regex_parser.trison"
 return new Branch(); 
#line 526 "barf_regex_parser.cpp"
}

// rule 6: branch_which_didnt_just_accept_an_atom <- branch_which_just_accepted_an_atom:branch bound:bound    
Ast::Base * Parser::ReductionRuleHandler0006 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Bound * bound = Dsc< Bound * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 336 "barf_regex_parser.trison"

        branch->AddBound(bound);
        return branch;
    
#line 542 "barf_regex_parser.cpp"
}

// rule 7: branch_which_just_accepted_an_atom <- branch_which_just_accepted_an_atom:branch atom:atom    
Ast::Base * Parser::ReductionRuleHandler0007 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Atom * atom = Dsc< Atom * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 345 "barf_regex_parser.trison"

        branch->AddAtom(atom);
        return branch;
    
#line 558 "barf_regex_parser.cpp"
}

// rule 8: branch_which_just_accepted_an_atom <- branch_which_didnt_just_accept_an_atom:branch atom:atom    
Ast::Base * Parser::ReductionRuleHandler0008 ()
{
    assert(0 < m_reduction_rule_token_count);
    Branch * branch = Dsc< Branch * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Atom * atom = Dsc< Atom * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 351 "barf_regex_parser.trison"

        branch->AddAtom(atom);
        return branch;
    
#line 574 "barf_regex_parser.cpp"
}

// rule 9: branch_which_just_accepted_an_atom <- atom:atom    
Ast::Base * Parser::ReductionRuleHandler0009 ()
{
    assert(0 < m_reduction_rule_token_count);
    Atom * atom = Dsc< Atom * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 357 "barf_regex_parser.trison"

        Branch *branch = new Branch();
        branch->AddAtom(atom);
        return branch;
    
#line 589 "barf_regex_parser.cpp"
}

// rule 10: atom <- '{' id:macro_name '}'    
Ast::Base * Parser::ReductionRuleHandler0010 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::Id * macro_name = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 371 "barf_regex_parser.trison"

        assert(macro_name != NULL);
        if (m_macro_map == NULL)
            THROW_STRING("can not use regex macros if no macro map was provided");
        RegularExpression *macro_regex = m_macro_map->GetElement(macro_name->GetText());
        if (macro_regex == NULL)
            THROW_STRING("undefined macro \"" + macro_name->GetText() + "\"");
        return macro_regex;
    
#line 608 "barf_regex_parser.cpp"
}

// rule 11: atom <- '(' regex:regex ')'    
Ast::Base * Parser::ReductionRuleHandler0011 ()
{
    assert(1 < m_reduction_rule_token_count);
    RegularExpression * regex = Dsc< RegularExpression * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 381 "barf_regex_parser.trison"
 return regex; 
#line 619 "barf_regex_parser.cpp"
}

// rule 12: atom <- '(' ')'    
Ast::Base * Parser::ReductionRuleHandler0012 ()
{

#line 382 "barf_regex_parser.trison"
 return new RegularExpression(); 
#line 628 "barf_regex_parser.cpp"
}

// rule 13: atom <- '^'    
Ast::Base * Parser::ReductionRuleHandler0013 ()
{

#line 383 "barf_regex_parser.trison"
 return new Char('\0', CT_BEGINNING_OF_LINE); 
#line 637 "barf_regex_parser.cpp"
}

// rule 14: atom <- '$'    
Ast::Base * Parser::ReductionRuleHandler0014 ()
{

#line 384 "barf_regex_parser.trison"
 return new Char('\0', CT_END_OF_LINE); 
#line 646 "barf_regex_parser.cpp"
}

// rule 15: atom <- '.'    
Ast::Base * Parser::ReductionRuleHandler0015 ()
{

#line 385 "barf_regex_parser.trison"
 return new BracketCharSet('\n', true); 
#line 655 "barf_regex_parser.cpp"
}

// rule 16: atom <- atom_normal_char:ch    
Ast::Base * Parser::ReductionRuleHandler0016 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * ch = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 386 "barf_regex_parser.trison"
 return ch; 
#line 666 "barf_regex_parser.cpp"
}

// rule 17: atom <- '\\' atom_normal_char:ch    
Ast::Base * Parser::ReductionRuleHandler0017 ()
{
    assert(1 < m_reduction_rule_token_count);
    Char * ch = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 387 "barf_regex_parser.trison"
 ch->Escape(); return ch; 
#line 677 "barf_regex_parser.cpp"
}

// rule 18: atom <- '\\' atom_control_char:ch    
Ast::Base * Parser::ReductionRuleHandler0018 ()
{
    assert(1 < m_reduction_rule_token_count);
    Char * ch = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 388 "barf_regex_parser.trison"
 return ch; 
#line 688 "barf_regex_parser.cpp"
}

// rule 19: atom <- bracket_expression:exp    
Ast::Base * Parser::ReductionRuleHandler0019 ()
{
    assert(0 < m_reduction_rule_token_count);
    Atom * exp = Dsc< Atom * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 389 "barf_regex_parser.trison"
 return exp; 
#line 699 "barf_regex_parser.cpp"
}

// rule 20: bound <- '*'    
Ast::Base * Parser::ReductionRuleHandler0020 ()
{

#line 394 "barf_regex_parser.trison"
 return new Bound(0, Bound::NO_UPPER_BOUND); 
#line 708 "barf_regex_parser.cpp"
}

// rule 21: bound <- '+'    
Ast::Base * Parser::ReductionRuleHandler0021 ()
{

#line 396 "barf_regex_parser.trison"
 return new Bound(1, Bound::NO_UPPER_BOUND); 
#line 717 "barf_regex_parser.cpp"
}

// rule 22: bound <- '?'    
Ast::Base * Parser::ReductionRuleHandler0022 ()
{

#line 398 "barf_regex_parser.trison"
 return new Bound(0, 1); 
#line 726 "barf_regex_parser.cpp"
}

// rule 23: bound <- '{' integer:exact_bound '}'    
Ast::Base * Parser::ReductionRuleHandler0023 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::SignedInteger * exact_bound = Dsc< Ast::SignedInteger * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 401 "barf_regex_parser.trison"

        assert(exact_bound->GetValue() >= 0);
        Bound *bound = new Bound(exact_bound->GetValue(), exact_bound->GetValue());
        delete exact_bound;
        return bound;
    
#line 742 "barf_regex_parser.cpp"
}

// rule 24: bound <- '{' integer:lower_bound ',' '}'    
Ast::Base * Parser::ReductionRuleHandler0024 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::SignedInteger * lower_bound = Dsc< Ast::SignedInteger * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 409 "barf_regex_parser.trison"

        assert(lower_bound->GetValue() >= 0);
        return new Bound(lower_bound->GetValue(), Bound::NO_UPPER_BOUND);
    
#line 756 "barf_regex_parser.cpp"
}

// rule 25: bound <- '{' integer:lower_bound ',' integer:upper_bound '}'    
Ast::Base * Parser::ReductionRuleHandler0025 ()
{
    assert(1 < m_reduction_rule_token_count);
    Ast::SignedInteger * lower_bound = Dsc< Ast::SignedInteger * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    Ast::SignedInteger * upper_bound = Dsc< Ast::SignedInteger * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 415 "barf_regex_parser.trison"

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
    
#line 795 "barf_regex_parser.cpp"
}

// rule 26: bracket_expression <- '[' bracket_char_set:bracket_char_set ']'    
Ast::Base * Parser::ReductionRuleHandler0026 ()
{
    assert(1 < m_reduction_rule_token_count);
    BracketCharSet * bracket_char_set = Dsc< BracketCharSet * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 451 "barf_regex_parser.trison"

        if (bracket_char_set->GetIsEmpty())
            THROW_STRING("invalid empty bracket expression");
        return bracket_char_set;
    
#line 810 "barf_regex_parser.cpp"
}

// rule 27: bracket_expression <- '[' '^' bracket_char_set:bracket_char_set ']'    
Ast::Base * Parser::ReductionRuleHandler0027 ()
{
    assert(2 < m_reduction_rule_token_count);
    BracketCharSet * bracket_char_set = Dsc< BracketCharSet * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 2]);

#line 458 "barf_regex_parser.trison"

        if (bracket_char_set->GetIsEmpty())
            THROW_STRING("invalid empty bracket expression");
        bracket_char_set->Negate();
        return bracket_char_set;
    
#line 826 "barf_regex_parser.cpp"
}

// rule 28: bracket_char_set <- bracket_char_set:bracket_char_set bracket_expression_char:ch    
Ast::Base * Parser::ReductionRuleHandler0028 ()
{
    assert(0 < m_reduction_rule_token_count);
    BracketCharSet * bracket_char_set = Dsc< BracketCharSet * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Char * ch = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 469 "barf_regex_parser.trison"

        bracket_char_set->AddChar(ch->GetChar());
        delete ch;
        return bracket_char_set;
    
#line 843 "barf_regex_parser.cpp"
}

// rule 29: bracket_char_set <- bracket_char_set:bracket_char_set bracket_expression_char:begin_range '-' bracket_expression_char:end_range    
Ast::Base * Parser::ReductionRuleHandler0029 ()
{
    assert(0 < m_reduction_rule_token_count);
    BracketCharSet * bracket_char_set = Dsc< BracketCharSet * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Char * begin_range = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);
    assert(3 < m_reduction_rule_token_count);
    Char * end_range = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 476 "barf_regex_parser.trison"

        if (end_range->GetChar() < begin_range->GetChar())
            THROW_STRING("invalid bracketed range [" << GetCharLiteral(begin_range->GetChar(), false) << '-' << GetCharLiteral(end_range->GetChar(), false) << ']');
        bracket_char_set->AddCharRange(
            begin_range->GetChar(),
            end_range->GetChar());
        delete begin_range;
        delete end_range;
        return bracket_char_set;
    
#line 867 "barf_regex_parser.cpp"
}

// rule 30: bracket_char_set <- bracket_char_set:bracket_char_set '[' ':' id:id ':' ']'    
Ast::Base * Parser::ReductionRuleHandler0030 ()
{
    assert(0 < m_reduction_rule_token_count);
    BracketCharSet * bracket_char_set = Dsc< BracketCharSet * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(3 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 3]);

#line 488 "barf_regex_parser.trison"

        bracket_char_set->AddCharClass(id->GetText());
        delete id;
        return bracket_char_set;
    
#line 884 "barf_regex_parser.cpp"
}

// rule 31: bracket_char_set <-     
Ast::Base * Parser::ReductionRuleHandler0031 ()
{

#line 495 "barf_regex_parser.trison"

        BracketCharSet *bracket_char_set = new BracketCharSet();
        return bracket_char_set;
    
#line 896 "barf_regex_parser.cpp"
}

// rule 32: bracket_expression_char <- bracket_expression_normal_char:normal_char    
Ast::Base * Parser::ReductionRuleHandler0032 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * normal_char = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 503 "barf_regex_parser.trison"
 return normal_char; 
#line 907 "barf_regex_parser.cpp"
}

// rule 33: bracket_expression_char <- '\\' bracket_expression_normal_char:normal_char    
Ast::Base * Parser::ReductionRuleHandler0033 ()
{
    assert(1 < m_reduction_rule_token_count);
    Char * normal_char = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 505 "barf_regex_parser.trison"
 normal_char->Escape(); return normal_char; 
#line 918 "barf_regex_parser.cpp"
}

// rule 34: bracket_expression_char <- '\\' bracket_expression_control_char:control_char    
Ast::Base * Parser::ReductionRuleHandler0034 ()
{
    assert(1 < m_reduction_rule_token_count);
    Char * control_char = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 507 "barf_regex_parser.trison"
 return control_char; 
#line 929 "barf_regex_parser.cpp"
}

// rule 35: atom_control_char <- '|'    
Ast::Base * Parser::ReductionRuleHandler0035 ()
{

#line 523 "barf_regex_parser.trison"
 return new Char('|'); 
#line 938 "barf_regex_parser.cpp"
}

// rule 36: atom_control_char <- '('    
Ast::Base * Parser::ReductionRuleHandler0036 ()
{

#line 525 "barf_regex_parser.trison"
 return new Char('('); 
#line 947 "barf_regex_parser.cpp"
}

// rule 37: atom_control_char <- ')'    
Ast::Base * Parser::ReductionRuleHandler0037 ()
{

#line 527 "barf_regex_parser.trison"
 return new Char(')'); 
#line 956 "barf_regex_parser.cpp"
}

// rule 38: atom_control_char <- '{'    
Ast::Base * Parser::ReductionRuleHandler0038 ()
{

#line 529 "barf_regex_parser.trison"
 return new Char('{'); 
#line 965 "barf_regex_parser.cpp"
}

// rule 39: atom_control_char <- '}'    
Ast::Base * Parser::ReductionRuleHandler0039 ()
{

#line 531 "barf_regex_parser.trison"
 return new Char('}'); 
#line 974 "barf_regex_parser.cpp"
}

// rule 40: atom_control_char <- '['    
Ast::Base * Parser::ReductionRuleHandler0040 ()
{

#line 533 "barf_regex_parser.trison"
 return new Char('['); 
#line 983 "barf_regex_parser.cpp"
}

// rule 41: atom_control_char <- ']'    
Ast::Base * Parser::ReductionRuleHandler0041 ()
{

#line 535 "barf_regex_parser.trison"
 return new Char(']'); 
#line 992 "barf_regex_parser.cpp"
}

// rule 42: atom_control_char <- '?'    
Ast::Base * Parser::ReductionRuleHandler0042 ()
{

#line 537 "barf_regex_parser.trison"
 return new Char('?'); 
#line 1001 "barf_regex_parser.cpp"
}

// rule 43: atom_control_char <- '*'    
Ast::Base * Parser::ReductionRuleHandler0043 ()
{

#line 539 "barf_regex_parser.trison"
 return new Char('*'); 
#line 1010 "barf_regex_parser.cpp"
}

// rule 44: atom_control_char <- '+'    
Ast::Base * Parser::ReductionRuleHandler0044 ()
{

#line 541 "barf_regex_parser.trison"
 return new Char('+'); 
#line 1019 "barf_regex_parser.cpp"
}

// rule 45: atom_control_char <- '.'    
Ast::Base * Parser::ReductionRuleHandler0045 ()
{

#line 543 "barf_regex_parser.trison"
 return new Char('.'); 
#line 1028 "barf_regex_parser.cpp"
}

// rule 46: atom_control_char <- '^'    
Ast::Base * Parser::ReductionRuleHandler0046 ()
{

#line 545 "barf_regex_parser.trison"
 return new Char('^'); 
#line 1037 "barf_regex_parser.cpp"
}

// rule 47: atom_control_char <- '$'    
Ast::Base * Parser::ReductionRuleHandler0047 ()
{

#line 547 "barf_regex_parser.trison"
 return new Char('$'); 
#line 1046 "barf_regex_parser.cpp"
}

// rule 48: atom_control_char <- '\\'    
Ast::Base * Parser::ReductionRuleHandler0048 ()
{

#line 549 "barf_regex_parser.trison"
 return new Char('\\'); 
#line 1055 "barf_regex_parser.cpp"
}

// rule 49: atom_normal_char <- ALPHA:alpha    
Ast::Base * Parser::ReductionRuleHandler0049 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * alpha = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 560 "barf_regex_parser.trison"
 return alpha; 
#line 1066 "barf_regex_parser.cpp"
}

// rule 50: atom_normal_char <- DIGIT:digit    
Ast::Base * Parser::ReductionRuleHandler0050 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * digit = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 561 "barf_regex_parser.trison"
 return digit; 
#line 1077 "barf_regex_parser.cpp"
}

// rule 51: atom_normal_char <- CHAR:ch    
Ast::Base * Parser::ReductionRuleHandler0051 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * ch = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 562 "barf_regex_parser.trison"
 return ch; 
#line 1088 "barf_regex_parser.cpp"
}

// rule 52: atom_normal_char <- ','    
Ast::Base * Parser::ReductionRuleHandler0052 ()
{

#line 563 "barf_regex_parser.trison"
 return new Char(','); 
#line 1097 "barf_regex_parser.cpp"
}

// rule 53: atom_normal_char <- '-'    
Ast::Base * Parser::ReductionRuleHandler0053 ()
{

#line 564 "barf_regex_parser.trison"
 return new Char('-'); 
#line 1106 "barf_regex_parser.cpp"
}

// rule 54: atom_normal_char <- ':'    
Ast::Base * Parser::ReductionRuleHandler0054 ()
{

#line 565 "barf_regex_parser.trison"
 return new Char(':'); 
#line 1115 "barf_regex_parser.cpp"
}

// rule 55: bracket_expression_control_char <- '-'    
Ast::Base * Parser::ReductionRuleHandler0055 ()
{

#line 577 "barf_regex_parser.trison"
 return new Char('-'); 
#line 1124 "barf_regex_parser.cpp"
}

// rule 56: bracket_expression_control_char <- '^'    
Ast::Base * Parser::ReductionRuleHandler0056 ()
{

#line 579 "barf_regex_parser.trison"
 return new Char('^'); 
#line 1133 "barf_regex_parser.cpp"
}

// rule 57: bracket_expression_control_char <- '['    
Ast::Base * Parser::ReductionRuleHandler0057 ()
{

#line 581 "barf_regex_parser.trison"
 return new Char('['); 
#line 1142 "barf_regex_parser.cpp"
}

// rule 58: bracket_expression_control_char <- ']'    
Ast::Base * Parser::ReductionRuleHandler0058 ()
{

#line 583 "barf_regex_parser.trison"
 return new Char(']'); 
#line 1151 "barf_regex_parser.cpp"
}

// rule 59: bracket_expression_control_char <- '\\'    
Ast::Base * Parser::ReductionRuleHandler0059 ()
{

#line 585 "barf_regex_parser.trison"
 return new Char('\\'); 
#line 1160 "barf_regex_parser.cpp"
}

// rule 60: bracket_expression_normal_char <- ALPHA:alpha    
Ast::Base * Parser::ReductionRuleHandler0060 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * alpha = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 596 "barf_regex_parser.trison"
 return alpha; 
#line 1171 "barf_regex_parser.cpp"
}

// rule 61: bracket_expression_normal_char <- DIGIT:digit    
Ast::Base * Parser::ReductionRuleHandler0061 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * digit = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 597 "barf_regex_parser.trison"
 return digit; 
#line 1182 "barf_regex_parser.cpp"
}

// rule 62: bracket_expression_normal_char <- CHAR:ch    
Ast::Base * Parser::ReductionRuleHandler0062 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * ch = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 598 "barf_regex_parser.trison"
 return ch; 
#line 1193 "barf_regex_parser.cpp"
}

// rule 63: bracket_expression_normal_char <- '|'    
Ast::Base * Parser::ReductionRuleHandler0063 ()
{

#line 599 "barf_regex_parser.trison"
 return new Char('|'); 
#line 1202 "barf_regex_parser.cpp"
}

// rule 64: bracket_expression_normal_char <- ':'    
Ast::Base * Parser::ReductionRuleHandler0064 ()
{

#line 600 "barf_regex_parser.trison"
 return new Char(':'); 
#line 1211 "barf_regex_parser.cpp"
}

// rule 65: bracket_expression_normal_char <- '?'    
Ast::Base * Parser::ReductionRuleHandler0065 ()
{

#line 601 "barf_regex_parser.trison"
 return new Char('?'); 
#line 1220 "barf_regex_parser.cpp"
}

// rule 66: bracket_expression_normal_char <- '*'    
Ast::Base * Parser::ReductionRuleHandler0066 ()
{

#line 602 "barf_regex_parser.trison"
 return new Char('*'); 
#line 1229 "barf_regex_parser.cpp"
}

// rule 67: bracket_expression_normal_char <- '+'    
Ast::Base * Parser::ReductionRuleHandler0067 ()
{

#line 603 "barf_regex_parser.trison"
 return new Char('+'); 
#line 1238 "barf_regex_parser.cpp"
}

// rule 68: bracket_expression_normal_char <- '.'    
Ast::Base * Parser::ReductionRuleHandler0068 ()
{

#line 604 "barf_regex_parser.trison"
 return new Char('.'); 
#line 1247 "barf_regex_parser.cpp"
}

// rule 69: bracket_expression_normal_char <- '$'    
Ast::Base * Parser::ReductionRuleHandler0069 ()
{

#line 605 "barf_regex_parser.trison"
 return new Char('$'); 
#line 1256 "barf_regex_parser.cpp"
}

// rule 70: bracket_expression_normal_char <- ','    
Ast::Base * Parser::ReductionRuleHandler0070 ()
{

#line 606 "barf_regex_parser.trison"
 return new Char(','); 
#line 1265 "barf_regex_parser.cpp"
}

// rule 71: bracket_expression_normal_char <- '('    
Ast::Base * Parser::ReductionRuleHandler0071 ()
{

#line 607 "barf_regex_parser.trison"
 return new Char('('); 
#line 1274 "barf_regex_parser.cpp"
}

// rule 72: bracket_expression_normal_char <- ')'    
Ast::Base * Parser::ReductionRuleHandler0072 ()
{

#line 608 "barf_regex_parser.trison"
 return new Char(')'); 
#line 1283 "barf_regex_parser.cpp"
}

// rule 73: bracket_expression_normal_char <- '{'    
Ast::Base * Parser::ReductionRuleHandler0073 ()
{

#line 609 "barf_regex_parser.trison"
 return new Char('{'); 
#line 1292 "barf_regex_parser.cpp"
}

// rule 74: bracket_expression_normal_char <- '}'    
Ast::Base * Parser::ReductionRuleHandler0074 ()
{

#line 610 "barf_regex_parser.trison"
 return new Char('}'); 
#line 1301 "barf_regex_parser.cpp"
}

// rule 75: id <- id:id ALPHA:alpha    
Ast::Base * Parser::ReductionRuleHandler0075 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Char * alpha = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 616 "barf_regex_parser.trison"

        assert(id != NULL);
        id->AppendChar(alpha->GetChar());
        delete alpha;
        return id;
    
#line 1319 "barf_regex_parser.cpp"
}

// rule 76: id <- id:id CHAR:ch    
Ast::Base * Parser::ReductionRuleHandler0076 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Char * ch = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 624 "barf_regex_parser.trison"

        assert(id != NULL);
        id->AppendChar(ch->GetChar());
        delete ch;
        return id;
    
#line 1337 "barf_regex_parser.cpp"
}

// rule 77: id <- id:id DIGIT:digit    
Ast::Base * Parser::ReductionRuleHandler0077 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::Id * id = Dsc< Ast::Id * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Char * digit = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 632 "barf_regex_parser.trison"

        assert(id != NULL);
        id->AppendChar(digit->GetChar());
        delete digit;
        return id;
    
#line 1355 "barf_regex_parser.cpp"
}

// rule 78: id <- ALPHA:alpha    
Ast::Base * Parser::ReductionRuleHandler0078 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * alpha = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 640 "barf_regex_parser.trison"

        string temp;
        temp += alpha->GetChar();
        Ast::Id *id = new Ast::Id(temp, FiLoc::ms_invalid);
        delete alpha;
        return id;
    
#line 1372 "barf_regex_parser.cpp"
}

// rule 79: id <- CHAR:ch    
Ast::Base * Parser::ReductionRuleHandler0079 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * ch = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 649 "barf_regex_parser.trison"

        string temp;
        temp += ch->GetChar();
        Ast::Id *id = new Ast::Id(temp, FiLoc::ms_invalid);
        delete ch;
        return id;
    
#line 1389 "barf_regex_parser.cpp"
}

// rule 80: integer <- integer:integer DIGIT:digit    
Ast::Base * Parser::ReductionRuleHandler0080 ()
{
    assert(0 < m_reduction_rule_token_count);
    Ast::SignedInteger * integer = Dsc< Ast::SignedInteger * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);
    assert(1 < m_reduction_rule_token_count);
    Char * digit = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 1]);

#line 661 "barf_regex_parser.trison"

        integer->ShiftAndAdd(digit->GetChar() - '0');
        if (integer->GetValue() > 255)
            integer->SetValue(255);
        delete digit;
        return integer;
    
#line 1408 "barf_regex_parser.cpp"
}

// rule 81: integer <- DIGIT:digit    
Ast::Base * Parser::ReductionRuleHandler0081 ()
{
    assert(0 < m_reduction_rule_token_count);
    Char * digit = Dsc< Char * >(m_token_stack[m_token_stack.size() - m_reduction_rule_token_count + 0]);

#line 670 "barf_regex_parser.trison"

        Ast::SignedInteger *integer = new Ast::SignedInteger(digit->GetChar() - '0', FiLoc::ms_invalid);
        delete digit;
        return integer;
    
#line 1423 "barf_regex_parser.cpp"
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
    {                 Token::atom__,  3, &Parser::ReductionRuleHandler0010, "rule 10: atom <- '{' id '}'    "},
    {                 Token::atom__,  3, &Parser::ReductionRuleHandler0011, "rule 11: atom <- '(' regex ')'    "},
    {                 Token::atom__,  2, &Parser::ReductionRuleHandler0012, "rule 12: atom <- '(' ')'    "},
    {                 Token::atom__,  1, &Parser::ReductionRuleHandler0013, "rule 13: atom <- '^'    "},
    {                 Token::atom__,  1, &Parser::ReductionRuleHandler0014, "rule 14: atom <- '$'    "},
    {                 Token::atom__,  1, &Parser::ReductionRuleHandler0015, "rule 15: atom <- '.'    "},
    {                 Token::atom__,  1, &Parser::ReductionRuleHandler0016, "rule 16: atom <- atom_normal_char    "},
    {                 Token::atom__,  2, &Parser::ReductionRuleHandler0017, "rule 17: atom <- '\\' atom_normal_char    "},
    {                 Token::atom__,  2, &Parser::ReductionRuleHandler0018, "rule 18: atom <- '\\' atom_control_char    "},
    {                 Token::atom__,  1, &Parser::ReductionRuleHandler0019, "rule 19: atom <- bracket_expression    "},
    {                Token::bound__,  1, &Parser::ReductionRuleHandler0020, "rule 20: bound <- '*'    "},
    {                Token::bound__,  1, &Parser::ReductionRuleHandler0021, "rule 21: bound <- '+'    "},
    {                Token::bound__,  1, &Parser::ReductionRuleHandler0022, "rule 22: bound <- '?'    "},
    {                Token::bound__,  3, &Parser::ReductionRuleHandler0023, "rule 23: bound <- '{' integer '}'    "},
    {                Token::bound__,  4, &Parser::ReductionRuleHandler0024, "rule 24: bound <- '{' integer ',' '}'    "},
    {                Token::bound__,  5, &Parser::ReductionRuleHandler0025, "rule 25: bound <- '{' integer ',' integer '}'    "},
    {   Token::bracket_expression__,  3, &Parser::ReductionRuleHandler0026, "rule 26: bracket_expression <- '[' bracket_char_set ']'    "},
    {   Token::bracket_expression__,  4, &Parser::ReductionRuleHandler0027, "rule 27: bracket_expression <- '[' '^' bracket_char_set ']'    "},
    {     Token::bracket_char_set__,  2, &Parser::ReductionRuleHandler0028, "rule 28: bracket_char_set <- bracket_char_set bracket_expression_char    "},
    {     Token::bracket_char_set__,  4, &Parser::ReductionRuleHandler0029, "rule 29: bracket_char_set <- bracket_char_set bracket_expression_char '-' bracket_expression_char    "},
    {     Token::bracket_char_set__,  6, &Parser::ReductionRuleHandler0030, "rule 30: bracket_char_set <- bracket_char_set '[' ':' id ':' ']'    "},
    {     Token::bracket_char_set__,  0, &Parser::ReductionRuleHandler0031, "rule 31: bracket_char_set <-     "},
    {Token::bracket_expression_char__,  1, &Parser::ReductionRuleHandler0032, "rule 32: bracket_expression_char <- bracket_expression_normal_char    "},
    {Token::bracket_expression_char__,  2, &Parser::ReductionRuleHandler0033, "rule 33: bracket_expression_char <- '\\' bracket_expression_normal_char    "},
    {Token::bracket_expression_char__,  2, &Parser::ReductionRuleHandler0034, "rule 34: bracket_expression_char <- '\\' bracket_expression_control_char    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0035, "rule 35: atom_control_char <- '|'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0036, "rule 36: atom_control_char <- '('    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0037, "rule 37: atom_control_char <- ')'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0038, "rule 38: atom_control_char <- '{'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0039, "rule 39: atom_control_char <- '}'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0040, "rule 40: atom_control_char <- '['    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0041, "rule 41: atom_control_char <- ']'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0042, "rule 42: atom_control_char <- '?'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0043, "rule 43: atom_control_char <- '*'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0044, "rule 44: atom_control_char <- '+'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0045, "rule 45: atom_control_char <- '.'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0046, "rule 46: atom_control_char <- '^'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0047, "rule 47: atom_control_char <- '$'    "},
    {    Token::atom_control_char__,  1, &Parser::ReductionRuleHandler0048, "rule 48: atom_control_char <- '\\'    "},
    {     Token::atom_normal_char__,  1, &Parser::ReductionRuleHandler0049, "rule 49: atom_normal_char <- ALPHA    "},
    {     Token::atom_normal_char__,  1, &Parser::ReductionRuleHandler0050, "rule 50: atom_normal_char <- DIGIT    "},
    {     Token::atom_normal_char__,  1, &Parser::ReductionRuleHandler0051, "rule 51: atom_normal_char <- CHAR    "},
    {     Token::atom_normal_char__,  1, &Parser::ReductionRuleHandler0052, "rule 52: atom_normal_char <- ','    "},
    {     Token::atom_normal_char__,  1, &Parser::ReductionRuleHandler0053, "rule 53: atom_normal_char <- '-'    "},
    {     Token::atom_normal_char__,  1, &Parser::ReductionRuleHandler0054, "rule 54: atom_normal_char <- ':'    "},
    {Token::bracket_expression_control_char__,  1, &Parser::ReductionRuleHandler0055, "rule 55: bracket_expression_control_char <- '-'    "},
    {Token::bracket_expression_control_char__,  1, &Parser::ReductionRuleHandler0056, "rule 56: bracket_expression_control_char <- '^'    "},
    {Token::bracket_expression_control_char__,  1, &Parser::ReductionRuleHandler0057, "rule 57: bracket_expression_control_char <- '['    "},
    {Token::bracket_expression_control_char__,  1, &Parser::ReductionRuleHandler0058, "rule 58: bracket_expression_control_char <- ']'    "},
    {Token::bracket_expression_control_char__,  1, &Parser::ReductionRuleHandler0059, "rule 59: bracket_expression_control_char <- '\\'    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0060, "rule 60: bracket_expression_normal_char <- ALPHA    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0061, "rule 61: bracket_expression_normal_char <- DIGIT    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0062, "rule 62: bracket_expression_normal_char <- CHAR    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0063, "rule 63: bracket_expression_normal_char <- '|'    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0064, "rule 64: bracket_expression_normal_char <- ':'    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0065, "rule 65: bracket_expression_normal_char <- '?'    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0066, "rule 66: bracket_expression_normal_char <- '*'    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0067, "rule 67: bracket_expression_normal_char <- '+'    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0068, "rule 68: bracket_expression_normal_char <- '.'    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0069, "rule 69: bracket_expression_normal_char <- '$'    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0070, "rule 70: bracket_expression_normal_char <- ','    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0071, "rule 71: bracket_expression_normal_char <- '('    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0072, "rule 72: bracket_expression_normal_char <- ')'    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0073, "rule 73: bracket_expression_normal_char <- '{'    "},
    {Token::bracket_expression_normal_char__,  1, &Parser::ReductionRuleHandler0074, "rule 74: bracket_expression_normal_char <- '}'    "},
    {                   Token::id__,  2, &Parser::ReductionRuleHandler0075, "rule 75: id <- id ALPHA    "},
    {                   Token::id__,  2, &Parser::ReductionRuleHandler0076, "rule 76: id <- id CHAR    "},
    {                   Token::id__,  2, &Parser::ReductionRuleHandler0077, "rule 77: id <- id DIGIT    "},
    {                   Token::id__,  1, &Parser::ReductionRuleHandler0078, "rule 78: id <- ALPHA    "},
    {                   Token::id__,  1, &Parser::ReductionRuleHandler0079, "rule 79: id <- CHAR    "},
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
    {     Token::atom_normal_char__, {                  TA_PUSH_STATE,   20}},

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
    {     Token::atom_normal_char__, {                  TA_PUSH_STATE,   20}},

// ///////////////////////////////////////////////////////////////////////////
// state    5
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {                  Token::ALPHA, {        TA_SHIFT_AND_PUSH_STATE,   23}},
    {                   Token::CHAR, {        TA_SHIFT_AND_PUSH_STATE,   24}},
    // nonterminal transitions
    {                   Token::id__, {                  TA_PUSH_STATE,   25}},

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
    {    Token::atom_control_char__, {                  TA_PUSH_STATE,   40}},
    {     Token::atom_normal_char__, {                  TA_PUSH_STATE,   41}},

// ///////////////////////////////////////////////////////////////////////////
// state   11
// ///////////////////////////////////////////////////////////////////////////
    // terminal transitions
    {              Token::Type('^'), {        TA_SHIFT_AND_PUSH_STATE,   42}},
    // default transition
    {               Token::DEFAULT_, {           TA_REDUCE_USING_RULE,   31}},
    // nonterminal transitions
    {     Token::bracket_char_set__, {                  TA_PUSH_STATE,   43}},

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
    {     Token::atom_normal_char__, {                  TA_PUSH_STATE,   20}},

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
    {     Token::atom_normal_char__, {                  TA_PUSH_STATE,   20}},

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
    {     Token::bracket_char_set__, {                  TA_PUSH_STATE,   58}},

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
    {Token::bracket_expression_char__, {                  TA_PUSH_STATE,   77}},
    {Token::bracket_expression_normal_char__, {                  TA_PUSH_STATE,   78}},

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
    {     Token::atom_normal_char__, {                  TA_PUSH_STATE,   20}},

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
    {                   Token::id__, {                  TA_PUSH_STATE,   25}},
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
    {Token::bracket_expression_char__, {                  TA_PUSH_STATE,   77}},
    {Token::bracket_expression_normal_char__, {                  TA_PUSH_STATE,   78}},

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
    {Token::bracket_expression_control_char__, {                  TA_PUSH_STATE,   88}},
    {Token::bracket_expression_normal_char__, {                  TA_PUSH_STATE,   89}},

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
    {                   Token::id__, {                  TA_PUSH_STATE,   95}},

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
    {Token::bracket_expression_char__, {                  TA_PUSH_STATE,   96}},
    {Token::bracket_expression_normal_char__, {                  TA_PUSH_STATE,   78}},

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


#line 86 "barf_regex_parser.trison"

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

Parser::ParserReturnCode Parser::Parse (Ast::Base **parsed_tree_root, RegularExpressionMap *macro_map)
{
    assert(m_macro_map == NULL);
    m_macro_map = macro_map;
    try {
        ParserReturnCode retval = Parse(parsed_tree_root);
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
            m_lookahead_token = new Char(c);
            return Token::DIGIT;
        }
        else if (c >= 'A' && c <= 'Z'
                 ||
                 c >= 'a' && c <= 'z')
        {
//             fprintf(stderr, "\n######## scanning CHAR '%c'\n", static_cast<char>(c));
            m_lookahead_token = new Char(c);
            return Token::ALPHA;
        }
        else if (c >= 128 && c < 256)
        {
//             fprintf(stderr, "\n######## scanning extended ascii CHAR \\0x%02X\n", c);
            m_lookahead_token = new Char(c);
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

                        m_lookahead_token = new Char(hex_value, true);
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
//                 fprintf(stderr, "\n######## skipping escaped char\n");
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
                m_lookahead_token = new Char(c);
                return Token::CHAR;

            default:
//                 fprintf(stderr, "\n######## skipping invalid char\n");
                break;
        }
    }
}

} // end of namespace Regex
} // end of namespace Barf

#line 2691 "barf_regex_parser.cpp"

