// 2006.11.10 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_INPUTBASE_HPP_)
#define BARF_INPUTBASE_HPP_

#include "barf.hpp"

#include <fstream>
#include <sstream>

#include "barf_filoc.hpp"

namespace Barf {

// provides input facilities -- OpenFile(), OpenString(), OpenUsingStream()
// and Close() -- scanners and other stuff should inherit this.
class InputBase
{
public:

    InputBase ();
    ~InputBase ();

    bool IsOpen () const;
    FiLoc const &GetFiLoc () const { return m_filoc; }
    string const &InputName () const { return m_filoc.Filename(); }

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
    FiLoc m_filoc;
}; // end of class InputBase

} // end of namespace Barf

#endif // !defined(BARF_INPUTBASE_HPP_)
