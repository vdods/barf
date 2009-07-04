// ///////////////////////////////////////////////////////////////////////////
// barf_inputbase.cpp by Victor Dods, created 2006/11/10
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_inputbase.hpp"

namespace Barf {

InputBase::InputBase ()
    :
    m_input_stream(NULL),
    m_filoc(FileLocation::ms_invalid)
{ }

InputBase::~InputBase ()
{
    Close();
}

bool InputBase::IsOpen () const
{
    // if no input stream, it can't be open.
    if (m_input_stream == NULL)
        return false;

    // if the input stream is an ifstream, use the is_open method.
    if (dynamic_cast<ifstream *>(m_input_stream) != NULL)
        return static_cast<ifstream *>(m_input_stream)->is_open();

    // otherwise, it's always open.
    return true;
}

bool InputBase::OpenFile (string const &input_filename)
{
    Close();

    assert(!m_ifstream.is_open());
    m_ifstream.open(input_filename.c_str());
    if (!m_ifstream.is_open())
        return false;

    m_input_stream = &m_ifstream;
    m_input_stream->unsetf(ios_base::skipws);
    m_filoc.SetFilename(input_filename);
    m_filoc.SetLineNumber(1);

    return true;
}

void InputBase::OpenString (string const &input_string, string const &input_name, bool use_line_numbers)
{
    Close();

    assert(m_istringstream.str().empty());
    m_istringstream.str(input_string);
    m_istringstream.clear(); // clear the eof and other flags

    m_input_stream = &m_istringstream;
    m_input_stream->unsetf(ios_base::skipws);
    m_filoc.SetFilename(input_name);
    m_filoc.SetLineNumber(use_line_numbers ? 1 : 0);
}

void InputBase::OpenUsingStream (istream *input_stream, string const &input_name, bool use_line_numbers)
{
    assert(input_stream != NULL);

    Close();

    m_input_stream = input_stream;
    m_input_stream->unsetf(ios_base::skipws);
    m_filoc.SetFilename(input_name);
    m_filoc.SetLineNumber(use_line_numbers ? 1 : 0);
}

bool InputBase::Close ()
{
    if (m_input_stream != NULL)
    {
        assert(IsOpen());
        if (m_input_stream == &m_istringstream)
            m_istringstream.str("");
        else if (m_input_stream == &m_ifstream)
            m_ifstream.close();
        m_input_stream = NULL;
        m_filoc.SetFilename("");
        m_filoc.SetLineNumber(0);
        return true;
    }
    else
    {
        assert(!IsOpen());
        assert(!m_filoc.IsValid());
        return false;
    }
}

void InputBase::IncrementLineNumber (Uint32 by_value)
{
    if (m_filoc.LineNumber() > 0)
        m_filoc.IncrementLineNumber(by_value);
}

} // end of namespace Barf
