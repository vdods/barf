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

string const &GetAstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AST_COUNT-CommonLang::AST_START_CUSTOM_TYPES_HERE_] =
    {
        "AST_PRIMARY_SOURCE",
        "AST_RULE",
        "AST_RULE_LIST",
        "AST_SCANNER_MODE",
        "AST_SCANNER_MODE_MAP",
        "AST_START_DIRECTIVE"
    };

    assert(ast_type < AST_COUNT);
    if (ast_type < CommonLang::AST_START_CUSTOM_TYPES_HERE_)
        return CommonLang::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-CommonLang::AST_START_CUSTOM_TYPES_HERE_];
}

void StartInScannerModeDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Directive::Print(stream, Stringify, indent_level);
    m_scanner_mode_id->Print(stream, Stringify, indent_level+1);
}

void ScannerMode::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << ' ' << m_scanner_mode_id->GetText() << endl;
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

Uint32 PrimarySource::GetRuleCount () const
{
    Uint32 accept_handler_count = 0;
    for (ScannerModeMap::const_iterator it = m_scanner_mode_map->begin(),
                                         it_end = m_scanner_mode_map->end();
         it != it_end;
         ++it)
    {
        ScannerMode const *scanner_mode = it->second;
        assert(scanner_mode != NULL);
        accept_handler_count += scanner_mode->GetRuleCount();
    }
    return accept_handler_count;
}

Rule const *PrimarySource::GetRule (Uint32 rule_index) const
{
    assert(rule_index < GetRuleCount());
    for (ScannerModeMap::const_iterator it = m_scanner_mode_map->begin(),
                                         it_end = m_scanner_mode_map->end();
         it != it_end;
         ++it)
    {
        ScannerMode const *scanner_mode = it->second;
        assert(scanner_mode != NULL);
        if (rule_index < scanner_mode->GetRuleCount())
            return scanner_mode->m_rule_list->GetElement(rule_index);
        else
            rule_index -= scanner_mode->GetRuleCount();
    }
    assert(false && "GetRuleCount() doesn't match reality");
    return NULL;
}

void PrimarySource::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, GetAstTypeString, indent_level);
}

void PrimarySource::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    m_target_map->Print(stream, Stringify, indent_level+1);
    m_regex_macro_map->Print(stream, indent_level+1);
    if (m_start_in_scanner_mode_directive != NULL)
        m_start_in_scanner_mode_directive->Print(stream, Stringify, indent_level+1);
    m_scanner_mode_map->Print(stream, Stringify, indent_level+1);
}

} // end of namespace Reflex
