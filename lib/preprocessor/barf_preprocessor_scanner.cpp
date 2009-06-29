// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY
// barf_preprocessor_scanner.cpp generated by reflex
// from barf_preprocessor_scanner.reflex using reflex.cpp.targetspec and reflex.cpp.implementation.codespec
// DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY ! DO NOT MODIFY

#include "barf_preprocessor_scanner.hpp"

#include <iostream>

#define REFLEX_CPP_DEBUG_CODE_(spew_code) if (DebugSpew()) { spew_code; }


#line 68 "barf_preprocessor_scanner.reflex"

#include "barf_preprocessor_ast.hpp"

namespace Barf {
namespace Preprocessor {

bool Scanner::OpenFile (string const &input_filename)
{
    bool open_succeeded = InputBase::OpenFile(input_filename);
    if (open_succeeded)
        ResetForNewInput();
    return open_succeeded;
}

void Scanner::OpenString (string const &input_string, string const &input_name, bool use_line_numbers)
{
    InputBase::OpenString(input_string, input_name, use_line_numbers);
    ResetForNewInput();
}

void Scanner::OpenUsingStream (istream *input_stream, string const &input_name, bool use_line_numbers)
{
    InputBase::OpenUsingStream(input_stream, input_name, use_line_numbers);
    ResetForNewInput();
}

Parser::Token Scanner::ParseKeyword (string const &accepted_string)
{
    if (accepted_string == "declare_array")         return Parser::Token(Parser::Terminal::DECLARE_ARRAY);
    if (accepted_string == "declare_map")           return Parser::Token(Parser::Terminal::DECLARE_MAP);
    if (accepted_string == "define")                return Parser::Token(Parser::Terminal::DEFINE);
    if (accepted_string == "dump_symbol_table")     return Parser::Token(Parser::Terminal::DUMP_SYMBOL_TABLE);
    if (accepted_string == "else")                  return Parser::Token(Parser::Terminal::ELSE);
    if (accepted_string == "else_if")               return Parser::Token(Parser::Terminal::ELSE_IF);
    if (accepted_string == "end_define")            return Parser::Token(Parser::Terminal::END_DEFINE);
    if (accepted_string == "end_for_each")          return Parser::Token(Parser::Terminal::END_FOR_EACH);
    if (accepted_string == "end_if")                return Parser::Token(Parser::Terminal::END_IF);
    if (accepted_string == "end_loop")              return Parser::Token(Parser::Terminal::END_LOOP);
    if (accepted_string == "error")                 return Parser::Token(Parser::Terminal::ERROR);
    if (accepted_string == "fatal_error")           return Parser::Token(Parser::Terminal::FATAL_ERROR);
    if (accepted_string == "for_each")              return Parser::Token(Parser::Terminal::FOR_EACH);
    if (accepted_string == "if")                    return Parser::Token(Parser::Terminal::IF);
    if (accepted_string == "include")               return Parser::Token(Parser::Terminal::INCLUDE);
    if (accepted_string == "int")                   return Parser::Token(Parser::Terminal::KEYWORD_INT);
    if (accepted_string == "is_defined")            return Parser::Token(Parser::Terminal::IS_DEFINED);
    if (accepted_string == "loop")                  return Parser::Token(Parser::Terminal::LOOP);
    if (accepted_string == "sandbox_include")       return Parser::Token(Parser::Terminal::SANDBOX_INCLUDE);
    if (accepted_string == "sizeof")                return Parser::Token(Parser::Terminal::SIZEOF);
    if (accepted_string == "string")                return Parser::Token(Parser::Terminal::KEYWORD_STRING);
    if (accepted_string == "string_length")         return Parser::Token(Parser::Terminal::STRING_LENGTH);
    if (accepted_string == "to_character_literal")  return Parser::Token(Parser::Terminal::TO_CHARACTER_LITERAL);
    if (accepted_string == "to_string_literal")     return Parser::Token(Parser::Terminal::TO_STRING_LITERAL);
    if (accepted_string == "undefine")              return Parser::Token(Parser::Terminal::UNDEFINE);
    if (accepted_string == "warning")               return Parser::Token(Parser::Terminal::WARNING);

    return Parser::Token(Parser::Terminal::ID, new Ast::Id(accepted_string, GetFiLoc()));
}

#line 73 "barf_preprocessor_scanner.cpp"

Scanner::Scanner ()
    :
    ReflexCpp_::AutomatonApparatus_(
        ms_state_table_,
        ms_state_count_,
        ms_transition_table_,
        ms_transition_count_,
        ms_accept_handler_count_,
        static_cast<ReflexCpp_::InputApparatus_::IsInputAtEndMethod_>(&Scanner::IsInputAtEnd_),
        static_cast<ReflexCpp_::InputApparatus_::ReadNextAtomMethod_>(&Scanner::ReadNextAtom_))
{
    DebugSpew(false);


#line 126 "barf_preprocessor_scanner.reflex"

    m_text = NULL;

#line 93 "barf_preprocessor_scanner.cpp"

    ResetForNewInput();
}

Scanner::~Scanner ()
{

#line 129 "barf_preprocessor_scanner.reflex"

    delete m_text;
    m_text = NULL;

#line 106 "barf_preprocessor_scanner.cpp"
}

Scanner::StateMachine::Name Scanner::CurrentStateMachine () const
{
    assert(InitialState_() != NULL);
    BarfCpp_::Size initial_node_index = InitialState_() - ms_state_table_;
    assert(initial_node_index < ms_state_count_);
    switch (initial_node_index)
    {
        default: assert(false && "invalid initial node index -- this should never happen"); return StateMachine::START_;
        case 0: return StateMachine::EXPECTING_END_OF_FILE;
        case 4: return StateMachine::READING_BODY;
        case 14: return StateMachine::READING_CODE;
        case 28: return StateMachine::READING_CODE_STRING_LITERAL_GUTS;
        case 38: return StateMachine::TRANSITION_TO_CODE;
    }
}

void Scanner::SwitchToStateMachine (StateMachine::Name state_machine)
{
    assert(
        state_machine == StateMachine::EXPECTING_END_OF_FILE ||
        state_machine == StateMachine::READING_BODY ||
        state_machine == StateMachine::READING_CODE ||
        state_machine == StateMachine::READING_CODE_STRING_LITERAL_GUTS ||
        state_machine == StateMachine::TRANSITION_TO_CODE ||
        (false && "invalid StateMachine::Name"));
    REFLEX_CPP_DEBUG_CODE_(
        std::cerr << 
#line 166 "barf_preprocessor_scanner.reflex"
"Preprocessor::Scanner" << (GetFiLoc().GetIsValid() ? " ("+GetFiLoc().GetAsString()+")" : g_empty_string) << ":"
#line 138 "barf_preprocessor_scanner.cpp"
 << " switching to state machine "
                  << ms_state_machine_name_[state_machine];
        if (ms_state_machine_mode_flags_[state_machine] != 0)
        {
            if ((ms_state_machine_mode_flags_[state_machine] & AutomatonApparatus_::MF_CASE_INSENSITIVE_) != 0)
                std::cerr << " %case_insensitive";
            if ((ms_state_machine_mode_flags_[state_machine] & AutomatonApparatus_::MF_UNGREEDY_) != 0)
                std::cerr << " %ungreedy";
        }
        std::cerr << std::endl)
    InitialState_(ms_state_table_ + ms_state_machine_start_state_index_[state_machine]);
    ModeFlags_(ms_state_machine_mode_flags_[state_machine]);
    assert(CurrentStateMachine() == state_machine);
}

void Scanner::ResetForNewInput ()
{
    REFLEX_CPP_DEBUG_CODE_(
        std::cerr << 
#line 166 "barf_preprocessor_scanner.reflex"
"Preprocessor::Scanner" << (GetFiLoc().GetIsValid() ? " ("+GetFiLoc().GetAsString()+")" : g_empty_string) << ":"
#line 160 "barf_preprocessor_scanner.cpp"
 << " executing reset-for-new-input actions and switching to state machine "
                  << ms_state_machine_name_[StateMachine::START_];
        if (ms_state_machine_mode_flags_[StateMachine::START_] != 0)
        {
            if ((ms_state_machine_mode_flags_[StateMachine::START_] & AutomatonApparatus_::MF_CASE_INSENSITIVE_) != 0)
                std::cerr << " %case_insensitive";
            if ((ms_state_machine_mode_flags_[StateMachine::START_] & AutomatonApparatus_::MF_UNGREEDY_) != 0)
                std::cerr << " %ungreedy";
        }
        std::cerr << std::endl)
    ReflexCpp_::AutomatonApparatus_::ResetForNewInput_(ms_state_table_ + ms_state_machine_start_state_index_[StateMachine::START_], ms_state_machine_mode_flags_[StateMachine::START_]);
    assert(CurrentStateMachine() == StateMachine::START_);


#line 156 "barf_preprocessor_scanner.reflex"

    delete m_text;
    m_text = NULL;

#line 180 "barf_preprocessor_scanner.cpp"
}

Parser::Token Scanner::Scan () throw()
{

    std::string work_string;
    // this is the main scanner loop.  it only breaks when an accept handler
    // returns or after the unmatched character handler, if certain conditions
    // exist (see comments below).
    while (true)
    {
        // clear the previous accepted/rejected string.
        work_string.clear();
        // reset the char buffer and other stuff
        PrepareToScan_();
    
        bool was_at_end_of_input_ = IsAtEndOfInput();

        BarfCpp_::Uint32 accept_handler_index_ = RunDfa_(work_string);
        // if no valid accept_handler_index_ was returned, then work_string
        // was filled with everything up to the char after the keep_string
        // cursor (i.e. the rejected atom).
        if (accept_handler_index_ >= ms_accept_handler_count_)
        {
            // if we were already at the end of input and no
            // rule was matched, break out of the loop.
            if (was_at_end_of_input_)
                break;

            std::string &rejected_string = work_string;
            BarfCpp_::Uint8 rejected_atom = rejected_string.empty() ? '\0' : *rejected_string.rbegin();
            
            REFLEX_CPP_DEBUG_CODE_(
                std::cerr << 
#line 166 "barf_preprocessor_scanner.reflex"
"Preprocessor::Scanner" << (GetFiLoc().GetIsValid() ? " ("+GetFiLoc().GetAsString()+")" : g_empty_string) << ":"
#line 217 "barf_preprocessor_scanner.cpp"
 << " rejecting string ";
                PrintString_(rejected_string);
                std::cerr << " (rejected_atom is \'";
                PrintAtom_(rejected_atom);
                std::cerr << "\')" << std::endl)

            // execute the rejection actions.  the do/while loop is so that a
            // break statement inside the rejection actions doesn't break out
            // of the main scanner loop.
            do
            {

#line 153 "barf_preprocessor_scanner.reflex"

    EmitError("unrecognized character " + GetCharLiteral(rejected_atom), GetFiLoc());

#line 234 "barf_preprocessor_scanner.cpp"

            }
            while (false);
        }
        // otherwise, call the appropriate accept handler code.
        else
        {
            std::string &accepted_string = work_string;
        
            REFLEX_CPP_DEBUG_CODE_(
                std::cerr << 
#line 166 "barf_preprocessor_scanner.reflex"
"Preprocessor::Scanner" << (GetFiLoc().GetIsValid() ? " ("+GetFiLoc().GetAsString()+")" : g_empty_string) << ":"
#line 248 "barf_preprocessor_scanner.cpp"
 << " accepting string ";
                PrintString_(accepted_string);
                std::cerr << " in state machine " << ms_state_machine_name_[CurrentStateMachine()] 
                          << " using regex (" << ms_accept_handler_regex_[accept_handler_index_] << ")" << std::endl)
                
            // execute the appropriate accept handler.
            // the accepted string is in accepted_string.
            switch (accept_handler_index_)
            {
                case 0:
                {

#line 403 "barf_preprocessor_scanner.reflex"

        assert(false && "this should never happen");
        return Parser::Token(Parser::Terminal::BAD_TOKEN);
    
#line 266 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 1:
                {

#line 409 "barf_preprocessor_scanner.reflex"

        return Parser::Token(Parser::Terminal::END_);
    
#line 278 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 2:
                {

#line 218 "barf_preprocessor_scanner.reflex"

        IncrementLineNumber(GetNewlineCount(accepted_string));

        // save whether or not we got <|
        m_is_reading_newline_sensitive_code = *accepted_string.rbegin() == '|';
        
        // take off the <| or <{ at the end
        assert(accepted_string.length() >= 2);
        accepted_string.resize(accepted_string.length()-2);

        // if there's already a body text in progress, continue it.
        if (m_text != NULL)
            m_text->AppendText(accepted_string);
        // otherwise start a new one.
        else
            m_text = new Text(accepted_string, GetFiLoc());

        SwitchToStateMachine(StateMachine::TRANSITION_TO_CODE);
        Ast::Base *token = m_text;
        m_text = NULL;
        return Parser::Token(Parser::Terminal::TEXT, token);
    
#line 309 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 3:
                {

#line 242 "barf_preprocessor_scanner.reflex"

        // if there's already a body text in progress, continue it.
        if (m_text != NULL)
            m_text->AppendText(accepted_string);
        // otherwise start a new one.
        else
            m_text = new Text(accepted_string, GetFiLoc());

        SwitchToStateMachine(StateMachine::EXPECTING_END_OF_FILE);
        Ast::Base *token = m_text;
        m_text = NULL;
        return Parser::Token(Parser::Terminal::TEXT, token);
    
#line 331 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 4:
                {

#line 275 "barf_preprocessor_scanner.reflex"

        // ignore whitespace
    
#line 343 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 5:
                {

#line 280 "barf_preprocessor_scanner.reflex"

        IncrementLineNumber(1);
        // we only return from the scanner on a newline if we're scanning
        // a newline-sensitive code line (i.e. one that starts with "<|").
        if (m_is_reading_newline_sensitive_code)
        {
            SwitchToStateMachine(StateMachine::READING_BODY);
            return Parser::Token(Parser::Terminal::CODE_NEWLINE);
        }
    
#line 362 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 6:
                {

#line 292 "barf_preprocessor_scanner.reflex"

        SwitchToStateMachine(StateMachine::EXPECTING_END_OF_FILE);
        if (m_is_reading_newline_sensitive_code)
            return Parser::Token(Parser::Terminal::CODE_NEWLINE);
        else
            EmitError("unexpected end of file encountered within preprocessor code section", GetFiLoc());
    
#line 378 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 7:
                {

#line 301 "barf_preprocessor_scanner.reflex"

        if (!m_is_reading_newline_sensitive_code)
        {
            SwitchToStateMachine(StateMachine::READING_BODY);
            return Parser::Token(Parser::Terminal::END_CODE);
        }
        else
        {
            EmitError("unexpected '}' encountered", GetFiLoc());
            return Parser::Token(Parser::Terminal::BAD_TOKEN);
        }
    
#line 399 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 8:
                {

#line 315 "barf_preprocessor_scanner.reflex"

        return Parser::Token(Parser::Token::Id(accepted_string[0]));
    
#line 411 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 9:
                {

#line 320 "barf_preprocessor_scanner.reflex"

        return ParseKeyword(accepted_string);
    
#line 423 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 10:
                {

#line 325 "barf_preprocessor_scanner.reflex"

        Sint32 value = 0;
        istringstream in(accepted_string);
        in >> value;
        return Parser::Token(Parser::Terminal::INTEGER_LITERAL, new Integer(value, GetFiLoc()));
    
#line 438 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 11:
                {

#line 333 "barf_preprocessor_scanner.reflex"

        assert(m_text == NULL);
        m_text = new Text("", GetFiLoc());
        SwitchToStateMachine(StateMachine::READING_CODE_STRING_LITERAL_GUTS);
    
#line 452 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 12:
                {

#line 340 "barf_preprocessor_scanner.reflex"

        EmitError("unrecognized character encountered within preprocessor code section", GetFiLoc());
        return Parser::Token(Parser::Terminal::BAD_TOKEN);
    
#line 465 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 13:
                {

#line 349 "barf_preprocessor_scanner.reflex"

        assert(m_text != NULL);
        IncrementLineNumber(GetNewlineCount(accepted_string));
        // get rid of the trailing endquote
        assert(accepted_string.length() >= 1);
        accepted_string.resize(accepted_string.length()-1);
        // escape the string in-place and handle the return code
        EscapeStringStatus status = EscapeString(accepted_string);
        switch (status.m_return_code)
        {
            case ESRC_SUCCESS:
                // awesome
                break;
                
            case ESRC_UNEXPECTED_EOI:
                assert(false && "the formulation of the regex should prevent this");
                break;
                
            case ESRC_MALFORMED_HEX_CHAR:
                EmitError(
                    "\\x with no trailing hex digits",
                    FiLoc(m_text->GetFiLoc().GetFilename(),
                          m_text->GetFiLoc().GetLineNumber() + status.m_line_number_offset));
                break;
                
            case ESRC_HEX_ESCAPE_SEQUENCE_OUT_OF_RANGE:
            case ESRC_OCTAL_ESCAPE_SEQUENCE_OUT_OF_RANGE:
                EmitError(
                    "hex/octal escape sequence out of range",
                    FiLoc(m_text->GetFiLoc().GetFilename(),
                          m_text->GetFiLoc().GetLineNumber() + status.m_line_number_offset));
                break;
        }
        m_text->AppendText(accepted_string);
        SwitchToStateMachine(StateMachine::READING_CODE);
        Ast::Base *token = m_text;
        m_text = NULL;
        return Parser::Token(Parser::Terminal::STRING_LITERAL, token);
    
#line 513 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 14:
                {

#line 390 "barf_preprocessor_scanner.reflex"

        EmitError("unterminated string literal", GetFiLoc());
        IncrementLineNumber(GetNewlineCount(accepted_string));
        assert(m_text != NULL);
        delete m_text;
        m_text = NULL;
        return Parser::Token(Parser::Terminal::END_);
    
#line 530 "barf_preprocessor_scanner.cpp"

                }
                break;

                case 15:
                {

#line 262 "barf_preprocessor_scanner.reflex"

        assert(m_text == NULL);
        SwitchToStateMachine(StateMachine::READING_CODE);
        if (m_is_reading_newline_sensitive_code)
            return Parser::Token(Parser::Terminal::CODE_LINE);
        else
            return Parser::Token(Parser::Terminal::START_CODE);
    
#line 547 "barf_preprocessor_scanner.cpp"

                }
                break;

                default: assert(false && "this should never happen"); break;
            }
        }
    }


#line 133 "barf_preprocessor_scanner.reflex"

    assert(false && "you didn't handle EOF properly");
    return Parser::Token(Parser::Terminal::END_);

#line 563 "barf_preprocessor_scanner.cpp"
}

// ///////////////////////////////////////////////////////////////////////
// begin internal reflex-generated parser guts -- don't use
// ///////////////////////////////////////////////////////////////////////

bool Scanner::IsInputAtEnd_ () throw()
{

#line 147 "barf_preprocessor_scanner.reflex"

    return In().peek() == char_traits<char>::eof();

#line 577 "barf_preprocessor_scanner.cpp"
}

BarfCpp_::Uint8 Scanner::ReadNextAtom_ () throw()
{

#line 150 "barf_preprocessor_scanner.reflex"

    return In().get();

#line 587 "barf_preprocessor_scanner.cpp"
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

BarfCpp_::Uint32 const Scanner::ms_state_machine_start_state_index_[] =
{
    0,
    4,
    14,
    28,
    38,
};
BarfCpp_::Uint8 const Scanner::ms_state_machine_mode_flags_[] =
{
    0,
    2,
    0,
    2,
    0,
};
char const *const Scanner::ms_state_machine_name_[] =
{
    "EXPECTING_END_OF_FILE",
    "READING_BODY",
    "READING_CODE",
    "READING_CODE_STRING_LITERAL_GUTS",
    "TRANSITION_TO_CODE",
};
BarfCpp_::Uint32 const Scanner::ms_state_machine_count_ = sizeof(Scanner::ms_state_machine_name_) / sizeof(*Scanner::ms_state_machine_name_);

// the order of the states indicates priority (only for accept states).
// the lower the state's index in this array, the higher its priority.
ReflexCpp_::AutomatonApparatus_::DfaState_ const Scanner::ms_state_table_[] =
{
    { 16, 2, ms_transition_table_+0 },
    { 16, 1, ms_transition_table_+2 },
    { 0, 1, ms_transition_table_+3 },
    { 1, 1, ms_transition_table_+4 },
    { 16, 2, ms_transition_table_+5 },
    { 16, 3, ms_transition_table_+7 },
    { 16, 2, ms_transition_table_+10 },
    { 3, 3, ms_transition_table_+12 },
    { 16, 2, ms_transition_table_+15 },
    { 16, 5, ms_transition_table_+17 },
    { 2, 2, ms_transition_table_+22 },
    { 2, 3, ms_transition_table_+24 },
    { 2, 3, ms_transition_table_+27 },
    { 3, 5, ms_transition_table_+30 },
    { 16, 2, ms_transition_table_+35 },
    { 16, 28, ms_transition_table_+37 },
    { 12, 0, ms_transition_table_+65 },
    { 4, 0, ms_transition_table_+65 },
    { 5, 0, ms_transition_table_+65 },
    { 8, 0, ms_transition_table_+65 },
    { 11, 0, ms_transition_table_+65 },
    { 10, 0, ms_transition_table_+65 },
    { 10, 1, ms_transition_table_+65 },
    { 10, 1, ms_transition_table_+66 },
    { 9, 4, ms_transition_table_+67 },
    { 9, 4, ms_transition_table_+71 },
    { 7, 0, ms_transition_table_+75 },
    { 6, 28, ms_transition_table_+75 },
    { 16, 2, ms_transition_table_+103 },
    { 16, 5, ms_transition_table_+105 },
    { 16, 2, ms_transition_table_+110 },
    { 14, 5, ms_transition_table_+112 },
    { 13, 2, ms_transition_table_+117 },
    { 13, 5, ms_transition_table_+119 },
    { 16, 2, ms_transition_table_+124 },
    { 16, 1, ms_transition_table_+126 },
    { 14, 1, ms_transition_table_+127 },
    { 13, 5, ms_transition_table_+128 },
    { 15, 0, ms_transition_table_+133 }
};
BarfCpp_::Size const Scanner::ms_state_count_ = sizeof(Scanner::ms_state_table_) / sizeof(*Scanner::ms_state_table_);

ReflexCpp_::AutomatonApparatus_::DfaTransition_ const Scanner::ms_transition_table_[] =
{
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+1 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+3 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 255, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 255, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 255, ms_state_table_+2 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+7 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 60, 0, ms_state_table_+8 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 59, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 61, 255, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+5 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+7 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 60, 0, ms_state_table_+8 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 59, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 61, 255, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+9 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+13 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 60, 0, ms_state_table_+8 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 59, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 61, 122, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 123, 124, ms_state_table_+10 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 125, 255, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+11 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+12 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 60, 0, ms_state_table_+8 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 59, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 61, 255, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 60, 0, ms_state_table_+8 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 59, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 61, 255, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 60, 0, ms_state_table_+8 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 59, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 61, 122, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 123, 124, ms_state_table_+10 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 125, 255, ms_state_table_+6 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+15 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+27 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 9, 0, ms_state_table_+17 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 10, 0, ms_state_table_+18 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 32, 0, ms_state_table_+17 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 33, 0, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 34, 0, ms_state_table_+20 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 39, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 48, 0, ms_state_table_+21 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 64, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 91, 0, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 92, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 93, 0, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 94, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 95, 0, ms_state_table_+24 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 96, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 123, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 124, 0, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 125, 0, ms_state_table_+26 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 8, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 11, 31, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 35, 36, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 37, 38, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 40, 47, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 49, 57, ms_state_table_+22 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 58, 59, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 60, 63, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 65, 90, ms_state_table_+24 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 97, 122, ms_state_table_+24 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 126, 255, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 48, 57, ms_state_table_+23 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 48, 57, ms_state_table_+23 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 95, 0, ms_state_table_+25 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 48, 57, ms_state_table_+25 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 65, 90, ms_state_table_+25 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 97, 122, ms_state_table_+25 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 95, 0, ms_state_table_+25 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 48, 57, ms_state_table_+25 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 65, 90, ms_state_table_+25 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 97, 122, ms_state_table_+25 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 9, 0, ms_state_table_+17 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 10, 0, ms_state_table_+18 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 32, 0, ms_state_table_+17 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 33, 0, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 34, 0, ms_state_table_+20 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 39, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 48, 0, ms_state_table_+21 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 64, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 91, 0, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 92, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 93, 0, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 94, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 95, 0, ms_state_table_+24 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 96, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 123, 0, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 124, 0, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 125, 0, ms_state_table_+26 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 8, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 11, 31, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 35, 36, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 37, 38, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 40, 47, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 49, 57, ms_state_table_+22 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 58, 59, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 60, 63, ms_state_table_+19 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 65, 90, ms_state_table_+24 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 97, 122, ms_state_table_+24 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 126, 255, ms_state_table_+16 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+29 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+31 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 34, 0, ms_state_table_+32 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 92, 0, ms_state_table_+34 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 33, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 35, 91, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 93, 255, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+29 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+31 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 34, 0, ms_state_table_+32 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 92, 0, ms_state_table_+34 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 33, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 35, 91, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 93, 255, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+33 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+37 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 34, 0, ms_state_table_+32 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 92, 0, ms_state_table_+34 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 33, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 35, 91, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 93, 255, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 0, ms_state_table_+35 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::CONDITIONAL, 2, 2, ms_state_table_+36 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 255, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 255, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 34, 0, ms_state_table_+32 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM, 92, 0, ms_state_table_+34 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 1, 33, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 35, 91, ms_state_table_+30 },
    { ReflexCpp_::AutomatonApparatus_::DfaTransition_::INPUT_ATOM_RANGE, 93, 255, ms_state_table_+30 }
};
BarfCpp_::Size const Scanner::ms_transition_count_ = sizeof(Scanner::ms_transition_table_) / sizeof(*Scanner::ms_transition_table_);

char const *const Scanner::ms_accept_handler_regex_[] =
{
    "{ANY}+",
    "{END_OF_FILE}",
    "{ANY}*(<\\||<\\{)",
    "{ANY}*{END_OF_FILE}",
    "{WHITESPACE}",
    "{NEWLINE}",
    "{END_OF_FILE}",
    "\\}",
    "{OPERATOR}",
    "{ID}",
    "{INTEGER_LITERAL}",
    "\"",
    "{ANY}",
    "([^\\\\]|\\\\{ANY})*\"",
    "([^\\\\]|\\\\{ANY})*\\\\?{END_OF_FILE}",
    ""
};
BarfCpp_::Uint32 const Scanner::ms_accept_handler_count_ = sizeof(Scanner::ms_accept_handler_regex_) / sizeof(*Scanner::ms_accept_handler_regex_);

// ///////////////////////////////////////////////////////////////////////
// end of internal reflex-generated parser guts
// ///////////////////////////////////////////////////////////////////////


#line 137 "barf_preprocessor_scanner.reflex"

} // end of namespace Preprocessor
} // end of namespace Barf

#line 878 "barf_preprocessor_scanner.cpp"
