// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY
// barf_preprocessor_parser.hpp generated by trison
// from barf_preprocessor_parser.trison using trison.cpp.targetspec and trison.cpp.header.codespec
// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY

#include <cassert>
#include <deque>
#include <iostream>

#if !defined(TrisonCpp_namespace_)
#define TrisonCpp_namespace_
namespace TrisonCpp_ {

// /////////////////////////////////////////////////////////////////////////////
// a bunch of template metaprogramming to intelligently determine what type to
// use for an integer of the given bit width or value range.
// /////////////////////////////////////////////////////////////////////////////

template <bool condition, typename Then, typename Else> struct If;
template <typename Then, typename Else> struct If<true,Then,Else> { typedef Then T; };
template <typename Then, typename Else> struct If<false,Then,Else> { typedef Else T; };

template <bool condition> struct Assert;
template <> struct Assert<true> { static bool const v = true; operator bool () { return v; } };

template <typename Sint, typename Uint> struct IntPair { typedef Sint S; typedef Uint U; };

template <int bits> struct Integer {
private:

    typedef
        typename If<bits == 8*sizeof(char),      IntPair<char,unsigned char>,
        typename If<bits == 8*sizeof(short),     IntPair<short,unsigned short>,
        typename If<bits == 8*sizeof(int),       IntPair<int,unsigned int>,
        typename If<bits == 8*sizeof(long),      IntPair<long,unsigned long>,
        typename If<bits == 8*sizeof(long long), IntPair<long long,unsigned long long>,
        Integer<0> // if no match, cause a compile error
        >::T >::T >::T >::T >::T PrivateIntPair;
    static bool const assert_size =
        Assert<bits == 8*sizeof(typename PrivateIntPair::S) &&
               bits == 8*sizeof(typename PrivateIntPair::U)>::v;

public:

    typedef typename PrivateIntPair::S Signed;
    typedef typename PrivateIntPair::U Unsigned;
};
template <> struct Integer<0> { }; // empty for intentional compile errors

typedef Integer<8> ::Signed                Sint8;
typedef Integer<8> ::Unsigned              Uint8;
typedef Integer<16>::Signed                Sint16;
typedef Integer<16>::Unsigned              Uint16;
typedef Integer<32>::Signed                Sint32;
typedef Integer<32>::Unsigned              Uint32;
typedef Integer<8*sizeof(void*)>::Signed   Diff;
typedef Integer<8*sizeof(void*)>::Unsigned Size;

enum
{
    TYPE_SIZE_ASSERTIONS =
        Assert<sizeof(Sint8)  == 1>::v &&
        Assert<sizeof(Uint8)  == 1>::v &&
        Assert<sizeof(Sint16) == 2>::v &&
        Assert<sizeof(Uint16) == 2>::v &&
        Assert<sizeof(Sint32) == 4>::v &&
        Assert<sizeof(Uint32) == 4>::v &&
        Assert<sizeof(Diff)   == sizeof(void*)>::v &&
        Assert<sizeof(Size)   == sizeof(void*)>::v
};

} // end of namespace TrisonCpp_
#endif // !defined(TrisonCpp_namespace_)


#line 18 "barf_preprocessor_parser.trison"

#if !defined(_BARF_PREPROCESSOR_PARSER_HPP_)
#define _BARF_PREPROCESSOR_PARSER_HPP_

#include "barf_preprocessor.hpp"

#include "barf_filoc.hpp"

namespace Barf {
namespace Ast {

class Base;

} // end of namespace Ast

namespace Preprocessor {

class Scanner;

#line 97 "barf_preprocessor_parser.hpp"

/** A parser class generated by trison
  * from barf_preprocessor_parser.trison using trison.cpp.targetspec and trison.cpp.header.codespec.
  *
  * The term "primary source" will be used to refer to the .trison source file from which
  * this file was generated.  Specifically, the primary source is barf_preprocessor_parser.trison.
  *
  * The term "client" will be used to refer to the programmer who is writing the trison
  * primary source file to generate a parser (e.g. "the client shouldn't return X from Y"
  * or "the client must provide a way to X and Y").
  *
  * @brief A parser class.
  */
class Parser
{
public:

    /// Return values for Parse().
    enum ParserReturnCode
    {
        /// Indicates the Parse() method returned successfully.
        PRC_SUCCESS = 0,
        /// Indicates an unhandled parse error occurred (i.e. no %error-accepting
        /// rules were encountered).
        PRC_UNHANDLED_PARSE_ERROR = 1
    }; // end of enum Parser::ParserReturnCode

    /// "Namespace" for Parser::Terminal::Name, which enumerates all valid
    /// token ids which this parser will accept as lookaheads.
    struct Terminal
    {
        /** There are two special terminals: END_ and ERROR_.
          *
          * Parser::Terminal::END_ should be returned in %target.cpp.scan_actions
          * by the client when the input source has reached the end of input.  The parser
          * will not request any more input after Parser::Terminal::END_ is received.
          *
          * Parser::Terminal::ERROR_ should not ever be used by the client, as
          * it is used internally by the parser.
          *
          * The rest are the terminals as declared in the primary source, and should
          * be used by the client when returning from %target.cpp.scan_actions.
          *
          * @brief Acceptable values returnable to the parser in %target.cpp.scan_actions.
          */
        enum Name
        {
            END_ = 256,
            ERROR_ = 257,
            TEXT = 258,
            START_CODE = 259,
            END_CODE = 260,
            CODE_LINE = 261,
            CODE_NEWLINE = 262,
            ID = 263,
            DUMP_SYMBOL_TABLE = 264,
            IF = 265,
            ELSE = 266,
            ELSE_IF = 267,
            END_IF = 268,
            UNDEFINE = 269,
            DECLARE_ARRAY = 270,
            DECLARE_MAP = 271,
            DEFINE = 272,
            END_DEFINE = 273,
            LOOP = 274,
            END_LOOP = 275,
            FOR_EACH = 276,
            END_FOR_EACH = 277,
            INCLUDE = 278,
            SANDBOX_INCLUDE = 279,
            WARNING = 280,
            ERROR = 281,
            FATAL_ERROR = 282,
            SIZEOF = 283,
            IS_DEFINED = 284,
            INTEGER_LITERAL = 285,
            STRING_LITERAL = 286,
            KEYWORD_INT = 287,
            KEYWORD_STRING = 288,
            STRING_LENGTH = 289,
            TO_CHARACTER_LITERAL = 290,
            TO_STRING_LITERAL = 291,
            BAD_TOKEN = 292
        }; // end of enum Parser::Terminal::Name
    }; // end of struct Parser::Terminal

    /// "Namespace" for Parser::ParseNonterminal::Name, which enumerates
    /// all valid nonterminals this parser can recognize.
    struct ParseNonterminal
    {
        /** The Parse() method doesn't necessarily have to start with the
          * value of the %default_parse_nonterminal directive; it can
          * attempt to parse any of the nonterminals defined in the primary
          * source file.  These enums are the way to specify which nonterminal
          * to parse.
          *
          * @brief Acceptable nonterminals recognizable by this parser.
          */
        enum Name
        {
            body = 0,
            code = 257,
            code_body = 261,
            conditional_series = 263,
            conditional_series_end = 267,
            define = 285,
            define_array_element = 299,
            define_map_element = 311,
            define_scalar = 289,
            else_if_statement = 277,
            else_statement = 273,
            end_define = 323,
            end_for_each = 339,
            end_if = 281,
            end_loop = 331,
            executable = 255,
            expression = 343,
            for_each = 335,
            if_statement = 271,
            loop = 327,
            /// Nonterminal which will be attempted to be parsed by the Parse()
            /// method by default (specified by the %default_parse_nonterminal
            /// directive).
            default_ = body
        }; // end of enum Parser::ParseNonterminal::Name
    }; // end of struct Parser::ParseNonterminal

    /** The client should package-up and return a Parser::Token from
      * the code specified by %target.cpp.scan_actions, which delivers the
      * token type and token data to the parser for input.  The constructor
      * takes one or two parameters; the second can be omitted, indicating
      * that the %target.cpp.token_data_default value will be used.
      *
      * @brief Return type for %target.cpp.scan_actions.
      */
    struct Token
    {
        typedef TrisonCpp_::Uint32 Id; // TODO -- smallest int
        typedef Ast::Base * Data;

        Id m_id;
        Data m_data;

        /** @param id Gives the token id, e.g. Terminal::END_ or whatever
          *        other terminals were declared in the primary source.
          * @param data Gives the data associated with this token, e.g. if
          *        you were constructing an AST, data would point to an AST
          *        node constructed during scanning.
          *
          * @brief Constructor for Token struct.
          */
        Token (Id id, Data const &data = NULL)
            :
            m_id(id),
            m_data(data)
        {
            assert(m_id != Nonterminal_::none_ &&
                   m_id != Terminal::ERROR_ &&
                   "do not construct a token with this id");
        }
    }; // end of struct Parser::Token

public:

    /// Constructor.  The client can specify parameters in the primary source
    /// via the %target.cpp.constructor_parameters directive.
    Parser ();
    /// Destructor.  The client can force the destructor to be declared virtual
    /// by specifying the %target.cpp.force_virtual_destructor directive in the
    /// primary source.
    ~Parser ();

    /// Returns true if and only if "debug spew" is enabled (which prints, to
    /// std::cerr, exactly what the parser is doing at each step).  This method,
    /// along with all other debug spew code can be removed by removing the
    /// %target.cpp.generate_debug_spew_code directive from the primary source.
    bool DebugSpew () const { return m_debug_spew_; }
    /// Sets the debug spew flag (see DebugSpew()).  This method,
    /// along with all other debug spew code can be removed by removing the
    /// %target.cpp.generate_debug_spew_code directive from the primary source.
    void DebugSpew (bool debug_spew) { m_debug_spew_ = debug_spew; }

    /** This parser is capable of attempting multiple contiguous parses from the
      * same input source.  The lookahead queue is preserved between calls to
      * Parse().  Therefore, if the input source changes, the lookahead queue
      * must be cleared so that the new input source can be read.  The client
      * must call this method if the input source changes.
      *
      * @brief This method must be called if the input source changes.
      */
    void ResetForNewInput ();

    /** The %target.cpp.parse_method_access directive can be used to specify the
      * access level of this method.
      *
      * The %target.cpp.start_of_parse_method_actions and
      * %target.cpp.end_of_parse_method_actions directives can be used to specify
      * code to execute at the beginning and end, respectively, of the Parse() method.
      * This includes the ability to enclose the body of the Parse() method within a
      * try {} block, for exception handling (if exceptions are thrown in reduction
      * rule code, then the %target.cpp.enable_parse_method_exception_handling directive
      * must be specified; this will cause the parser to catch and rethrow any exceptions
      * thrown in reduction rule code, allowing it to clean up dynamically allocated
      * memory, etc.
      *
      * @param return_token A pointer to the value which will be assigned to upon
      *        successfully parsing the requested nonterminal. If the parse fails,
      *        the value of the %target.cpp.token_data_default directive will
      *        be assigned.
      * @param nonterminal_to_parse The Parse() method can attempt to parse any
      *        nonterminal declared in the primary source.  If unspecified, the
      *        Parse() method will attempt to parse the nonterminal specified by the
      *        %default_parse_nonterminal directive.
      * @return Parser::PRC_SUCCESS if the parse was successful (which includes
      *         occurrences of parse errors which were handled by client-specified
      *         %error-accepting rules), or Parser::PRC_UNHANDLED_PARSE_ERROR
      *         if a parse error was not handled by any %error-accepting rules.
      *
      * @brief This is the main method of the parser; it will attempt to parse
      *        the nonterminal specified.
      */
    ParserReturnCode Parse (Ast::Base * *return_token, ParseNonterminal::Name nonterminal_to_parse = ParseNonterminal::body);


#line 38 "barf_preprocessor_parser.trison"

    FiLoc const &GetFiLoc () const;

    bool ScannerDebugSpew () const { /* TODO: implement */ return false; }
    void ScannerDebugSpew (bool debug_spew) { /* TODO: implement */ }

    bool OpenFile (string const &input_filename);
    void OpenString (string const &input_string, string const &input_name, bool use_line_numbers = false);
    void OpenUsingStream (istream *input_stream, string const &input_name, bool use_line_numbers);

private:

    Scanner *m_scanner;

#line 338 "barf_preprocessor_parser.hpp"


private:

    // ///////////////////////////////////////////////////////////////////////
    // begin internal trison-generated parser guts -- don't use
    // ///////////////////////////////////////////////////////////////////////

    struct Nonterminal_
    {
        enum Name
        {
            none_ = 0,
            body = 293,
            executable = 294,
            code = 295,
            code_body = 296,
            conditional_series = 297,
            conditional_series_end = 298,
            if_statement = 299,
            else_statement = 300,
            else_if_statement = 301,
            end_if = 302,
            define = 303,
            define_scalar = 304,
            define_array_element = 305,
            define_map_element = 306,
            end_define = 307,
            loop = 308,
            end_loop = 309,
            for_each = 310,
            end_for_each = 311,
            expression = 312
        }; // end of enum Parser::Nonterminal_::Name
    }; // end of struct Parser::Nonterminal_
    struct Transition_;

    struct StackElement_
    {
        TrisonCpp_::Uint32 m_state_index;
        Token::Data m_token_data;

        StackElement_ ()
            :
            m_state_index(TrisonCpp_::Uint32(-1)),
            m_token_data(NULL)
        { }
        StackElement_ (TrisonCpp_::Uint32 state_index, Token::Data token_data)
            :
            m_state_index(state_index),
            m_token_data(token_data)
        { }
    }; // end of struct Parser::StackElement_

    typedef std::deque<StackElement_> Stack_;
    typedef std::deque<Token> LookaheadQueue_;

    ParserReturnCode Parse_ (Ast::Base * *return_token, ParseNonterminal::Name nonterminal_to_parse);
    void ThrowAwayToken_ (Token::Data &token_data) throw();
    void ResetForNewInput_ () throw();
    Token Scan_ () throw();
    void ClearStack_ () throw();
    void ClearLookaheadQueue_ () throw();
    Token const &Lookahead_ (LookaheadQueue_::size_type index) throw();
    bool ExerciseTransition_ (Transition_ const &transition);
    Token::Data ExecuteReductionRule_ (TrisonCpp_::Uint32 const rule_index_);
    // debug spew methods
    void PrintParserStatus_ (std::ostream &stream) const;
    void PrintIndented_ (std::ostream &stream, char const *string) const;

    struct Rule_
    {
        Token::Id m_reduction_nonterminal_token_id;
        TrisonCpp_::Uint32 m_token_count;
        char const *m_description;
    }; // end of struct Parser::Rule_

    struct State_
    {
        TrisonCpp_::Size const m_transition_count; // TODO: smallest int
        Transition_ const *m_transition_table;
        char const *m_description;
    }; // end of struct Parser::State_

    struct Transition_
    {
        enum Type { TT_REDUCE = 1, TT_RETURN, TT_SHIFT, TT_ERROR_PANIC };
        TrisonCpp_::Uint8 m_type;
        TrisonCpp_::Uint32 m_data; // TODO: smallest int
        TrisonCpp_::Uint32 m_lookahead_count; // TODO smallest int
        Token::Id const *m_lookahead_sequence;
    }; // end of struct Parser::Transition_

    Stack_ m_stack_;
    LookaheadQueue_ m_lookahead_queue_;
    bool m_is_in_error_panic_;
    bool m_debug_spew_;

    static Rule_ const ms_rule_table_[];
    static TrisonCpp_::Size const ms_rule_count_;
    static State_ const ms_state_table_[];
    static TrisonCpp_::Size const ms_state_count_;
    static Transition_ const ms_transition_table_[];
    static TrisonCpp_::Size const ms_transition_count_;
    static Token::Id const ms_lookahead_table_[];
    static TrisonCpp_::Size const ms_lookahead_count_;
    static char const *const ms_token_name_table_[];
    static TrisonCpp_::Size const ms_token_name_count_;

    friend std::ostream &operator << (std::ostream &stream, Parser::Token const &token);

    // ///////////////////////////////////////////////////////////////////////
    // end of internal trison-generated parser guts
    // ///////////////////////////////////////////////////////////////////////
}; // end of class Parser

std::ostream &operator << (std::ostream &stream, Parser::Token const &token);


#line 52 "barf_preprocessor_parser.trison"

} // end of namespace Preprocessor
} // end of namespace Barf

#endif // !defined(_BARF_PREPROCESSOR_PARSER_HPP_)

#line 465 "barf_preprocessor_parser.hpp"
