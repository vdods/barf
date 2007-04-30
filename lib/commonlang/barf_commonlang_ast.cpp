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

#include "barf_langspec_ast.hpp"
#include "barf_langspec_parser.hpp"
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
    static string const s_ast_type_string[CommonLang::AT_START_CUSTOM_TYPES_HERE_-AstCommon::AT_START_CUSTOM_TYPES_HERE_] =
    {
        "AT_LANGUAGE_DIRECTIVE",
        "AT_TARGET_LANGUAGE",
        "AT_TARGET_LANGUAGE_MAP",
        "AT_RULE_HANDLER",
        "AT_RULE_HANDLER_MAP"
    };

    assert(ast_type < CommonLang::AT_START_CUSTOM_TYPES_HERE_);
    if (ast_type < AstCommon::AT_START_CUSTOM_TYPES_HERE_)
        return AstCommon::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-AstCommon::AT_START_CUSTOM_TYPES_HERE_];
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

string LanguageDirective::GetDirectiveString () const
{
    assert(m_language_identifier != NULL);
    assert(m_directive_identifier != NULL);
    return GetText() + "." + m_language_identifier->GetText() + "." + m_directive_identifier->GetText();
}

void LanguageDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Directive::Print(stream, Stringify, indent_level);
    m_language_identifier->Print(stream, Stringify, indent_level+1);
    m_directive_identifier->Print(stream, Stringify, indent_level+1);
    if (m_directive_value != NULL)
        m_directive_value->Print(stream, Stringify, indent_level+1);
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

TargetLanguage::TargetLanguage (string const &language_identifier)
    :
    AstCommon::AstMap<LanguageDirective>(AT_TARGET_LANGUAGE),
    m_language_identifier(language_identifier),
    m_is_enabled_for_code_generation(false)
{
    assert(!m_language_identifier.empty());
}

void TargetLanguage::SetSourcePath (string const &source_path)
{
    assert(!source_path.empty());
    m_source_path = source_path;
}

void TargetLanguage::Add (LanguageDirective *language_directive)
{
    if (m_is_enabled_for_code_generation)
        Add(language_directive->m_directive_identifier->GetText(), language_directive);
    else
        EmitWarning(language_directive->GetFiLoc(), "undeclared target language \"" + language_directive->m_language_identifier->GetText() + "\"");
}

void TargetLanguage::ParseLangSpec (string const &tool_prefix, LangSpec::Parser &parser) const
{
    try {
        string filename(tool_prefix + '.' + m_language_identifier + ".langspec");
        m_lang_spec.m_source_path = g_options->GetFilePath(filename);
        if (m_lang_spec.m_source_path.empty())
            EmitError(FiLoc(g_options->GetInputFilename()), "file \"" + filename + "\" not found in search path " + g_options->GetSearchPathString());
        else if (!parser.OpenFile(m_lang_spec.m_source_path))
            EmitError(FiLoc(g_options->GetInputFilename()), "unable to open file \"" + m_lang_spec.m_source_path + "\" for reading");
        else if (parser.Parse() != LangSpec::Parser::PRC_SUCCESS)
            EmitError(FiLoc(m_lang_spec.m_source_path), "general langspec parse error");
        else
        {
            m_lang_spec.m_specification = Dsc<LangSpec::Specification *>(parser.GetAcceptedToken());
            assert(m_lang_spec.m_specification != NULL);
            if (g_options->GetShowLangSpecSyntaxTree())
                m_lang_spec.m_specification->Print(cerr);
            if (!g_errors_encountered)
                CheckAgainstLangSpec(*m_lang_spec.m_specification);
        }
    } catch (string const &exception) {
        cerr << exception << endl;
    }
}

void TargetLanguage::ParseCodeSpecs (string const &tool_prefix, Preprocessor::Parser &parser) const
{
    assert(!m_lang_spec.m_source_path.empty());
    assert(m_lang_spec.m_specification != NULL);

    for (LangSpec::AddCodeSpecList::const_iterator
             it = m_lang_spec.m_specification->m_add_codespec_list->begin(),
             it_end = m_lang_spec.m_specification->m_add_codespec_list->end();
         it != it_end;
         ++it)
    {
        LangSpec::AddCodeSpec const *add_codespec = *it;
        assert(add_codespec != NULL);

        try {
            string filename(tool_prefix + '.' + m_language_identifier + '.' + add_codespec->m_filename->GetText() + ".codespec");
            string code_spec_filename(g_options->GetFilePath(filename));
            if (code_spec_filename.empty())
                EmitError(FiLoc(m_lang_spec.m_source_path), "file \"" + filename + "\" not found in search path " + g_options->GetSearchPathString());
            else if (!parser.OpenFile(code_spec_filename))
                EmitError(FiLoc(m_lang_spec.m_source_path), "unable to open file \"" + code_spec_filename + "\" for reading");
            else if (parser.Parse() != Preprocessor::Parser::PRC_SUCCESS)
                EmitError(FiLoc(code_spec_filename), "general preprocessor parse error");
            else
            {
                Preprocessor::Body *codespec_body =
                    Dsc<Preprocessor::Body *>(parser.GetAcceptedToken());
                assert(codespec_body != NULL);
                m_code_spec_list.push_back(ParsedCodeSpec(add_codespec, codespec_body, code_spec_filename));
                if (g_options->GetShowPreprocessorSyntaxTree())
                    codespec_body->Print(cerr);
            }
        } catch (string const &exception) {
            cerr << exception << endl;
        }
    }
}

void TargetLanguage::GenerateCode (Preprocessor::SymbolTable const &symbol_table) const
{
    if (!m_is_enabled_for_code_generation)
    {
        EmitWarning("skipping code generation for target language " + m_language_identifier);
        return;
    }

    Preprocessor::SymbolTable target_language_symbol_table(symbol_table);
    GenerateTargetLanguageSymbols(target_language_symbol_table);

    target_language_symbol_table.DefineScalarSymbolAsText(
        "_creation_timestamp",
        FiLoc::ms_invalid,
        GetCurrentDateAndTimeString());

    for (ParsedCodeSpecList::const_iterator it = m_code_spec_list.begin(),
                                            it_end = m_code_spec_list.end();
         it != it_end;
         ++it)
    {
        ParsedCodeSpec const &code_spec = *it;

        string filename_directive_identifier(code_spec.m_add_codespec->m_filename_directive_identifier->GetText());
        string filename(g_options->GetOutputDir() + GetElement(filename_directive_identifier)->m_directive_value->GetText());
        ofstream stream;
        stream.open(filename.c_str());
        if (!stream.is_open())
            EmitError(FiLoc(g_options->GetInputFilename()), "could not open file \"" + filename + "\" for writing");
        else
        {
            Preprocessor::SymbolTable code_spec_symbol_table(target_language_symbol_table);

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

void TargetLanguage::CheckAgainstLangSpec (LangSpec::Specification const &specification) const
{
    // if checks are enabled, check that all the required directives are present
    if (m_is_enabled_for_code_generation)
    {
        for (LangSpec::AddDirectiveMap::const_iterator it = specification.m_add_directive_map->begin(),
                                                       it_end = specification.m_add_directive_map->end();
            it != it_end;
            ++it)
        {
            LangSpec::AddDirective const *add_directive = it->second;
            assert(add_directive != NULL);
            LanguageDirective const *language_directive =
                GetElement(add_directive->m_directive_to_add_identifier->GetText());
            CheckAgainstAddDirective(*add_directive, language_directive);
        }
    }

    // check that all the language directives are validly specified in the langspec
    for (const_iterator it = begin(),
                        it_end = end();
         it != it_end;
         ++it)
    {
        LanguageDirective const *language_directive = it->second;
        assert(language_directive != NULL);
        if (specification.m_add_directive_map->GetElement(language_directive->m_directive_identifier->GetText()) == NULL)
            EmitError(
                language_directive->GetFiLoc(),
                "directive " + language_directive->GetDirectiveString() +
                " does not exist in langspec for " + m_language_identifier);
    }
}

void TargetLanguage::CheckAgainstAddDirective (
    LangSpec::AddDirective const &add_directive,
    LanguageDirective const *language_directive) const
{
    if (language_directive == NULL)
    {
        if (add_directive.GetIsRequired())
            EmitError(FiLoc(g_options->GetInputFilename()), "missing required directive %language." + m_language_identifier + "." + add_directive.m_directive_to_add_identifier->GetText());
    }
    else if (add_directive.m_param_type == AstCommon::AT_NONE)
    {
        if (language_directive->m_directive_value != NULL)
            EmitError(language_directive->GetFiLoc(), "superfluous parameter given for directive %language." + language_directive->m_language_identifier->GetText() + "." + add_directive.m_directive_to_add_identifier->GetText() + " which does not accept a parameter");
    }
    else if (language_directive->m_directive_value->GetAstType() != add_directive.m_param_type)
    {
        EmitError(language_directive->GetFiLoc(), "directive %language." + language_directive->m_language_identifier->GetText() + "." + add_directive.m_directive_to_add_identifier->GetText() + " expects " + LangSpec::ParamType::GetParamTypeString(add_directive.m_param_type) + ", got " + LangSpec::ParamType::GetParamTypeString(language_directive->m_directive_value->GetAstType()));
    }
}

void TargetLanguage::GenerateTargetLanguageSymbols (Preprocessor::SymbolTable &symbol_table) const
{
    symbol_table.DefineScalarSymbolAsText("_source_directory", FiLoc::ms_invalid, GetDirectoryPortion(m_source_path));
    symbol_table.DefineScalarSymbolAsText("_source_filename", FiLoc::ms_invalid, GetFilenamePortion(m_source_path));

    symbol_table.DefineScalarSymbolAsText("_langspec_directory", FiLoc::ms_invalid, GetDirectoryPortion(m_lang_spec.m_source_path));
    symbol_table.DefineScalarSymbolAsText("_langspec_filename", FiLoc::ms_invalid, GetFilenamePortion(m_lang_spec.m_source_path));

    symbol_table.DefineScalarSymbolAsText("_output_directory", FiLoc::ms_invalid, g_options->GetOutputDir());

    // define symbols for each of the specified language directives
    for (const_iterator it = begin(),
                        it_end = end();
         it != it_end;
         ++it)
    {
        LanguageDirective const *language_directive = it->second;
        assert(language_directive != NULL);
        Preprocessor::ScalarSymbol *symbol =
            symbol_table.DefineScalarSymbol(
                language_directive->m_directive_identifier->GetText(),
                language_directive->GetFiLoc());

        if (language_directive->m_directive_value != NULL)
        {
            FiLoc const &source_filoc =
                language_directive->m_directive_value->GetIsCodeBlock() ?
                language_directive->m_directive_value->GetFiLoc() :
                FiLoc::ms_invalid;
            symbol->SetScalarBody(new Preprocessor::Body(language_directive->m_directive_value->GetText(), source_filoc));
        }
        else
            symbol->SetScalarBody(new Preprocessor::Body(gs_empty_string, FiLoc::ms_invalid));
    }

    // define symbols for unspecified language directives which have
    // default values in the langspec
    for (LangSpec::AddDirectiveMap::const_iterator it = m_lang_spec.m_specification->m_add_directive_map->begin(),
                                                   it_end = m_lang_spec.m_specification->m_add_directive_map->end();
         it != it_end;
         ++it)
    {
        string const &directive_identifier = it->first;
        LangSpec::AddDirective const *add_directive = it->second;
        assert(add_directive != NULL);
        if (GetElement(directive_identifier) == NULL && add_directive->GetDefaultValue() != NULL)
        {
            Preprocessor::ScalarSymbol *symbol =
                symbol_table.DefineScalarSymbol(
                    directive_identifier,
                    FiLoc::ms_invalid);
            symbol->SetScalarBody(new Preprocessor::Body(add_directive->GetDefaultValue()->GetText(), FiLoc::ms_invalid));
        }
    }
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void TargetLanguageMap::SetSourcePath (string const &source_path)
{
    assert(!source_path.empty());
    for (iterator it = begin(), it_end = end(); it != it_end; ++it)
    {
        TargetLanguage *target_language = it->second;
        assert(target_language != NULL);
        target_language->SetSourcePath(source_path);
    }
}

void TargetLanguageMap::AddLanguageDirective (LanguageDirective *language_directive)
{
    assert(language_directive != NULL);
    TargetLanguage *target_language = GetElement(language_directive->m_language_identifier->GetText());
    if (target_language == NULL)
    {
        target_language = new TargetLanguage(language_directive->m_language_identifier->GetText());
        Add(language_directive->m_language_identifier->GetText(), target_language);
    }
    assert(target_language != NULL);
    target_language->Add(language_directive);
}

// ///////////////////////////////////////////////////////////////////////////
//
// ///////////////////////////////////////////////////////////////////////////

void RuleHandler::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << Stringify(GetAstType()) << endl;
    if (m_language_identifier != NULL)
        m_language_identifier->Print(stream, Stringify, indent_level+1);
    m_rule_handler_code_block->Print(stream, Stringify, indent_level+1);
}

} // end of namespace CommonLang
} // end of namespace Barf
