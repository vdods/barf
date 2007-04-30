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

#include "barf_regex_ast.hpp"

namespace Reflex {

string const &GetAstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AT_COUNT-CommonLang::AT_START_CUSTOM_TYPES_HERE_] =
    {
        "AT_REPRESENTATION",
        "AT_RULE",
        "AT_RULE_LIST",
        "AT_SCANNER_STATE",
        "AT_SCANNER_STATE_MAP",
        "AT_START_DIRECTIVE"
    };

    assert(ast_type < AT_COUNT);
    if (ast_type < CommonLang::AT_START_CUSTOM_TYPES_HERE_)
        return CommonLang::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-CommonLang::AT_START_CUSTOM_TYPES_HERE_];
}

void StartDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Directive::Print(stream, Stringify, indent_level);
    m_start_state_id->Print(stream, Stringify, indent_level+1);
}

void ScannerState::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << m_scanner_state_id->GetText() << endl;
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
    m_rule_regex->Print(stream, indent_level+1);
    m_rule_handler_map->Print(stream, Stringify, indent_level+1);
}

void Representation::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, GetAstTypeString, indent_level);
}

void Representation::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    m_target_language_map->Print(stream, Stringify, indent_level+1);
    m_regex_macro_map->Print(stream, indent_level+1);
    if (m_start_directive != NULL)
        m_start_directive->Print(stream, Stringify, indent_level+1);
    m_scanner_state_map->Print(stream, Stringify, indent_level+1);
}

} // end of namespace Reflex
