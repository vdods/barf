// 2006.10.14 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_HPP_)
#define BARF_HPP_

// EVERY HPP AND CPP FILE IN BARF SHOULD INCLUDE THIS FILE
// EVERY HPP AND CPP FILE IN BARF SHOULD INCLUDE THIS FILE
// EVERY HPP AND CPP FILE IN BARF SHOULD INCLUDE THIS FILE

#include <cassert>
#include <string>

using namespace std;

#if defined(HAVE_CONFIG_H)
    #include "config.h"
#endif // defined(HAVE_CONFIG_H)

#include "barf_types.hpp"

// set the value of DEBUG (which must be defined and be 0 or 1)
#if defined(NDEBUG)
    #define DEBUG 0
#else
    #define DEBUG 1
#endif

// these either will go away, or will be specified by config.h (or whatever)
#define DIRECTORY_SLASH_CHAR '/'
#define DIRECTORY_SLASH_STRING "/"
// handy li'l macro for stringifying a boolean
#define BOOL_TO_STRING(x) ((x) ? "true" : "false")
// handy li'l macro to throw a string using stringstream semantics
#define THROW_STRING(x) do { ostringstream out; out << x; throw out.str(); } while (false)
// super handy inline ostringstream formatting macro -- returns std::string
#define FORMAT(x) static_cast<ostringstream &>(ostringstream().flush() << x).str()

namespace Barf {

// useful little ready-made empty string
extern string const g_empty_string;

// this should be used as a "safe" static_cast -- in debug mode, it asserts
// on the validity of the dynamic_cast result if the pointer isn't NULL to
// begin with.  in release mode, it just uses static_cast.
template <typename CastToType, typename CastFromType>
inline CastToType Dsc (CastFromType cast_from)
{
    assert(cast_from == static_cast<CastFromType>(0) ||
           dynamic_cast<CastToType>(cast_from) != static_cast<CastToType>(0));
    return static_cast<CastToType>(cast_from);
}

// useful in std::for_each()
template <typename T>
struct DeleteFunctor
{
    void operator () (T *pointer)
    {
        delete pointer;
    }
}; // end of struct DeleteFunctor<T>

// also useful in std::for_each()
template <typename T>
struct DeletePairSecondFunctor
{
    void operator () (T pair)
    {
        delete pair.second;
    }
}; // end of struct DeletePairSecondFunctor<T>

// for testing for existence in an STL container
template <typename ContainerType, typename KeyType>
bool Contains (ContainerType &container, KeyType const &key)
{
    return container.find(key) != container.end();
}

// for testing for existence in an STL container and returning the result
// from find() in the given iterator reference.
template <typename ContainerType, typename KeyType, typename IteratorType>
bool Contains (ContainerType &container, KeyType const &key, IteratorType &it)
{
    return (it = container.find(key)) != container.end();
}

} // end of namespace Barf

#endif // !defined(BARF_HPP_)
