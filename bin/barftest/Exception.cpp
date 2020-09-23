// 2016.08.16 - Victor Dods

#include "barftest/Exception.hpp"

namespace barftest {

Error::~Error () { }

ProgrammerError::~ProgrammerError () { }

TypeError::~TypeError () { }

WellFormednessError::~WellFormednessError () { }

} // end namespace barftest
