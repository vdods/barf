// ///////////////////////////////////////////////////////////////////////////
// barf_commandlineparser.cpp by Victor Dods, created 2006/02/18
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#include "barf_commandlineparser.hpp"

namespace Barf {

CommandLineParser::~CommandLineParser () { }

void CommandLineParser::Parse (int argc, char const *const *argv)
{
    assert(argc >= 1);
    assert(argv != NULL);

    try
    {
        while (++argv, --argc > 0)
        {
            assert(*argv != NULL);
            char const *arg = *argv;
            if (*arg == '-' && arg != string("-") && arg != string("--"))
            {
                ++arg;

                char const *next_arg = (argc == 1) ? NULL : *(argv + 1);

                if (*arg == '-')
                {
                    if (HandleLongNameOption(++arg, next_arg))
                        ++argv, --argc;
                }
                else
                {
                    if (HandleShortNameOption(arg, next_arg))
                        ++argv, --argc;
                }
            }
            else
            {
                (this->*m_non_option_argument_handler_method)(arg);
            }
        }
    }
    catch (string const &exception)
    {
        cerr << exception << "\n";
        m_parse_succeeded = false;
    }
}

void CommandLineParser::PrintHelpMessage (ostream &stream) const
{
    if (!m_program_description.empty())
        stream << m_program_description << "\n\n";

    stream << "usage: " << m_executable_filename << " " << m_usage_message << "\n";

    if (!IsAControlOption(m_option[0]))
        stream << "\n";

    for (CommandLineOption const *option = m_option,
                                 *option_end = m_option + m_option_count;
         option != option_end;
         ++option)
    {
        if (IsAControlOption(*option))
        {
            if (option->m_description.empty())
                stream << endl;
            else
                stream << "\n" << option->m_description << "\n" << endl;
        }
        else
        {
            assert(option->m_short_name != '\0' || !option->m_long_name.empty());

            if (option->m_short_name != '\0')
                stream << "-" << option->m_short_name;

            if (option->m_short_name != '\0' && !option->m_long_name.empty())
                stream << ",";

            if (!option->m_long_name.empty())
                stream << "--" << option->m_long_name;

            if (option->m_requires_an_argument)
                stream << " <argument>";

            if (!option->m_description.empty())
                stream << "\n" << option->m_description;

            stream << endl;
        }
    }
}

bool CommandLineParser::IsAControlOption (CommandLineOption const &option)
{
    return option.m_short_name == '\n';
}

bool CommandLineParser::IsAShortNameCollision (CommandLineOption const &option_0, CommandLineOption const &option_1)
{
    if (IsAControlOption(option_0) || IsAControlOption(option_1))
        return false;

    if (option_0.m_short_name == '\0' || option_1.m_short_name == '\0')
        return false;

    return option_0.m_short_name == option_1.m_short_name;
}

bool CommandLineParser::IsALongNameCollision (CommandLineOption const &option_0, CommandLineOption const &option_1)
{
    if (IsAControlOption(option_0) || IsAControlOption(option_1))
        return false;

    if (option_0.m_long_name.empty() || option_1.m_long_name.empty())
        return false;

    return option_0.m_long_name == option_1.m_long_name;
}

void CommandLineParser::PerformOptionConsistencyCheck () const
{
    // make sure if a special short name is specified, then the
    // long name isn't specified.
    for (CommandLineOption const *option = m_option,
                                 *option_end = m_option + m_option_count;
         option != option_end;
         ++option)
    {
        if (IsAControlOption(*option))
        {
            assert(option->m_long_name.empty() && "must not supply a long name for a control option");
            assert(option->m_handler_method_with_argument == NULL && "must not supply a handler method for a control option");
            assert(option->m_handler_method_without_argument == NULL && "must not supply a handler method for a control option");
        }
        else
        {
            if (option->m_short_name != '\0')
                assert(option->m_short_name != ' ' && option->m_short_name != '\t' && option->m_short_name != '\n' && "must not use whitespace in option short names");

            if (!option->m_long_name.empty())
                assert(option->m_long_name.find_first_of(" \t\n") == string::npos && "must not use whitespace in option long names");

            if (option->m_requires_an_argument)
            {
                assert(option->m_handler_method_with_argument != NULL && "must specify an argument-accepting handler method for a normal argument-accepting option");
                assert(option->m_handler_method_without_argument == NULL && "must not specify a no-argument handler method for a normal argument-accepting option");
            }
            else
            {
                assert(option->m_handler_method_with_argument == NULL && "must not specify an argument-accepting handler method for a normal no-argument option");
                assert(option->m_handler_method_without_argument != NULL && "must specify a no-argument handler method for a normal no-argument option");
            }
        }
    }

    // make sure that there are no short/long option name collisions.
    for (CommandLineOption const *option_0 = m_option,
                                 *option_end_0 = m_option + m_option_count;
         option_0 != option_end_0;
         ++option_0)
    {
        // loop over all options after this one in the array, so that
        // every option is compared with every other option.
        for (CommandLineOption const *option_1 = option_0 + 1;
             option_1 != option_end_0;
             ++option_1)
        {
            assert(!IsAShortNameCollision(*option_0, *option_1) && "option short-name collision");
            assert(!IsALongNameCollision(*option_0, *option_1) && "option long-name collision");
        }
    }
}

bool CommandLineParser::HandleShortNameOption (
    char const *arg,
    char const *const next_arg)
{
    assert(arg != NULL);
    // next_arg may be NULL

    if (*arg == '\0')
        throw string("error: no option letter specified after \"-\"");

    CommandLineOption const *option = FindOptionByShortName(*arg);
    if (option == NULL)
        throw string("error: no such option: -") + *arg;

    ++arg;

    if (*arg != '\0')
    {
        // if the option takes an argument, call the option handler
        // method with the argument.
        if (option->m_requires_an_argument)
        {
            assert(option->m_handler_method_with_argument != NULL);
            assert(option->m_handler_method_without_argument == NULL);
            (this->*(option->m_handler_method_with_argument))(string(arg));
        }
        // otherwise this argument is a concatenated string of short options
        else
        {
            // iterate through the concatenated short options, processing
            // each one, but throwing an error if one requires an argument
            while (true)
            {
                // call the option handler method with an empty argument
                assert(option->m_handler_method_with_argument == NULL);
                assert(option->m_handler_method_without_argument != NULL);
                (this->*(option->m_handler_method_without_argument))();

                if (*arg == '\0')
                    break;

                option = FindOptionByShortName(*arg);
                if (option == NULL)
                    throw string("error: no such option: -") + *arg;
                else if (option->m_requires_an_argument)
                    throw string("error: may not concatenate short options which require an argument (option -") + *arg + ")";

                ++arg;
            }
        }

        return false;
    }
    else
    {
        if (option->m_requires_an_argument)
        {
            if (next_arg == NULL)
                throw string("error: option -") + option->m_short_name + " requires an argument";

            // call the option handler method with the argument
            assert(option->m_handler_method_with_argument != NULL);
            assert(option->m_handler_method_without_argument == NULL);
            (this->*(option->m_handler_method_with_argument))(string(next_arg));

            return true;
        }
        else
        {
            // call the option handler method with an empty argument
            assert(option->m_handler_method_with_argument == NULL);
            assert(option->m_handler_method_without_argument != NULL);
            (this->*(option->m_handler_method_without_argument))();

            return false;
        }
    }
}

bool CommandLineParser::HandleLongNameOption (
    char const *arg,
    char const *const next_arg)
{
    assert(arg != NULL);
    // next_arg may be NULL

    if (*arg == '\0')
        throw string("error: no option name specified after \"--\"");

    CommandLineOption const *option = FindOptionByLongName(arg);
    if (option == NULL)
        throw string("error: no such option --") + arg;

    arg += option->m_long_name.length();

    if (*arg != '\0')
    {
        if (!option->m_requires_an_argument)
            throw string("error: option --") + option->m_long_name + " does not take an argument";

        assert(*arg == '=');
        ++arg;

        // call the option handler method with the argument
        assert(option->m_handler_method_with_argument != NULL);
        assert(option->m_handler_method_without_argument == NULL);
        (this->*(option->m_handler_method_with_argument))(string(arg));

        return false;
    }
    else
    {
        if (option->m_requires_an_argument)
        {
            if (next_arg == NULL)
                throw string("error: option --") + option->m_long_name + " requires an argument";

            // call the option handler method with the argument
            assert(option->m_handler_method_with_argument != NULL);
            assert(option->m_handler_method_without_argument == NULL);
            (this->*(option->m_handler_method_with_argument))(string(next_arg));

            return true;
        }
        else
        {
            // call the option handler method with an empty argument
            assert(option->m_handler_method_with_argument == NULL);
            assert(option->m_handler_method_without_argument != NULL);
            (this->*(option->m_handler_method_without_argument))();

            return false;
        }
    }
}

CommandLineOption const *CommandLineParser::FindOptionByShortName (char const short_name) const
{
    assert(short_name != '\0');

    for (CommandLineOption const *option = m_option,
                                 *option_end = m_option + m_option_count;
         option != option_end;
         ++option)
    {
        if (option->m_short_name != '\0' && short_name == option->m_short_name)
            return option;
    }

    return NULL;
}

CommandLineOption const *CommandLineParser::FindOptionByLongName (char const *const long_name) const
{
    assert(long_name != NULL);
    assert(*long_name != '\0');

    string option_name = long_name;
    if (option_name.find_first_of('=') != string::npos)
        option_name = option_name.substr(0, option_name.find_first_of('='));

    for (CommandLineOption const *option = m_option,
                                 *option_end = m_option + m_option_count;
         option != option_end;
         ++option)
    {
        if (!option->m_long_name.empty() && option_name == option->m_long_name)
            return option;
    }

    return NULL;
}

} // end of namespace Barf
