// ///////////////////////////////////////////////////////////////////////////
// trison_preprocessor.cpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "trison_preprocessor.hpp"

#include <fstream>

#include "barf_util.hpp"
#include "trison_options.hpp"

namespace Trison {

Preprocessor::Preprocessor ()
{
    m_was_successful = true;
}

void Preprocessor::SetReplacementValue (
    Replacement replacement,
    string const &replacement_value)
{
    assert(replacement < REPLACEMENT_COUNT);
    m_replacement_value[replacement] = replacement_value;
}

string Preprocessor::ProcessString (string const &string_to_process, string const &filename) const
{
    assert(!filename.empty());

    string processed_string(string_to_process);

    for (Uint32 i = 0; i < REPLACEMENT_COUNT; ++i)
    {
        Replacement replacement = Replacement(i);
        ReplaceAllInString(
            &processed_string,
            GetReplacementIdentifier(replacement),
            GetReplacementValue(replacement));
    }

    // add in the returning #line directives.
    // yes, it's slow, but on the other hand, shut the hell up!
    {
        Uint32 pos = 0;
        Uint32 line = 1;
        while (pos < processed_string.length())
        {
            if (processed_string[pos] == '\n')
            {
                ++line;
                ++pos;
            }
            else if (processed_string.substr(pos, GetReturningLineDirectiveTag().length())
                     ==
                     GetReturningLineDirectiveTag())
            {
                assert(GetOptions()->GetWithLineDirectives());
                FiLoc filoc(filename, line+2);
                string line_directive("\n" + filoc.GetLineDirectiveString() + "\n");
                assert(line_directive[line_directive.length() - 1] == '\n');
                line_directive.erase(line_directive.length() - 1, 1);
                processed_string.replace(pos, GetReturningLineDirectiveTag().length(), line_directive);
                pos += line_directive.length();
                assert(GetNewlineCount(line_directive) == 1);
                line += 1;
            }
            else
                ++pos;
        }
    }

    m_was_successful = true;
    return processed_string;
}

string Preprocessor::ProcessFile (string const &filename) const
{
    ifstream input_file(filename.c_str());

    if (!input_file)
    {
        m_was_successful = false;
        return string("");
    }
    else
    {
        char input_buffer[MAX_FILE_LENGTH + 1];
        string file_contents;

        input_file.get(input_buffer, MAX_FILE_LENGTH + 1, '\0');
        assert(input_file.gcount() <= MAX_FILE_LENGTH);
        assert(input_buffer[input_file.gcount()] == '\0');
        input_file.close();

        m_was_successful = true;
        return ProcessString(string(input_buffer), filename);
    }
}

string const &Preprocessor::GetReplacementIdentifier (Replacement replacement) const
{
    static string const s_replacement_identifier[REPLACEMENT_COUNT] =
    {
        "$$HEADER_FILE_TOP$$",
        "$$HEADER_FILE_BOTTOM$$",
        "$$CLASS_NAME$$",
        "$$CLASS_INHERITANCE$$",
        "$$SUPERCLASS_AND_MEMBER_CONSTRUCTORS$$",
        "$$FORCE_VIRTUAL_DESTRUCTOR$$",
        "$$CONSTRUCTOR_ACTIONS$$",
        "$$DESTRUCTOR_ACTIONS$$",
        "$$PARSE_METHOD_ACCESS$$",
        "$$CLASS_METHODS_AND_MEMBERS$$",
        "$$START_OF_PARSE_METHOD_ACTIONS$$",
        "$$END_OF_PARSE_METHOD_ACTIONS$$",
        "$$THROW_AWAY_TOKEN_ACTIONS$$",
        "$$IMPLEMENTATION_FILE_TOP$$",
        "$$IMPLEMENTATION_FILE_BOTTOM$$",
        "$$BASE_ASSIGNED_TYPE$$",
        "$$BASE_ASSIGNED_TYPE_SENTINEL$$",
        "$$CUSTOM_CAST$$",
        "$$TERMINAL_TOKEN_DECLARATIONS$$",
        "$$NONTERMINAL_TOKEN_DECLARATIONS$$",
        "$$TERMINAL_TOKEN_STRINGS$$",
        "$$NONTERMINAL_TOKEN_STRINGS$$",
        "$$REDUCTION_RULE_HANDLER_DECLARATIONS$$",
        "$$REDUCTION_RULE_HANDLER_DEFINITIONS$$",
        "$$REDUCTION_RULE_LOOKUP_TABLE$$",
        "$$STATE_TRANSITION_LOOKUP_TABLE$$",
        "$$STATE_TRANSITION_TABLE$$",
        "$$HEADER_FILENAME$$"
    };

    assert(replacement < REPLACEMENT_COUNT);
    return s_replacement_identifier[replacement];
}

string const &Preprocessor::GetReplacementValue (Replacement replacement) const
{
    assert(replacement < REPLACEMENT_COUNT);
    return m_replacement_value[replacement];
}

} // end of namespace Trison
