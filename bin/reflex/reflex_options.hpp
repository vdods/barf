// 2006.10.20 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(REFLEX_OPTIONS_HPP_)
#define REFLEX_OPTIONS_HPP_

#include "reflex.hpp"

#include "barf_optionsbase.hpp"

namespace Reflex {

class Options : public OptionsBase
{
public:

    Options (string const &executable_filename);

    virtual void Parse (int argc, char const *const *argv);

private:

    static CommandLineOption const ms_option[];
    static Uint32 const ms_option_count;
}; // end of class Options

} // end of namespace Reflex

inline Reflex::Options const &ReflexOptions () { return *Dsc<Reflex::Options const *>(&GetOptions()); }

#endif // !defined(REFLEX_OPTIONS_HPP_)
