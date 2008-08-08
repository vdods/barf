// ///////////////////////////////////////////////////////////////////////////
// barf_commonlang_ast.cpp by Victor Dods, created 2006/10/22
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_commonlang_ast.hpp"

#include <fstream>
#include <sstream>

#include "barf_targetspec_ast.hpp"
#include "barf_targetspec_parser.hpp"
#include "barf_message.hpp"
#include "barf_optionsbase.hpp"
#include "barf_preprocessor_ast.hpp"
#include "barf_preprocessor_parser.hpp"
#include "barf_preprocessor_symboltable.hpp"
#include "barf_preprocessor_textifier.hpp"

namespace Barf {
namespace CommonLang {

string const &GetAstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[CommonLang::AST_START_CUSTOM_TYPES_HERE_-Ast::AST_START_CUSTOM_TYPES_HERE_] =
    {
        "AST_RULE_HANDLER",
        "AST_RULE_HANDLER_MAP",
        "AST_TARGET",
        "AST_TARGET_DIRECTIVE",
        "AST_TARGET_MAP"
    };

    assert(ast_type < CommonLang::AST_START_CUSTOM_TYPES_HERE_);
    if (ast_type < Ast::AST_START_CUSTOM_TYPES_HERE_)
        return Ast::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-Ast::AST_START_CUSTOM_TYPES_HERE_];
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void TargetDirective::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, GetAstTypeString, indent_level);
}

string TargetDirective::GetDirectiveString () const
{
    assert(m_target_id != NULL);
    assert(m_directive_id != NULL);
    return GetText() + "." + m_target_id->GetText() + "." + m_directive_id->GetText();
}

void TargetDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Directive::Print(stream, Stringify, indent_level);
    m_target_id->Print(stream, Stringify, indent_level+1);
    m_directive_id->Print(stream, Stringify, indent_level+1);
    if (m_directive_value != NULL)
        m_directive_value->Print(stream, Stringify, indent_level+1);
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

Target::Target (string const &target_id)
    :
    Ast::AstMap<TargetDirective>(AST_TARGET),
    m_target_id(target_id)
{
    assert(!m_target_id.empty());
}

void Target::SetSourcePath (string const &source_path)
{
    assert(!source_path.empty());
    m_source_path = source_path;
}

void Target::Add (TargetDirective *target_directive)
{
    Add(target_directive->m_directive_id->GetText(), target_directive);
}

void Target::Set (TargetDirective *target_directive)
{
    Set(target_directive->m_directive_id->GetText(), target_directive);
}

void Target::ParseTargetspec (Targetspec::Parser &parser) const
{
    try {
        string filename(GetOptions().GetProgramName() + '.' + m_target_id + ".targetspec");
        EmitExecutionMessage("beginning parse procedure of targetspec file \"" + filename + "\"");
        m_targetspec.m_source_path = GetOptions().GetSearchPath().GetFilePath(filename);
        Ast::Base *parsed_tree_root = NULL;
        if (m_targetspec.m_source_path.empty())
            EmitError("file \"" + filename + "\" not found in search path " + GetOptions().GetSearchPath().GetAsString(), FiLoc(GetOptions().GetInputFilename()));
        else if (!parser.OpenTargetspec(m_targetspec.m_source_path, m_target_id))
            EmitError("unable to open file \"" + m_targetspec.m_source_path + "\" for reading", FiLoc(GetOptions().GetInputFilename()));
        else if (parser.Parse(&parsed_tree_root) != Targetspec::Parser::PRC_SUCCESS)
            EmitError("general targetspec parse error -- " + GetOptions().HowtoReportError(), FiLoc(m_targetspec.m_source_path));
        else
        {
            m_targetspec.m_specification = Dsc<Targetspec::Specification *>(parsed_tree_root);
            assert(m_targetspec.m_specification != NULL);
            if (GetOptions().GetIsVerbose(OptionsBase::V_TARGETSPEC_AST))
                m_targetspec.m_specification->Print(cerr);
            if (!g_errors_encountered)
            {
                CheckAgainstTargetspec(*m_targetspec.m_specification);
                EmitExecutionMessage("done with parse procedure of targetspec file \"" + filename + "\"");
            }
        }
    } catch (string const &exception) {
        cerr << exception << endl;
    }
}

void Target::ParseCodespecs (Preprocessor::Parser &parser) const
{
    assert(!m_targetspec.m_source_path.empty());
    assert(m_targetspec.m_specification != NULL);

    for (Targetspec::AddCodespecList::const_iterator
             it = m_targetspec.m_specification->m_add_codespec_list->begin(),
             it_end = m_targetspec.m_specification->m_add_codespec_list->end();
         it != it_end;
         ++it)
    {
        Targetspec::AddCodespec const *add_codespec = *it;
        assert(add_codespec != NULL);

        try {
            string filename(GetOptions().GetProgramName()  + '.' + m_target_id + '.' + add_codespec->m_filename->GetText() + ".codespec");
            EmitExecutionMessage("beginning parse procedure of codespec file \"" + filename + "\"");
            string codespec_filename(GetOptions().GetSearchPath().GetFilePath(filename));
            Ast::Base *parsed_tree_root = NULL;
            if (codespec_filename.empty())
                EmitError("file \"" + filename + "\" not found in search path " + GetOptions().GetSearchPath().GetAsString(), FiLoc(m_targetspec.m_source_path));
            else if (!parser.OpenFile(codespec_filename))
                EmitError("unable to open file \"" + codespec_filename + "\" for reading", FiLoc(m_targetspec.m_source_path));
            else if (parser.Parse(&parsed_tree_root) != Preprocessor::Parser::PRC_SUCCESS)
                EmitError("general preprocessor parse error -- " + GetOptions().HowtoReportError(), FiLoc(codespec_filename));
            else
            {
                Preprocessor::Body *codespec_body = Dsc<Preprocessor::Body *>(parsed_tree_root);
                assert(codespec_body != NULL);
                m_codespec_list.push_back(ParsedCodespec(add_codespec, codespec_body, codespec_filename));
                if (GetOptions().GetIsVerbose(OptionsBase::V_CODESPEC_AST))
                    codespec_body->Print(cerr);
                EmitExecutionMessage("done with parse procedure of codespec file \"" + filename + "\"");
            }
        } catch (string const &exception) {
            cerr << exception << endl;
        }
    }
}

void Target::GenerateCode (Preprocessor::SymbolTable const &symbol_table) const
{
    Preprocessor::SymbolTable target_symbol_table(symbol_table);
    GenerateTargetSymbols(target_symbol_table);

    target_symbol_table.DefineScalarSymbolAsText(
        "_creation_timestamp",
        FiLoc::ms_invalid,
        GetCurrentDateAndTimeString());

    EmitExecutionMessage("generating code");

    for (ParsedCodespecList::const_iterator it = m_codespec_list.begin(),
                                            it_end = m_codespec_list.end();
         it != it_end;
         ++it)
    {
        ParsedCodespec const &codespec = *it;

        string filename_directive_id(codespec.m_add_codespec->m_filename_directive_id->GetText());
        string filename(GetOptions().GetOutputDirectory() + GetElement(filename_directive_id)->m_directive_value->GetText());
        ofstream stream;
        EmitExecutionMessage("opening file \"" + filename + "\" for output");
        stream.open(filename.c_str());
        if (!stream.is_open())
            EmitError("could not open file \"" + filename + "\" for writing", FiLoc(GetOptions().GetInputFilename()));
        else
        {
            EmitExecutionMessage("opened file \"" + filename + "\" successfully");
            Preprocessor::SymbolTable codespec_symbol_table(target_symbol_table);

            codespec_symbol_table.DefineScalarSymbolAsText(
                "_codespec_directory",
                FiLoc::ms_invalid,
                GetDirectoryPortion(codespec.m_source_path));
            codespec_symbol_table.DefineScalarSymbolAsText(
                "_codespec_filename",
                FiLoc::ms_invalid,
                GetFilenamePortion(codespec.m_source_path));

            try {
                Preprocessor::Textifier textifier(stream, GetFilenamePortion(filename));
                // TODO: here's where you indicate line directives shouldn't be used
                EmitExecutionMessage("generating code for file \"" + filename + "\"");
                codespec.m_codespec_body->Execute(textifier, codespec_symbol_table);
                EmitExecutionMessage("done generating code for file \"" + filename + "\"");
            } catch (string const &exception) {
                cerr << exception << endl;
            }
            stream.close();
        }
    }
}

void Target::CheckAgainstTargetspec (Targetspec::Specification const &specification) const
{
    EmitExecutionMessage("checking target directives");
    // check that all the required directives are present
    for (Targetspec::AddDirectiveMap::const_iterator it = specification.m_add_directive_map->begin(),
                                                     it_end = specification.m_add_directive_map->end();
         it != it_end;
         ++it)
    {
        Targetspec::AddDirective const *add_directive = it->second;
        assert(add_directive != NULL);
        TargetDirective const *target_directive =
            GetElement(add_directive->m_directive_to_add_id->GetText());
        CheckAgainstAddDirective(*add_directive, target_directive);
    }

    EmitExecutionMessage("checking validity of specified directives");
    // check that all the target directives are validly specified in the targetspec
    for (const_iterator it = begin(),
                        it_end = end();
         it != it_end;
         ++it)
    {
        TargetDirective const *target_directive = it->second;
        assert(target_directive != NULL);
        if (specification.m_add_directive_map->GetElement(target_directive->m_directive_id->GetText()) == NULL)
            EmitError(
                "directive " + target_directive->GetDirectiveString() +
                " does not exist in targetspec for " + m_target_id,
                target_directive->GetFiLoc());
    }
}

void Target::CheckAgainstAddDirective (
    Targetspec::AddDirective const &add_directive,
    TargetDirective const *target_directive) const
{
    if (target_directive == NULL)
    {
        if (add_directive.GetIsRequired())
            EmitError("missing required directive %target." + m_target_id + "." + add_directive.m_directive_to_add_id->GetText(), FiLoc(GetOptions().GetInputFilename()));
    }
    else if (add_directive.m_param_type == Ast::AST_NONE)
    {
        if (target_directive->m_directive_value != NULL)
            EmitError("superfluous parameter given for directive %target." + target_directive->m_target_id->GetText() + "." + add_directive.m_directive_to_add_id->GetText() + " which does not accept a parameter", target_directive->GetFiLoc());
    }
    else if (target_directive->m_directive_value->GetAstType() != add_directive.m_param_type)
    {
        EmitError("directive %target." + target_directive->m_target_id->GetText() + "." + add_directive.m_directive_to_add_id->GetText() + " expects " + Targetspec::ParamType::GetParamTypeString(add_directive.m_param_type) + ", got " + Targetspec::ParamType::GetParamTypeString(target_directive->m_directive_value->GetAstType()), target_directive->GetFiLoc());
    }
}

void Target::GenerateTargetSymbols (Preprocessor::SymbolTable &symbol_table) const
{
    symbol_table.DefineScalarSymbolAsText("_source_directory", FiLoc::ms_invalid, GetDirectoryPortion(m_source_path));
    symbol_table.DefineScalarSymbolAsText("_source_filename", FiLoc::ms_invalid, GetFilenamePortion(m_source_path));

    symbol_table.DefineScalarSymbolAsText("_targetspec_directory", FiLoc::ms_invalid, GetDirectoryPortion(m_targetspec.m_source_path));
    symbol_table.DefineScalarSymbolAsText("_targetspec_filename", FiLoc::ms_invalid, GetFilenamePortion(m_targetspec.m_source_path));

    symbol_table.DefineScalarSymbolAsText("_output_directory", FiLoc::ms_invalid, GetOptions().GetOutputDirectory());

    // define symbols for each of the specified target directives
    for (const_iterator it = begin(),
                        it_end = end();
         it != it_end;
         ++it)
    {
        TargetDirective const *target_directive = it->second;
        assert(target_directive != NULL);
        Preprocessor::ScalarSymbol *symbol =
            symbol_table.DefineScalarSymbol(
                target_directive->m_directive_id->GetText(),
                target_directive->GetFiLoc());

        if (target_directive->m_directive_value != NULL)
        {
            FiLoc const &source_filoc =
                target_directive->m_directive_value->GetIsCodeBlock() ?
                target_directive->m_directive_value->GetFiLoc() :
                FiLoc::ms_invalid;
            symbol->SetScalarBody(new Preprocessor::Body(target_directive->m_directive_value->GetText(), source_filoc));
        }
        else
            symbol->SetScalarBody(new Preprocessor::Body(g_empty_string));
    }

    // define symbols for unspecified target directives which have
    // default values in the targetspec
    for (Targetspec::AddDirectiveMap::const_iterator it = m_targetspec.m_specification->m_add_directive_map->begin(),
                                                     it_end = m_targetspec.m_specification->m_add_directive_map->end();
         it != it_end;
         ++it)
    {
        string const &directive_id = it->first;
        Targetspec::AddDirective const *add_directive = it->second;
        assert(add_directive != NULL);
        if (GetElement(directive_id) == NULL && add_directive->GetDefaultValue() != NULL)
        {
            Preprocessor::ScalarSymbol *symbol =
                symbol_table.DefineScalarSymbol(
                    directive_id,
                    FiLoc::ms_invalid);
            symbol->SetScalarBody(new Preprocessor::Body(add_directive->GetDefaultValue()->GetText()));
        }
    }
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void TargetMap::SetSourcePath (string const &source_path)
{
    assert(!source_path.empty());
    for (iterator it = begin(), it_end = end(); it != it_end; ++it)
    {
        Target *target = it->second;
        assert(target != NULL);
        target->SetSourcePath(source_path);
    }
}

void TargetMap::SetTargetDirective (TargetDirective *target_directive)
{
    assert(target_directive != NULL);
    Target *target = GetElement(target_directive->m_target_id->GetText());
    if (target == NULL)
    {
        target = new Target(target_directive->m_target_id->GetText());
        Add(target_directive->m_target_id->GetText(), target);
    }
    assert(target != NULL);

    // if there's a collision between directives, then we need to check
    // if the previously defined directive was defined in the primary
    // source or not.  collisions where one of the directives is a pre
    // or post define are allowed.
    TargetDirective *old_target_directive = target->GetElement(target_directive->m_directive_id->GetText());
    if (old_target_directive != NULL &&
        old_target_directive->GetFiLoc().GetHasLineNumber() &&
        target_directive->GetFiLoc().GetHasLineNumber())
    {
        EmitError(FORMAT(target_directive->GetDirectiveString() << " previously specified at " << old_target_directive->GetFiLoc()), target_directive->GetFiLoc());
    }
    target->Set(target_directive);
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void RuleHandler::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    if (m_target_id != NULL)
        m_target_id->Print(stream, Stringify, indent_level+1);
    m_rule_handler_code_block->Print(stream, Stringify, indent_level+1);
}

} // end of namespace CommonLang
} // end of namespace Barf
