// 2006.10.22 - Copyright Victor Dods - Licensed under Apache 2.0

#include "barf_targetspec_ast.hpp"

#include "barf_message.hpp"

namespace Barf {
namespace Targetspec {

string const &AstTypeString (AstType ast_type)
{
    static string const s_ast_type_string[AST_COUNT-Ast::AST_START_CUSTOM_TYPES_HERE_] =
    {
        "AST_ADD_CODESPEC",
        "AST_ADD_CODESPEC_LIST",
        "AST_ADD_DIRECTIVE",
        "AST_ADD_DIRECTIVE_MAP",
        "AST_PARAM_TYPE",
        "AST_SPECIFICATION",
        "AST_SPECIFICATION_MAP"
    };

    assert(ast_type < AST_COUNT);
    if (ast_type < Ast::AST_START_CUSTOM_TYPES_HERE_)
        return Ast::AstTypeString(ast_type);
    else
        return s_ast_type_string[ast_type-Ast::AST_START_CUSTOM_TYPES_HERE_];
}

void Specification::Print (ostream &stream, Uint32 indent_level) const
{
    Print(stream, AstTypeString, indent_level);
}

void Specification::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Base::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << "target: " << m_target_id->GetText() << endl;
    m_add_directive_map->Print(stream, Stringify, indent_level+1);
    m_add_codespec_list->Print(stream, Stringify, indent_level+1);
}

void AddDirective::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Base::Print(stream, Stringify, indent_level);
    stream << Tabs(indent_level+1) << (IsRequired() ? "required" : "optional") << endl;
    m_directive_to_add_id->Print(stream, Stringify, indent_level+1);
    stream << Tabs(indent_level+1) << ParamType::ParamTypeString(m_param_type) << endl;
}

void AddCodespec::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    Ast::Base::Print(stream, Stringify, indent_level);
    m_filename->Print(stream, Stringify, indent_level+1);
    m_filename_directive_id->Print(stream, Stringify, indent_level+1);
}

string const &ParamType::ParamTypeString (AstType ast_type)
{
    static string const s_id("%identifier");
    static string const s_string("%string");
    static string const s_dumb_code_block("%dumb_code_block");
    static string const s_strict_code_block("%strict_code_block");

    switch (ast_type)
    {
        case Ast::AST_ID:                return s_id;
        case Ast::AST_STRING:            return s_string;
        case Ast::AST_DUMB_CODE_BLOCK:   return s_dumb_code_block;
        case Ast::AST_STRICT_CODE_BLOCK: return s_strict_code_block;
        default:                        return g_empty_string;
    }
}

} // end of namespace Targetspec
} // end of namespace Barf
