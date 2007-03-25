// ///////////////////////////////////////////////////////////////////////////
// barf_preprocessor_symboltable.hpp by Victor Dods, created 2006/10/16
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_PREPROCESSOR_SYMBOLTABLE_HPP_)
#define _BARF_PREPROCESSOR_SYMBOLTABLE_HPP_

#include "barf_preprocessor.hpp"

#include <map>
#include <vector>

#include "barf_filelocation.hpp"

namespace Barf {
namespace Preprocessor {

class Body;

// NOTE: Symbol and SymbolTable are not allowed to delete Ast pointers.

// ///////////////////////////////////////////////////////////////////////////
// Symbol
// ///////////////////////////////////////////////////////////////////////////

class Symbol
{
public:

    Symbol (string const &identifier)
        :
        m_identifier(identifier)
    {
        assert(!m_identifier.empty());
    }
    virtual ~Symbol () { }

    virtual bool GetIsScalarSymbol () const { return false; }
    virtual bool GetIsArraySymbol () const { return false; }
    virtual bool GetIsMapSymbol () const { return false; }

    virtual Uint32 Sizeof () const = 0;
    virtual Symbol *Clone () const = 0;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const = 0;

protected:

    string const m_identifier;
}; // end of class Symbol

// ///////////////////////////////////////////////////////////////////////////
// ScalarSymbol
// ///////////////////////////////////////////////////////////////////////////

class ScalarSymbol : public Symbol
{
public:

    ScalarSymbol (string const &identifier) : Symbol(identifier) { }

    virtual bool GetIsScalarSymbol () const { return true; }
    Body const *GetScalarBody () const { return m_body; }

    void SetScalarBody (Body const *body) { m_body = body; }

    virtual Uint32 Sizeof () const { return 1; }
    virtual Symbol *Clone () const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Body const *m_body;
}; // end of class ScalarSymbol

// ///////////////////////////////////////////////////////////////////////////
// ArraySymbol
// ///////////////////////////////////////////////////////////////////////////

class ArraySymbol : public Symbol
{
public:

    ArraySymbol (string const &identifier) : Symbol(identifier) { }

    virtual bool GetIsArraySymbol () const { return true; }
    Uint32 GetArrayElementCount () const { return m_body_vector.size(); }
    Body const *GetArrayElement (Uint32 index) const { return (index < m_body_vector.size()) ? m_body_vector[index] : NULL; }

    void AppendArrayElement (Body const *element) { assert(element != NULL); m_body_vector.push_back(element); }
    void ClearArrayElements () { m_body_vector.clear(); }

    virtual Uint32 Sizeof () const { return m_body_vector.size(); }
    virtual Symbol *Clone () const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    typedef vector<Body const *> BodyVector;

    BodyVector m_body_vector;
}; // end of class ArraySymbol

// ///////////////////////////////////////////////////////////////////////////
// MapSymbol
// ///////////////////////////////////////////////////////////////////////////

class MapSymbol : public Symbol
{
public:

    typedef map<string, Body const *> BodyMap;

    MapSymbol (string const &identifier) : Symbol(identifier) { }

    virtual bool GetIsMapSymbol () const { return true; }
    Uint32 GetMapElementCount () const { return m_body_map.size(); }
    BodyMap::const_iterator GetBegin () const { return m_body_map.begin(); }
    BodyMap::const_iterator GetEnd () const { return m_body_map.end(); }
    Body const *GetMapElement (string const &key) const
    {
        BodyMap::const_iterator it = m_body_map.find(key);
        return (it != m_body_map.end()) ? it->second : NULL;
    }

    void SetMapElement (string const &key, Body const *element)
    {
        assert(!key.empty());
        m_body_map[key] = element;
    }
    void ClearMapElements () { m_body_map.clear(); }

    virtual Uint32 Sizeof () const { return m_body_map.size(); }
    virtual Symbol *Clone () const;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    BodyMap m_body_map;
}; // end of class MapSymbol

// ///////////////////////////////////////////////////////////////////////////
// SymbolTable
// ///////////////////////////////////////////////////////////////////////////

class SymbolTable
{
public:

    SymbolTable () { }
    SymbolTable (SymbolTable const &symbol_table);
    ~SymbolTable ();

    Symbol *GetSymbol (string const &identifier);

    ScalarSymbol *DefineScalarSymbol (string const &identifier, FileLocation const &file_location);
    ScalarSymbol *DefineScalarSymbolAsText (string const &identifier, FileLocation const &file_location, string const &text);
    ScalarSymbol *DefineScalarSymbolAsInteger (string const &identifier, FileLocation const &file_location, Sint32 integer);
    ArraySymbol *DefineArraySymbol (string const &identifier, FileLocation const &file_location);
    MapSymbol *DefineMapSymbol (string const &identifier, FileLocation const &file_location);
    void UndefineSymbol (string const &identifier, FileLocation const &file_location);

    void Print (ostream &stream) const;

private:

    typedef map<string, Symbol *> SymbolMap;

    SymbolMap m_symbol_map;
}; // end of class SymbolTable

} // end of namespace Preprocessor
} // end of namespace Barf

#endif // !defined(_BARF_PREPROCESSOR_SYMBOLTABLE_HPP_)
