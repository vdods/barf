// 2016.08.14 - Victor Dods

#include "barftest/sem/Declaration.hpp"
#include "barftest/sem/Identifier.hpp"
#include "barftest/sem/PointerType.hpp"
#include "barftest/sem/SymbolSpecifier.hpp"
#include "barftest/sem/TypeIdentifier.hpp"
#include "barftest/sem/Vector.hpp"
#include <lvd/test.hpp>
#include "UnitTestParser.hpp"

namespace barftest {
namespace text {

LVD_TEST_BEGIN(00__parser__13__statement_list__000)
    g_unit_test_parser.parse_and_validate(
        req_context,
        {
            "p : T@", "p :\nT@",
        },
        Parser::Nonterminal::statement_list,
        sem::make_statement_list(
            sem::make_declaration(
                sem::make_symbol_specifier(sem::make_identifier("p")),
                sem::make_pointer_type(FiRange::INVALID, sem::make_type_identifier(sem::make_identifier("T")))
            )
        ),
        9
    );
LVD_TEST_END

} // end namespace text
} // end namespace barftest
