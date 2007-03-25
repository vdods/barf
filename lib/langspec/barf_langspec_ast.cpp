// ///////////////////////////////////////////////////////////////////////////
// barf_langspec_ast.cpp by Victor Dods, created 2006/10/22
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_langspec_ast.hpp"

#include "barf_message.hpp"

namespace Barf {
namespace LangSpec {

string const &GetAstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AT_COUNT-AstCommon::AT_START_CUSTOM_TYPES_HERE_] =
    {
        "AT_SPECIFICATION_MAP",
        "AT_ADD_CODESPEC_LIST",
        "AT_ADD_DIRECTIVE_MAP",
        "AT_SPECIFICATION",
        "AT_ADD_CODESPEC",
        "AT_ADD_DIRECTIVE",
        "AT_PARAM_TYPE",
    };

    assert(ast_type < AT_COUNT);
    if (ast_type < AstCommon::AT_START_CUSTOM_TYPES_HERE_)
        return AstCommon::GetAstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-AstCommon::AT_START_CUSTOM_TYPES_HERE_];
}

void Specification::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, GetAstTypeString, indent_level);
}

void Specification::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    AstCommon::Ast::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "target language: " << m_target_language_identifier->GetText() << endl;
    m_add_directive_map->Print(stream, Stringify, indent_level+1);
    m_add_codespec_list->Print(stream, Stringify, indent_level+1);
}

void AddDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    AstCommon::Ast::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << (GetIsRequired() ? "required" : "optional") << endl;
    m_directive_to_add_identifier->Print(stream, Stringify, indent_level+1);
    stream << Tabs(indent_level+1) << ParamType::GetParamTypeString(m_param_type) << endl;
}

void AddCodeSpec::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    AstCommon::Ast::Print(stream, Stringify, indent_level);
    m_filename->Print(stream, Stringify, indent_level+1);
    m_filename_directive_identifier->Print(stream, Stringify, indent_level+1);
}

string const &ParamType::GetParamTypeString (AstType ast_type)
{
    static string const s_identifier("%identifier");
    static string const s_string("%string");
    static string const s_dumb_code_block("%dumb_code_block");
    static string const s_strict_code_block("%strict_code_block");

    switch (ast_type)
    {
        case AstCommon::AT_IDENTIFIER:        return s_identifier;
        case AstCommon::AT_STRING:            return s_string;
        case AstCommon::AT_DUMB_CODE_BLOCK:   return s_dumb_code_block;
        case AstCommon::AT_STRICT_CODE_BLOCK: return s_strict_code_block;
        default:                              return gs_empty_string;
    }
}

} // end of namespace LangSpec
} // end of namespace Barf
