// 2020.08.26 - Victor Dods

#pragma once

#include <lvd/g_log.hpp>
#include <lvd/Log.hpp>

namespace barftest {

// This is to avoid having to qualify the symbol all the time, and to place it within the barftest
// namespace once that actually exists.
using lvd::Log;
// Helper types
using lvd::Indent;
using lvd::IndentGuard;
using lvd::PopPrefix;
using lvd::Prefix;
using lvd::PrefixGuard;
using lvd::PushPrefix;
using lvd::Unindent;

// Global Log object.
using lvd::g_log;

} // end namespace barftest
