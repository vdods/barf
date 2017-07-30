// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY
// Parser.hpp generated by trison
// from Parser.trison using trison.cpp.targetspec and trison.cpp.header.codespec
// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY

#pragma once


#include <cassert>
#include <cstdint>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <utility>
#include <vector>


#line 12 "Parser.trison"

#include <iostream>

class Scanner;

#line 26 "Parser.hpp"

/** A parser class generated by trison
  * from Parser.trison using trison.cpp.targetspec and trison.cpp.header.codespec.
  *
  * The term "primary source" will be used to refer to the .trison source file from which
  * this file was generated.  Specifically, the primary source is Parser.trison.
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
        PRC_UNHANDLED_PARSE_ERROR = 1,
        /// Indicates that the parse didn't complete because of some internal error.
        PRC_INTERNAL_ERROR = 2
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
            NUM = 258,
            BAD_TOKEN = 259
        }; // end of enum Parser::Terminal::Name
    }; // end of struct Parser::Terminal

    /// "Namespace" for Parser::Nonterminal::Name, which enumerates all nonterminals.
    /// This is used internally by the parser, but is also used by the client to specify which
    /// nonterminal should be parsed by the parser.
    struct Nonterminal
    {
        /** There is one special nonterminal: none_.  This should not be used by the client,
          * as it is only used internally by the parser.
          */
        enum Name
        {
            none_ = 0,
            statement_then_end = 260,
            statement = 261,
            expression = 262
        }; // end of enum Parser::Nonterminal::Name
    }; // end of struct Parser::Nonterminal

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
        typedef std::uint32_t Id; // TODO -- smallest int
        typedef double Data;

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
        Token (Id id, Data const &data = 0.0) : m_id(id), m_data(data) { }
    }; // end of struct Parser::Token

public:

    /// Constructor.  The client can specify parameters in the primary source
    /// via the %target.cpp.constructor_parameters directive.
    Parser ();
    /// Destructor.  The client can force the destructor to be declared virtual
    /// by specifying the %target.cpp.force_virtual_destructor directive in the
    /// primary source.
    ~Parser ();

    /** It is not sufficient to just check the EOF condition on the input
      * source (e.g. the scanner, cin, etc), because the parser may have read,
      * but not consumed, additional lookaheads up to EOF.  Thus checking
      * the input source for EOF condition may give false positives.  This
      * method should be the preferred means to check EOF condition.
      *
      * It should be noted that this may cause the parser to read (but never
      * consume) up to one additional lookahead token, owing to the necessity
      * of checking what the next lookahead token is.
      *
      * @brief Returns true if and only if the next unshifted lookahead
      *        token is Terminal::END_.
      */
    bool IsAtEndOfInput ();

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
      * The %target.cpp.top_of_parse_method_actions and
      * %target.cpp.bottom_of_parse_method_actions directives can be used to specify
      * code to execute at the beginning and end, respectively, of the Parse() method.
      * This includes the ability to enclose the body of the Parse() method within a
      * try {} block, for exception handling (if exceptions are thrown in scan_actions
      * or any reduction rule code, then the %target.cpp.enable_scan_actions_exceptions
      * or %target.cpp.enable_reduction_rule_exceptions directives must be specified
      * respectively; this will cause the parser to catch and rethrow any exceptions
      * thrown by scan_actions or reduction rule code, allowing it to clean up
      * dynamically allocated memory, etc.
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
    ParserReturnCode Parse (double *return_token, Nonterminal::Name nonterminal_to_parse = Nonterminal::statement_then_end);


#line 19 "Parser.trison"

    void attach_istream (std::istream &in);

private:

    Scanner *m_scanner;

#line 213 "Parser.hpp"


private:

    // ///////////////////////////////////////////////////////////////////////
    // begin internal trison-generated parser guts -- don't use
    // ///////////////////////////////////////////////////////////////////////

    struct Rule_;
    struct State_;
    struct Transition_;

    struct StackElement_
    {
        std::uint32_t m_state_index;
        Token m_token;

        StackElement_ ()
            :
            m_state_index(std::uint32_t(-1)),
            m_token(Nonterminal::none_, 0.0)
        { }
        StackElement_ (std::uint32_t state_index, Token const &token)
            :
            m_state_index(state_index),
            m_token(token)
        { }

        bool operator == (StackElement_ const &other) const { return m_state_index == other.m_state_index; }
    }; // end of struct Parser::StackElement_

    typedef std::deque<StackElement_> Stack_;
    typedef std::deque<Token> LookaheadQueue_;

    // debug spew methods
    void PrintIndented_ (std::ostream &stream, char const *string) const;

    bool m_debug_spew_;

    static char const *const ms_token_name_table_[];
    static std::size_t const ms_token_name_count_;

    static std::uint32_t NonterminalStartStateIndex_ (Nonterminal::Name nonterminal);
    ParserReturnCode Parse_ (double *return_token, Nonterminal::Name nonterminal_to_parse);
    void ThrowAwayToken_ (Token &token) throw();
    void ThrowAwayStackElement_ (StackElement_ &stack_element) throw();
    void ThrowAwayTokenData_ (double &token_data) throw();
    void ResetForNewInput_ () throw();
    Token Scan_ () throw();
    // debug spew methods
    void PrintParserStatus_ (std::ostream &out) const;

private:

    void ExecuteAndRemoveTrunkActions_ (bool &should_return, ParserReturnCode &parser_return_code, double *&return_token);
    void ContinueNPDAParse_ (bool &should_return);
    Token::Data ExecuteReductionRule_ (std::uint32_t const rule_index_, Stack_ &stack) throw();

    struct Precedence_
    {
        std::int32_t m_level; // default precedence is 0, higher values have higher precedence.
        std::uint32_t m_associativity_index; // 0 for %left, 1 for %nonassoc, and 2 for %right.
        char const *m_name;
    };

    struct Rule_
    {
        Token::Id m_reduction_nonterminal_token_id;
        std::uint32_t m_token_count;
        std::uint32_t m_precedence_index;
        char const *m_description;
    }; // end of struct Parser::Rule_

    struct State_
    {
        std::size_t m_transition_count; // TODO: smallest int
        Transition_ const *m_transition_table;
        std::uint32_t m_associated_rule_index;
        char const *m_description;
    }; // end of struct Parser::State_

    struct Transition_
    {
        enum Type { RETURN = 1, REDUCE, SHIFT, INSERT_LOOKAHEAD_ERROR, DISCARD_LOOKAHEAD, POP_STACK, EPSILON };
        std::uint8_t m_type;
        std::uint32_t m_token_index; // TODO: smallest int
        std::uint32_t m_data_index; // TODO: smallest int

        // Lexicographic ordering on the tuple (m_type, m_token_index, m_data_index).
        struct Order
        {
            static std::uint32_t SortedTypeIndex (Type type)
            {
                switch (type)
                {
                    case REDUCE:
                    case SHIFT:
                        return 0;

                    case DISCARD_LOOKAHEAD:
                    case POP_STACK:
                        return 1;

                    case RETURN:
                        return 2;

                    case INSERT_LOOKAHEAD_ERROR:
                    case EPSILON:
                        return 3;
                }
            }

            bool operator () (Transition_ const &lhs, Transition_ const &rhs) const
            {
                std::uint32_t sorted_type_index_lhs = SortedTypeIndex(Type(lhs.m_type));
                std::uint32_t sorted_type_index_rhs = SortedTypeIndex(Type(rhs.m_type));
                if (sorted_type_index_lhs != sorted_type_index_rhs)
                    return sorted_type_index_lhs < sorted_type_index_rhs;
                else if (lhs.m_type != rhs.m_type)
                    return lhs.m_type < rhs.m_type;
                else if (lhs.m_token_index != rhs.m_token_index)
                    return lhs.m_token_index < rhs.m_token_index;
                else
                    return lhs.m_data_index < rhs.m_data_index;
            }
        };
    }; // end of struct Parser::Transition_

    typedef std::set<std::uint32_t> StateSet_;
    typedef std::vector<std::uint32_t> StateVector_;
    typedef std::set<Transition_,Transition_::Order> TransitionSet_;
    typedef std::vector<Transition_> TransitionVector_;

    struct Npda_;

    struct ParseStackTreeNode_
    {
        // The values of RETURN through POP_STACK coincide with the same in Transition_::Type.
        // Note: HPS stands for "Hypothetical Parser State", which represents one of possibly many
        // ways the non-deterministic parser can parse the input.
        // TODO: probably order this so that the Spec::Order gives an obvious way to do error handling action last
        enum Type { ROOT = 0, RETURN, REDUCE, SHIFT, INSERT_LOOKAHEAD_ERROR, DISCARD_LOOKAHEAD, POP_STACK, HPS, COUNT_ };
        static std::uint32_t const UNUSED_DATA = std::uint32_t(-1);

        struct Spec
        {
            Type m_type;
            // Only used by REDUCE, POP_STACK.
            std::uint32_t m_single_data;

            Spec (Type type, std::uint32_t single_data = UNUSED_DATA)
                : m_type(type)
                , m_single_data(single_data)
            {
                if (m_type != REDUCE && m_type != POP_STACK)
                {
                    assert(m_single_data == UNUSED_DATA);
                }
            }

            // Lexicographic ordering on the tuple (m_type, m_single_data).
            struct Order
            {
                bool operator () (Spec const &lhs, Spec const &rhs) const
                {
                    if (lhs.m_type != rhs.m_type)
                        return lhs.m_type < rhs.m_type;

                    switch (lhs.m_type) // Note that lhs.m_type == rhs.m_type at this point.
                    {
                        case POP_STACK:
                            return lhs.m_single_data < rhs.m_single_data;

                        case REDUCE:
                            // assert(lhs.m_single_data != UNUSED_DATA);
                            // assert(rhs.m_single_data != UNUSED_DATA);
                            return false;

                        default:
                            assert(lhs.m_single_data == UNUSED_DATA);
                            assert(rhs.m_single_data == UNUSED_DATA);
                            return false;
                    }
                }
            };
        };

        static char const *AsString (Type type);


        struct ParseStackTreeNodeOrder
        {
            bool operator () (ParseStackTreeNode_ const *lhs, ParseStackTreeNode_ const *rhs) const;
        };

        typedef std::set<ParseStackTreeNode_ *,ParseStackTreeNodeOrder> ParseStackTreeNodeSet;
        typedef std::map<Spec,ParseStackTreeNodeSet,Spec::Order> ChildMap;
        typedef std::pair<std::int32_t,std::int32_t> PrecedenceLevelRange;

        Spec m_spec;
        Stack_ m_stack;
        // m_local_lookahead_queue comes before the "global" lookahead queue, and m_global_lookahead_cursor
        // is the index into the "global" lookahead queue for where the end of m_local_lookahead_queue
        // lands.  In other words, this node's "total" lookahead
        LookaheadQueue_ m_local_lookahead_queue;
        std::uint32_t m_global_lookahead_cursor; // this is an index into the "global" lookahead queue.
        // StateSet_ m_top_states; // used for infinite loop detection
        ParseStackTreeNode_ *m_parent_node;
        ChildMap m_child_nodes;

        ParseStackTreeNode_ (Spec const &spec)
            : m_spec(spec)
            , m_global_lookahead_cursor(0)
            , m_parent_node(NULL)
        { }
        ~ParseStackTreeNode_ ();

        bool HasTrunkChild () const;
        ParseStackTreeNode_ *PopTrunkChild ();
        bool HasChildrenHavingSpec (Spec const &spec) const { return m_child_nodes.find(spec) != m_child_nodes.end(); }
        ParseStackTreeNodeSet const &ChildrenHavingSpec (Spec const &spec) const { return m_child_nodes.at(spec); }
        ParseStackTreeNodeSet &ChildrenHavingSpec (Spec const &spec) { return m_child_nodes.at(spec); }
        Token const &Lookahead (Parser &parser) const;
        // Any HPS whose parent action pops the stack at all is blocked (because its future depends on the
        // values of the stack below the top).  Also, RETURN is considered to block, since nothing can happen after.
        bool IsBlockedHPS () const;
        PrecedenceLevelRange ComputePrecedenceLevelRange () const;
        // Returns true if and only if there is exactly one SHIFT child and one REDUCE child.
        bool HasShiftReduceConflict (ParseStackTreeNode_ *&shift, ParseStackTreeNode_ *&reduce);

        void AddChild (ParseStackTreeNode_ *child);
        void RemoveChild (ParseStackTreeNode_ *child);
        void RemoveFromParent ();

        ParseStackTreeNode_ *CloneLeafNode () const;
        // orphan_target must not have a parent (because its m_spec may change and affect its relationship with its parent).
        void CloneLeafNodeInto (ParseStackTreeNode_ &orphan_target) const;

        void Print (std::ostream &out, Parser const &parser, std::uint32_t indent_level = 0) const;
    };

    typedef std::deque<ParseStackTreeNode_ *> HPSQueue_;

    Token const &Lookahead_ (LookaheadQueue_::size_type index) throw();

    ParseStackTreeNode_ *TakeHypotheticalActionOnHPS_ (ParseStackTreeNode_ const &hps, ParseStackTreeNode_::Type action_type, std::uint32_t action_data);

    struct Npda_
    {
        ParseStackTreeNode_ *m_root_;
        // TODO/NOTE: The hps-es should really have "npda stack" and "npda lookahead queue",
        // which don't have any token data, whereas Npda_ should have "real stack" and "real lookahead queue",
        // which do have (and own) the token data.
        Stack_ m_global_stack_;
        LookaheadQueue_ m_global_lookahead_queue_;
        HPSQueue_ m_hps_queue_;
        HPSQueue_ m_new_hps_queue_; // This is stored so new memory isn't allocated for each parse iteration.

        Npda_ () : m_root_(NULL) { }
        ~Npda_ ();

        void PopFrontGlobalLookahead ();
        void PushFrontGlobalLookahead (Token const &lookahead);
    };

    Npda_ m_npda_;

    // Returns true iff lhs_rule_index denotes a rule with a higher precedence than that denoted by rhs_rule_index.
    static bool CompareRuleByPrecedence (std::uint32_t lhs_rule_index, std::uint32_t rhs_rule_index);
    static bool CompareToken (Token const &lhs, Token const &rhs) { return lhs.m_id < rhs.m_id; }
    static bool CompareStackElement (StackElement_ const &lhs, StackElement_ const &rhs) { return lhs.m_state_index < rhs.m_state_index; }

    static StateVector_ const &EpsilonClosureOfState_ (std::uint32_t state_index);
    static TransitionVector_ const &NonEpsilonTransitionsOfState_ (std::uint32_t state_index);

    friend struct ParseStackTreeNode_;

    static Precedence_ const ms_precedence_table_[];
    static std::size_t const ms_precedence_count_;
    static Rule_ const ms_rule_table_[];
    static std::size_t const ms_rule_count_;
    static State_ const ms_state_table_[];
    static std::size_t const ms_state_count_;
    static Transition_ const ms_transition_table_[];
    static std::size_t const ms_transition_count_;

    // ///////////////////////////////////////////////////////////////////////
    // end of internal trison-generated parser guts
    // ///////////////////////////////////////////////////////////////////////

    friend std::ostream &operator << (std::ostream &stream, Parser::Token const &token);
}; // end of class Parser

std::ostream &operator << (std::ostream &stream, Parser::Token const &token);

