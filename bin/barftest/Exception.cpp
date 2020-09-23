// 2016.08.16 - Victor Dods

#include "Exception.hpp"

namespace cbz {

Error::~Error () { }

ProgrammerError::~ProgrammerError () { }

TypeError::~TypeError () { }

WellFormednessError::~WellFormednessError () { }

} // end namespace cbz
