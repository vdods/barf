// ///////////////////////////////////////////////////////////////////////////
// trison_scanner.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_scanner.hpp"

#include "barf_util.hpp"
#include "trison_ast.hpp"
#include "trison_message.hpp"

namespace Trison {

#undef FL
#define FL FileLocation(m_input_filename, m_line_number)

Scanner::Scanner ()
{
    m_line_number = 0;
}

Scanner::~Scanner ()
{
    Close();
}

bool Scanner::Open (string const &input_filename)
{
    assert(!m_input.is_open() && "you must call Close() first");

    m_input.open(input_filename.c_str());
    m_input.unsetf(ios_base::skipws);
    if (m_input.is_open())
        m_input_filename = input_filename;
    else
        m_input_filename.clear();
    m_text.clear();
    m_line_number = 1;
    m_in_preamble = true;
    return m_input.is_open();
}

void Scanner::Close ()
{
    if (m_input.is_open())
        m_input.close();
    m_line_number = 0;
}

Parser::Token::Type Scanner::Scan (AstCommon::Ast **const scanned_token)
{
    assert(scanned_token != NULL);
    assert(*scanned_token == NULL);

    while (true)
    {
        m_text.clear();

        char c;

        while (!IsNextCharEOF(&c) && IsWhitespace(c))
            m_input >> c;

        m_input >> c;
        if (m_input.eof())
            return Parser::Token::END_;
        m_text += c;

        if (IsAlpha(c))
            return ScanIdentifier(scanned_token);
        else if (IsOperator(c))
            return ScanOperator(scanned_token);
        else switch (c)
        {
            case '%': return ScanDirective(scanned_token);
            case '{': return ScanStrictCodeBlock(scanned_token);
            case '"': return ScanStringLiteral(scanned_token);
            case '\'': return ScanCharacterLiteral(scanned_token);
            case '\n':
                ++m_line_number;
                if (m_in_preamble)
                    return Parser::Token::NEWLINE;
                else
                    break;
            case '/':
                try
                {
                    ScanComment();
                }
                catch (Parser::Token::Type token_type)
                {
                    if (token_type == Parser::Token::END_)
                        EmitWarning(FL, "unterminated comment");
                    return token_type;
                }
                break;
            default:
                return Parser::Token::BAD_TOKEN;
        }
    }
}

Parser::Token::Type Scanner::ScanIdentifier (AstCommon::Ast **const scanned_token)
{
    assert(scanned_token != NULL);
    assert(*scanned_token == NULL);
    assert(!m_input.eof());

    char c;

    while (!IsNextCharEOF(&c) && (IsAlpha(c) || IsDigit(c) || c == '_'))
    {
        m_input >> c;
        m_text += c;
    }

    assert(!m_text.empty());
    // if the last character is underscore, squirt an error, because ending
    // identifiers with underscores is a method used by the parser generator
    // to ensure unique state machine token types.
    if (*m_text.rbegin() == '_')
        EmitError(FL, "identifiers can not end with an underscore (\"" + m_text + "\")");

    *scanned_token = new AstCommon::Identifier(m_text, FL);
    return Parser::Token::IDENTIFIER;
}

Parser::Token::Type Scanner::ScanOperator (AstCommon::Ast **const scanned_token)
{
    assert(scanned_token != NULL);
    assert(*scanned_token == NULL);
    assert(!m_input.eof());

    return Parser::Token::Type(m_text[0]);
}

Parser::Token::Type Scanner::ScanDirective (AstCommon::Ast **const scanned_token)
{
    assert(scanned_token != NULL);
    assert(*scanned_token == NULL);
    assert(!m_input.eof());

    char c;

    // "%{" is special
    if (!IsNextCharEOF(&c) && c == '{')
    {
        m_input >> c;
        m_text += c;
        return ScanDumbCodeBlock(scanned_token);
    }
    // "%%" is special
    else if (c == '%')
    {
        m_input >> c;
        m_text += c;
        *scanned_token = new AstCommon::ThrowAway(FL);
        m_in_preamble = false;
        return Parser::Token::END_PREAMBLE;
    }

    while (!IsNextCharEOF(&c) && (IsLowercaseAlpha(c) || IsDigit(c) || c == '_'))
    {
        m_input >> c;
        m_text += c;
    }

    if (m_text.substr(0, 8) == "%parser_" &&
        ParserDirective::GetParserDirectiveType(m_text) < PDT_COUNT)
    {
        *scanned_token = new ParserDirective(m_text, FL);
        return
            ParserDirective::GetDoesParserDirectiveTypeRequireAParameter(
                static_cast<ParserDirective *>(*scanned_token)->
                    GetParserDirectiveType()) ?
            Parser::Token::DIRECTIVE_PARSER_WITH_PARAMETER :
            Parser::Token::DIRECTIVE_PARSER_WITHOUT_PARAMETER;
    }
    else if (m_text == "%token")
    {
        *scanned_token = new AstCommon::ThrowAway(FL);
        return Parser::Token::DIRECTIVE_TOKEN;
    }
    else if (m_text == "%left")
    {
        *scanned_token = new AstCommon::ThrowAway(FL);
        return Parser::Token::LEFT;
    }
    else if (m_text == "%right")
    {
        *scanned_token = new AstCommon::ThrowAway(FL);
        return Parser::Token::RIGHT;
    }
    else if (m_text == "%nonassoc")
    {
        *scanned_token = new AstCommon::ThrowAway(FL);
        return Parser::Token::NONASSOC;
    }
    else if (m_text == "%prec")
    {
        *scanned_token = new AstCommon::ThrowAway(FL);
        return Parser::Token::PREC;
    }
    else if (m_text == "%start")
    {
        *scanned_token = new AstCommon::ThrowAway(FL);
        return Parser::Token::START;
    }
    else if (m_text == "%type")
    {
        *scanned_token = new AstCommon::ThrowAway(FL);
        return Parser::Token::TYPE;
    }
    else if (m_text == "%error")
    {
        *scanned_token = new AstCommon::ThrowAway(FL);
        return Parser::Token::DIRECTIVE_ERROR;
    }

    return Parser::Token::BAD_TOKEN;
}

Parser::Token::Type Scanner::ScanStrictCodeBlock (AstCommon::Ast **const scanned_token)
{
    assert(scanned_token != NULL);
    assert(*scanned_token == NULL);
    assert(!m_input.eof());

    char c;
    Uint32 bracket_level = 0;
    Uint32 starting_line_number = m_line_number;

    m_text.clear();

    while (!IsNextCharEOF(&c) && !(c == '}' && bracket_level == 0))
    {
        m_input >> c;
        m_text += c;

        if (c == '"')
            ScanStringLiteralInsideCodeBlock();
        else if (c == '\'')
        {
            try
            {
                ScanCharacterLiteral();
            }
            catch (Parser::Token::Type token_type)
            {
                if (token_type == Parser::Token::BAD_TOKEN)
                    EmitFatalError(FL, "malformed character literal");
                else
                    EmitFatalError(FL, "unterminated character literal");
                // this is moot because EmitFatalError throws
                return Parser::Token::BAD_TOKEN;
            }
        }
        else if (c == '/' && !IsNextCharEOF(&c) && (c == '/' || c == '*'))
        {
            try
            {
                ScanComment();
            }
            catch (Parser::Token::Type token_type)
            {
                EmitFatalError("unterminated code block");
                // this is moot because EmitFatalError throws
                return Parser::Token::BAD_TOKEN;
            }
        }
        else if (c == '{')
            ++bracket_level;
        else if (c == '}')
            --bracket_level;
        else if (c == '\n')
            ++m_line_number;
    }

    if (c == '}')
    {
        assert(bracket_level == 0);
        m_input >> c;
        *scanned_token = new AstCommon::StrictCodeBlock(FileLocation(m_input_filename, starting_line_number));
        static_cast<AstCommon::CodeBlock *>(*scanned_token)->AppendText(m_text);
        return Parser::Token::STRICT_CODE_BLOCK;
    }
    else
    {
        EmitFatalError(FL, "unterminated code block");
        // this is moot because EmitFatalError throws
        return Parser::Token::BAD_TOKEN;
    }
}

Parser::Token::Type Scanner::ScanDumbCodeBlock (AstCommon::Ast **const scanned_token)
{
    assert(scanned_token != NULL);
    assert(*scanned_token == NULL);
    assert(!m_input.eof());

    char c;
    Uint32 starting_line_number = m_line_number;

    m_text.clear();

    while (!IsNextCharEOF(&c))
    {
        if (c == '\n')
            ++m_line_number;

        if (c == '%')
        {
            m_input >> c;

            if (IsNextCharEOF(&c))
            {
                EmitFatalError(FL, "unterminated code block");
                // this is moot because EmitFatalError throws
                return Parser::Token::BAD_TOKEN;
            }
            m_input >> c;

            if (c == '}')
            {
                *scanned_token = new AstCommon::DumbCodeBlock(FileLocation(m_input_filename, starting_line_number));
                static_cast<AstCommon::CodeBlock *>(*scanned_token)->AppendText(m_text);
                return Parser::Token::DUMB_CODE_BLOCK;
            }
            else
            {
                m_text += '%';
                m_text += c;
            }
        }
        else
        {
            m_input >> c;
            m_text += c;
        }
    }

    EmitFatalError(FL, "unterminated code block");
    // this is moot because EmitFatalError throws
    return Parser::Token::BAD_TOKEN;
}

Parser::Token::Type Scanner::ScanStringLiteral (AstCommon::Ast **const scanned_token)
{
    assert(scanned_token != NULL);
    assert(*scanned_token == NULL);
    assert(!m_input.eof());

    char c;
    Uint32 starting_line_number = m_line_number;

    m_text.clear();

    while (!IsNextCharEOF(&c) && c != '"')
    {
        m_input >> c;

        if (c == '\\')
        {
            if (IsNextCharEOF(&c))
            {
                EmitFatalError(FL, "unterminated string");
                // this is moot because EmitFatalError throws
                return Parser::Token::BAD_TOKEN;
            }
            else if (c == '\n')
            {
                ++m_line_number;
                // ignore this escaped newline
                m_input >> c;
            }
            else
            {
                m_input >> c;
                m_text += GetEscapedChar(c);
            }
        }
        else if (c == '\n')
        {
            ++m_line_number;
            m_text += c;
        }
        else
        {
            m_text += c;
        }
    }

    if (c == '"')
    {
        m_input >> c;
        *scanned_token = new AstCommon::String(FileLocation(m_input_filename, starting_line_number));
        static_cast<AstCommon::String *>(*scanned_token)->AppendText(m_text);
        return Parser::Token::STRING;
    }
    else
    {
        EmitFatalError(FL, "unterminated string");
        // this is moot because EmitFatalError throws
        return Parser::Token::BAD_TOKEN;
    }
}

Parser::Token::Type Scanner::ScanCharacterLiteral (AstCommon::Ast **const scanned_token)
{
    assert(scanned_token != NULL);
    assert(*scanned_token == NULL);
    assert(!m_input.eof());

    m_text.clear();
    m_text += '\'';

    try
    {
        ScanCharacterLiteral();
    }
    catch (Parser::Token::Type token_type)
    {
        if (token_type == Parser::Token::BAD_TOKEN)
            EmitFatalError(FL, "malformed character literal");
        else
            EmitFatalError(FL, "unterminated character literal");
        // this is moot because EmitFatalError throws
        return Parser::Token::BAD_TOKEN;
    }

    if (m_text[1] == '\\')
    {
        assert(m_text.length() == 4);
        *scanned_token = new TokenIdentifierCharacter(GetEscapedChar(m_text[2]), FL);
    }
    else
    {
        assert(m_text.length() == 3);
        *scanned_token = new TokenIdentifierCharacter(m_text[1], FL);
    }

    return Parser::Token::TOKEN_IDENTIFIER_CHARACTER;
}

void Scanner::ScanStringLiteralInsideCodeBlock ()
{
    assert(!m_input.eof());

    char c;

    while (!IsNextCharEOF(&c) && c != '"')
    {
        m_input >> c;
        m_text += c;
        if (c == '\\')
        {
            if (IsNextCharEOF(&c))
                EmitFatalError(FL, "unterminated code block");
            m_input >> c;
            m_text += c;
            if (c == '\n')
                ++m_line_number;
        }
    }

    if (c != '"')
        EmitFatalError(FL, "unterminated code block");
    else
    {
        m_input >> c;
        m_text += c;
    }
}

void Scanner::ScanCharacterLiteral ()
{
    assert(!m_input.eof());

    char c;

    if (IsNextCharEOF(&c))
        throw Parser::Token::END_;

    if (c == '\\')
    {
        m_input >> c;
        m_text += c;

        if (IsNextCharEOF(&c))
            throw Parser::Token::END_;
    }

    if (c == '\n')
        throw Parser::Token::BAD_TOKEN;

    m_input >> c;
    m_text += c;

    if (IsNextCharEOF(&c))
        throw Parser::Token::END_;

    if (c == '\'')
    {
        m_input >> c;
        m_text += c;
    }
    else
        throw Parser::Token::BAD_TOKEN;
}

void Scanner::ScanComment ()
{
    assert(!m_input.eof());

    char c;

    if (IsNextCharEOF(&c))
        throw Parser::Token::BAD_TOKEN;
    else if (c == '*')
    {
        m_input >> c;
        m_text += c;
        while (!IsNextCharEOF(&c))
        {
            m_input >> c;
            m_text += c;
            if (c == '*')
            {
                if (IsNextCharEOF(&c))
                    throw Parser::Token::END_;
                else if (c == '/')
                {
                    m_input >> c;
                    m_text += c;
                    break;
                }
            }
            else if (c == '\n')
                ++m_line_number;
        }

        if (IsNextCharEOF())
            throw Parser::Token::END_;
    }
    else if (c == '/')
    {
        m_input >> c;
        m_text += c;
        while (!IsNextCharEOF(&c) && c != '\n')
        {
            m_input >> c;
            m_text += c;
        }

        if (IsNextCharEOF())
            throw Parser::Token::END_;
    }
    else
        throw Parser::Token::BAD_TOKEN;
}

} // end of namespace Trison
