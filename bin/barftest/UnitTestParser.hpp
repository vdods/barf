// 2016.08.14 - Victor Dods

#pragma once

#include "core.hpp"
#include <lvd/req.hpp>
#include "sem/Base.hpp"
#include "text/Parser.hpp"

namespace cbz {
namespace text {

enum class RecoverableError
{
    NOT_EXPECTED,
    EXPECTED
};

struct UnitTestParser
{
    bool m_error_encountered;
    std::size_t m_error_count;
    std::size_t m_max_realized_lookahead_count;
    std::size_t m_max_realized_lookahead_queue_size;
    std::uint32_t m_max_realized_parse_tree_depth;
    std::size_t m_force_debug_spew_test_index;

    UnitTestParser ()
        :   m_error_encountered(false)
        ,   m_error_count(0)
        ,   m_max_realized_lookahead_count(0)
        ,   m_max_realized_lookahead_queue_size(0)
        ,   m_max_realized_parse_tree_depth(0)
        ,   m_force_debug_spew_test_index(std::size_t(-1))
    { }

    void parse_and_validate (
        lvd::req::Context &req_context,
        std::initializer_list<std::string> const &equivalent_source_codes,
        Parser::Nonterminal::Name nonterminal_to_parse,
        sem::Base const &expected_parsed_root,
        std::uint32_t expected_max_realized_parse_tree_depth = 1,
        std::size_t expected_max_realized_lookahead_count = 1,
        std::size_t expected_max_realized_lookahead_queue_size = 2,
        Parser::ParserReturnCode expected_parser_return_code = Parser::ParserReturnCode::PRC_SUCCESS,
        RecoverableError recoverable_error = RecoverableError::NOT_EXPECTED,
        bool debug_spew = false
    );
};

// UnitTestParser singleton which all Parser test functions use to run their test case.
extern UnitTestParser g_unit_test_parser;

} // end namespace text
} // end namespace cbz
