// ///////////////////////////////////////////////////////////////////////////
// barf_preprocessor_scanner.hpp by Victor Dods, created 2006/10/15
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_PREPROCESSOR_SCANNER_HPP_)
#define _BARF_PREPROCESSOR_SCANNER_HPP_

#include "barf_preprocessor.hpp"

#include <fstream>
#include <vector>

#include "barf_filelocation.hpp"
#include "barf_inputbase.hpp"
#include "barf_preprocessor_parser.hpp"

namespace Barf {
namespace AstCommon {

class Ast;

} // end of namespace AstCommon

namespace Preprocessor {

class Scanner : protected InputBase
{
public:

    Scanner ();
    ~Scanner ();

    using InputBase::GetIsOpen;
    using InputBase::GetFileLocation;

    inline string const &GetStartCodeDelimiter () const { return m_start_code_delimiter; }
    inline string const &GetEndCodeDelimiter () const { return m_end_code_delimiter; }
    inline string const &GetCodeLineDelimiter () const { return m_code_line_delimiter; }

    void SetStartCodeDelimiter (string const &start_code_delimiter);
    void SetEndCodeDelimiter (string const &end_code_delimiter);
    void SetCodeLineDelimiter (string const &code_line_delimiter);

    bool OpenFile (string const &input_filename);
    void OpenString (string const &input_string, string const &input_name, bool use_line_numbers = false);
    void OpenUsingStream (istream *input_stream, string const &input_name, bool use_line_numbers);

    bool Close ();

    Parser::Token::Type Scan (AstCommon::Ast **scanned_token);

protected:

    void HandleOpened ();
    void HandleClosed ();

private:

    static inline bool IsWhitespace (char c) { return c == ' ' || c == '\t' || c == '\n'; }
    static inline bool IsBracket (char c) { return c == '(' || c == ')' || c == '[' || c == ']'; }
    static inline bool IsOperator (char c) { return c == ',' || c == '.' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '!' || c == '=' || c == '<' || c == '>' || c == '|' || c == '&' || c == '?'; }
    static inline bool IsAlpha (char c) { return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_'; }
    static inline bool IsDigit (char c) { return c >= '0' && c <= '9'; }

    static char GetMatchingBracket (char c)
    {
        switch (c)
        {
            case '(': return ')';
            case ')': return '(';
            case '[': return ']';
            case ']': return '[';
            // case '{': return '}'; // not used
            // case '}': return '{'; // not used
            default : return c;
        }
    }

    Parser::Token::Type ScanBody (AstCommon::Ast **scanned_token);
    Parser::Token::Type ScanCode (AstCommon::Ast **scanned_token);

    enum State
    {
        READING_BODY = 0,
        READING_CODE
    }; // end of enum Scanner::State

    string m_text;
    string::size_type m_current_position;
    State m_state;
    bool m_is_reading_code_line;
    string m_start_code_delimiter;
    string m_end_code_delimiter;
    string m_code_line_delimiter;
    Uint32 m_bracket_level;
    vector<char> m_bracket;
}; // end of class Scanner

} // end of namespace Preprocessor
} // end of namespace Barf

#endif // !defined(_BARF_PREPROCESSOR_SCANNER_HPP_)
