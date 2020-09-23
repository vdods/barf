// 2006.02.11 - Copyright Victor Dods - Licensed under Apache 2.0

#include "barf_filoc.hpp"

#include <sstream>

#include "barf_message.hpp"
#include "barf_path.hpp"
#include "barf_util.hpp"

namespace Barf {

FiLoc const FiLoc::ms_invalid;

string FiLoc::AsString () const
{
    assert(this != &ms_invalid && "can't use FiLoc::ms_invalid in this manner");
    assert(IsValid());

    ostringstream out;
    out << m_filename;
    if (m_line_number > 0)
        out << ":" << m_line_number;
    return out.str();
}

string FiLoc::LineDirectiveString (string const &relative_to_path) const
{
    assert(this != &ms_invalid && "do not use this on a FiLoc without a line number");
    assert(IsValid());

    string line_directive_path;
    if (!relative_to_path.empty())
        try {
            line_directive_path = Path(m_filename).make_absolute().relative_to(relative_to_path).as_string();
        } catch (std::exception const &e) {
            EmitWarning(string("caught exception while trying to generate #line directive path \"" + m_filename + "\" relative to path \"" + relative_to_path + "\"; exception was \"") + e.what() + "\"");
            line_directive_path = FilenamePortion(m_filename);
        }
    else
        line_directive_path = FilenamePortion(m_filename);

    ostringstream out;
    out << "#line " << m_line_number << " \"" << line_directive_path << "\"";
    return out.str();
}

void FiLoc::IncrementLineNumber (Uint32 by_value)
{
    assert(m_line_number > 0 && "don't use this on non-line-number-using FiLocs");
    m_line_number += by_value;
}

ostream &operator << (ostream &stream, FiLoc const &filoc)
{
    return stream << filoc.AsString();
}

} // end of namespace Barf
