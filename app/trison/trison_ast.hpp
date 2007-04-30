// ///////////////////////////////////////////////////////////////////////////
// trison_ast.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_AST_HPP_)
#define _TRISON_AST_HPP_

#include "trison.hpp"

#include <vector>

#include "barf_astcommon.hpp"
#include "barf_filoc.hpp"
#include "barf_util.hpp"
#include "trison_enums.hpp"
#include "trison_rulephase.hpp"

namespace Trison {

class Nonterminal;
class ParserDirective;
class ParserDirectiveSet;
class PrecedenceDirective;
class Rule;
class RuleToken;
class StartDirective;
class TokenDirective;
class TokenIdentifier;
class TokenIdentifierCharacter;
class TokenIdentifierIdentifier;

/*

// ///////////////////////////////////////////////////////////////////////////
// class hierarchy
// ///////////////////////////////////////////////////////////////////////////

AstCommon::Ast (abstract)
    Grammar
    DirectiveList (actually AstList<Directive>)
    ParserDirectiveSet
    TokenDirectiveList (actually AstList<TokenDirective>)
    TokenIdentifierList (actually AstList<TokenIdentifier>)
    PrecedenceDirectiveList (actually AstList<PrecedenceDirective>)
    Directive (abstract)
        ParserDirective
        TokenDirective
        PrecedenceDirective
        StartDirective
    NonterminalList (actually AstList<Nonterminal>)
    Nonterminal
    RuleList (actually AstList<Rule>)
    Rule
    RuleTokenList (actually AstList<RuleToken>)
    RuleToken
    AstCommon::TextBase (abstract)
        TokenIdentifier (abstract)
            TokenIdentifierIdentifier
            TokenIdentifierCharacter

// ///////////////////////////////////////////////////////////////////////////
// class composition
// ///////////////////////////////////////////////////////////////////////////

Grammar
    ParserDirectiveSet // options to customize the generated parser class
        ParserDirective[]
            ParserDirectiveType
            AstCommon::TextBase // the directive's value (if any)
    TokenDirectiveList // list of token declarations
        TokenDirective[]
            TokenIdentifierList
                TokenIdentifier[]
                    std::string // stores the identifier or the character
    PrecedenceDirectiveList // list of allowable rule precedences
        PrecedenceDirective[]
    StartDirective // stores the start nonterminal identifier
    NonterminalList // the list of other nonterminals (may be empty)
        Nonterminal[]
            AstCommon::Identifier // the name of the nonterminal
            AstCommon::String // the optional assigned variable type
            RuleList // list of rules that the nonterminal can reduce with
                Rule[]
                    TokenIdentifier // optional precedence token identifier
                    AstCommon::CodeBlock // optional code for when the rule is reduced
                    RuleTokenList // stores the rule tokens necessary to match
                        RuleToken[] // pairs a token/nonterminal identifier with a variable identifier
                            TokenIdentifier // the rule token identifier
                            AstCommon::Identifier // optional identifier to assign to the rule token

*/

enum
{
    AT_GRAMMAR = AstCommon::AT_START_CUSTOM_TYPES_HERE_,
    AT_DIRECTIVE_LIST,
    AT_PARSER_DIRECTIVE_SET,
    AT_TOKEN_DIRECTIVE,
    AT_PRECEDENCE_DIRECTIVE,
    AT_PARSER_DIRECTIVE,
    AT_START_DIRECTIVE,
    AT_TOKEN_DIRECTIVE_LIST,
    AT_TOKEN_IDENTIFIER_LIST,
    AT_PRECEDENCE_DIRECTIVE_LIST,
    AT_NONTERMINAL_LIST,
    AT_NONTERMINAL,
    AT_RULE_LIST,
    AT_RULE,
    AT_RULE_TOKEN_LIST,
    AT_RULE_TOKEN,
    AT_TOKEN_IDENTIFIER_IDENTIFIER,
    AT_TOKEN_IDENTIFIER_CHARACTER,
    AT_IDENTIFIER,
    AT_CODE_BLOCK,
    AT_STRING,
    AT_THROW_AWAY,

    AT_COUNT
};

string const &GetAstTypeString (AstType ast_type);

class TokenDirectiveList : public AstCommon::AstList<TokenDirective>
{
public:

    TokenDirectiveList () : AstCommon::AstList<TokenDirective>(AT_TOKEN_DIRECTIVE_LIST) { }
}; // end of class TokenDirectiveList

class TokenIdentifierList : public AstCommon::AstList<TokenIdentifier>
{
public:

    TokenIdentifierList () : AstCommon::AstList<TokenIdentifier>(AT_TOKEN_IDENTIFIER_LIST) { }
}; // end of class TokenIdentifierList

class PrecedenceDirectiveList : public AstCommon::AstList<PrecedenceDirective>
{
public:

    PrecedenceDirectiveList () : AstCommon::AstList<PrecedenceDirective>(AT_PRECEDENCE_DIRECTIVE_LIST) { }
}; // end of class PrecedenceDirectiveList

class NonterminalList : public AstCommon::AstList<Nonterminal>
{
public:

    NonterminalList () : AstCommon::AstList<Nonterminal>(AT_NONTERMINAL_LIST) { }
}; // end of class NonterminalList

class RuleList : public AstCommon::AstList<Rule>
{
public:

    RuleList () : AstCommon::AstList<Rule>(AT_RULE_LIST) { }
}; // end of class RuleList

class RuleTokenList : public AstCommon::AstList<RuleToken>
{
public:

    RuleTokenList () : AstCommon::AstList<RuleToken>(AT_RULE_TOKEN_LIST) { }
    RuleTokenList (FiLoc const &filoc) : AstCommon::AstList<RuleToken>(filoc, AT_RULE_TOKEN_LIST) { }
}; // end of class RuleTokenList

class TokenIdentifier : public AstCommon::TextBase
{
public:

    TokenIdentifier (
        FiLoc const &filoc,
        AstType ast_type)
        :
        AstCommon::TextBase(filoc, ast_type)
    { }
    TokenIdentifier (
        string const &identifier_text,
        FiLoc const &filoc,
        AstType ast_type)
        :
        AstCommon::TextBase(identifier_text, filoc, ast_type)
    { }
    virtual ~TokenIdentifier () = 0;
}; // end of class TokenIdentifier

class TokenIdentifierIdentifier : public TokenIdentifier
{
public:

    TokenIdentifierIdentifier (
        string const &identifier_text,
        FiLoc const &filoc)
        :
        TokenIdentifier(
            identifier_text,
            filoc,
            AT_TOKEN_IDENTIFIER_IDENTIFIER)
    { }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class TokenIdentifierIdentifier

class TokenIdentifierCharacter : public TokenIdentifier
{
public:

    TokenIdentifierCharacter (
        char identifier_character,
        FiLoc const &filoc)
        :
        TokenIdentifier(
            GetCharacterLiteral(identifier_character),
            filoc,
            AT_TOKEN_IDENTIFIER_CHARACTER),
        m_identifier_character(identifier_character)
    { }

    inline char GetCharacter () const { return m_identifier_character; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    char const m_identifier_character;
}; // end of class TokenIdentifierCharacter

class RuleToken : public AstCommon::Ast
{
public:

    RuleToken (
        TokenIdentifier const *token_identifier,
        AstCommon::Identifier const *assigned_identifier)
        :
        AstCommon::Ast(token_identifier->GetFiLoc(), AT_RULE_TOKEN),
        m_token_identifier(token_identifier),
        m_assigned_identifier(assigned_identifier)
    {
        assert(m_token_identifier != NULL);
        // m_assigned_identifier may be NULL
    }
    virtual ~RuleToken ()
    {
        delete m_token_identifier;
        delete m_assigned_identifier;
    }

    inline TokenIdentifier const *GetTokenIdentifier () const { return m_token_identifier; }
    inline AstCommon::Identifier const *GetAssignedIdentifier () const { return m_assigned_identifier; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
    void PrettyPrint (ostream &stream) const;
    void PrettyPrintWithAssignedIdentifier (ostream &stream) const;

private:

    TokenIdentifier const *const m_token_identifier;
    AstCommon::Identifier const *const m_assigned_identifier;
}; // end of class RuleToken

class Rule : public AstCommon::Ast
{
public:

    Rule (
        RuleTokenList const *rule_token_list,
        Associativity associativity,
        AstCommon::Identifier const *precedence_directive_identifier)
        :
        AstCommon::Ast(rule_token_list->GetFiLoc(), AT_RULE),
        m_rule_token_list(rule_token_list),
        m_associativity(associativity),
        m_precedence_directive_identifier(precedence_directive_identifier)
    {
        assert(m_rule_token_list != NULL);
        assert(m_associativity < A_COUNT);
        // m_precedence_directive_identifier may be NULL
        m_code_block = NULL;
        m_owner_nonterminal = NULL;
    }
    virtual ~Rule ()
    {
        delete m_rule_token_list;
        delete m_precedence_directive_identifier;
        delete m_code_block;
    }

    inline Associativity GetAssociativity () const { return m_associativity; }
    inline AstCommon::Identifier const *GetPrecedenceDirectiveIdentifier () const { return m_precedence_directive_identifier; }
    inline AstCommon::CodeBlock const *GetCodeBlock () const { return m_code_block; }
    inline Nonterminal *GetOwnerNonterminal () const
    {
        assert(m_owner_nonterminal != NULL);
        return m_owner_nonterminal;
    }
    inline Uint32 GetIndex () const { return m_index; }
    inline Uint32 GetRuleTokenCount () const { return m_rule_token_list->size(); }
    inline RuleToken const *GetRuleToken (Uint32 index) const
    {
        assert(index < m_rule_token_list->size());
        return m_rule_token_list->GetElement(index);
    }
    inline RuleToken const *GetRuleTokenToLeft (Uint32 phase) const
    {
        assert(phase > 0);
        return GetRuleToken(phase - 1);
    }
    inline RuleToken const *GetRuleTokenToRight (Uint32 phase) const
    {
        return GetRuleToken(phase);
    }
    bool GetIsABinaryOperation () const;

    inline void SetCodeBlock (AstCommon::CodeBlock const *code_block) { m_code_block = code_block; }
    inline void SetOwnerNonterminal (Nonterminal *owner_nonterminal)
    {
        assert(owner_nonterminal != NULL);
        assert(m_owner_nonterminal == NULL);
        m_owner_nonterminal = owner_nonterminal;
    }
    inline void SetIndex (Uint32 index) { m_index = index; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
    void PrettyPrint (ostream &stream) const;
    void PrettyPrint (ostream &stream, RulePhase const &rule_phase) const;
    void PrettyPrintWithAssignedIdentifiers (ostream &stream) const;

private:

    RuleTokenList const *const m_rule_token_list;
    Associativity const m_associativity;
    AstCommon::Identifier const *const m_precedence_directive_identifier;
    AstCommon::CodeBlock const *m_code_block;
    Nonterminal *m_owner_nonterminal;
    Uint32 m_index;
}; // end of class Rule

class Nonterminal : public AstCommon::Ast
{
public:

    Nonterminal (
        AstCommon::Identifier const *identifier,
        AstCommon::String const *assigned_variable_type)
        :
        AstCommon::Ast(identifier->GetFiLoc(), AT_NONTERMINAL),
        m_identifier(identifier),
        m_assigned_variable_type(assigned_variable_type)
    {
        assert(m_identifier != NULL);
        // m_assigned_variable_type may be NULL
        m_rule_list = NULL;
    }
    virtual ~Nonterminal ()
    {
        delete m_identifier;
        delete m_assigned_variable_type;
        delete m_rule_list;
    }

    string GetImplementationFileIdentifier () const;
    inline AstCommon::Identifier const *GetIdentifier () const { return m_identifier; }
    inline AstCommon::String const *GetAssignedVariableType () const { return m_assigned_variable_type; }
    inline RuleList::const_iterator GetRuleListBegin () const { return m_rule_list->begin(); }
    inline RuleList::const_iterator GetRuleListEnd () const { return m_rule_list->end(); }

    inline void SetRuleList (RuleList const *rule_list)
    {
        assert(rule_list != NULL);
        m_rule_list = rule_list;
        for (RuleList::const_iterator it = m_rule_list->begin(),
                                     it_end = m_rule_list->end();
             it != it_end;
             ++it)
        {
            Rule *rule = *it;
            assert(rule != NULL);
            rule->SetOwnerNonterminal(this);
        }
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    AstCommon::Identifier const *const m_identifier;
    AstCommon::String const *const m_assigned_variable_type;
    RuleList const *m_rule_list;
}; // end of class Nonterminal

class PrecedenceDirective : public AstCommon::Directive
{
public:

    PrecedenceDirective (AstCommon::Identifier const *identifier)
        :
        AstCommon::Directive("%prec", identifier->GetFiLoc(), AT_PRECEDENCE_DIRECTIVE),
        m_identifier(identifier),
        m_precedence_level(0)
    {
        assert(m_identifier != NULL);
    }
    virtual ~PrecedenceDirective ()
    {
        delete m_identifier;
    }

    inline AstCommon::Identifier const *GetIdentifier () const { return m_identifier; }
    inline Uint32 GetPrecedenceLevel () const { return m_precedence_level; }

    inline void SetPrecedenceLevel (Uint32 precedence_level)
    {
        m_precedence_level = precedence_level;
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    AstCommon::Identifier const *const m_identifier;
    Uint32 m_precedence_level;
}; // end of class PrecedenceDirective

class TokenDirective : public AstCommon::Directive
{
public:

    TokenDirective (
        TokenIdentifierList const *token_identifier_list,
        AstCommon::String const *assigned_type)
        :
        AstCommon::Directive("%token", token_identifier_list->GetFiLoc(), AT_TOKEN_DIRECTIVE),
        m_token_identifier_list(token_identifier_list),
        m_assigned_type(assigned_type)
    {
        assert(m_token_identifier_list != NULL);
        // m_assigned_type may be NULL
    }
    virtual ~TokenDirective ()
    {
        delete m_token_identifier_list;
        delete m_assigned_type;
    }

    inline TokenIdentifierList::const_iterator GetTokenIdentifierBegin () const { return m_token_identifier_list->begin(); }
    inline TokenIdentifierList::const_iterator GetTokenIdentifierEnd () const { return m_token_identifier_list->end(); }
    inline AstCommon::String const *GetAssignedType () const { return m_assigned_type; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    TokenIdentifierList const *const m_token_identifier_list;
    AstCommon::String const *const m_assigned_type;
}; // end of class TokenDirective

class ParserDirective : public AstCommon::Directive
{
public:

    ParserDirective (
        string const &directive_text,
        FiLoc const &filoc)
        :
        AstCommon::Directive(directive_text, filoc, AT_PARSER_DIRECTIVE),
        m_parser_directive_type(GetParserDirectiveType(directive_text))
    {
        assert(m_parser_directive_type < PDT_COUNT);
        m_value = NULL;
    }
    virtual ~ParserDirective ()
    {
        delete m_value;
    }

    static string const &GetString (ParserDirectiveType parser_directive_type);
    static ParserDirectiveType GetParserDirectiveType (string const &directive_text);
    static bool GetDoesParserDirectiveTypeRequireAParameter (
        ParserDirectiveType parser_directive_type);
    static bool GetDoesParserDirectiveAcceptDumbCodeBlock (
        ParserDirectiveType parser_directive_type);

    inline ParserDirectiveType GetParserDirectiveType () const { return m_parser_directive_type; }
    inline AstCommon::TextBase const *GetValue () const { return m_value; }

    inline void SetValue (AstCommon::TextBase const *value)
    {
        assert(value != NULL);
        assert(m_value == NULL);
        m_value = value;
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    ParserDirectiveType const m_parser_directive_type;
    AstCommon::TextBase const *m_value;
}; // end of class ParserDirective

class StartDirective : public AstCommon::Directive
{
public:

    StartDirective (AstCommon::Identifier const *start_nonterminal_identifier)
        :
        AstCommon::Directive("%start", start_nonterminal_identifier->GetFiLoc(), AT_START_DIRECTIVE),
        m_start_nonterminal_identifier(start_nonterminal_identifier)
    {
        assert(m_start_nonterminal_identifier != NULL);
    }
    virtual ~StartDirective ()
    {
        delete m_start_nonterminal_identifier;
    }

    inline AstCommon::Identifier const *GetStartNonterminalIdentifier () const { return m_start_nonterminal_identifier; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    AstCommon::Identifier const *const m_start_nonterminal_identifier;
}; // end of class StartDirective

class ParserDirectiveSet : public AstCommon::Ast
{
public:

    ParserDirectiveSet ()
        :
        AstCommon::Ast(FiLoc::ms_invalid, AT_PARSER_DIRECTIVE_SET)
    {
        for (Uint32 i = 0; i < PDT_COUNT; ++i)
            m_parser_directive[i] = NULL;
    }
    virtual ~ParserDirectiveSet ()
    {
        for (Uint32 i = 0; i < PDT_COUNT; ++i)
            delete m_parser_directive[i];
    }

    inline bool GetIsDirectiveSpecified (ParserDirectiveType parser_directive_type) const
    {
        assert(parser_directive_type < PDT_COUNT);
        return m_parser_directive[parser_directive_type] != NULL;
    }
    inline FiLoc const &GetDirectiveFiLoc (ParserDirectiveType parser_directive_type) const
    {
        assert(parser_directive_type < PDT_COUNT);
        if (m_parser_directive[parser_directive_type] != NULL)
            return m_parser_directive[parser_directive_type]->GetFiLoc();
        else
            return FiLoc::ms_invalid;
    }
    inline string const &GetDirectiveValueText (ParserDirectiveType parser_directive_type) const
    {
        assert(parser_directive_type < PDT_COUNT);
        if (m_parser_directive[parser_directive_type] != NULL &&
            m_parser_directive[parser_directive_type]->GetValue() != NULL)
            return m_parser_directive[parser_directive_type]->GetValue()->GetText();
        else
            return gs_empty_string;
    }

    void AddDirective (ParserDirective const *parser_directive);
    void CheckDependencies ();
    void FillInMissingOptionalDirectivesWithDefaults ();

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    ParserDirective const *m_parser_directive[PDT_COUNT];
}; // end of class ParserDirectiveSet

class Grammar : public AstCommon::Ast
{
public:

    Grammar (
        AstCommon::DirectiveList *directive_list,
        FiLoc const &end_preamble_filoc,
        NonterminalList *nonterminal_list);
    virtual ~Grammar ();

    inline ParserDirectiveSet const *GetParserDirectiveSet () const { return m_parser_directive_set; }
    inline TokenDirectiveList const *GetTokenDirectiveList () const { return m_token_directive_list; }
    inline PrecedenceDirectiveList const *GetPrecedenceDirectiveList () const { return m_precedence_directive_list; }
    inline Nonterminal *GetStartNonterminal () const { return m_start_nonterminal; }
    inline NonterminalList *GetNonterminalList () const { return m_nonterminal_list; }

    // this is the non-virtual, top-level Print method, not
    // to be confused with AstCommon::Ast::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    ParserDirectiveSet const *m_parser_directive_set;
    TokenDirectiveList const *m_token_directive_list;
    PrecedenceDirectiveList const *m_precedence_directive_list;
    Nonterminal *m_start_nonterminal;
    NonterminalList *const m_nonterminal_list;
}; // end of class Grammar

} // end of namespace Trison

#endif // !defined(_TRISON_AST_HPP_)
