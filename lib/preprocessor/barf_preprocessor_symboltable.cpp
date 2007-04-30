// ///////////////////////////////////////////////////////////////////////////
// barf_preprocessor_symboltable.cpp by Victor Dods, created 2006/10/16
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

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
    ScalarSymbol *retval = new ScalarSymbol(m_identifier);
    retval->m_body = m_body;
    return retval;
}

void ScalarSymbol::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << m_identifier << " (scalar)" << endl;
    assert(m_body != NULL);
    m_body->Print(stream, Stringify, indent_level+1);
}

// ///////////////////////////////////////////////////////////////////////////
// ArraySymbol
// ///////////////////////////////////////////////////////////////////////////

Symbol *ArraySymbol::Clone () const
{
    ArraySymbol *retval = new ArraySymbol(m_identifier);
    retval->m_body_vector = m_body_vector;
    return retval;
}

void ArraySymbol::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << m_identifier << " (array - " << m_body_vector.size() << " elements)" << endl;
    stream << Tabs(indent_level) << '{' << endl;
    for (Uint32 i = 0; i < m_body_vector.size(); ++i)
    {
        Body const *body = m_body_vector[i];
        assert(body != NULL);
        stream << Tabs(indent_level+1) << "element " << i << endl;
        body->Print(stream, Stringify, indent_level+2);
    }
    stream << Tabs(indent_level) << '}' << endl;
}

// ///////////////////////////////////////////////////////////////////////////
// MapSymbol
// ///////////////////////////////////////////////////////////////////////////

Symbol *MapSymbol::Clone () const
{
    MapSymbol *retval = new MapSymbol(m_identifier);
    retval->m_body_map = m_body_map;
    return retval;
}

void MapSymbol::Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level) const
{
    stream << Tabs(indent_level) << m_identifier << " (map - " << m_body_map.size() << " elements)" << endl;
    stream << Tabs(indent_level) << '{' << endl;
    for (BodyMap::const_iterator it = m_body_map.begin(),
                              it_end = m_body_map.end();
         it != it_end;
         ++it)
    {
        string const &key = it->first;
        Body const *body = it->second;
        assert(body != NULL);
        stream << Tabs(indent_level+1) << "element \"" << key << '\"' << endl;
        body->Print(stream, Stringify, indent_level+2);
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

Symbol *SymbolTable::GetSymbol (string const &identifier)
{
    assert(!identifier.empty());
    SymbolMap::iterator it;
    return Contains(m_symbol_map, identifier, it) ? it->second : NULL;
}

ScalarSymbol *SymbolTable::DefineScalarSymbol (string const &identifier, FiLoc const &filoc)
{
    assert(!identifier.empty());

    if (GetSymbol(identifier) != NULL)
    {
        EmitWarning(filoc, "redefinition of previously defined macro \"" + identifier + "\"");
        UndefineSymbol(identifier, filoc);
    }

    ScalarSymbol *symbol = new ScalarSymbol(identifier);
    m_symbol_map[identifier] = symbol;
    return symbol;
}

ScalarSymbol *SymbolTable::DefineScalarSymbolAsText (string const &identifier, FiLoc const &filoc, string const &text)
{
    assert(!identifier.empty());

    if (GetSymbol(identifier) != NULL)
    {
        EmitWarning(filoc, "redefinition of previously defined macro \"" + identifier + "\"");
        UndefineSymbol(identifier, filoc);
    }

    ScalarSymbol *symbol = new ScalarSymbol(identifier);
    m_symbol_map[identifier] = symbol;
    symbol->SetScalarBody(new Body(text, FiLoc::ms_invalid));
    return symbol;
}

ScalarSymbol *SymbolTable::DefineScalarSymbolAsInteger (string const &identifier, FiLoc const &filoc, Sint32 integer)
{
    assert(!identifier.empty());

    if (GetSymbol(identifier) != NULL)
    {
        EmitWarning(filoc, "redefinition of previously defined macro \"" + identifier + "\"");
        UndefineSymbol(identifier, filoc);
    }

    ScalarSymbol *symbol = new ScalarSymbol(identifier);
    m_symbol_map[identifier] = symbol;
    symbol->SetScalarBody(new Body(integer, FiLoc::ms_invalid));
    return symbol;
}

ArraySymbol *SymbolTable::DefineArraySymbol (string const &identifier, FiLoc const &filoc)
{
    assert(!identifier.empty());

    if (GetSymbol(identifier) != NULL)
    {
        EmitWarning(filoc, "redefinition of previously defined macro \"" + identifier + "\"");
        UndefineSymbol(identifier, filoc);
    }

    ArraySymbol *symbol = new ArraySymbol(identifier);
    m_symbol_map[identifier] = symbol;
    return symbol;
}

MapSymbol *SymbolTable::DefineMapSymbol (string const &identifier, FiLoc const &filoc)
{
    assert(!identifier.empty());

    if (GetSymbol(identifier) != NULL)
    {
        EmitWarning(filoc, "redefinition of previously defined macro \"" + identifier + "\"");
        UndefineSymbol(identifier, filoc);
    }

    MapSymbol *symbol = new MapSymbol(identifier);
    m_symbol_map[identifier] = symbol;
    return symbol;
}

void SymbolTable::UndefineSymbol (string const &identifier, FiLoc const &filoc)
{
    assert(!identifier.empty());
    SymbolMap::iterator it;
    if (Contains(m_symbol_map, identifier, it))
    {
        assert(it->second != NULL);
        delete it->second;
        m_symbol_map.erase(it);
    }
    else
        EmitWarning(filoc, "macro \"" + identifier + "\" is not currently defined");
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
        symbol->Print(stream, GetAstTypeString, 1);
    }
    stream << '}' << endl;
}

} // end of namespace Preprocessor
} // end of namespace Barf
