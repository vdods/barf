// 2006.10.18 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(REFLEX_AST_HPP_)
#define REFLEX_AST_HPP_

#include "reflex.hpp"

#include <map>
#include <vector>

#include "barf_commonlang_ast.hpp"
#include "barf_filoc.hpp"
#include "barf_graph.hpp"
#include "barf_regex_ast.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace Preprocessor {

class ArraySymbol;
class SymbolTable;

} // end of namespace Preprocessor

namespace Regex {

struct RegularExpression;

} // end of namespace Regex
} // end of namespace Barf

namespace Reflex {

enum
{
    AST_PRIMARY_SOURCE = CommonLang::AST_START_CUSTOM_TYPES_HERE_,
    AST_MACRO,
    AST_RULE,
    AST_RULE_LIST,
    AST_START_DIRECTIVE,
    AST_STATE_MACHINE,
    AST_STATE_MACHINE_MAP,

    AST_COUNT
};

string const &AstTypeString (AstType ast_type);

struct Macro : public Ast::Base
{
    Ast::Id const *m_macro_id;
    Regex::RegularExpression *m_macro_regex;

    Macro (Ast::Id const *macro_id, Regex::RegularExpression *macro_regex)
        :
        Ast::Base(macro_id->GetFiLoc(), AST_MACRO),
        m_macro_id(macro_id),
        m_macro_regex(macro_regex)
    {
        assert(m_macro_id != NULL);
        assert(m_macro_regex != NULL);
    }
    virtual ~Macro ()
    {
        delete m_macro_id;
        delete m_macro_regex;
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct Macro

struct StartWithStateMachineDirective : public Ast::Directive
{
    Ast::Id const *const m_state_machine_id;

    StartWithStateMachineDirective (Ast::Id const *state_machine_id)
        :
        Ast::Directive("%start_with_state_machine", state_machine_id->GetFiLoc(), AST_START_DIRECTIVE),
        m_state_machine_id(state_machine_id)
    {
        assert(m_state_machine_id != NULL);
    }
    virtual ~StartWithStateMachineDirective ()
    {
        delete m_state_machine_id;
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class StartWithStateMachineDirective

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

struct StateMachine : public Ast::Base
{
    typedef Uint8 ModeFlags;

    enum
    {
        MF_CASE_INSENSITIVE = 0x01,
        MF_UNGREEDY         = 0x02,

        MF_NONE             = 0x00,
        MF_ALL              = 0x03
    };

    Ast::Id const *const m_state_machine_id;
    RuleList const *const m_rule_list;
    ModeFlags const m_mode_flags;

    StateMachine (
        Ast::Id const *state_machine_id,
        RuleList *rule_list,
        ModeFlags mode_flags)
        :
        Ast::Base(state_machine_id->GetFiLoc(), AST_STATE_MACHINE),
        m_state_machine_id(state_machine_id),
        m_rule_list(rule_list),
        m_mode_flags(mode_flags)
    {
        assert(m_state_machine_id != NULL);
        assert(m_rule_list != NULL);
        assert((m_mode_flags & ~MF_ALL) == 0);
    }
    virtual ~StateMachine ()
    {
        delete m_state_machine_id;
        delete m_rule_list;
    }

    Uint32 RuleCount () const { return m_rule_list->size(); }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class StateMachine

struct StateMachineMap : public Ast::AstMap<StateMachine>
{
    StateMachineMap () : Ast::AstMap<StateMachine>(AST_STATE_MACHINE_MAP) { }
};

struct PrimarySource : public Ast::Base
{
public:

    Regex::RegularExpressionMap *const m_regex_macro_map; // this could technically go away
    StartWithStateMachineDirective const *const m_start_with_state_machine_directive;
    StateMachineMap const *const m_state_machine_map;

    PrimarySource (
        Regex::RegularExpressionMap *regex_macro_map,
        StartWithStateMachineDirective const *start_with_state_machine_directive,
        FiLoc const &end_preamble_filoc,
        StateMachineMap const *state_machine_map)
        :
        Ast::Base(FiLoc::ms_invalid, AST_PRIMARY_SOURCE),
        m_regex_macro_map(regex_macro_map),
        m_start_with_state_machine_directive(start_with_state_machine_directive),
        m_state_machine_map(state_machine_map),
        m_target_map(NULL)
    {
        assert(m_regex_macro_map != NULL);
        // m_start_with_state_machine_directive can be NULL if an error happened
        assert(m_state_machine_map != NULL);
    }

    Uint32 RuleCount () const;
    Rule const *GetRule (Uint32 rule_index) const;
    CommonLang::TargetMap const &GetTargetMap () const
    {
        assert(m_target_map != NULL && "no target map set");
        return *m_target_map;
    }

    void SetTargetMap (CommonLang::TargetMap const *target_map)
    {
        assert(m_target_map == NULL && "target map already set");
        assert(target_map != NULL);
        m_target_map = target_map;
    }

    // this is the non-virtual, top-level Print method, not
    // to be confused with Ast::Base::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    CommonLang::TargetMap const *m_target_map;
}; // end of struct PrimarySource

} // end of namespace Reflex

#endif // !defined(REFLEX_AST_HPP_)
