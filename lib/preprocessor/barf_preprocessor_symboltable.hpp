// 2006.10.16 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_PREPROCESSOR_SYMBOLTABLE_HPP_)
#define BARF_PREPROCESSOR_SYMBOLTABLE_HPP_

#include "barf_preprocessor.hpp"

#include <map>
#include <vector>

#include "barf_filoc.hpp"

namespace Barf {
namespace Preprocessor {

class Body;

// NOTE: Symbol and SymbolTable are not allowed to delete Base pointers.

// ///////////////////////////////////////////////////////////////////////////
// Symbol
// ///////////////////////////////////////////////////////////////////////////

class Symbol
{
public:

    Symbol (string const &id)
        :
        m_id(id)
    {
        assert(!m_id.empty());
    }
    virtual ~Symbol () { }

    string const &Id () const { return m_id; }

    virtual bool IsScalarSymbol () const { return false; }
    virtual bool IsArraySymbol () const { return false; }
    virtual bool IsMapSymbol () const { return false; }

    virtual Uint32 Sizeof () const = 0;
    virtual Symbol *Clone () const = 0;
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const = 0;

protected:

    string const m_id;
}; // end of class Symbol

// ///////////////////////////////////////////////////////////////////////////
// ScalarSymbol
// ///////////////////////////////////////////////////////////////////////////

class ScalarSymbol : public Symbol
{
public:

    ScalarSymbol (string const &id) : Symbol(id), m_body(NULL) { }

    virtual bool IsScalarSymbol () const { return true; }
    Body const *ScalarBody () const { return m_body; }

    void SetScalarBody (Body const *body) { assert(m_body == NULL); assert(body != NULL); m_body = body; }

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

    ArraySymbol (string const &id) : Symbol(id) { }

    virtual bool IsArraySymbol () const { return true; }
    Uint32 ArrayElementCount () const { return m_body_vector.size(); }
    Body const *ArrayElement (Uint32 index) const { return (index < m_body_vector.size()) ? m_body_vector[index] : NULL; }

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

    MapSymbol (string const &id) : Symbol(id) { }

    virtual bool IsMapSymbol () const { return true; }
    Uint32 MapElementCount () const { return m_body_map.size(); }
    BodyMap::const_iterator Begin () const { return m_body_map.begin(); }
    BodyMap::const_iterator End () const { return m_body_map.end(); }
    Body const *MapElement (string const &key) const
    {
        BodyMap::const_iterator it;
        return Contains(m_body_map, key, it) ? it->second : NULL;
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

    Symbol *GetSymbol (string const &id);

    ScalarSymbol *DefineScalarSymbol (string const &id, FiLoc const &filoc);
    ScalarSymbol *DefineScalarSymbolAsText (string const &id, FiLoc const &filoc, string const &text);
    ScalarSymbol *DefineScalarSymbolAsInteger (string const &id, FiLoc const &filoc, Sint32 integer);
    ArraySymbol *DefineArraySymbol (string const &id, FiLoc const &filoc);
    MapSymbol *DefineMapSymbol (string const &id, FiLoc const &filoc);
    void UndefineSymbol (string const &id, FiLoc const &filoc);

    void Print (ostream &stream) const;

private:

    typedef map<string, Symbol *> SymbolMap;

    SymbolMap m_symbol_map;
}; // end of class SymbolTable

} // end of namespace Preprocessor
} // end of namespace Barf

#endif // !defined(BARF_PREPROCESSOR_SYMBOLTABLE_HPP_)
