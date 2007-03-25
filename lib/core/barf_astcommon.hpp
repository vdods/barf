// ///////////////////////////////////////////////////////////////////////////
// barf_astcommon.hpp by Victor Dods, created 2006/10/22
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_ASTCOMMON_HPP_)
#define _BARF_ASTCOMMON_HPP_

#include "barf.hpp"

#include <ostream>
#include <map>
#include <vector>

#include "barf_filelocation.hpp"
#include "barf_message.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace AstCommon {

// when the parser subsystem is defining its own concrete Ast subclasses, it
// should start its enum values at AT_START_CUSTOM_TYPES_HERE_.
enum
{
    AT_THROW_AWAY = 0,
    AT_CHARACTER,
    AT_SIGNED_INTEGER,
    AT_UNSIGNED_INTEGER,
    AT_STRING,
    AT_IDENTIFIER,
    AT_DUMB_CODE_BLOCK,
    AT_STRICT_CODE_BLOCK,
    AT_DIRECTIVE,
    AT_IDENTIFIER_LIST,
    AT_IDENTIFIER_MAP,
    AT_DIRECTIVE_LIST,

    AT_START_CUSTOM_TYPES_HERE_,

    AT_NONE = AstType(-1)
};

string const &GetAstTypeString (AstType ast_type);

class Ast
{
public:

    Ast (FileLocation const &file_location, AstType ast_type)
        :
        m_file_location(file_location),
        m_ast_type(ast_type)
    { }
    virtual ~Ast () { }

    inline FileLocation const &GetFileLocation () const { return m_file_location; }
    inline AstType GetAstType () const { return m_ast_type; }

    virtual bool GetIsCodeBlock () const { return false; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    inline void SetFileLocation (FileLocation const &file_location)
    {
        assert(file_location.GetHasLineNumber());
        m_file_location = file_location;
    }

private:

    FileLocation m_file_location;
    AstType const m_ast_type;
}; // end of class Ast

template <typename ElementType>
struct List : private vector<ElementType *>
{
    typedef typename vector<ElementType *>::iterator iterator;
    typedef typename vector<ElementType *>::const_iterator const_iterator;

    using vector<ElementType *>::size;
    using vector<ElementType *>::empty;
    using vector<ElementType *>::clear;
    using vector<ElementType *>::begin;
    using vector<ElementType *>::end;

    virtual ~List ()
    {
        for_each(begin(), end(), DeleteFunctor<ElementType>());
    }

    inline ElementType const *GetElement (Uint32 index) const
    {
        assert(index < size());
        return vector<ElementType *>::operator[](index);
    }
    inline ElementType *GetElement (Uint32 index)
    {
        assert(index < size());
        return vector<ElementType *>::operator[](index);
    }

    virtual void Append (ElementType *element)
    {
        assert(element != NULL);
        vector<ElementType *>::push_back(element);
    }
}; // end of class List<ElementType>

template <typename ElementType>
struct AstList : public Ast, public List<ElementType>
{
    typedef typename List<ElementType>::iterator iterator;
    typedef typename List<ElementType>::const_iterator const_iterator;

    using List<ElementType>::size;
    using List<ElementType>::empty;
    using List<ElementType>::clear;
    using List<ElementType>::begin;
    using List<ElementType>::end;

    AstList (AstType ast_type)
        :
        Ast(FileLocation::ms_invalid, ast_type),
        List<ElementType>()
    { }
    AstList (FileLocation const &file_location, AstType ast_type)
        :
        Ast(file_location, ast_type),
        List<ElementType>()
    {
        assert(file_location.GetHasLineNumber());
    }

    virtual void Append (ElementType *element)
    {
        assert(element != NULL);
        List<ElementType>::Append(element);
        // if this list doesn't yet have a valid file location, and
        // the appending element does, use it as the list's file location
        if (!GetFileLocation().GetHasLineNumber() && element->GetFileLocation().GetHasLineNumber())
            SetFileLocation(element->GetFileLocation());
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const
    {
        stream << Tabs(indent_level) << Stringify(GetAstType()) << " (" << size() << " elements)" << endl;
        for_each(begin(), end(), PrintAst<Ast>(stream, Stringify, indent_level+1));
    }
}; // end of struct AstList<ElementType>

template <typename ElementType>
struct Map : private map<string, ElementType *>
{
    typedef typename map<string, ElementType *>::iterator iterator;
    typedef typename map<string, ElementType *>::const_iterator const_iterator;

    using map<string, ElementType *>::size;
    using map<string, ElementType *>::empty;
    using map<string, ElementType *>::find;
    using map<string, ElementType *>::clear;
    using map<string, ElementType *>::begin;
    using map<string, ElementType *>::end;

    virtual ~Map ()
    {
        for_each(begin(), end(), DeletePairSecondFunctor<typename map<string, ElementType *>::value_type>());
    }

    inline ElementType const *GetElement (string const &key) const
    {
        const_iterator it = find(key);
        return (it != end()) ? it->second : NULL;
    }
    inline ElementType *GetElement (string const &key)
    {
        iterator it = find(key);
        return (it != end()) ? it->second : NULL;
    }

    virtual void Add (string const &key, ElementType *element)
    {
        assert(element != NULL);
        iterator it = find(key);
        if (it != end())
            EmitError(element->GetFileLocation(), "key \"" + key + "\" previously specified at " + it->second->GetFileLocation().GetAsString());
        else
            map<string, ElementType *>::operator[](key) = element;
    }
}; // end of class Map<ElementType>

template <typename ElementType>
struct AstMap : public Ast, public Map<ElementType>
{
    typedef typename Map<ElementType>::iterator iterator;
    typedef typename Map<ElementType>::const_iterator const_iterator;

    using Map<ElementType>::size;
    using Map<ElementType>::empty;
    using Map<ElementType>::find;
    using Map<ElementType>::clear;
    using Map<ElementType>::begin;
    using Map<ElementType>::end;

    AstMap (AstType ast_type)
        :
        Ast(FileLocation::ms_invalid, ast_type),
        Map<ElementType>()
    { }
    AstMap (FileLocation const &file_location, AstType ast_type)
        :
        Ast(file_location, ast_type),
        Map<ElementType>()
    {
        assert(file_location.GetHasLineNumber());
    }

    virtual void Add (string const &key, ElementType *element)
    {
        assert(element != NULL);
        Map<ElementType>::Add(key, element);
        // if this map doesn't yet have a valid file location, and
        // the appending element does, use it as the map's file location
        if (!GetFileLocation().GetHasLineNumber() && element->GetFileLocation().GetHasLineNumber())
            SetFileLocation(element->GetFileLocation());
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const
    {
        stream << Tabs(indent_level) << Stringify(GetAstType()) << " (" << size() << " elements)" << endl;
        for (const_iterator it = begin(),
                            it_end = end();
             it != it_end;
             ++it)
        {
            stream << Tabs(indent_level+1) << "key \"" + it->first + "\"" << endl;
            it->second->Print(stream, Stringify, indent_level+2);
        }
    }
}; // end of struct AstMap<ElementType>

struct ThrowAway : public Ast
{
    ThrowAway (FileLocation const &file_location)
        :
        Ast(file_location, AT_THROW_AWAY)
    {
        assert(file_location.GetIsValid());
    }
}; // end of struct ThrowAway

class Character : public Ast
{
public:

    Character (Uint8 character, FileLocation const &file_location)
        :
        Ast(file_location, AT_CHARACTER),
        m_character(character)
    { }

    inline Uint8 GetCharacter () const { return m_character; }
    inline string GetCharacterLiteral () const { return Barf::GetCharacterLiteral(m_character); }

    virtual void Escape ();
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    Character (Uint8 character, FileLocation const &file_location, AstType ast_type)
        :
        Ast(file_location, ast_type),
        m_character(character)
    { }

    Uint8 m_character;
}; // end of class Character

class SignedInteger : public Ast
{
public:

    SignedInteger (Sint32 value, FileLocation const &file_location)
        :
        Ast(file_location, AT_SIGNED_INTEGER),
        m_value(value)
    { }

    inline Sint32 GetValue () const { return m_value; }
    inline void SetValue (Sint32 value) { m_value = value; }

    void ShiftAndAdd (Sint32 value);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Sint32 m_value;
}; // end of class SignedInteger

class UnsignedInteger : public Ast
{
public:

    UnsignedInteger (Uint32 value, FileLocation const &file_location)
        :
        Ast(file_location, AT_UNSIGNED_INTEGER),
        m_value(value)
    { }

    inline Uint32 GetValue () const { return m_value; }
    inline void SetValue (Uint32 value) { m_value = value; }

    void ShiftAndAdd (Uint32 value);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Uint32 m_value;
}; // end of class UnsignedInteger

class TextBase : public Ast
{
public:

    TextBase (FileLocation const &file_location, AstType ast_type)
        :
        Ast(file_location, ast_type)
    { }
    TextBase (string const &text, FileLocation const &file_location, AstType ast_type)
        :
        Ast(file_location, ast_type),
        m_text(text)
    { }
    virtual ~TextBase () = 0;

    static string GetDirectiveTypeString (AstType ast_type);

    inline string const &GetText () const { return m_text; }

    inline void AppendText (string const &text) { m_text += text; }
    inline void AppendCharacter (char character) { m_text += character; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    string m_text;
}; // end of class TextBase

struct String : public TextBase
{
    String (FileLocation const &file_location)
        :
        TextBase(file_location, AT_STRING)
    { }
    String (string const &str, FileLocation const &file_location)
        :
        TextBase(str, file_location, AT_STRING)
    { }

    inline string GetStringLiteral () const { return Barf::GetStringLiteral(GetText()); }
}; // end of struct String

struct Identifier : public TextBase
{
    Identifier (string const &identifier_text, FileLocation const &file_location)
        :
        TextBase(identifier_text, file_location, AT_IDENTIFIER)
    {
        assert(!GetText().empty());
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct Identifier

struct CodeBlock : public TextBase
{
    CodeBlock (FileLocation const &file_location, AstType ast_type)
        :
        TextBase(file_location, ast_type)
    { }
    virtual ~CodeBlock () = 0;

    virtual bool GetIsCodeBlock () const { return true; }

    // just use Ast's default print, because we don't want
    // to print out the entire contents of the CodeBlock.
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const
    {
        Ast::Print(stream, Stringify, indent_level);
    }
}; // end of struct CodeBlock

struct DumbCodeBlock : public CodeBlock
{
    DumbCodeBlock (FileLocation const &file_location) : CodeBlock(file_location, AT_DUMB_CODE_BLOCK) { }
}; // end of struct DumbCodeBlock

struct StrictCodeBlock : public CodeBlock
{
    StrictCodeBlock (FileLocation const &file_location) : CodeBlock(file_location, AT_STRICT_CODE_BLOCK) { }
}; // end of struct StrictCodeBlock

class Directive : public TextBase
{
public:

    Directive (
        string const &directive_identifier,
        FileLocation const &file_location)
        :
        TextBase(directive_identifier, file_location, AT_DIRECTIVE)
    { }

    virtual string GetDirectiveString () const { return GetText(); }
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    Directive (
        string const &directive_identifier,
        FileLocation const &file_location,
        AstType ast_type)
        :
        TextBase(directive_identifier, file_location, ast_type)
    { }
}; // end of class Directive

struct IdentifierList : public AstList<Identifier>
{
    IdentifierList () : AstList<Identifier>(AT_IDENTIFIER_LIST) { }
}; // end of struct IdentifierList

struct IdentifierMap : public AstMap<Identifier>
{
    IdentifierMap () : AstMap<Identifier>(AT_IDENTIFIER_MAP) { }
}; // end of struct IdentifierMap

struct DirectiveList : public AstList<Directive>
{
    DirectiveList () : AstList<Directive>(AT_DIRECTIVE_LIST) { }
}; // end of struct DirectiveList

} // end of namespace AstCommon
} // end of namespace Barf

#endif // !defined(_BARF_ASTCOMMON_HPP_)
