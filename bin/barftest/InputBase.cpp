// 2006.11.10 - Victor Dods

#include "InputBase.hpp"

namespace cbz {

InputBase::InputBase ()
    :   m_input_stream(NULL)
    ,   m_use_line_numbers(false)
    ,   m_input_name() // empty
    ,   m_fipos(1, 1) // starting location
{ }

InputBase::~InputBase ()
{
    close();
}

bool InputBase::is_open () const
{
    // if no input stream, it can't be open.
    if (m_input_stream == NULL)
        return false;

    // if the input stream is an std::ifstream, use the is_open method.
    if (dynamic_cast<std::ifstream *>(m_input_stream) != NULL)
        return static_cast<std::ifstream *>(m_input_stream)->is_open();

    // otherwise, it's always open.
    return true;
}

FiLoc InputBase::filoc () const
{
    if (m_input_name.empty())
        return FiLoc::INVALID;

    FiLoc retval(m_input_name);
    assert(!retval.has_line_number());
    if (m_use_line_numbers)
        retval.set_line_number(m_fipos.line());
    return retval;
}

FiRange InputBase::firange () const
{
    if (m_input_name.empty() || !m_fipos.is_valid())
        return FiRange::INVALID;

    return FiRange(m_input_name, m_fipos);
}

bool InputBase::open_file (std::string const &input_filename)
{
    close();

    assert(!m_ifstream.is_open());
    m_ifstream.open(input_filename.c_str());
    if (!m_ifstream.is_open())
        return false;

    m_input_stream = &m_ifstream;
    m_input_stream->unsetf(std::ios_base::skipws);

    m_use_line_numbers = true;
    m_input_name = input_filename;
    m_fipos = FiPos(1, 1);

    return true;
}

void InputBase::open_string (std::string const &input_string, std::string const &input_name, bool use_line_numbers)
{
    close();

    assert(m_istringstream.str().empty());
    m_istringstream.str(input_string);
    m_istringstream.clear(); // clear the eof and other flags

    m_input_stream = &m_istringstream;
    m_input_stream->unsetf(std::ios_base::skipws);

    m_use_line_numbers = use_line_numbers;
    m_input_name = input_name;
    m_fipos = FiPos(1, 1);
}

void InputBase::open_using_stream (std::istream *input_stream, std::string const &input_name, bool use_line_numbers)
{
    assert(input_stream != NULL);

    close();

    m_input_stream = input_stream;
    m_input_stream->unsetf(std::ios_base::skipws);

    m_use_line_numbers = use_line_numbers;
    m_input_name = input_name;
    m_fipos = FiPos(1, 1);
}

bool InputBase::close ()
{
    if (m_input_stream != NULL)
    {
        assert(is_open());
        if (m_input_stream == &m_istringstream)
            m_istringstream.str("");
        else if (m_input_stream == &m_ifstream)
            m_ifstream.close();
        m_input_stream = NULL;

        m_use_line_numbers = false;
        m_input_name.clear();
        m_fipos = FiPos(1, 1);
        return true;
    }
    else
    {
        assert(!is_open());
        assert(m_use_line_numbers == false);
        assert(m_input_name.empty());
        return false;
    }
}

void InputBase::increment_line_number (uint32_t by_value)
{
    m_fipos.increment_line(by_value);
}

void InputBase::set_column_number (uint32_t column)
{
    m_fipos.set_column(column);
}

void InputBase::increment_column_number (uint32_t by_value)
{
    m_fipos.increment_column(by_value);
}

} // end namespace cbz
