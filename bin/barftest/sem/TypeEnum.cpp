// 2016.08.09 - Victor Dods

#include "sem/TypeEnum.hpp"

namespace cbz {
namespace sem {

std::string const &as_string (TypeEnum type_enum)
{
    static std::string const STRING_TABLE[uint32_t(TypeEnum::__COUNT__)] = {
        "ASSIGNMENT",
        "BINARY_OPERATION",
        "BOOLEAN",
        "BOOLEAN_VALUE",
        "BREAK",
        "CONDITIONAL_EXPRESSION",
        "CONDITIONAL_STATEMENT",
        "DECLARATION",
        "DECLARATION_TUPLE",
        "DEFINITION",
        "DUMMY",
        "ELEMENT_ACCESS",
        "ERROR_DUMMY",
        "FLOAT32",
        "FLOAT32_VALUE",
        "FLOAT64",
        "FLOAT64_VALUE",
        "FUNCTION",
        "FUNCTION_EVALUATION",
        "FUNCTION_PROTOTYPE",
        "FUNCTION_TYPE",
        "GLOBAL_VALUE_LIFETIME",
        "IDENTIFIER",
        "IDENTIFIER_TUPLE",
        "INITIALIZATION",
        "LLVM_CAST",
        "LLVM_CAST_KEYWORD",
        "LOOP",
        "NULL_PTR",
        "NULL_TYPE",
        "NULL_VALUE",
        "PARAMETER_LIST",
        "POINTER_TYPE",
        "REFERENCE_TYPE",
        "RETURN",
        "SINT16",
        "SINT16_VALUE",
        "SINT32",
        "SINT32_VALUE",
        "SINT64",
        "SINT64_VALUE",
        "SINT8",
        "SINT8_VALUE",
        "STATEMENT_LIST",
        "STRING_LITERAL",
        "SYMBOL_SPECIFIER",
        "TUPLE",
        "TYPE_ARRAY",
        "TYPE_DUMMY",
        "TYPE_IDENTIFIER",
        "TYPE_KEYWORD",
        "TYPE_TUPLE",
        "UINT16",
        "UINT16_VALUE",
        "UINT32",
        "UINT32_VALUE",
        "UINT64",
        "UINT64_VALUE",
        "UINT8",
        "UINT8_VALUE",
        "UNARY_OPERATION",
        "VALUE_KIND_SPECIFIER",
        "VALUE_LIFETIME_SPECIFIER",
        "VALUE_TUPLE",
        "VOID_TYPE",
        "VOID_VALUE",
    };
    return STRING_TABLE[uint32_t(type_enum)];
}

} // end namespace sem
} // end namespace cbz
