// ///////////////////////////////////////////////////////////////////////////
// calculator_main.cpp by Victor Dods, created 2007/11/10
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "calculator.hpp"

#include "calculator_parser.hpp"

using namespace Calculator;

#define BUFFER_SIZE 0x10000 // 64 kilobytes

int main (int argc, char **argv)
{
    char buffer[BUFFER_SIZE];
    double accepted_token;
    Parser parser;

    cout << "BARF calculator by Victor Dods - type \\help for help." << endl;
    cout.precision(15); // about the number of digits a double is good for.

    while (!cin.eof())
    {
        cin.getline(buffer, BUFFER_SIZE, '\n');
        buffer[BUFFER_SIZE-1] = '\0'; // just to make sure
        parser.SetInputString(buffer);
        switch (parser.Parse(&accepted_token))
        {
            case Parser::PRC_SUCCESS:
                if (parser.ShouldPrintResult())
                {
                    cout << " = " << accepted_token << endl;
                }
                break;

            case Parser::PRC_UNHANDLED_PARSE_ERROR:
                cerr << "error: general parse error" << endl;
                break;
        }
    }

    return 0;
}
