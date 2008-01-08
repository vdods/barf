// ///////////////////////////////////////////////////////////////////////////
// barf_ast.hpp by Victor Dods, created 2006/10/22
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_AST_HPP_)
#define _BARF_AST_HPP_

#include "barf.hpp"

#include <ostream>
#include <map>
#include <sstream>
#include <vector>

#include "barf_filoc.hpp"
#include "barf_message.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace Ast {

// when the parser subsystem is defining its own concrete Base subclasses, it
// should start its enum values at AT_START_CUSTOM_TYPES_HERE_.
enum
{
    AT_CHAR = 0,
    AT_DIRECTIVE,
    AT_DIRECTIVE_LIST,
    AT_DUMB_CODE_BLOCK,
    AT_ID,
    AT_ID_LIST,
    AT_ID_MAP,
    AT_SIGNED_INTEGER,
    AT_STRICT_CODE_BLOCK,
    AT_STRING,
    AT_THROW_AWAY,
    AT_UNSIGNED_INTEGER,

    AT_START_CUSTOM_TYPES_HERE_,

    AT_NONE = AstType(-1)
};

string const &GetAstTypeString (AstType ast_type);

class Base
{
public:

    Base (FiLoc const &filoc, AstType ast_type)
        :
        m_filoc(filoc),
        m_ast_type(ast_type)
    { }
    virtual ~Base () { }

    inline FiLoc const &GetFiLoc () const { return m_filoc; }
    inline AstType GetAstType () const { return m_ast_type; }

    virtual bool GetIsCodeBlock () const { return false; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    inline void SetFiLoc (FiLoc const &filoc)
    {
        assert(filoc.GetHasLineNumber());
        m_filoc = filoc;
    }

private:

    FiLoc m_filoc;
    AstType const m_ast_type;
}; // end of class Base

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
        Base(FiLoc::ms_invalid, ast_type),
        List<ElementType>()
    { }
    AstList (FiLoc const &filoc, AstType ast_type)
        :
        Base(filoc, ast_type),
        List<ElementType>()
    {
        assert(filoc.GetHasLineNumber());
    }

    virtual void Append (ElementType *element)
    {
        assert(element != NULL);
        List<ElementType>::Append(element);
        // if this list doesn't yet have a valid file location, and
        // the appending element does, use it as the list's file location
        if (!GetFiLoc().GetHasLineNumber() && element->GetFiLoc().GetHasLineNumber())
            SetFiLoc(element->GetFiLoc());
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
            EmitError(FORMAT("key \"" << key << "\" previously specified at " << it->second->GetFiLoc()), element->GetFiLoc());
        else
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
        Base(FiLoc::ms_invalid, ast_type),
        Map<ElementType>()
    { }
    AstMap (FiLoc const &filoc, AstType ast_type)
        :
        Base(filoc, ast_type),
        Map<ElementType>()
    {
        assert(filoc.GetHasLineNumber());
    }

    virtual void Add (string const &key, ElementType *element)
    {
        assert(element != NULL);
        Map<ElementType>::Add(key, element);
        // if this map doesn't yet have a valid file location, and
        // the appending element does, use it as the map's file location
        if (!GetFiLoc().GetHasLineNumber() && element->GetFiLoc().GetHasLineNumber())
            SetFiLoc(element->GetFiLoc());
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
    ThrowAway (FiLoc const &filoc)
        :
        Base(filoc, AT_THROW_AWAY)
    {
        assert(filoc.GetIsValid());
    }
}; // end of struct ThrowAway

class Char : public Base
{
public:

    Char (Uint8 ch, FiLoc const &filoc)
        :
        Base(filoc, AT_CHAR),
        m_char(ch)
    { }

    inline Uint8 GetChar () const { return m_char; }
    inline string GetCharLiteral () const { return Barf::GetCharLiteral(m_char); }

    virtual void Escape ();
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    Char (Uint8 ch, FiLoc const &filoc, AstType ast_type)
        :
        Base(filoc, ast_type),
        m_char(ch)
    { }

    Uint8 m_char;
}; // end of class Char

class SignedInteger : public Base
{
public:

    SignedInteger (Sint32 value, FiLoc const &filoc)
        :
        Base(filoc, AT_SIGNED_INTEGER),
        m_value(value)
    { }

    inline Sint32 GetValue () const { return m_value; }
    inline void SetValue (Sint32 value) { m_value = value; }

    void ShiftAndAdd (Sint32 value);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Sint32 m_value;
}; // end of class SignedInteger

class UnsignedInteger : public Base
{
public:

    UnsignedInteger (Uint32 value, FiLoc const &filoc)
        :
        Base(filoc, AT_UNSIGNED_INTEGER),
        m_value(value)
    { }

    inline Uint32 GetValue () const { return m_value; }
    inline void SetValue (Uint32 value) { m_value = value; }

    void ShiftAndAdd (Uint32 value);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Uint32 m_value;
}; // end of class UnsignedInteger

class TextBase : public Base
{
public:

    TextBase (FiLoc const &filoc, AstType ast_type)
        :
        Base(filoc, ast_type)
    { }
    TextBase (string const &text, FiLoc const &filoc, AstType ast_type)
        :
        Base(filoc, ast_type),
        m_text(text)
    { }
    virtual ~TextBase () = 0;

    static string GetDirectiveTypeString (AstType ast_type);

    inline string const &GetText () const { return m_text; }

    inline void AppendText (string const &text) { m_text += text; }
    inline void AppendChar (char ch) { m_text += ch; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    string m_text;
}; // end of class TextBase

struct String : public TextBase
{
    String (FiLoc const &filoc)
        :
        TextBase(filoc, AT_STRING)
    { }
    String (string const &str, FiLoc const &filoc)
        :
        TextBase(str, filoc, AT_STRING)
    { }

    inline string GetStringLiteral () const { return Barf::GetStringLiteral(GetText()); }
}; // end of struct String

struct Id : public TextBase
{
    Id (string const &id_text, FiLoc const &filoc)
        :
        TextBase(id_text, filoc, AT_ID)
    {
        assert(!GetText().empty());
    }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct Id

struct CodeBlock : public TextBase
{
    CodeBlock (FiLoc const &filoc, AstType ast_type)
        :
        TextBase(filoc, ast_type)
    { }
    virtual ~CodeBlock () = 0;

    virtual bool GetIsCodeBlock () const { return true; }

    // just use Base's default print, because we don't want
    // to print out the entire contents of the CodeBlock.
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const
    {
        Base::Print(stream, Stringify, indent_level);
    }
}; // end of struct CodeBlock

struct DumbCodeBlock : public CodeBlock
{
    DumbCodeBlock (FiLoc const &filoc) : CodeBlock(filoc, AT_DUMB_CODE_BLOCK) { }
}; // end of struct DumbCodeBlock

struct StrictCodeBlock : public CodeBlock
{
    StrictCodeBlock (FiLoc const &filoc) : CodeBlock(filoc, AT_STRICT_CODE_BLOCK) { }
}; // end of struct StrictCodeBlock

class Directive : public TextBase
{
public:

    Directive (
        string const &directive_id,
        FiLoc const &filoc)
        :
        TextBase(directive_id, filoc, AT_DIRECTIVE)
    { }

    virtual string GetDirectiveString () const { return GetText(); }
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

protected:

    Directive (
        string const &directive_id,
        FiLoc const &filoc,
        AstType ast_type)
        :
        TextBase(directive_id, filoc, ast_type)
    { }
}; // end of class Directive

struct IdList : public AstList<Id>
{
    IdList () : AstList<Id>(AT_ID_LIST) { }
}; // end of struct IdList

struct IdMap : public AstMap<Id>
{
    IdMap () : AstMap<Id>(AT_ID_MAP) { }
}; // end of struct IdMap

struct DirectiveList : public AstList<Directive>
{
    DirectiveList () : AstList<Directive>(AT_DIRECTIVE_LIST) { }
}; // end of struct DirectiveList

} // end of namespace Ast
} // end of namespace Barf

#endif // !defined(_BARF_AST_HPP_)
