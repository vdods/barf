// ///////////////////////////////////////////////////////////////////////////
// barf_preprocessor_scanner.cpp by Victor Dods, created 2006/10/15
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_preprocessor_scanner.hpp"

#include <sstream>

#include "barf_preprocessor_ast.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace Preprocessor {

Scanner::Scanner ()
    :
    InputBase(),
    m_start_code_delimiter("<{"),
    m_end_code_delimiter("}"),
    m_code_line_delimiter("<*{")
{ }

Scanner::~Scanner ()
{
    Close();
}

void Scanner::SetStartCodeDelimiter (string const &start_code_delimiter)
{
    assert(!start_code_delimiter.empty());
    assert(start_code_delimiter != m_code_line_delimiter);
    // TODO: better checking for allowable delimiters
    m_start_code_delimiter = start_code_delimiter;
}

void Scanner::SetEndCodeDelimiter (string const &end_code_delimiter)
{
    assert(!end_code_delimiter.empty());
    // TODO: better checking for allowable delimiters
    m_end_code_delimiter = end_code_delimiter;
}

void Scanner::SetCodeLineDelimiter (string const &code_line_delimiter)
{
    assert(!code_line_delimiter.empty());
    assert(code_line_delimiter != m_start_code_delimiter);
    // TODO: better checking for allowable delimiters
    m_code_line_delimiter = code_line_delimiter;
}

bool Scanner::OpenFile (string const &input_filename)
{
    bool open_succeeded = InputBase::OpenFile(input_filename);
    if (open_succeeded)
        HandleOpened();
    return open_succeeded;
}

void Scanner::OpenString (string const &input_string, string const &input_name, bool use_line_numbers)
{
    InputBase::OpenString(input_string, input_name, use_line_numbers);
    HandleOpened();
}

void Scanner::OpenUsingStream (istream *input_stream, string const &input_name, bool use_line_numbers)
{
    InputBase::OpenUsingStream(input_stream, input_name, use_line_numbers);
    HandleOpened();
}

bool Scanner::Close ()
{
    bool close_succeeded = InputBase::Close();
    if (close_succeeded)
        HandleClosed();
    return close_succeeded;
}

Parser::Token::Type Scanner::Scan (AstCommon::Ast **const scanned_token)
{
    assert(scanned_token != NULL);
    assert(*scanned_token == NULL);

    if (m_current_position == m_text.length())
        return Parser::Token::END_;

    switch (m_state)
    {
        case READING_BODY: return ScanBody(scanned_token);
        case READING_CODE: return ScanCode(scanned_token);
        default: assert(false && "this should never happen"); return Parser::Token::BAD_TOKEN;
    }
}

void Scanner::HandleOpened ()
{
    m_text.clear();
    while (true)
    {
        char c = In().get();
        if (In().eof())
            break;
        m_text += c;
    }
    m_current_position = 0;
    m_state = READING_BODY;
    m_is_reading_code_line = false;
    m_bracket_level = 0;
    m_bracket.clear();
}

void Scanner::HandleClosed ()
{
    m_text.clear();
}

Parser::Token::Type Scanner::ScanBody (AstCommon::Ast **scanned_token)
{
    assert(m_state == READING_BODY);
    assert(m_bracket_level == 0);

    // check for start code delimiter
    string::size_type start_code_delimiter_position = m_text.find(m_start_code_delimiter, m_current_position);
    string::size_type code_line_delimiter_position = m_text.find(m_code_line_delimiter, m_current_position);
    string::size_type end_of_text_position = min(min(start_code_delimiter_position, code_line_delimiter_position), m_text.length());
    if (start_code_delimiter_position == m_current_position)
    {
        assert(code_line_delimiter_position == string::npos || code_line_delimiter_position > m_current_position);
        m_current_position += m_start_code_delimiter.length();
        m_state = READING_CODE;
        m_is_reading_code_line = false;
        return Parser::Token::START_CODE;
    }
    else if (code_line_delimiter_position == m_current_position)
    {
        assert(start_code_delimiter_position == string::npos || start_code_delimiter_position > m_current_position);
        m_current_position += m_code_line_delimiter.length();
        m_state = READING_CODE;
        m_is_reading_code_line = true;
        return Parser::Token::CODE_LINE;
    }
    else
    {
        string matched_text(m_text.substr(m_current_position, end_of_text_position-m_current_position));
        *scanned_token = new Text(matched_text, GetFiLoc());
        IncrementLineNumber(GetNewlineCount(matched_text));
        m_current_position = end_of_text_position;
        return Parser::Token::TEXT;
    }
}

Parser::Token::Type Scanner::ScanCode (AstCommon::Ast **scanned_token)
{
    assert(m_state == READING_CODE);

    while (m_current_position < m_text.length() && IsWhitespace(m_text[m_current_position]))
    {
        if (m_text[m_current_position] == '\n')
        {
            IncrementLineNumber();
            if (m_is_reading_code_line)
            {
                ++m_current_position;
                m_state = READING_BODY;
                m_is_reading_code_line = false;
                return Parser::Token::CODE_NEWLINE;
            }
        }
        ++m_current_position;
    }

    if (m_current_position >= m_text.length())
    {
        if (m_is_reading_code_line)
            return Parser::Token::CODE_NEWLINE;
        else
            return Parser::Token::END_;
    }
    else if (m_bracket_level == 0 && m_text.substr(m_current_position, m_end_code_delimiter.length()) == m_end_code_delimiter)
    {
        m_state = READING_BODY;
        m_current_position += m_end_code_delimiter.length();
        return Parser::Token::END_CODE;
    }
    else if (m_bracket_level > 0 && m_text[m_current_position] == *m_bracket.rbegin())
    {
        assert(m_bracket.size() == m_bracket_level);
        --m_bracket_level;
        m_bracket.pop_back();
        return Parser::Token::Type(m_text[m_current_position++]);
    }
    else if (IsBracket(m_text[m_current_position]))
    {
        char c = m_text[m_current_position];
        switch (c)
        {
            case '(': ++m_bracket_level; m_bracket.push_back(GetMatchingBracket(c)); break;
            case '[': ++m_bracket_level; m_bracket.push_back(GetMatchingBracket(c)); break;
            // case '{': ++m_bracket_level; m_bracket.push_back(GetMatchingBracket(c)); break; // not used

            case ')':
            case ']':
            // case '}': // not used
                // TODO: emit "unmatched bracket" warning
                // panic.
                m_bracket_level = 0;
                m_bracket.clear();
                break;
        }
        return Parser::Token::Type(m_text[m_current_position++]);
    }
    else if (IsOperator(m_text[m_current_position]))
    {
        return Parser::Token::Type(m_text[m_current_position++]);
    }
    else if (IsAlpha(m_text[m_current_position]))
    {
        string::size_type id_end =
            m_text.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyz"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "0123456789_",
                m_current_position);
        string id = m_text.substr(m_current_position, id_end-m_current_position);
        m_current_position = id_end;

        if (id == "undefine") return Parser::Token::UNDEFINE;
        if (id == "declare_array") return Parser::Token::DECLARE_ARRAY;
        if (id == "declare_map") return Parser::Token::DECLARE_MAP;
        if (id == "define") return Parser::Token::DEFINE;
        if (id == "end_define") return Parser::Token::END_DEFINE;
        if (id == "loop") return Parser::Token::LOOP;
        if (id == "end_loop") return Parser::Token::END_LOOP;
        if (id == "for_each") return Parser::Token::FOR_EACH;
        if (id == "end_for_each") return Parser::Token::END_FOR_EACH;
        if (id == "include") return Parser::Token::INCLUDE;
        if (id == "sandbox_include") return Parser::Token::SANDBOX_INCLUDE;
        if (id == "sizeof") return Parser::Token::SIZEOF;
        if (id == "is_defined") return Parser::Token::IS_DEFINED;
        if (id == "dump_symbol_table") return Parser::Token::DUMP_SYMBOL_TABLE;
        if (id == "if") return Parser::Token::IF;
        if (id == "else") return Parser::Token::ELSE;
        if (id == "else_if") return Parser::Token::ELSE_IF;
        if (id == "end_if") return Parser::Token::END_IF;
        if (id == "int") return Parser::Token::KEYWORD_INT;
        if (id == "string") return Parser::Token::KEYWORD_STRING;
        if (id == "string_length") return Parser::Token::STRING_LENGTH;
        if (id == "warning") return Parser::Token::WARNING;
        if (id == "error") return Parser::Token::ERROR;
        if (id == "fatal_error") return Parser::Token::FATAL_ERROR;

        *scanned_token = new AstCommon::Id(id, GetFiLoc());
        return Parser::Token::ID;
    }
    else if (IsDigit(m_text[m_current_position]))
    {
        string::size_type integer_end = m_text.find_first_not_of("0123456789_", m_current_position);
        string integer_text = m_text.substr(m_current_position, integer_end-m_current_position);
        m_current_position = integer_end;

        Sint32 value = 0;
        istringstream in(integer_text);
        in >> value;
        *scanned_token = new Integer(value, GetFiLoc());
        return Parser::Token::INTEGER;
    }
    else if (m_text[m_current_position] == '\"')
    {
        string::size_type string_start = ++m_current_position;
        while (m_current_position < m_text.length() && m_text[m_current_position] != '\"')
        {
            if (m_text[m_current_position] == '\\')
                ++m_current_position;
            ++m_current_position;
        }

        if (m_current_position >= m_text.length())
        {
            EmitError(GetFiLoc(), "unterminated string literal");
            return Parser::Token::END_;
        }
        else
        {
            assert(m_text[m_current_position] == '\"');
        }

        string string_literal(
            m_text.substr(
                string_start,
                min(m_current_position, m_text.length())-string_start));
        ++m_current_position;
        *scanned_token = new Text(GetEscapedString(string_literal), GetFiLoc());
        IncrementLineNumber(GetNewlineCount(string_literal));
        return Parser::Token::STRING;
    }
    else
    {
        ++m_current_position;
        return Parser::Token::BAD_TOKEN;
    }
}

} // end of namespace Preprocessor
} // end of namespace Barf

