// 2006.11.04 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BPP_OPTIONS_HPP_)
#define BPP_OPTIONS_HPP_

#include "bpp.hpp"

#include "barf_optionsbase.hpp"

namespace Bpp {

class Options : public OptionsBase
{
public:

    Options (string const &executable_filename);

    string const &OutputFilename () const { return m_output_filename; }

    virtual void Parse (int argc, char const *const *argv);

    // output options
    void SetOutputFilename (string const &output_path);

private:

    // output option values
    string m_output_filename;

    static CommandLineOption const ms_option[];
    static Uint32 const ms_option_count;
}; // end of class Options

} // end of namespace Bpp

inline Bpp::Options const &BppOptions () { return *Dsc<Bpp::Options const *>(&GetOptions()); }

#endif // !defined(BPP_OPTIONS_HPP_)
