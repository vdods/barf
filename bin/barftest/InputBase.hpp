// 2006.11.10 - Victor Dods

#pragma once

#include <cstdint>
#include "barftest/FiLoc.hpp"
#include "barftest/FiPos.hpp"
#include "barftest/FiRange.hpp"
#include <fstream>
#include <sstream>

namespace barftest {

// Provides input facilities -- open_file(), open_string(), open_using_stream()
// and close() -- scanners and other stuff should inherit this.
struct InputBase
{
    InputBase ();
    ~InputBase ();

    bool is_open () const;
    std::string const &input_name () const { return m_input_name; }
    FiLoc filoc () const;
    FiPos const &fipos () const { return m_fipos; }
    FiRange firange () const;

    bool open_file (std::string const &input_filename);
    void open_string (std::string const &input_string, std::string const &input_name, bool use_line_numbers = false);
    void open_using_stream (std::istream *input_stream, std::string const &input_name, bool use_line_numbers);

    // if you supplied your own std::istream to open_using_stream(), a call to close()
    // will NOT close your std::istream.
    bool close ();

protected:

    std::istream &in ()
    {
        assert(m_input_stream != NULL && "no input stream attached");
        return *m_input_stream;
    }

    // This and set_column_number can be used to efficiently track the current line and column
    // if newlines are automatically tracked.  If not (e.g. accepting multi-line tokens),
    // then UpdateLineAndColumn should be used, passing in the string to advance the line
    // and column numbers by.
    void increment_line_number (uint32_t by_value = 1);
    void set_column_number (uint32_t column);
    void increment_column_number (uint32_t by_value);

private:

    std::istringstream m_istringstream;
    std::ifstream m_ifstream;
    std::istream *m_input_stream;

    bool m_use_line_numbers;
    std::string m_input_name;
    FiPos m_fipos;
}; // end of class InputBase

} // end namespace barftest
