// 2006.02.18 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_COMMANDLINEPARSER_HPP_)
#define BARF_COMMANDLINEPARSER_HPP_

#include "barf.hpp"

#include <iostream>

namespace Barf {

struct CommandLineOption;

class CommandLineParser
{
public:

    typedef void (CommandLineParser::*HandlerMethodWithArgument)(string const &);
    typedef void (CommandLineParser::*HandlerMethodWithoutArgument)();

    template <typename CommandLineParserSubclass>
    CommandLineParser (
        void (CommandLineParserSubclass::*non_option_argument_handler_method)(string const &),
        CommandLineOption const *option,
        Uint32 option_count,
        string const &executable_filename,
        string const &program_description,
        string const &usage_message)
        :
        m_non_option_argument_handler_method(static_cast<HandlerMethodWithArgument>(non_option_argument_handler_method)),
        m_option(option),
        m_option_count(option_count),
        m_executable_filename(executable_filename),
        m_program_description(program_description),
        m_usage_message(usage_message),
        m_parse_succeeded(true)
    {
        assert(m_non_option_argument_handler_method != NULL);
        assert(m_option != NULL);
        assert(m_option_count > 0);
        assert(!m_executable_filename.empty());

        PerformOptionConsistencyCheck();
    }
    virtual ~CommandLineParser () = 0;

    inline bool ParseSucceeded () const { return m_parse_succeeded; }

    virtual void Parse (int argc, char const *const *argv);
    void PrintHelpMessage (ostream &stream) const;

private:

    static bool IsAControlOption (CommandLineOption const &option);
    static bool IsAShortNameCollision (CommandLineOption const &option_0, CommandLineOption const &option_1);
    static bool IsALongNameCollision (CommandLineOption const &option_0, CommandLineOption const &option_1);

    void PerformOptionConsistencyCheck () const;

    // the return value indicates if next_arg was eaten up and should be skipped
    bool HandleShortNameOption (char const *arg, char const *next_arg);
    // the return value indicates if next_arg was eaten up and should be skipped
    bool HandleLongNameOption (char const *arg, char const *next_arg);

    CommandLineOption const *FindOptionByShortName (char short_name) const;
    CommandLineOption const *FindOptionByLongName (char const *long_name) const;

    HandlerMethodWithArgument const m_non_option_argument_handler_method;
    CommandLineOption const *const m_option;
    Uint32 const m_option_count;
    string const m_executable_filename;
    string const m_program_description;
    string const m_usage_message;
    bool m_parse_succeeded;
}; // end of class CommandLineParser

struct CommandLineOption
{
public:

    CommandLineOption (string const &header_text)
        :
        m_short_name('\n'),
        m_long_name(),
        m_requires_an_argument(false),
        m_handler_method_with_argument(NULL),
        m_handler_method_without_argument(NULL),
        m_description(header_text)
    { }
    template <typename CommandLineParserSubclass>
    CommandLineOption (
        char short_name,
        void (CommandLineParserSubclass::*handler_method_with_argument)(string const &),
        string const &description)
        :
        m_short_name(short_name),
        m_long_name(),
        m_requires_an_argument(true),
        m_handler_method_with_argument(static_cast<HandlerMethodWithArgument>(handler_method_with_argument)),
        m_handler_method_without_argument(NULL),
        m_description(description)
    { }
    template <typename CommandLineParserSubclass>
    CommandLineOption (
        string const &long_name,
        void (CommandLineParserSubclass::*handler_method_with_argument)(string const &),
        string const &description)
        :
        m_short_name('\0'),
        m_long_name(long_name),
        m_requires_an_argument(true),
        m_handler_method_with_argument(static_cast<HandlerMethodWithArgument>(handler_method_with_argument)),
        m_handler_method_without_argument(NULL),
        m_description(description)
    { }
    template <typename CommandLineParserSubclass>
    CommandLineOption (
        char short_name,
        string const &long_name,
        void (CommandLineParserSubclass::*handler_method_with_argument)(string const &),
        string const &description)
        :
        m_short_name(short_name),
        m_long_name(long_name),
        m_requires_an_argument(true),
        m_handler_method_with_argument(static_cast<HandlerMethodWithArgument>(handler_method_with_argument)),
        m_handler_method_without_argument(NULL),
        m_description(description)
    { }
    template <typename CommandLineParserSubclass>
    CommandLineOption (
        char short_name,
        void (CommandLineParserSubclass::*handler_method_without_argument)(),
        string const &description)
        :
        m_short_name(short_name),
        m_long_name(),
        m_requires_an_argument(false),
        m_handler_method_with_argument(NULL),
        m_handler_method_without_argument(static_cast<HandlerMethodWithoutArgument>(handler_method_without_argument)),
        m_description(description)
    { }
    template <typename CommandLineParserSubclass>
    CommandLineOption (
        string const &long_name,
        void (CommandLineParserSubclass::*handler_method_without_argument)(),
        string const &description)
        :
        m_short_name('\0'),
        m_long_name(long_name),
        m_requires_an_argument(false),
        m_handler_method_with_argument(NULL),
        m_handler_method_without_argument(static_cast<HandlerMethodWithoutArgument>(handler_method_without_argument)),
        m_description(description)
    { }
    template <typename CommandLineParserSubclass>
    CommandLineOption (
        char short_name,
        string const &long_name,
        void (CommandLineParserSubclass::*handler_method_without_argument)(),
        string const &description)
        :
        m_short_name(short_name),
        m_long_name(long_name),
        m_requires_an_argument(false),
        m_handler_method_with_argument(NULL),
        m_handler_method_without_argument(static_cast<HandlerMethodWithoutArgument>(handler_method_without_argument)),
        m_description(description)
    { }

private:

    typedef void (CommandLineParser::*HandlerMethodWithArgument)(string const &);
    typedef void (CommandLineParser::*HandlerMethodWithoutArgument)();

    char const m_short_name;
    string const m_long_name;
    bool const m_requires_an_argument;
    HandlerMethodWithArgument const m_handler_method_with_argument;
    HandlerMethodWithoutArgument const m_handler_method_without_argument;
    string const m_description;

    // kludgey, but it's using this as a struct anyway
    friend class CommandLineParser;
}; // end of struct CommandLineOption

} // end of namespace Barf

#endif // !defined(BARF_COMMANDLINEPARSER_HPP_)
