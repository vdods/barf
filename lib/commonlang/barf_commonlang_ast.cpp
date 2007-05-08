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
    static string const s_ast_type_string[CommonLang::AT_START_CUSTOM_TYPES_HERE_-Ast::AT_START_CUSTOM_TYPES_HERE_] =
    {
        "AT_RULE_HANDLER",
        "AT_RULE_HANDLER_MAP",
        "AT_TARGET",
        "AT_TARGET_DIRECTIVE",
        "AT_TARGET_MAP"
    };

    assert(ast_type < CommonLang::AT_START_CUSTOM_TYPES_HERE_);
    if (ast_type < Ast::AT_START_CUSTOM_TYPES_HERE_)
        return Ast::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-Ast::AT_START_CUSTOM_TYPES_HERE_];
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

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
    Ast::AstMap<TargetDirective>(AT_TARGET),
    m_target_id(target_id),
    m_is_enabled_for_code_generation(false)
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
    if (m_is_enabled_for_code_generation)
        Add(target_directive->m_directive_id->GetText(), target_directive);
    else
        EmitWarning(target_directive->GetFiLoc(), "undeclared target \"" + target_directive->m_target_id->GetText() + "\"");
}

void Target::ParseTargetSpec (string const &tool_prefix, TargetSpec::Parser &parser) const
{
    try {
        string filename(tool_prefix + '.' + m_target_id + ".targetspec");
        m_target_spec.m_source_path = g_options->GetTargetsSearchPath().GetFilePath(filename);
        if (m_target_spec.m_source_path.empty())
            EmitError(FiLoc(g_options->GetInputFilename()), "file \"" + filename + "\" not found in search path " + g_options->GetTargetsSearchPath().GetAsString());
        else if (!parser.OpenFile(m_target_spec.m_source_path))
            EmitError(FiLoc(g_options->GetInputFilename()), "unable to open file \"" + m_target_spec.m_source_path + "\" for reading");
        else if (parser.Parse() != TargetSpec::Parser::PRC_SUCCESS)
            EmitError(FiLoc(m_target_spec.m_source_path), "general targetspec parse error");
        else
        {
            m_target_spec.m_specification = Dsc<TargetSpec::Specification *>(parser.GetAcceptedToken());
            assert(m_target_spec.m_specification != NULL);
            if (g_options->GetIsVerbose(OptionsBase::V_TARGETSPEC_AST))
                m_target_spec.m_specification->Print(cerr);
            if (!g_errors_encountered)
                CheckAgainstTargetSpec(*m_target_spec.m_specification);
        }
    } catch (string const &exception) {
        cerr << exception << endl;
    }
}

void Target::ParseCodeSpecs (string const &tool_prefix, Preprocessor::Parser &parser) const
{
    assert(!m_target_spec.m_source_path.empty());
    assert(m_target_spec.m_specification != NULL);

    for (TargetSpec::AddCodeSpecList::const_iterator
             it = m_target_spec.m_specification->m_add_codespec_list->begin(),
             it_end = m_target_spec.m_specification->m_add_codespec_list->end();
         it != it_end;
         ++it)
    {
        TargetSpec::AddCodeSpec const *add_codespec = *it;
        assert(add_codespec != NULL);

        try {
            string filename(tool_prefix + '.' + m_target_id + '.' + add_codespec->m_filename->GetText() + ".codespec");
            string code_spec_filename(g_options->GetTargetsSearchPath().GetFilePath(filename));
            if (code_spec_filename.empty())
                EmitError(FiLoc(m_target_spec.m_source_path), "file \"" + filename + "\" not found in search path " + g_options->GetTargetsSearchPath().GetAsString());
            else if (!parser.OpenFile(code_spec_filename))
                EmitError(FiLoc(m_target_spec.m_source_path), "unable to open file \"" + code_spec_filename + "\" for reading");
            else if (parser.Parse() != Preprocessor::Parser::PRC_SUCCESS)
                EmitError(FiLoc(code_spec_filename), "general preprocessor parse error");
            else
            {
                Preprocessor::Body *codespec_body =
                    Dsc<Preprocessor::Body *>(parser.GetAcceptedToken());
                assert(codespec_body != NULL);
                m_code_spec_list.push_back(ParsedCodeSpec(add_codespec, codespec_body, code_spec_filename));
                if (g_options->GetIsVerbose(OptionsBase::V_CODESPEC_AST))
                    codespec_body->Print(cerr);
            }
        } catch (string const &exception) {
            cerr << exception << endl;
        }
    }
}

void Target::GenerateCode (Preprocessor::SymbolTable const &symbol_table) const
{
    if (!m_is_enabled_for_code_generation)
    {
        EmitWarning("skipping code generation for target " + m_target_id);
        return;
    }

    Preprocessor::SymbolTable target_symbol_table(symbol_table);
    GenerateTargetSymbols(target_symbol_table);

    target_symbol_table.DefineScalarSymbolAsText(
        "_creation_timestamp",
        FiLoc::ms_invalid,
        GetCurrentDateAndTimeString());

    for (ParsedCodeSpecList::const_iterator it = m_code_spec_list.begin(),
                                            it_end = m_code_spec_list.end();
         it != it_end;
         ++it)
    {
        ParsedCodeSpec const &code_spec = *it;

        string filename_directive_id(code_spec.m_add_codespec->m_filename_directive_id->GetText());
        string filename(g_options->GetOutputDir() + GetElement(filename_directive_id)->m_directive_value->GetText());
        ofstream stream;
        stream.open(filename.c_str());
        if (!stream.is_open())
            EmitError(FiLoc(g_options->GetInputFilename()), "could not open file \"" + filename + "\" for writing");
        else
        {
            Preprocessor::SymbolTable code_spec_symbol_table(target_symbol_table);

            code_spec_symbol_table.DefineScalarSymbolAsText(
                "_codespec_directory",
                FiLoc::ms_invalid,
                GetDirectoryPortion(code_spec.m_source_path));
            code_spec_symbol_table.DefineScalarSymbolAsText(
                "_codespec_filename",
                FiLoc::ms_invalid,
                GetFilenamePortion(code_spec.m_source_path));

            try {
                Preprocessor::Textifier textifier(stream, GetFilenamePortion(filename));
                // TODO: here's where you indicate line directives shouldn't be used
                code_spec.m_codespec_body->Execute(textifier, code_spec_symbol_table);
            } catch (string const &exception) {
                cerr << exception << endl;
            }
            stream.close();
        }
    }
}

void Target::CheckAgainstTargetSpec (TargetSpec::Specification const &specification) const
{
    // if checks are enabled, check that all the required directives are present
    if (m_is_enabled_for_code_generation)
    {
        for (TargetSpec::AddDirectiveMap::const_iterator it = specification.m_add_directive_map->begin(),
                                                         it_end = specification.m_add_directive_map->end();
            it != it_end;
            ++it)
        {
            TargetSpec::AddDirective const *add_directive = it->second;
            assert(add_directive != NULL);
            TargetDirective const *target_directive =
                GetElement(add_directive->m_directive_to_add_id->GetText());
            CheckAgainstAddDirective(*add_directive, target_directive);
        }
    }

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
                target_directive->GetFiLoc(),
                "directive " + target_directive->GetDirectiveString() +
                " does not exist in targetspec for " + m_target_id);
    }
}

void Target::CheckAgainstAddDirective (
    TargetSpec::AddDirective const &add_directive,
    TargetDirective const *target_directive) const
{
    if (target_directive == NULL)
    {
        if (add_directive.GetIsRequired())
            EmitError(FiLoc(g_options->GetInputFilename()), "missing required directive %target." + m_target_id + "." + add_directive.m_directive_to_add_id->GetText());
    }
    else if (add_directive.m_param_type == Ast::AT_NONE)
    {
        if (target_directive->m_directive_value != NULL)
            EmitError(target_directive->GetFiLoc(), "superfluous parameter given for directive %target." + target_directive->m_target_id->GetText() + "." + add_directive.m_directive_to_add_id->GetText() + " which does not accept a parameter");
    }
    else if (target_directive->m_directive_value->GetAstType() != add_directive.m_param_type)
    {
        EmitError(target_directive->GetFiLoc(), "directive %target." + target_directive->m_target_id->GetText() + "." + add_directive.m_directive_to_add_id->GetText() + " expects " + TargetSpec::ParamType::GetParamTypeString(add_directive.m_param_type) + ", got " + TargetSpec::ParamType::GetParamTypeString(target_directive->m_directive_value->GetAstType()));
    }
}

void Target::GenerateTargetSymbols (Preprocessor::SymbolTable &symbol_table) const
{
    symbol_table.DefineScalarSymbolAsText("_source_directory", FiLoc::ms_invalid, GetDirectoryPortion(m_source_path));
    symbol_table.DefineScalarSymbolAsText("_source_filename", FiLoc::ms_invalid, GetFilenamePortion(m_source_path));

    symbol_table.DefineScalarSymbolAsText("_targetspec_directory", FiLoc::ms_invalid, GetDirectoryPortion(m_target_spec.m_source_path));
    symbol_table.DefineScalarSymbolAsText("_targetspec_filename", FiLoc::ms_invalid, GetFilenamePortion(m_target_spec.m_source_path));

    symbol_table.DefineScalarSymbolAsText("_output_directory", FiLoc::ms_invalid, g_options->GetOutputDir());

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
            symbol->SetScalarBody(new Preprocessor::Body(gs_empty_string));
    }

    // define symbols for unspecified target directives which have
    // default values in the targetspec
    for (TargetSpec::AddDirectiveMap::const_iterator it = m_target_spec.m_specification->m_add_directive_map->begin(),
                                                     it_end = m_target_spec.m_specification->m_add_directive_map->end();
         it != it_end;
         ++it)
    {
        string const &directive_id = it->first;
        TargetSpec::AddDirective const *add_directive = it->second;
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

void TargetMap::AddTargetDirective (TargetDirective *target_directive)
{
    assert(target_directive != NULL);
    Target *target = GetElement(target_directive->m_target_id->GetText());
    if (target == NULL)
    {
        target = new Target(target_directive->m_target_id->GetText());
        Add(target_directive->m_target_id->GetText(), target);
    }
    assert(target != NULL);
    target->Add(target_directive);
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
