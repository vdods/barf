// 2006.10.16 - Copyright Victor Dods - Licensed under Apache 2.0

#include "barf_preprocessor_symboltable.hpp"

#include <sstream>

#include "barf_message.hpp"
#include "barf_preprocessor_ast.hpp"

namespace Barf {
namespace Preprocessor {

// ///////////////////////////////////////////////////////////////////////////
// ScalarSymbol
// ///////////////////////////////////////////////////////////////////////////

Symbol *ScalarSymbol::Clone () const
{
    ScalarSymbol *retval = new ScalarSymbol(m_id);
    retval->m_body = m_body;
    return retval;
}

void ScalarSymbol::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << m_id << " (scalar)" << endl;
    assert(m_body != NULL);
    assert(m_body->size() == 1);
    assert(m_body->Element(0) != NULL);
    m_body->Element(0)->Print(stream, Stringify, indent_level+1);
}

// ///////////////////////////////////////////////////////////////////////////
// ArraySymbol
// ///////////////////////////////////////////////////////////////////////////

Symbol *ArraySymbol::Clone () const
{
    ArraySymbol *retval = new ArraySymbol(m_id);
    retval->m_body_vector = m_body_vector;
    return retval;
}

void ArraySymbol::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << m_id << " (array - " << m_body_vector.size() << " elements)" << endl;
    stream << Tabs(indent_level) << '{' << endl;
    for (Uint32 i = 0; i < m_body_vector.size(); ++i)
    {
        Body const *body = m_body_vector[i];
        assert(body != NULL);
        assert(body->size() == 1);
        assert(body->Element(0) != NULL);
        stream << Tabs(indent_level+1) << "element " << i << endl;
        body->Element(0)->Print(stream, Stringify, indent_level+2);
    }
    stream << Tabs(indent_level) << '}' << endl;
}

// ///////////////////////////////////////////////////////////////////////////
// MapSymbol
// ///////////////////////////////////////////////////////////////////////////

Symbol *MapSymbol::Clone () const
{
    MapSymbol *retval = new MapSymbol(m_id);
    retval->m_body_map = m_body_map;
    return retval;
}

void MapSymbol::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << m_id << " (map - " << m_body_map.size() << " elements)" << endl;
    stream << Tabs(indent_level) << '{' << endl;
    for (BodyMap::const_iterator it = m_body_map.begin(),
                                 it_end = m_body_map.end();
         it != it_end;
         ++it)
    {
        string const &key = it->first;
        Body const *body = it->second;
        assert(body != NULL);
        assert(body->size() == 1);
        assert(body->Element(0) != NULL);
        stream << Tabs(indent_level+1) << "element \"" << key << '\"' << endl;
        body->Element(0)->Print(stream, Stringify, indent_level+2);
    }
    stream << Tabs(indent_level) << '}' << endl;
}

// ///////////////////////////////////////////////////////////////////////////
// SymbolTable
// ///////////////////////////////////////////////////////////////////////////

SymbolTable::SymbolTable (SymbolTable const &symbol_table)
{
    for (SymbolMap::const_iterator it = symbol_table.m_symbol_map.begin(),
                                   it_end = symbol_table.m_symbol_map.end();
         it != it_end;
         ++it)
    {
        m_symbol_map[it->first] = it->second->Clone();
    }
}

SymbolTable::~SymbolTable ()
{
    for_each(m_symbol_map.begin(), m_symbol_map.end(), DeletePairSecondFunctor<pair<string, Symbol *> >());
}

Symbol *SymbolTable::GetSymbol (string const &id)
{
    assert(!id.empty());
    SymbolMap::iterator it;
    return Contains(m_symbol_map, id, it) ? it->second : NULL;
}

ScalarSymbol *SymbolTable::DefineScalarSymbol (string const &id, FiLoc const &filoc)
{
    assert(!id.empty());

    if (GetSymbol(id) != NULL)
    {
        EmitWarning("redefinition of previously defined macro \"" + id + "\"", filoc);
        UndefineSymbol(id, filoc);
    }

    ScalarSymbol *symbol = new ScalarSymbol(id);
    m_symbol_map[id] = symbol;
    return symbol;
}

ScalarSymbol *SymbolTable::DefineScalarSymbolAsText (string const &id, FiLoc const &filoc, string const &text)
{
    assert(!id.empty());

    if (GetSymbol(id) != NULL)
    {
        EmitWarning("redefinition of previously defined macro \"" + id + "\"", filoc);
        UndefineSymbol(id, filoc);
    }

    ScalarSymbol *symbol = new ScalarSymbol(id);
    m_symbol_map[id] = symbol;
    symbol->SetScalarBody(new Body(text, FiLoc::ms_invalid));
    return symbol;
}

ScalarSymbol *SymbolTable::DefineScalarSymbolAsInteger (string const &id, FiLoc const &filoc, Sint32 integer)
{
    assert(!id.empty());

    if (GetSymbol(id) != NULL)
    {
        EmitWarning("redefinition of previously defined macro \"" + id + "\"", filoc);
        UndefineSymbol(id, filoc);
    }

    ScalarSymbol *symbol = new ScalarSymbol(id);
    m_symbol_map[id] = symbol;
    symbol->SetScalarBody(new Body(integer, FiLoc::ms_invalid));
    return symbol;
}

ArraySymbol *SymbolTable::DefineArraySymbol (string const &id, FiLoc const &filoc)
{
    assert(!id.empty());

    if (GetSymbol(id) != NULL)
    {
        EmitWarning("redefinition of previously defined macro \"" + id + "\"", filoc);
        UndefineSymbol(id, filoc);
    }

    ArraySymbol *symbol = new ArraySymbol(id);
    m_symbol_map[id] = symbol;
    return symbol;
}

MapSymbol *SymbolTable::DefineMapSymbol (string const &id, FiLoc const &filoc)
{
    assert(!id.empty());

    if (GetSymbol(id) != NULL)
    {
        EmitWarning("redefinition of previously defined macro \"" + id + "\"", filoc);
        UndefineSymbol(id, filoc);
    }

    MapSymbol *symbol = new MapSymbol(id);
    m_symbol_map[id] = symbol;
    return symbol;
}

void SymbolTable::UndefineSymbol (string const &id, FiLoc const &filoc)
{
    assert(!id.empty());
    SymbolMap::iterator it;
    if (Contains(m_symbol_map, id, it))
    {
        assert(it->second != NULL);
        delete it->second;
        m_symbol_map.erase(it);
    }
    else
        EmitWarning("macro \"" + id + "\" is not currently defined", filoc);
}

void SymbolTable::Print (ostream &stream) const
{
    stream << "SymbolTable dump (" << m_symbol_map.size() << " elements)" << endl;
    stream << '{' << endl;
    for (SymbolMap::const_iterator it = m_symbol_map.begin(),
                                   it_end = m_symbol_map.end();
         it != it_end;
         ++it)
    {
        Symbol const *symbol = it->second;
        assert(symbol != NULL);
        symbol->Print(stream, AstTypeString, 1);
    }
    stream << '}' << endl;
}

} // end of namespace Preprocessor
} // end of namespace Barf
