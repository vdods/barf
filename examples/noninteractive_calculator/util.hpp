// 2017.08.17 - Copyright Victor Dods - Licensed under Apache 2.0

#pragma once

#include <cstdint>
#include <string>

enum EscapeStringReturnCode : std::uint32_t
{
    SUCCESS = 0,
    // if there's a backslash immediately before the end of the string
    UNEXPECTED_EOI,
    // if there's \x without a hex digit after it
    MALFORMED_HEX_CHAR,
    // if the hex code's value exceeded 255
    HEX_ESCAPE_SEQUENCE_OUT_OF_RANGE,
    // if the octal code's value exceeded 255
    OCTAL_ESCAPE_SEQUENCE_OUT_OF_RANGE
};

struct EscapeStringStatus
{
    EscapeStringReturnCode m_return_code;
    std::uint32_t m_line_number_offset;

    EscapeStringStatus (EscapeStringReturnCode return_code, std::uint32_t line_number_offset)
        :
        m_return_code(return_code),
        m_line_number_offset(line_number_offset)
    { }
};

bool IsOctalDigit (std::uint8_t c);
bool IsHexDigit (std::uint8_t c);
std::uint8_t HexChar (std::uint8_t hex_digit);
std::string HexCharLiteral (std::uint8_t c, bool with_quotes = true);
bool CharNeedsHexEscaping (std::uint8_t c);
bool CharLiteralCharNeedsNormalEscaping (std::uint8_t c);
bool StringLiteralCharNeedsNormalEscaping (std::uint8_t c);
std::uint8_t EscapeCode (std::uint8_t c);
std::string CharLiteral (std::uint8_t c, bool with_quotes = true);
std::string StringLiteral (std::string const &text, bool with_quotes = true);
std::uint8_t EscapedChar (std::uint8_t c);
// this function relies on the fact that an escaped string (i.e. a string with
// escape codes such as \n or \xA7 converted into single chars) is not longer
// than the original string.
EscapeStringStatus EscapeString (std::string &text);
