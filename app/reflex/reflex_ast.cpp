// ///////////////////////////////////////////////////////////////////////////
// reflex_ast.cpp by Victor Dods, created 2006/10/18
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "reflex_ast.hpp"

namespace Reflex {

string const &AstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AST_COUNT-CommonLang::AST_START_CUSTOM_TYPES_HERE_] =
    {
        "AST_PRIMARY_SOURCE",
        "AST_RULE",
        "AST_RULE_LIST",
        "AST_START_DIRECTIVE",
        "AST_STATE_MACHINE",
        "AST_STATE_MACHINE_MAP"
    };

    assert(ast_type < AST_COUNT);
    if (ast_type < CommonLang::AST_START_CUSTOM_TYPES_HERE_)
        return CommonLang::AstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-CommonLang::AST_START_CUSTOM_TYPES_HERE_];
}

void StartWithStateMachineDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Directive::Print(stream, Stringify, indent_level);
    m_state_machine_id->Print(stream, Stringify, indent_level+1);
}

void StateMachine::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << m_state_machine_id->GetText();
    if ((m_mode_flags & MF_CASE_INSENSITIVE) != 0)
        stream << " %case_insensitive";
    if ((m_mode_flags & MF_UNGREEDY) != 0)
        stream << " %ungreedy";
    stream << endl;
    for (RuleList::const_iterator it = m_rule_list->begin(),
                                 it_end = m_rule_list->end();
         it != it_end;
         ++it)
    {
        Rule const *rule = *it;
        assert(rule != NULL);
        rule->Print(stream, Stringify, indent_level+1);
    }
}

void Rule::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << m_rule_regex_string << endl;
    m_rule_regex->TopLevelPrint(stream, indent_level+1);
    m_rule_handler_map->Print(stream, Stringify, indent_level+1);
}

Uint32 PrimarySource::RuleCount () const
{
    Uint32 accept_handler_count = 0;
    for (StateMachineMap::const_iterator it = m_state_machine_map->begin(),
                                         it_end = m_state_machine_map->end();
         it != it_end;
         ++it)
    {
        StateMachine const *state_machine = it->second;
        assert(state_machine != NULL);
        accept_handler_count += state_machine->RuleCount();
    }
    return accept_handler_count;
}

Rule const *PrimarySource::GetRule (Uint32 rule_index) const
{
    assert(rule_index < RuleCount());
    for (StateMachineMap::const_iterator it = m_state_machine_map->begin(),
                                         it_end = m_state_machine_map->end();
         it != it_end;
         ++it)
    {
        StateMachine const *state_machine = it->second;
        assert(state_machine != NULL);
        if (rule_index < state_machine->RuleCount())
            return state_machine->m_rule_list->Element(rule_index);
        else
            rule_index -= state_machine->RuleCount();
    }
    assert(false && "RuleCount() doesn't match reality");
    return NULL;
}

void PrimarySource::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, AstTypeString, indent_level);
}

void PrimarySource::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    assert(m_target_map != NULL && "no target map set");
    
    Ast::Base::Print(stream, Stringify, indent_level);
    m_target_map->Print(stream, Stringify, indent_level+1);
    m_regex_macro_map->TopLevelPrint(stream, indent_level+1);
    if (m_start_with_state_machine_directive != NULL)
        m_start_with_state_machine_directive->Print(stream, Stringify, indent_level+1);
    m_state_machine_map->Print(stream, Stringify, indent_level+1);
}

} // end of namespace Reflex
