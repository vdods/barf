// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY
// calculator_scanner.cpp generated by reflex
// from calculator_scanner.reflex using reflex.cpp.targetspec and reflex.cpp.implementation.codespec
// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY

#include "calculator_scanner.hpp"

#include <iostream>

#define REFLEX_CPP_DEBUG_CODE_(spew_code) if (DebugSpew()) { spew_code; }


#line 46 "calculator_scanner.reflex"

namespace Calculator {

#line 18 "calculator_scanner.cpp"

Scanner::Scanner (
#line 31 "calculator_scanner.reflex"
 string const &input_string 
#line 23 "calculator_scanner.cpp"
)
    :
    ReflexCpp_::AutomatonApparatus(
        ms_state_table_,
        ms_state_count_,
        ms_transition_table_,
        ms_transition_count_,
        ms_accept_handler_count_,
        static_cast<ReflexCpp_::InputApparatus::IsInputAtEndMethod>(&Scanner::IsInputAtEnd_),
        static_cast<ReflexCpp_::InputApparatus::ReadNextAtomMethod>(&Scanner::ReadNextAtom_))
{
    DebugSpew(false);


#line 49 "calculator_scanner.reflex"

    m_input.str(input_string);

#line 42 "calculator_scanner.cpp"

    ResetForNewInput();
}

Scanner::~Scanner ()
{
}

Scanner::Mode::Name Scanner::ScannerMode () const
{
    assert(InitialState_() != NULL);
    BarfCpp_::Size initial_node_index = InitialState_() - ms_state_table_;
    assert(initial_node_index < ms_state_count_);
    switch (initial_node_index)
    {
        default: assert(false && "invalid initial node index -- this should never happen"); return Mode::START_;
        case 0: return Mode::MAIN;
    }
}

void Scanner::ScannerMode (Mode::Name mode)
{
    assert(
        mode == Mode::MAIN ||
        (false && "invalid Mode::Name"));
    InitialState_(ms_state_table_ + mode);
    REFLEX_CPP_DEBUG_CODE_(
        std::cerr << "Scanner:" << " transitioning to mode ";
        PrintScannerMode_(mode);
        std::cerr << std::endl)
    assert(ScannerMode() == mode);
}

void Scanner::ResetForNewInput ()
{
    REFLEX_CPP_DEBUG_CODE_(std::cerr << "Scanner:" << " executing reset-for-new-input actions" << std::endl)
                
    ReflexCpp_::AutomatonApparatus::ResetForNewInput_(ms_state_table_ + Mode::START_);


#line 73 "calculator_scanner.reflex"


#line 86 "calculator_scanner.cpp"
}

Parser::Token Scanner::Scan ()
{

    std::string accepted_string;
    // this is the main scanner loop.  it only breaks when an accept handler
    // returns or after the unmatched character handler, if certain conditions
    // exist (see comments below).
    while (true)
    {
        bool was_at_end_of_input_ = IsAtEndOfInput();

        BarfCpp_::Uint32 accept_handler_index_ = RunDfa_(accepted_string);
        // if no valid accept_handler_index_ was returned, then accepted_string
        // was filled with the first unaccepted input atom (i.e. the rejected
        // atom).  we'll call the HandleUnmatchedCharacter_ method on it.
        if (accept_handler_index_ >= ms_accept_handler_count_)
        {
            // if we were already at the end of input and no
            // rule was matched, break out of the loop.
            if (was_at_end_of_input_)
                break;

            assert(accepted_string.length() == 1);
            BarfCpp_::Uint8 rejected_atom = accepted_string[0];
            REFLEX_CPP_DEBUG_CODE_(
                std::cerr << "Scanner:" << " rejecting atom '";
                PrintAtom_(rejected_atom);
                std::cerr << '\'' << std::endl)

            // execute the rejected-atom-handling actions.  the rejected atom
            // is in rejected_atom.  the loop is so a break statement inside
            // rejection_actions doesn't break out of the main scanner loop.
            do
            {

#line 70 "calculator_scanner.reflex"

    assert(false && "we should have handled this in the catch-all rule");

#line 128 "calculator_scanner.cpp"

            }
            while (false);
        }
        // otherwise, call the appropriate accept handler code.
        else
        {
            REFLEX_CPP_DEBUG_CODE_(
                std::cerr << "Scanner:" << " accepting string ";
                PrintString_(accepted_string);
                std::cerr << ", corresponding to mode ";
                PrintScannerMode_(ScannerMode());
                std::cerr << ", regex (" << ms_accept_handler_regex_[accept_handler_index_] << ")." << std::endl)
            // execute the appropriate accept handler.
            // the accepted string is in accepted_string.
            switch (accept_handler_index_)
            {
                case 0:
                {

#line 111 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::NUMBER, strtod(accepted_string.c_str(), NULL));
    
#line 153 "calculator_scanner.cpp"

                }
                break;

                case 1:
                {

#line 116 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::NUMBER, strtod(accepted_string.c_str(), NULL));
    
#line 165 "calculator_scanner.cpp"

                }
                break;

                case 2:
                {

#line 121 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::NUMBER, M_PI);
    
#line 177 "calculator_scanner.cpp"

                }
                break;

                case 3:
                {

#line 126 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::NUMBER, M_E);
    
#line 189 "calculator_scanner.cpp"

                }
                break;

                case 4:
                {

#line 131 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::RESULT);
    
#line 201 "calculator_scanner.cpp"

                }
                break;

                case 5:
                {

#line 136 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::LOG);
    
#line 213 "calculator_scanner.cpp"

                }
                break;

                case 6:
                {

#line 141 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::HELP);
    
#line 225 "calculator_scanner.cpp"

                }
                break;

                case 7:
                {

#line 146 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::MOD);
    
#line 237 "calculator_scanner.cpp"

                }
                break;

                case 8:
                {

#line 151 "calculator_scanner.reflex"

        return Parser::Token(Parser::Token::Id(accepted_string[0]));
    
#line 249 "calculator_scanner.cpp"

                }
                break;

                case 9:
                {

#line 156 "calculator_scanner.reflex"

        // ignore all non-newline whitespace
    
#line 261 "calculator_scanner.cpp"

                }
                break;

                case 10:
                {

#line 161 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::NEWLINE);
    
#line 273 "calculator_scanner.cpp"

                }
                break;

                case 11:
                {

#line 166 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::END_);
    
#line 285 "calculator_scanner.cpp"

                }
                break;

                case 12:
                {

#line 171 "calculator_scanner.reflex"

        return Parser::Token(Parser::Terminal::BAD_TOKEN);
    
#line 297 "calculator_scanner.cpp"

                }
                break;

                default: assert(false && "this should never happen"); break;
            }
        }
    }


#line 52 "calculator_scanner.reflex"

    return Parser::Token(Parser::Terminal::BAD_TOKEN);

#line 312 "calculator_scanner.cpp"
}

// ///////////////////////////////////////////////////////////////////////
// begin internal reflex-generated parser guts -- don't use
// ///////////////////////////////////////////////////////////////////////

bool Scanner::IsInputAtEnd_ ()
{

#line 64 "calculator_scanner.reflex"

    return m_input.peek() == char_traits<char>::eof();

#line 326 "calculator_scanner.cpp"
}

BarfCpp_::Uint8 Scanner::ReadNextAtom_ ()
{

#line 67 "calculator_scanner.reflex"

    return m_input.get();

#line 336 "calculator_scanner.cpp"
}

void Scanner::PrintAtom_ (BarfCpp_::Uint8 atom)
{
    if (atom == '\\')                    std::cerr << "\\\\";
    else if (atom == '"')                std::cerr << "\\\"";
    else if (atom >= ' ' && atom <= '~') std::cerr << atom;
    else if (atom == '\n')               std::cerr << "\\n";
    else if (atom == '\t')               std::cerr << "\\t";
    else if (atom == '\0')               std::cerr << "\\0";
    else
    {
        std::cerr.width(2);
        std::cerr << "\\x" << std::hex << std::uppercase << BarfCpp_::Uint16(atom);
        std::cerr.width(1);
    }
}

void Scanner::PrintString_ (std::string const &s)
{
    // save the existing std::cerr properties for later restoration
    std::ios_base::fmtflags saved_stream_flags = std::cerr.flags();
    char saved_stream_fill = std::cerr.fill();
    std::streamsize saved_stream_width = std::cerr.width();
    std::streamsize saved_stream_precision = std::cerr.precision();

    // clear all format flags to a neutral state
    std::cerr.unsetf(
        std::ios_base::boolalpha|std::ios_base::dec|std::ios_base::fixed|
        std::ios_base::hex|std::ios_base::internal|std::ios_base::left|
        std::ios_base::oct|std::ios_base::right|std::ios_base::scientific|
        std::ios_base::showbase|std::ios_base::showpoint|std::ios_base::showpos|
        std::ios_base::skipws|std::ios_base::unitbuf|std::ios_base::uppercase|
        std::ios_base::adjustfield|std::ios_base::basefield|std::ios_base::floatfield);
    // the '0' char is used hex escape chars, which always have width 2
    std::cerr.fill('0');

    std::cerr << '"';
    for (std::string::size_type i = 0; i < s.size(); ++i)
        PrintAtom_(s[i]);
    std::cerr << '"';

    // restore the saved std::cerr properties
    std::cerr.setf(saved_stream_flags);
    std::cerr.fill(saved_stream_fill);
    std::cerr.width(saved_stream_width);
    std::cerr.precision(saved_stream_precision);
}

void Scanner::PrintScannerMode_ (Mode::Name mode)
{
    if (false) { }
    else if (mode == Mode::MAIN) { std::cerr << "MAIN"; }
}

// the order of the states indicates priority (only for accept states).
// the lower the state's index in this array, the higher its priority.
ReflexCpp_::AutomatonApparatus::DfaState_ const Scanner::ms_state_table_[] =
{
    { 13, 2, ms_transition_table_+0 },
    { 13, 29, ms_transition_table_+2 },
    { 12, 0, ms_transition_table_+31 },
    { 9, 0, ms_transition_table_+31 },
    { 10, 0, ms_transition_table_+31 },
    { 8, 0, ms_transition_table_+31 },
    { 0, 1, ms_transition_table_+31 },
    { 13, 1, ms_transition_table_+32 },
    { 1, 3, ms_transition_table_+33 },
    { 13, 4, ms_transition_table_+36 },
    { 13, 2, ms_transition_table_+40 },
    { 1, 0, ms_transition_table_+42 },
    { 1, 1, ms_transition_table_+42 },
    { 0, 2, ms_transition_table_+43 },
    { 0, 2, ms_transition_table_+45 },
    { 3, 0, ms_transition_table_+47 },
    { 12, 1, ms_transition_table_+47 },
    { 13, 1, ms_transition_table_+48 },
    { 13, 1, ms_transition_table_+49 },
    { 6, 0, ms_transition_table_+50 },
    { 12, 1, ms_transition_table_+50 },
    { 13, 1, ms_transition_table_+51 },
    { 5, 0, ms_transition_table_+52 },
    { 12, 1, ms_transition_table_+52 },
    { 13, 1, ms_transition_table_+53 },
    { 7, 0, ms_transition_table_+54 },
    { 12, 1, ms_transition_table_+54 },
    { 2, 0, ms_transition_table_+55 },
    { 4, 0, ms_transition_table_+55 },
    { 11, 29, ms_transition_table_+55 }
};
BarfCpp_::Size const Scanner::ms_state_count_ = sizeof(Scanner::ms_state_table_) / sizeof(*Scanner::ms_state_table_);

ReflexCpp_::AutomatonApparatus::DfaTransition_ const Scanner::ms_transition_table_[] =
{
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+1 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+29 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 9, 0, ms_state_table_+3 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 10, 0, ms_state_table_+4 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 32, 0, ms_state_table_+3 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 44, 0, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 45, 0, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 46, 0, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 47, 0, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 48, 0, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 92, 0, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 93, 0, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 94, 0, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 101, 0, ms_state_table_+15 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 104, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 108, 0, ms_state_table_+20 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 109, 0, ms_state_table_+23 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 112, 0, ms_state_table_+26 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 113, 0, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 114, 0, ms_state_table_+28 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 1, 8, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 11, 31, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 33, 39, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 40, 43, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 49, 57, ms_state_table_+13 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 58, 91, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 95, 100, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 102, 103, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 105, 107, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 110, 111, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 115, 255, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 46, 0, ms_state_table_+7 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 48, 57, ms_state_table_+8 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 69, 0, ms_state_table_+9 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 101, 0, ms_state_table_+9 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 48, 57, ms_state_table_+8 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 43, 0, ms_state_table_+10 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 45, 0, ms_state_table_+10 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 48, 0, ms_state_table_+11 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 49, 57, ms_state_table_+12 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 48, 0, ms_state_table_+11 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 49, 57, ms_state_table_+12 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 48, 57, ms_state_table_+12 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 46, 0, ms_state_table_+7 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 48, 57, ms_state_table_+14 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 46, 0, ms_state_table_+7 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 48, 57, ms_state_table_+14 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 101, 0, ms_state_table_+17 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 108, 0, ms_state_table_+18 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 112, 0, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 111, 0, ms_state_table_+21 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 103, 0, ms_state_table_+22 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 111, 0, ms_state_table_+24 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 100, 0, ms_state_table_+25 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 105, 0, ms_state_table_+27 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 9, 0, ms_state_table_+3 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 10, 0, ms_state_table_+4 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 32, 0, ms_state_table_+3 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 44, 0, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 45, 0, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 46, 0, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 47, 0, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 48, 0, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 92, 0, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 93, 0, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 94, 0, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 101, 0, ms_state_table_+15 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 104, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 108, 0, ms_state_table_+20 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 109, 0, ms_state_table_+23 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 112, 0, ms_state_table_+26 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 113, 0, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM, 114, 0, ms_state_table_+28 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 1, 8, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 11, 31, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 33, 39, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 40, 43, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 49, 57, ms_state_table_+13 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 58, 91, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 95, 100, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 102, 103, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 105, 107, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 110, 111, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus::DfaTransition_::INPUT_ATOM_RANGE, 115, 255, ms_state_table_+2 }
};
BarfCpp_::Size const Scanner::ms_transition_count_ = sizeof(Scanner::ms_transition_table_) / sizeof(*Scanner::ms_transition_table_);

char const *const Scanner::ms_accept_handler_regex_[] =
{
    "{INTEGER}",
    "{FLOAT}",
    "pi",
    "e",
    "r",
    "log",
    "help",
    "mod",
    "{OPERATOR}",
    "{WHITESPACE}",
    "{NEWLINE}",
    "{END_OF_FILE}",
    "."
};
BarfCpp_::Uint32 const Scanner::ms_accept_handler_count_ = sizeof(Scanner::ms_accept_handler_regex_) / sizeof(*Scanner::ms_accept_handler_regex_);

// ///////////////////////////////////////////////////////////////////////
// end of internal reflex-generated parser guts
// ///////////////////////////////////////////////////////////////////////


#line 55 "calculator_scanner.reflex"

} // end of namespace Calculator

#line 545 "calculator_scanner.cpp"