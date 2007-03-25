// ///////////////////////////////////////////////////////////////////////////
// barf_inputbase.hpp by Victor Dods, created 2006/11/10
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_INPUTBASE_HPP_)
#define _BARF_INPUTBASE_HPP_

#include "barf.hpp"

#include <fstream>
#include <sstream>

#include "barf_filelocation.hpp"

namespace Barf {

// provides input facilities -- OpenFile(), OpenString(), OpenUsingStream()
// and Close() -- scanners and other stuff should inherit this.
class InputBase
{
public:

    InputBase ();
    ~InputBase ();

    bool GetIsOpen () const;
    FileLocation const &GetFileLocation () const { return m_file_location; }
    string const &GetInputName () const { return m_file_location.GetFilename(); }

    bool OpenFile (string const &input_filename);
    void OpenString (string const &input_string, string const &input_name, bool use_line_numbers = false);
    void OpenUsingStream (istream *input_stream, string const &input_name, bool use_line_numbers);

    // if you supplied your own istream to OpenUsingStream(), a call to Close()
    // will NOT close your istream.
    bool Close ();

protected:

    inline istream &In ()
    {
        assert(m_input_stream != NULL && "no input stream attached");
        return *m_input_stream;
    }

    // this will only increment the line number if line numbers are in use.
    void IncrementLineNumber (Uint32 by_value = 1);

private:

    istringstream m_istringstream;
    ifstream m_ifstream;
    istream *m_input_stream;
    FileLocation m_file_location;
}; // end of class InputBase

} // end of namespace Barf

#endif // !defined(_BARF_INPUTBASE_HPP_)
