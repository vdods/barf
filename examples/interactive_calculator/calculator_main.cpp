// 2007.11.10 - Copyright Victor Dods - Licensed under Apache 2.0

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
        Parser::ParserReturnCode return_code = parser.Parse(&accepted_token);
        switch (return_code)
        {
            case Parser::PRC_SUCCESS:
                if (parser.ShouldPrintResult())
                    cout << " = " << accepted_token << '\n';
                break;

            default:
                cerr << "error: " << return_code << '\n';
        }
    }

    return 0;
}
