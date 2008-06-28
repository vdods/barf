// ///////////////////////////////////////////////////////////////////////////
// reflex_ast.hpp by Victor Dods, created 2006/10/18
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_REFLEX_AST_HPP_)
#define _REFLEX_AST_HPP_

#include "reflex.hpp"

#include <map>
#include <vector>

#include "barf_commonlang_ast.hpp"
#include "barf_filoc.hpp"
#include "barf_graph.hpp"
#include "barf_regex_ast.hpp"
#include "barf_util.hpp"
#include "reflex_enums.hpp"

namespace Barf {
namespace Preprocessor {

class ArraySymbol;
class SymbolTable;

} // end of namespace Preprocessor

namespace Regex {

class RegularExpression;

} // end of namespace Regex
} // end of namespace Barf

namespace Reflex {

enum
{
    AST_PRIMARY_SOURCE = CommonLang::AST_START_CUSTOM_TYPES_HERE_,
    AST_RULE,
    AST_RULE_LIST,
    AST_SCANNER_MODE,
    AST_SCANNER_MODE_MAP,
    AST_START_DIRECTIVE,

    AST_COUNT
};

string const &GetAstTypeString (AstType ast_type);

struct StartInScannerModeDirective : public Ast::Directive
{
    Ast::Id const *const m_scanner_mode_id;

    StartInScannerModeDirective (Ast::Id const *scanner_mode_id)
        :
        Ast::Directive("%start_in_scanner_mode", scanner_mode_id->GetFiLoc(), AST_START_DIRECTIVE),
        m_scanner_mode_id(scanner_mode_id)
    {
        assert(m_scanner_mode_id != NULL);
    }
    virtual ~StartInScannerModeDirective ()
    {
        delete m_scanner_mode_id;
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class StartInScannerModeDirective

struct Rule : public Ast::Base
{
    string const m_rule_regex_string;
    Regex::RegularExpression const *const m_rule_regex;
    CommonLang::RuleHandlerMap const *const m_rule_handler_map;

    Rule (
        string const &rule_regex_string,
        Regex::RegularExpression const *rule_regex,
        CommonLang::RuleHandlerMap const *rule_handler_map)
        :
        Ast::Base(rule_regex->GetFiLoc(), AST_RULE),
        m_rule_regex_string(rule_regex_string),
        m_rule_regex(rule_regex),
        m_rule_handler_map(rule_handler_map)
    {
        assert(m_rule_regex != NULL);
        assert(m_rule_handler_map != NULL);
    }
    virtual ~Rule ()
    {
        delete m_rule_regex;
        delete m_rule_handler_map;
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class Rule

struct RuleList : public Ast::AstList<Rule>
{
    RuleList () : Ast::AstList<Rule>(AST_RULE_LIST) { }
};

struct ScannerMode : public Ast::Base
{
    Ast::Id const *const m_scanner_mode_id;
    RuleList const *const m_rule_list;

    ScannerMode (
        Ast::Id const *scanner_mode_id,
        RuleList *rule_list)
        :
        Ast::Base(scanner_mode_id->GetFiLoc(), AST_SCANNER_MODE),
        m_scanner_mode_id(scanner_mode_id),
        m_rule_list(rule_list)
    {
        assert(m_scanner_mode_id != NULL);
        assert(m_rule_list != NULL);
    }
    virtual ~ScannerMode ()
    {
        delete m_scanner_mode_id;
        delete m_rule_list;
    }

    Uint32 GetRuleCount () const { return m_rule_list->size(); }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class ScannerMode

struct ScannerModeMap : public Ast::AstMap<ScannerMode>
{
    ScannerModeMap () : Ast::AstMap<ScannerMode>(AST_SCANNER_MODE_MAP) { }
};

struct PrimarySource : public Ast::Base
{
public:

    CommonLang::TargetMap const *const m_target_map;
    Regex::RegularExpressionMap *const m_regex_macro_map; // this could technically go away
    StartInScannerModeDirective const *const m_start_in_scanner_mode_directive;
    ScannerModeMap const *const m_scanner_mode_map;

    PrimarySource (
        CommonLang::TargetMap const *target_map,
        Regex::RegularExpressionMap *regex_macro_map,
        StartInScannerModeDirective const *start_in_scanner_mode_directive,
        FiLoc const &end_preamble_filoc,
        ScannerModeMap const *scanner_mode_map)
        :
        Ast::Base(target_map->GetFiLoc(), AST_PRIMARY_SOURCE),
        m_target_map(target_map),
        m_regex_macro_map(regex_macro_map),
        m_start_in_scanner_mode_directive(start_in_scanner_mode_directive),
        m_scanner_mode_map(scanner_mode_map)
    {
        assert(m_target_map != NULL);
        assert(m_regex_macro_map != NULL);
        // m_start_in_scanner_mode_directive can be NULL if an error happened
        assert(m_scanner_mode_map != NULL);
    }

    Uint32 GetRuleCount () const;
    Rule const *GetRule (Uint32 rule_index) const;

    // this is the non-virtual, top-level Print method, not
    // to be confused with Ast::Base::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct PrimarySource

} // end of namespace Reflex

#endif // !defined(_REFLEX_AST_HPP_)
