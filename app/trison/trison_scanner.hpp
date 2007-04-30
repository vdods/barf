// ///////////////////////////////////////////////////////////////////////////
// trison_scanner.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_SCANNER_HPP_)
#define _TRISON_SCANNER_HPP_

#include "trison.hpp"

#include <fstream>

#include "trison_parser.hpp"

namespace Barf {
namespace AstCommon {

class Ast;

} // end of namespace AstCommon
} // end of namespace Barf

namespace Trison {

class Scanner
{
public:

    Scanner ();
    ~Scanner ();

    inline string const &GetInputFilename () const { return m_input_filename; }
    inline Uint32 GetLineNumber () const { return m_line_number; }

    bool Open (string const &input_filename);
    void Close ();
    Parser::Token::Type Scan (AstCommon::Ast **scanned_token);

private:

    Parser::Token::Type ScanId (AstCommon::Ast **scanned_token);
    Parser::Token::Type ScanOperator (AstCommon::Ast **scanned_token);
    Parser::Token::Type ScanDirective (AstCommon::Ast **scanned_token);
    Parser::Token::Type ScanStrictCodeBlock (AstCommon::Ast **scanned_token);
    Parser::Token::Type ScanDumbCodeBlock (AstCommon::Ast **scanned_token);
    Parser::Token::Type ScanStringLiteral (AstCommon::Ast **scanned_token);
    Parser::Token::Type ScanCharacterLiteral (AstCommon::Ast **scanned_token);

    void ScanStringLiteralInsideCodeBlock ();
    void ScanCharacterLiteralInsideCodeBlock ();
    void ScanCharacterLiteral ();
    void ScanComment ();

    inline bool IsNextCharEOF (char *c = NULL)
    {
        if (c != NULL)
            *c = m_input.peek();
        return m_input.peek() == EOF;
    }

    static inline bool IsWhitespace (char c) { return c == ' ' || c == '\t'; }
    static inline bool IsOperator (char c) { return c == ':' || c == ';' || c == '|'; }
    static inline bool IsAlpha (char c) { return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z'; }
    static inline bool IsLowercaseAlpha (char c) { return c >= 'a' && c <= 'z'; }
    static inline bool IsDigit (char c) { return c >= '0' && c <= '9'; }
    static inline bool IsEscapableCharacter (char c) { return c == '0' || c == 'a' || c == 'b' || c == 't' || c == 'n' || c == 'v' || c == 'f' || c == 'r' || c == '\\' || c == '"'; }

    string m_input_filename;
    ifstream m_input;
    string m_text;
    Uint32 m_line_number;
    bool m_in_preamble;
}; // end of class Scanner

} // end of namespace Trison

#endif // !defined(_TRISON_SCANNER_HPP_)
