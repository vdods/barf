// ///////////////////////////////////////////////////////////////////////////
// trison_state.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_state.hpp"

#include <iomanip>

#include "trison_ast.hpp"
#include "trison_statemachine.hpp"

namespace Trison {

void State::PrettyPrint (ostream &stream, StateMachine const *state_machine) const
{
    assert(state_machine != NULL);

    // print the header
    stream << "state " << m_state_index << endl;

    // print the rules that could be matched in this state
    assert(!m_state_id.empty());
    for (StateId::const_iterator it = m_state_id.begin(),
                                      it_end = m_state_id.end();
         it != it_end;
         ++it)
    {
        RulePhase const &rule_phase = *it;
        Rule const *rule = state_machine->GetRule(rule_phase.m_rule_index);
        stream << Tabs(1);
        rule->PrettyPrint(stream, rule_phase);
        stream << endl;
    }

    if (!m_spawned_rule_phase_set.empty())
    {
        stream << Tabs(1) << "+\n";
        for (set<RulePhase>::const_iterator
                 it = m_spawned_rule_phase_set.begin(),
                 it_end = m_spawned_rule_phase_set.end();
             it != it_end;
             ++it)
        {
            RulePhase const &rule_phase = *it;
            Rule const *rule = state_machine->GetRule(rule_phase.m_rule_index);
            stream << Tabs(1);
            rule->PrettyPrint(stream, rule_phase);
            stream << endl;
        }
    }

    // print the terminal actions
    if (!m_terminal_transition_map.empty())
    {
        stream << endl;
        for (TerminalTransitionMap::const_iterator
                 it = m_terminal_transition_map.begin(),
                 it_end = m_terminal_transition_map.end();
             it != it_end;
             ++it)
        {
            Terminal const *terminal = it->first;
            Transition const &transition = it->second;
            assert(terminal != NULL);
            assert(transition.GetIsNontrivial());
            stream << Tabs(1);
            transition.PrettyPrint(stream, terminal->GetTokenId()->GetText(), state_machine);
            stream << endl;
        }
    }

    // print the default action
    if (!m_default_transition.GetTransitionId().empty())
    {
        stream << endl << Tabs(1);
        m_default_transition.PrettyPrint(stream, "default", state_machine);
        stream << endl;
    }

    // print the nonterminal actions
    if (!m_nonterminal_transition_map.empty())
    {
        stream << endl;
        for (NonterminalTransitionMap::const_iterator it = m_nonterminal_transition_map.begin(),
                                                   it_end = m_nonterminal_transition_map.end();
             it != it_end;
             ++it)
        {
            Nonterminal const *nonterminal = it->first;
            Transition const &transition = it->second;
            assert(nonterminal != NULL);
            assert(!transition.GetTransitionId().empty());
            stream << Tabs(1);
            transition.PrettyPrint(stream, nonterminal->GetId()->GetText(), state_machine);
            stream << endl;
        }
    }

    // print the conflicted terminal transitions
    if (!m_conflicted_terminal_transition_map.empty())
    {
        stream << endl;
        for (ConflictedTerminalTransitionMap::const_iterator
                 it = m_conflicted_terminal_transition_map.begin(),
                 it_end = m_conflicted_terminal_transition_map.end();
             it != it_end;
             ++it)
        {
            Terminal const *terminal = it->first;
            Transition const &transition = it->second;
            assert(transition.GetIsNontrivial());
            // if terminal is not NULL, then it's a terminal transition.
            if (terminal != NULL)
            {
                stream << Tabs(1) << "[[ ";
                transition.PrettyPrint(stream, terminal->GetTokenId()->GetText(), state_machine);
                stream << " ]]" << endl;
            }
            // if terminal is NULL, then it's a default transition.
            else
            {
                stream << Tabs(1) << "[[ ";
                transition.PrettyPrint(stream, "default", state_machine);
                stream << " ]]" << endl;
            }
        }
    }
}

void State::PrintIndices (ostream &stream) const
{
    stream << "{"
           << setw(4) << m_terminal_transitions_offset << ", "
           << setw(4) << m_terminal_transition_map.size() << ", "
           << setw(4) << m_default_transition_offset << ", "
           << setw(4) << m_nonterminal_transitions_offset << ", "
           << setw(4) << m_nonterminal_transition_map.size()
           << "}";
}

void State::PrintTransitions (ostream &stream, bool is_last_state, StateMachine const *state_machine) const
{
    assert(state_machine != NULL);

    Uint32 total_transition_count = 0;
    total_transition_count += m_terminal_transition_map.size();
    if (!m_default_transition.GetTransitionId().empty())
        ++total_transition_count;
    total_transition_count += m_nonterminal_transition_map.size();

    Uint32 transition_count = 0;

    if (!m_terminal_transition_map.empty())
        stream << "    // terminal transitions\n";
    for (TerminalTransitionMap::const_iterator it = m_terminal_transition_map.begin(),
                                            it_end = m_terminal_transition_map.end();
         it != it_end;
         ++it)
    {
        Terminal const *terminal = it->first;
        assert(terminal != NULL);
        Transition const &transition = it->second;
        assert(transition.GetIsNontrivial());

        TokenId const *token_id = terminal->GetTokenId();
        assert(token_id != NULL);
        if (token_id->GetAstType() == AT_TOKEN_ID_ID)
        {
            stream << "    {" << setw(30) << ("Token::" + terminal->GetImplementationFileId()) << ", ";
            transition.PrintTransitionArrayElement(stream, state_machine);
            stream << '}';
        }
        else
        {
            TokenIdCharacter const *token_id_character =
                Dsc<TokenIdCharacter const *>(token_id);
            string token_string("Token::Type(");
            token_string += GetCharacterLiteral(token_id_character->GetCharacter());
            token_string += ')';
            stream << "    {" << setw(30) << token_string << ", ";
            transition.PrintTransitionArrayElement(stream, state_machine);
            stream << '}';
        }

        if (transition_count++ < total_transition_count - 1 || !is_last_state)
            stream << ",\n";
        else
            stream << '\n';
    }

    if (!m_default_transition.GetTransitionId().empty())
    {
        stream << "    // default transition\n"
               << "    {               Token::DEFAULT_, ";
        m_default_transition.PrintTransitionArrayElement(stream, state_machine);
        if (transition_count++ < total_transition_count - 1 || !is_last_state)
            stream << "},\n";
        else
            stream << "}\n";
    }

    if (!m_nonterminal_transition_map.empty())
        stream << "    // nonterminal transitions\n";
    for (NonterminalTransitionMap::const_iterator it = m_nonterminal_transition_map.begin(),
                                            it_end = m_nonterminal_transition_map.end();
         it != it_end;
         ++it)
    {
        Nonterminal const *nonterminal = it->first;
        assert(nonterminal != NULL);
        Transition const &transition = it->second;
        assert(!transition.GetTransitionId().empty());

        stream << "    {" << setw(30)
               << ("Token::" + nonterminal->GetImplementationFileId()) << ", ";
        transition.PrintTransitionArrayElement(stream, state_machine);
        stream << "}";

        if (transition_count++ < total_transition_count - 1 || !is_last_state)
            stream << ",\n";
        else
            stream << "\n";
    }

    assert(transition_count == total_transition_count);
}

} // end of namespace Trison
