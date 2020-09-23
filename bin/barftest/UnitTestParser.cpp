// 2016.08.14 - Victor Dods

#include "UnitTestParser.hpp"

#include "barftest/literal.hpp"
#include <lvd/req.hpp>
#include <lvd/test.hpp>

namespace barftest {
namespace sem {

// This is just used to allow LVD_TEST_REQ_EQ to be able to test barftest::sem::Base equality via operator==
bool operator == (Base const &lhs, Base const &rhs) { return barftest::sem::are_equal(lhs, rhs); }

} // end namespace sem

namespace text {

UnitTestParser g_unit_test_parser;

void UnitTestParser::parse_and_validate (
    lvd::req::Context &req_context,
    std::initializer_list<std::string> const &equivalent_source_codes,
    Parser::Nonterminal::Name nonterminal_to_parse,
    sem::Base const &expected_parsed_root,
    std::uint32_t expected_max_realized_parse_tree_depth,
    std::size_t expected_max_realized_lookahead_count,
    std::size_t expected_max_realized_lookahead_queue_size,
    Parser::ParserReturnCode expected_parser_return_code,
    RecoverableError recoverable_error,
    bool debug_spew
)
{
    auto &test_log = req_context.log();

    std::array<std::size_t,2> lookahead_count_padding_counts{10};
    std::array<std::size_t,2> lookahead_queue_size_padding_counts{10};
//     std::array<std::size_t,2> lookahead_count_padding_counts{1, 0};
//     std::array<std::size_t,2> lookahead_queue_size_padding_counts{1, 0};
    // padding is the padding used for expected_max_realized_lookahead_count.  Run the whole test
    // twice once with 1 extra token of padding (so that we can measure the max lookahead count),
    // and 1 with no padding so we can test the tight case.
    for (auto lookahead_count_padding_count : lookahead_count_padding_counts)
    {
        for (auto lookahead_queue_size_padding_count : lookahead_queue_size_padding_counts)
        {
            for (auto const &source_code : equivalent_source_codes)
            {
                std::ostringstream out;
                Log log_out(out);

                test_log << Log::dbg() << "Source code:\n"
                         << lvd::IndentGuard() << source_code << '\n';
                test_log << Log::dbg() << "Attempting to parse nonterminal:\n"
                         << lvd::IndentGuard() << Parser::TokenIdAsString(Parser::Token::Id(nonterminal_to_parse)) << '\n';
                test_log << Log::dbg() << "Using " << LVD_REFLECT(lookahead_count_padding_count) << '\n'
                                            << "Using " << LVD_REFLECT(lookahead_queue_size_padding_count) << '\n';

                Parser parser;
                // Set the max allowable lookahead count to just above the expected, so we can test it.
                if (false)
                {
                    parser.SetMaxAllowableLookaheadCount(expected_max_realized_lookahead_count + lookahead_count_padding_count);
                    parser.SetMaxAllowableLookaheadQueueSize(expected_max_realized_lookahead_queue_size + lookahead_queue_size_padding_count);
                }
                else
                {
                    parser.SetMaxAllowableLookaheadCount(-1);
                    parser.SetMaxAllowableLookaheadQueueSize(-1);
                }
                // This is where the parser's output will go.
                parser.log_out(&log_out);
                if (debug_spew)
                {
//                     parser.SetScannerDebugSpewStream(&out);
//                     parser.SetDebugSpewStream(&out);
                    parser.SetScannerDebugSpewStream(&std::cerr);
                    parser.SetDebugSpewStream(&std::cerr);
                }
                parser.open_string(source_code, LVD_FMT("<str>"));
                // This is a little kludgey, but it has to be something not null.
                nnup<sem::Base> parsed_root(sem::make_dummy());
                auto parser_return_code = parser.Parse(&parsed_root, nonterminal_to_parse);

                // Print the parser's log output to test_log.
                test_log << Log::trc() << out.str();

                LVD_TEST_REQ_EQ(parser_return_code, expected_parser_return_code);
                LVD_TEST_REQ_EQ(*parsed_root, expected_parsed_root);

                bool expected_recoverable_error_encountered = recoverable_error == RecoverableError::EXPECTED;
                LVD_TEST_REQ_EQ(parser.recoverable_error_encountered(), expected_recoverable_error_encountered);

                if (false)
                {
                    LVD_TEST_REQ_EQ(parser.MaxRealizedLookaheadCount(), expected_max_realized_lookahead_count);
                    LVD_TEST_REQ_EQ(parser.MaxRealizedLookaheadQueueSize(), expected_max_realized_lookahead_queue_size);
                    LVD_TEST_REQ_EQ(parser.MaxRealizedParseTreeDepth(), expected_max_realized_parse_tree_depth);
                }

                // Track the max realized lookahead count over all parses.
                m_max_realized_lookahead_count = std::max(m_max_realized_lookahead_count, parser.MaxRealizedLookaheadCount());
                m_max_realized_lookahead_queue_size = std::max(m_max_realized_lookahead_queue_size, parser.MaxRealizedLookaheadQueueSize());
                m_max_realized_parse_tree_depth = std::max(m_max_realized_parse_tree_depth, parser.MaxRealizedParseTreeDepth());
            }
        }
    }
}

// This prints a summary
LVD_TEST_BEGIN(00__parser__99__summary)
    g_log << Log::inf() << "Max realized lookahead count (over all parses) was " << g_unit_test_parser.m_max_realized_lookahead_count << "\n\n";
    g_log << Log::inf() << "Max realized lookahead queue size (over all parses) was " << g_unit_test_parser.m_max_realized_lookahead_queue_size << "\n\n";
    g_log << Log::inf() << "Max realized parse tree depth (over all parses) was " << g_unit_test_parser.m_max_realized_parse_tree_depth << "\n\n";

    LVD_TEST_REQ_LEQ(g_unit_test_parser.m_max_realized_lookahead_count, 4);
    LVD_TEST_REQ_LEQ(g_unit_test_parser.m_max_realized_lookahead_queue_size, 5);
    LVD_TEST_REQ_LEQ(g_unit_test_parser.m_max_realized_parse_tree_depth, 10);
LVD_TEST_END

} // end namespace text
} // end namespace barftest
