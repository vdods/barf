// ///////////////////////////////////////////////////////////////////////////
// trison_preprocessor.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_PREPROCESSOR_HPP_)
#define _TRISON_PREPROCESSOR_HPP_

#include "trison.hpp"

#include "trison_enums.hpp"

namespace Trison {

class Preprocessor
{
public:

    enum
    {
        MAX_FILE_LENGTH = 0x40000 - 1
    };

    enum Replacement
    {
        HEADER_FILE_TOP = 0,
        HEADER_FILE_BOTTOM,
        CLASS_NAME,
        CLASS_INHERITANCE,
        SUPERCLASS_AND_MEMBER_CONSTRUCTORS,
        FORCE_VIRTUAL_DESTRUCTOR,
        CONSTRUCTOR_ACTIONS,
        DESTRUCTOR_ACTIONS,
        PARSE_METHOD_ACCESS,
        CLASS_METHODS_AND_MEMBERS,
        START_OF_PARSE_METHOD_ACTIONS,
        END_OF_PARSE_METHOD_ACTIONS,
        THROW_AWAY_TOKEN_ACTIONS,
        IMPLEMENTATION_FILE_TOP,
        IMPLEMENTATION_FILE_BOTTOM,
        BASE_ASSIGNED_TYPE,
        BASE_ASSIGNED_TYPE_SENTINEL,
        CUSTOM_CAST,
        TERMINAL_TOKEN_DECLARATIONS,
        NONTERMINAL_TOKEN_DECLARATIONS,
        TERMINAL_TOKEN_STRINGS,
        NONTERMINAL_TOKEN_STRINGS,
        REDUCTION_RULE_HANDLER_DECLARATIONS,
        REDUCTION_RULE_HANDLER_DEFINITIONS,
        REDUCTION_RULE_LOOKUP_TABLE,
        STATE_TRANSITION_LOOKUP_TABLE,
        STATE_TRANSITION_TABLE,
        HEADER_FILENAME,

        REPLACEMENT_COUNT
    }; // end of enum Replacement

    Preprocessor ();

    inline bool GetWasSuccessful () const { return m_was_successful; }
    static Replacement GetReplacementFromParserDirectiveType (
        ParserDirectiveType parser_directive_type)
    {
        assert(parser_directive_type < PDT_COUNT);
        return Replacement(parser_directive_type);
    }
    static inline string const &GetReturningLineDirectiveTag ()
    {
        static string const s_returning_line_directive_tag("$$RETURNING_LINE_DIRECTIVE$$");
        return s_returning_line_directive_tag;
    }
    string const &GetReplacementValue (Replacement replacement) const;

    void SetReplacementValue (Replacement replacement, string const &replacement_value);

    string ProcessString (string const &string_to_process, string const &filename) const;
    string ProcessFile (string const &filename) const;

private:

    string const &GetReplacementIdentifier (Replacement replacement) const;

    string m_replacement_value[REPLACEMENT_COUNT];
    mutable bool m_was_successful;
}; // end of class Preprocessor

} // end of namespace Trison

#endif // !defined(_TRISON_PREPROCESSOR_HPP_)
