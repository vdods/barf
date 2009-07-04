// ///////////////////////////////////////////////////////////////////////////
// barf_ast.hpp by Victor Dods, created 2006/10/22
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_AST_HPP_)
#define BARF_AST_HPP_

#include "barf.hpp"

#include <algorithm>
#include <ostream>
#include <map>
#include <sstream>
#include <vector>

#include "barf_filelocation.hpp"
#include "barf_message.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace Ast {

// when the parser subsystem is defining its own concrete Base subclasses, it
// should start its enum values at AST_START_CUSTOM_TYPES_HERE_.
enum
{
    AST_CHAR = 0,
    AST_DIRECTIVE,
    AST_DIRECTIVE_LIST,
    AST_DUMB_CODE_BLOCK,
    AST_ID,
    AST_ID_LIST,
    AST_ID_MAP,
    AST_SIGNED_INTEGER,
    AST_STRICT_CODE_BLOCK,
    AST_STRING,
    AST_THROW_AWAY,
    AST_UNSIGNED_INTEGER,

    AST_START_CUSTOM_TYPES_HERE_,

    AST_NONE = AstType(-1)
};

string const &AstTypeString (AstType ast_type);

class Base
{
public:

    Base (FileLocation const &filoc, AstType ast_type)
        :
        m_filoc(filoc),
        m_ast_type(ast_type)
    { }
    virtual ~Base () { }

    inline FileLocation const &FiLoc () const { return m_filoc; }
    inline AstType GetAstType () const { return m_ast_type; }

    virtual bool IsCodeBlock () const { return false; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    inline void SetFiLoc (FileLocation const &filoc)
    {
        assert(filoc.HasLineNumber());
        m_filoc = filoc;
    }

private:

    FileLocation m_filoc;
    AstType const m_ast_type;
}; // end of class Base

template <typename ElementType>
struct List : private vector<ElementType *>
{
    typedef typename vector<ElementType *>::iterator iterator;
    typedef typename vector<ElementType *>::const_iterator const_iterator;

    using vector<ElementType *>::size;
    using vector<ElementType *>::size_type;
    using vector<ElementType *>::empty;
    using vector<ElementType *>::clear;
    using vector<ElementType *>::begin;
    using vector<ElementType *>::end;

    virtual ~List ()
    {
        for_each(begin(), end(), DeleteFunctor<ElementType>());
    }

    inline ElementType const *Element (Uint32 index) const
    {
        assert(index < size());
        return vector<ElementType *>::operator[](index);
    }
    inline ElementType *Element (Uint32 index)
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
struct AstList : public Base, public List<ElementType>
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
        Base(FileLocation::ms_invalid, ast_type),
        List<ElementType>()
    { }
    AstList (FileLocation const &filoc, AstType ast_type)
        :
        Base(filoc, ast_type),
        List<ElementType>()
    {
        assert(filoc.HasLineNumber());
    }

    virtual void Append (ElementType *element)
    {
        assert(element != NULL);
        List<ElementType>::Append(element);
        // if this list doesn't yet have a valid file location, and
        // the appending element does, use it as the list's file location
        if (!FiLoc().HasLineNumber() && element->FiLoc().HasLineNumber())
            SetFiLoc(element->FiLoc());
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const
    {
        stream << Tabs(indent_level) << Stringify(GetAstType()) << " (" << size() << " elements)" << endl;
        for_each(begin(), end(), PrintAst<Base>(stream, Stringify, indent_level+1));
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

    inline ElementType const *Element (string const &key) const
    {
        const_iterator it = find(key);
        return (it != end()) ? it->second : NULL;
    }
    inline ElementType *Element (string const &key)
    {
        iterator it = find(key);
        return (it != end()) ? it->second : NULL;
    }

    virtual void Add (string const &key, ElementType *element)
    {
        assert(element != NULL);
        iterator it = find(key);
        if (it != end())
            EmitError(FORMAT("key \"" << key << "\" previously specified at " << it->second->FiLoc()), element->FiLoc());
        else
            map<string, ElementType *>::operator[](key) = element;
    }
    virtual void Set (string const &key, ElementType *element)
    {
        assert(element != NULL);
        iterator it = find(key);
        if (it != end())
            delete it->second;
        map<string, ElementType *>::operator[](key) = element;
    }
}; // end of class Map<ElementType>

template <typename ElementType>
struct AstMap : public Base, public Map<ElementType>
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
        Base(FileLocation::ms_invalid, ast_type),
        Map<ElementType>()
    { }
    AstMap (FileLocation const &filoc, AstType ast_type)
        :
        Base(filoc, ast_type),
        Map<ElementType>()
    {
        assert(filoc.HasLineNumber());
    }

    virtual void Add (string const &key, ElementType *element)
    {
        assert(element != NULL);
        Map<ElementType>::Add(key, element);
        // if this map doesn't yet have a valid file location, and
        // the appending element does, use it as the map's file location
        if (!FiLoc().HasLineNumber() && element->FiLoc().HasLineNumber())
            SetFiLoc(element->FiLoc());
    }
    virtual void Set (string const &key, ElementType *element)
    {
        assert(element != NULL);
        Map<ElementType>::Set(key, element);
        // if this map doesn't yet have a valid file location, and
        // the appending element does, use it as the map's file location
        if (!FiLoc().HasLineNumber() && element->FiLoc().HasLineNumber())
            SetFiLoc(element->FiLoc());
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

struct ThrowAway : public Base
{
    ThrowAway (FileLocation const &filoc)
        :
        Base(filoc, AST_THROW_AWAY)
    {
        assert(filoc.IsValid());
    }
}; // end of struct ThrowAway

class Char : public Base
{
public:

    Char (Uint8 ch, FileLocation const &filoc)
        :
        Base(filoc, AST_CHAR),
        m_char(ch)
    { }

    inline Uint8 GetChar () const { return m_char; }
    inline string CharLiteral () const { return Barf::CharLiteral(m_char); }

    virtual void Escape ();
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    Char (Uint8 ch, FileLocation const &filoc, AstType ast_type)
        :
        Base(filoc, ast_type),
        m_char(ch)
    { }

    Uint8 m_char;
}; // end of class Char

class SignedInteger : public Base
{
public:

    SignedInteger (Sint32 value, FileLocation const &filoc)
        :
        Base(filoc, AST_SIGNED_INTEGER),
        m_value(value)
    { }

    inline Sint32 Value () const { return m_value; }
    inline void SetValue (Sint32 value) { m_value = value; }

    void ShiftAndAdd (Sint32 value);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Sint32 m_value;
}; // end of class SignedInteger

class UnsignedInteger : public Base
{
public:

    UnsignedInteger (Uint32 value, FileLocation const &filoc)
        :
        Base(filoc, AST_UNSIGNED_INTEGER),
        m_value(value)
    { }

    inline Uint32 Value () const { return m_value; }
    inline void SetValue (Uint32 value) { m_value = value; }

    void ShiftAndAdd (Uint32 value);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Uint32 m_value;
}; // end of class UnsignedInteger

class TextBase : public Base
{
public:

    TextBase (FileLocation const &filoc, AstType ast_type)
        :
        Base(filoc, ast_type)
    { }
    TextBase (string const &text, FileLocation const &filoc, AstType ast_type)
        :
        Base(filoc, ast_type),
        m_text(text)
    { }
    virtual ~TextBase () = 0;

    static string DirectiveTypeString (AstType ast_type);

    inline string const &GetText () const { return m_text; }

    inline void AppendText (string const &text) { m_text += text; }
    inline void AppendChar (char ch) { m_text += ch; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    string m_text;
}; // end of class TextBase

struct String : public TextBase
{
    String (FileLocation const &filoc)
        :
        TextBase(filoc, AST_STRING)
    { }
    String (string const &str, FileLocation const &filoc)
        :
        TextBase(str, filoc, AST_STRING)
    { }

    inline string StringLiteral () const { return Barf::StringLiteral(GetText()); }
}; // end of struct String

struct Id : public TextBase
{
    Id (string const &id_text, FileLocation const &filoc)
        :
        TextBase(id_text, filoc, AST_ID)
    {
        assert(!GetText().empty());
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct Id

struct CodeBlock : public TextBase
{
    CodeBlock (FileLocation const &filoc, AstType ast_type)
        :
        TextBase(filoc, ast_type)
    { }
    virtual ~CodeBlock () = 0;

    virtual bool IsCodeBlock () const { return true; }

    // just use Base's default print, because we don't want
    // to print out the entire contents of the CodeBlock.
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const
    {
        Base::Print(stream, Stringify, indent_level);
    }
}; // end of struct CodeBlock

struct DumbCodeBlock : public CodeBlock
{
    DumbCodeBlock (FileLocation const &filoc) : CodeBlock(filoc, AST_DUMB_CODE_BLOCK) { }
}; // end of struct DumbCodeBlock

struct StrictCodeBlock : public CodeBlock
{
    StrictCodeBlock (FileLocation const &filoc) : CodeBlock(filoc, AST_STRICT_CODE_BLOCK) { }
}; // end of struct StrictCodeBlock

class Directive : public TextBase
{
public:

    Directive (
        string const &directive_id,
        FileLocation const &filoc)
        :
        TextBase(directive_id, filoc, AST_DIRECTIVE)
    { }

    virtual string DirectiveString () const { return GetText(); }
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    Directive (
        string const &directive_id,
        FileLocation const &filoc,
        AstType ast_type)
        :
        TextBase(directive_id, filoc, ast_type)
    { }
}; // end of class Directive

struct IdList : public AstList<Id>
{
    IdList () : AstList<Id>(AST_ID_LIST) { }
}; // end of struct IdList

struct IdMap : public AstMap<Id>
{
    IdMap () : AstMap<Id>(AST_ID_MAP) { }
}; // end of struct IdMap

struct DirectiveList : public AstList<Directive>
{
    DirectiveList () : AstList<Directive>(AST_DIRECTIVE_LIST) { }
}; // end of struct DirectiveList

} // end of namespace Ast
} // end of namespace Barf

#endif // !defined(BARF_AST_HPP_)
