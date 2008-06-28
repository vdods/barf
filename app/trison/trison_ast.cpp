// ///////////////////////////////////////////////////////////////////////////
// trison_ast.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_ast.hpp"

#include <algorithm>

#include "barf_optionsbase.hpp"
#include "barf_util.hpp"
#include "trison_message.hpp"

namespace Trison {

string const &GetAstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AST_COUNT-Ast::AST_START_CUSTOM_TYPES_HERE_] =
    {
        "AST_GRAMMAR",
        "AST_DIRECTIVE_LIST",
        "AST_PARSER_DIRECTIVE_SET",
        "AST_TOKEN_DIRECTIVE",
        "AST_PRECEDENCE_DIRECTIVE",
        "AST_PARSER_DIRECTIVE",
        "AST_START_DIRECTIVE",
        "AST_TOKEN_DIRECTIVE_LIST",
        "AST_TOKEN_ID_LIST",
        "AST_PRECEDENCE_DIRECTIVE_LIST",
        "AST_NONTERMINAL_LIST",
        "AST_NONTERMINAL",
        "AST_RULE_LIST",
        "AST_RULE",
        "AST_RULE_TOKEN_LIST",
        "AST_RULE_TOKEN",
        "AST_TOKEN_ID_ID",
        "AST_TOKEN_ID_CHAR",
        "AST_ID",
        "AST_CODE_BLOCK",
        "AST_STRING",
        "AST_THROW_AWAY"
    };

    assert(ast_type < AST_COUNT);
    if (ast_type < Ast::AST_START_CUSTOM_TYPES_HERE_)
        return Ast::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-Ast::AST_START_CUSTOM_TYPES_HERE_];
}

TokenId::~TokenId ()
{
}

void TokenIdId::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << " " << GetText() << endl;
}

void TokenIdChar::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << " " << GetText() << endl;
}

void RuleToken::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << " " << m_token_id->GetText();
    if (m_assigned_id != NULL)
        stream << ":" << m_assigned_id->GetText();
    stream << endl;
}

void RuleToken::PrettyPrint (ostream &stream) const
{
    stream << m_token_id->GetText();
}

void RuleToken::PrettyPrintWithAssignedId (ostream &stream) const
{
    stream << m_token_id->GetText();
    if (m_assigned_id != NULL)
        stream << ":" << m_assigned_id->GetText();
}

bool Rule::GetIsABinaryOperation () const
{
    if (GetRuleTokenCount() != 3)
        return false;

    string const &owner_nonterminal_name = GetOwnerNonterminal()->GetId()->GetText();
    string const &left_operand_name = GetRuleToken(0)->GetTokenId()->GetText();
    string const &operator_name = GetRuleToken(1)->GetTokenId()->GetText();
    string const &right_operand_name = GetRuleToken(2)->GetTokenId()->GetText();
    // both operands must have the same name as the target nonterminal.
    return
        owner_nonterminal_name == left_operand_name &&
        owner_nonterminal_name != operator_name &&
        owner_nonterminal_name == right_operand_name;
}

void Rule::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    m_rule_token_list->Print(stream, Stringify, indent_level+1);
    if (GetIsABinaryOperation())
        stream << Tabs(indent_level+1) << "associativity: "
               << m_associativity << endl;
    if (m_precedence_directive_id != NULL)
        stream << Tabs(indent_level+1) << "precedence token: "
               << m_precedence_directive_id->GetText() << endl;
    if (m_code_block != NULL)
        m_code_block->Print(stream, Stringify, indent_level+1);
}

void Rule::PrettyPrint (ostream &stream) const
{
    assert(GetOwnerNonterminal() != NULL);

    streamsize old_width = stream.width(4);
    stream << "rule " << m_index << ": "
           << m_owner_nonterminal->GetId()->GetText() << " <- ";
    for (RuleTokenList::const_iterator it = m_rule_token_list->begin(),
                                       it_end = m_rule_token_list->end();
         it != it_end;
         ++it)
    {
        RuleToken const *rule_token = *it;
        rule_token->PrettyPrint(stream);

        RuleTokenList::const_iterator test_it = it;
        ++test_it;
        if (test_it != it_end)
            stream << " ";
    }
    stream << "    ";
    if (GetIsABinaryOperation())
        Trison::PrettyPrint(stream, m_associativity);
    if (m_precedence_directive_id != NULL)
        stream << " %prec " << m_precedence_directive_id->GetText();
    stream.width(old_width);
}

void Rule::PrettyPrint (ostream &stream, RulePhase const &rule_phase) const
{
    assert(rule_phase.m_rule_index == m_index);
    assert(rule_phase.m_phase <= m_rule_token_list->size());
    assert(GetOwnerNonterminal() != NULL);

    streamsize old_width = stream.width(4);
    stream << "rule " << rule_phase << ": "
           << m_owner_nonterminal->GetId()->GetText() << " <- ";
    Uint32 current_token = 0;
    Uint32 token_count = m_rule_token_list->size();
    for (RuleTokenList::const_iterator it = m_rule_token_list->begin(),
                                       it_end = m_rule_token_list->end();
         it != it_end;
         ++it)
    {
        RuleToken const *rule_token = *it;

        if (current_token == rule_phase.m_phase)
        {
            stream << ".";
            if (current_token < token_count)
                stream << " ";
        }
        ++current_token;

        rule_token->PrettyPrint(stream);

        RuleTokenList::const_iterator test_it = it;
        ++test_it;
        if (test_it != it_end)
            stream << " ";
    }
    if (current_token == rule_phase.m_phase)
    {
        if (token_count > 0)
            stream << " .";
        else
            stream << ".";
    }
    stream << "    ";
    if (GetIsABinaryOperation())
        Trison::PrettyPrint(stream, m_associativity);
    if (m_precedence_directive_id != NULL)
        stream << " %prec " << m_precedence_directive_id->GetText();
    stream.width(old_width);
}

void Rule::PrettyPrintWithAssignedIds (ostream &stream) const
{
    assert(GetOwnerNonterminal() != NULL);

    streamsize old_width = stream.width(4);
    stream << "rule " << m_index << ": "
           << m_owner_nonterminal->GetId()->GetText() << " <- ";
    for (RuleTokenList::const_iterator it = m_rule_token_list->begin(),
                                       it_end = m_rule_token_list->end();
         it != it_end;
         ++it)
    {
        RuleToken const *rule_token = *it;
        rule_token->PrettyPrintWithAssignedId(stream);

        RuleTokenList::const_iterator test_it = it;
        ++test_it;
        if (test_it != it_end)
            stream << " ";
    }
    stream << "    ";
    if (GetIsABinaryOperation())
        Trison::PrettyPrint(stream, m_associativity);
    if (m_precedence_directive_id != NULL)
        stream << " %prec " << m_precedence_directive_id->GetText();
    stream.width(old_width);
}

string Nonterminal::GetImplementationFileId () const
{
    assert(m_id != NULL);
    if (m_id->GetText() == "%start")
        return "START_";
    else
        return m_id->GetText() + "__";
}

void Nonterminal::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    m_id->Print(stream, Stringify, indent_level+1);
    if (m_assigned_variable_type != NULL)
        stream << Tabs(indent_level+1) << "assigned variable type: "
               << m_assigned_variable_type->GetText() << endl;
    m_rule_list->Print(stream, Stringify, indent_level+1);
}

void PrecedenceDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << " level " << m_precedence_level << endl;
}

void TokenDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    m_token_id_list->Print(stream, Stringify, indent_level+1);
}

string const &ParserDirective::GetString (ParserDirectiveType parser_directive_type)
{
    static string const s_parser_directive_type_string[PDT_COUNT] =
    {
        "%parser_header_file_top",
        "%parser_header_file_bottom",
        "%parser_class_name",
        "%parser_class_inheritance",
        "%parser_superclass_and_member_constructors",
        "%parser_force_virtual_destructor",
        "%parser_constructor_actions",
        "%parser_destructor_actions",
        "%parser_parse_method_access",
        "%parser_class_methods_and_members",
        "%parser_start_of_parse_method_actions",
        "%parser_end_of_parse_method_actions",
        "%parser_throw_away_token_actions",
        "%parser_implementation_file_top",
        "%parser_implementation_file_bottom",
        "%parser_base_assigned_type",
        "%parser_base_assigned_type_sentinel",
        "%parser_custom_cast"
    };

    assert(parser_directive_type < PDT_COUNT);
    return s_parser_directive_type_string[parser_directive_type];
}

ParserDirectiveType ParserDirective::GetParserDirectiveType (string const &directive_text)
{
    assert(!directive_text.empty());
    assert(directive_text[0] == '%');
    for (Uint32 pdt = 0; pdt < PDT_COUNT; ++pdt)
    {
        if (directive_text == GetString(ParserDirectiveType(pdt)))
            return ParserDirectiveType(pdt);
    }
    // error condition
    return ParserDirectiveType(PDT_COUNT);
}

bool ParserDirective::GetDoesParserDirectiveTypeRequireAParameter (
    ParserDirectiveType parser_directive_type)
{
    switch (parser_directive_type)
    {
        case PDT_HEADER_FILE_TOP:
        case PDT_HEADER_FILE_BOTTOM:
        case PDT_CLASS_NAME:
        case PDT_CLASS_INHERITANCE:
        case PDT_SUPERCLASS_AND_MEMBER_CONSTRUCTORS:
        case PDT_CONSTRUCTOR_ACTIONS:
        case PDT_DESTRUCTOR_ACTIONS:
        case PDT_PARSE_METHOD_ACCESS:
        case PDT_CLASS_METHODS_AND_MEMBERS:
        case PDT_START_OF_PARSE_METHOD_ACTIONS:
        case PDT_END_OF_PARSE_METHOD_ACTIONS:
        case PDT_THROW_AWAY_TOKEN_ACTIONS:
        case PDT_IMPLEMENTATION_FILE_TOP:
        case PDT_IMPLEMENTATION_FILE_BOTTOM:
        case PDT_BASE_ASSIGNED_TYPE:
        case PDT_BASE_ASSIGNED_TYPE_SENTINEL:
        case PDT_CUSTOM_CAST:
            return true;

        case PDT_FORCE_VIRTUAL_DESTRUCTOR:
            return false;

        default:
            assert(false && "Invalid ParserDirectiveType");
            return false;
    }
}

bool ParserDirective::GetDoesParserDirectiveAcceptDumbCodeBlock (
    ParserDirectiveType parser_directive_type)
{
    assert(GetDoesParserDirectiveTypeRequireAParameter(parser_directive_type));
    switch (parser_directive_type)
    {
        case PDT_HEADER_FILE_TOP:
        case PDT_HEADER_FILE_BOTTOM:
        case PDT_START_OF_PARSE_METHOD_ACTIONS:
        case PDT_END_OF_PARSE_METHOD_ACTIONS:
        case PDT_IMPLEMENTATION_FILE_TOP:
        case PDT_IMPLEMENTATION_FILE_BOTTOM:
            return true;

        case PDT_CLASS_NAME:
        case PDT_CLASS_INHERITANCE:
        case PDT_SUPERCLASS_AND_MEMBER_CONSTRUCTORS:
        case PDT_CONSTRUCTOR_ACTIONS:
        case PDT_DESTRUCTOR_ACTIONS:
        case PDT_PARSE_METHOD_ACCESS:
        case PDT_CLASS_METHODS_AND_MEMBERS:
        case PDT_THROW_AWAY_TOKEN_ACTIONS:
        case PDT_BASE_ASSIGNED_TYPE:
        case PDT_BASE_ASSIGNED_TYPE_SENTINEL:
        case PDT_CUSTOM_CAST:
        case PDT_FORCE_VIRTUAL_DESTRUCTOR:
            return false;

        default:
            assert(false && "Invalid ParserDirectiveType");
            return false;
    }
}

void ParserDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << " " << GetParserDirectiveType() << endl;
    if (m_value != NULL)
        m_value->Print(stream, Stringify, indent_level+1);
}

void StartDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << " " << GetStartNonterminalId()->GetText() << endl;
}

void ParserDirectiveSet::AddDirective (
    ParserDirective const *const parser_directive)
{
    assert(parser_directive != NULL);
    ParserDirectiveType parser_directive_type = parser_directive->GetParserDirectiveType();
    assert(parser_directive_type < PDT_COUNT);

    if (m_parser_directive[parser_directive_type] != NULL)
    {
        // throw an error upon duplicate parser directives
        EmitError(
            "duplicate parser directive \"" +
            ParserDirective::GetString(parser_directive_type) +
            "\" (first encountered at " +
            m_parser_directive[parser_directive_type]->GetFiLoc().GetAsString() +
            ")",
            parser_directive->GetFiLoc());
        delete parser_directive;
    }
    else
    {
        // if the file location isn't set, use the one from the parser directive
        if (!GetFiLoc().GetHasLineNumber())
            SetFiLoc(parser_directive->GetFiLoc());
        // store the parser directive
        m_parser_directive[parser_directive_type] = parser_directive;
    }
}

void ParserDirectiveSet::CheckDependencies ()
{
    // check for required parser directives

    if (m_parser_directive[PDT_CLASS_NAME] == NULL)
        EmitError("required directive %parser_class_name is missing", FiLoc(g_options->GetInputFilename()));

    // check for other directive dependencies

    if (m_parser_directive[PDT_PARSE_METHOD_ACCESS] == NULL)
    {
        Ast::String *value = new Ast::String(FiLoc::ms_invalid);
        value->AppendText("public:");
        ParserDirective *parser_directive =
            new ParserDirective("%parser_parse_method_access", FiLoc::ms_invalid);
        parser_directive->SetValue(value);
        m_parser_directive[PDT_PARSE_METHOD_ACCESS] = parser_directive;
    }
    else
    {
        string const &text = GetDirectiveValueText(PDT_PARSE_METHOD_ACCESS);
        if (text != "public:" && text != "protected:" && text != "private:")
        {
            EmitError(
                "%parser_parse_method_access must be one of \"public:\", "
                "\"protected:\" or \"private:\" (if unspecified, \"public:\" is used)",
                GetDirectiveFiLoc(PDT_PARSE_METHOD_ACCESS));
        }
    }

    if (m_parser_directive[PDT_BASE_ASSIGNED_TYPE_SENTINEL] != NULL)
        if (m_parser_directive[PDT_BASE_ASSIGNED_TYPE] == NULL)
            EmitError(
                "%parser_base_assigned_type must be specified if "
                "%parser_base_assigned_type_sentinel is",
                FiLoc(g_options->GetInputFilename()));
}

void ParserDirectiveSet::FillInMissingOptionalDirectivesWithDefaults ()
{
    if (m_parser_directive[PDT_BASE_ASSIGNED_TYPE] == NULL)
    {
        assert(m_parser_directive[PDT_BASE_ASSIGNED_TYPE_SENTINEL] == NULL);

        ParserDirective *parser_directive =
            new ParserDirective("%parser_base_assigned_type", FiLoc::ms_invalid);
        Ast::String *directive_string = new Ast::String(FiLoc::ms_invalid);
        directive_string->AppendText("int");
        parser_directive->SetValue(directive_string);

        m_parser_directive[PDT_BASE_ASSIGNED_TYPE] = parser_directive;
    }

    if (m_parser_directive[PDT_BASE_ASSIGNED_TYPE_SENTINEL] == NULL)
    {
        assert(m_parser_directive[PDT_BASE_ASSIGNED_TYPE] != NULL);

        ParserDirective *parser_directive =
            new ParserDirective("%parser_base_assigned_type_sentinel", FiLoc::ms_invalid);
        Ast::String *directive_string = new Ast::String(FiLoc::ms_invalid);
        directive_string->AppendText(
            "static_cast<" +
            m_parser_directive[PDT_BASE_ASSIGNED_TYPE]->GetValue()->GetText() +
            ">(0)");
        parser_directive->SetValue(directive_string);

        m_parser_directive[PDT_BASE_ASSIGNED_TYPE_SENTINEL] = parser_directive;
    }
}

void ParserDirectiveSet::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    for (Uint32 i = 0; i < PDT_COUNT; ++i)
        if (m_parser_directive[i] != NULL)
            m_parser_directive[i]->Print(stream, Stringify, indent_level+1);
}

Grammar::Grammar (
    Ast::DirectiveList *directive_list,
    FiLoc const &end_preamble_filoc,
    NonterminalList *nonterminal_list)
    :
    Ast::Base(FiLoc::ms_invalid, AST_GRAMMAR),
    m_nonterminal_list(nonterminal_list)
{
    assert(m_nonterminal_list != NULL);

    // create the necessary directive lists/set/etc
    ParserDirectiveSet *parser_directive_set = new ParserDirectiveSet();
    m_parser_directive_set = parser_directive_set;

    TokenDirectiveList *token_directive_list = new TokenDirectiveList();
    m_token_directive_list = token_directive_list;

    PrecedenceDirectiveList *precedence_directive_list = new PrecedenceDirectiveList();
    m_precedence_directive_list = precedence_directive_list;

    StartDirective *start_directive = NULL;

    // traverse the directive list sorting each out into the proper container
    for (Ast::DirectiveList::iterator it = directive_list->begin(),
                                            it_end = directive_list->end();
         it != it_end;
         ++it)
    {
        Ast::Directive *directive = *it;
        assert(directive != NULL);
        switch (directive->GetAstType())
        {
            case AST_PARSER_DIRECTIVE:
                parser_directive_set->AddDirective(
                    Dsc<ParserDirective *>(directive));
                break;

            case AST_TOKEN_DIRECTIVE:
                token_directive_list->Append(
                    Dsc<TokenDirective *>(directive));
                break;

            case AST_PRECEDENCE_DIRECTIVE:
            {
                PrecedenceDirective *precedence_directive =
                    Dsc<PrecedenceDirective *>(directive);
                precedence_directive->SetPrecedenceLevel(precedence_directive_list->size() + 1);
                precedence_directive_list->Append(precedence_directive);
                break;
            }

            case AST_START_DIRECTIVE:
                if (start_directive != NULL)
                    EmitError("duplicate %start directive", directive->GetFiLoc());
                delete start_directive;
                start_directive = Dsc<StartDirective *>(directive);
                break;

            default:
                assert(false && "Invalid directive");
                break;
        }
    }
    // the directive list's contents have now been reassigned into the
    // proper specific containers in Grammar, so we can clear and delete it.
    directive_list->clear();
    delete directive_list;

    // check the parser directive set for errors
    parser_directive_set->CheckDependencies();
    // make sure the defaults for missing directives are filled in
    parser_directive_set->FillInMissingOptionalDirectivesWithDefaults();

    // create m_start_nonterminal from start_directive
    if (start_directive == NULL)
        EmitError("missing %start directive", end_preamble_filoc);
    else
    {
        Ast::Id *start_nonterminal_id =
            new Ast::Id("%start", start_directive->GetFiLoc());
        m_start_nonterminal =
            new Nonterminal(start_nonterminal_id, NULL);

        RuleTokenList *start_nonterminal_rule_token_list = new RuleTokenList();
        {
            string const &start_nonterminal_name = start_directive->GetStartNonterminalId()->GetText();
            TokenIdId *first_rule_token_id =
                new TokenIdId(start_nonterminal_name, FiLoc::ms_invalid);
            RuleToken *first_rule_token =
                new RuleToken(first_rule_token_id, NULL);
            start_nonterminal_rule_token_list->Append(first_rule_token);
        }
        {
            TokenIdId *second_rule_token_id =
                new TokenIdId("END_", FiLoc::ms_invalid);
            RuleToken *second_rule_token =
                new RuleToken(second_rule_token_id, NULL);
            start_nonterminal_rule_token_list->Append(second_rule_token);
        }

        Rule *start_nonterminal_rule =
            new Rule(start_nonterminal_rule_token_list, A_LEFT, NULL);

        Ast::CodeBlock *start_nonterminal_rule_code_block = new Ast::StrictCodeBlock(FiLoc::ms_invalid);
        start_nonterminal_rule_code_block->AppendText(
            "    assert(0 < m_reduction_rule_token_count);\n"
            "    return m_token_stack[m_token_stack.size() - m_reduction_rule_token_count];\n");

        start_nonterminal_rule->SetCodeBlock(start_nonterminal_rule_code_block);

        RuleList *start_nonterminal_rule_list = new RuleList();
        start_nonterminal_rule_list->Append(start_nonterminal_rule);

        m_start_nonterminal->SetRuleList(start_nonterminal_rule_list);

        delete start_directive;
    }
}

Grammar::~Grammar ()
{
    delete m_parser_directive_set;
    delete m_token_directive_list;
    delete m_precedence_directive_list;
    delete m_start_nonterminal;
    delete m_nonterminal_list;
}

void Grammar::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, GetAstTypeString, indent_level);
}

void Grammar::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    m_parser_directive_set->Print(stream, Stringify, indent_level+1);
    m_token_directive_list->Print(stream, Stringify, indent_level+1);
    m_start_nonterminal->Print(stream, Stringify, indent_level+1);
    m_nonterminal_list->Print(stream, Stringify, indent_level+1);
}

} // end of namespace Trison
