// ///////////////////////////////////////////////////////////////////////////
// barf_regex_ast.hpp by Victor Dods, created 2006/01/27
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_BARF_REGEX_AST_HPP_)
#define _BARF_REGEX_AST_HPP_

#include "barf_regex.hpp"

#include <bitset>
#include <vector>

#include "barf_ast.hpp"
#include "barf_graph.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace Regex {

/*

// ///////////////////////////////////////////////////////////////////////////
// class hierarchy
// ///////////////////////////////////////////////////////////////////////////

Ast::Base (abstract)
    Branch
    Piece
    Atom (abstract)
        RegularExpression
        ControlChar
        NormalChar
        BracketCharSet
    Bound

// ///////////////////////////////////////////////////////////////////////////
// class composition
// ///////////////////////////////////////////////////////////////////////////

RegularExpression
    Branch
        Piece[]
            Atom
            Bound

*/

enum
{
    AT_BOUND = Ast::AT_START_CUSTOM_TYPES_HERE_,
    AT_BRACKET_CHAR_SET,
    AT_BRANCH,
    AT_CHAR,
    AT_PIECE,
    AT_REGULAR_EXPRESSION,
    AT_REGULAR_EXPRESSION_MAP,

    AT_COUNT
};

string const &GetAstTypeString (AstType ast_type);

struct Atom : public Ast::Base
{
    Atom (AstType ast_type) : Ast::Base(FiLoc::ms_invalid, ast_type) { }
}; // end of class Atom

struct Bound : public Ast::Base
{
    Sint16 const m_lower_bound;
    Sint16 const m_upper_bound;

    enum
    {
        // this value is why the bounds must be of type Sint16 and not Uint8
        NO_UPPER_BOUND = -1
    };

    Bound (Sint16 lower_bound, Sint16 upper_bound)
        :
        Ast::Base(FiLoc::ms_invalid, AT_BOUND),
        m_lower_bound(lower_bound),
        m_upper_bound(upper_bound)
    {
        assert(m_lower_bound >= 0);
        // either the lower bound <= the upper bound, or the special
        // case of the upper bound being -1 (indicating that there
        // is no upper bound).
        assert(m_lower_bound <= m_upper_bound || GetHasNoUpperBound());
    }

    static Sint16 GetMaximumBoundValue () { return 255; }

    inline bool GetHasNoUpperBound () const { return m_upper_bound < 0; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class Bound

struct Piece : public Ast::Base
{
    Piece (Atom *atom, Bound *bound)
        :
        Ast::Base(FiLoc::ms_invalid, AT_PIECE),
        m_atom(atom),
        m_bound(bound)
    {
        assert(m_atom != NULL);
        assert(m_bound != NULL);
    }

    inline Atom const *GetAtom () const { return m_atom; }
    inline Bound const *GetBound () const { return m_bound; }

    void SetBound (Bound *bound);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Atom *m_atom;
    Bound *m_bound;
}; // end of class Piece

struct Branch : public Ast::AstList<Piece>
{
    Branch ()
        :
        Ast::AstList<Piece>(AT_BRANCH),
        m_last_modification_was_atom_addition(false)
    { }

    void AddAtom (Atom *atom);
    void AddBound (Bound *bound);

private:

    bool m_last_modification_was_atom_addition;
}; // end of class Branch

struct Char : public Atom
{
    Char (Uint8 ch, ConditionalType conditional_type = CT_COUNT)
        :
        Atom(AT_CHAR),
        m_char(ch),
        m_conditional_type(conditional_type)
    {
        // only one of m_char or m_conditional_type may be specified
        assert(m_char != '\0' && m_conditional_type == CT_COUNT ||
               m_char == '\0' && m_conditional_type < CT_COUNT);
    }

    inline Uint8 GetChar () const { return m_char; }
    inline bool GetIsControlChar () const { return m_conditional_type != CT_COUNT; }
    inline ConditionalType GetConditionalType () const { return m_conditional_type; }

    void Escape ();

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Uint8 m_char;
    ConditionalType m_conditional_type;
}; // end of class Char

struct BracketCharSet : public Atom
{
    BracketCharSet ()
        :
        Atom(AT_BRACKET_CHAR_SET)
    {
        m_char_set.reset();
    }
    BracketCharSet (Uint8 ch, bool negate)
        :
        Atom(AT_BRACKET_CHAR_SET)
    {
        m_char_set.reset();
        AddChar(ch);
        if (negate)
            Negate();
    }

    inline bool GetIsEmpty () const { return m_char_set.none(); }
    inline bool GetIsCharInSet (Uint8 ch) const { return m_char_set.test(ch); }

    void AddChar (Uint8 ch);
    void AddCharRange (Uint8 low_char, Uint8 high_char);
    void AddCharClass (string const &char_class);
    void Negate ();

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    typedef bitset<256> CharSet;

    CharSet m_char_set;
}; // end of BracketCharSet

struct RegularExpression : public Atom, public Ast::List<Branch>
{
    RegularExpression () : Atom(AT_REGULAR_EXPRESSION), Ast::List<Branch>() { }

    // this is the non-virtual, top-level Print method, not
    // to be confused with Ast::Base::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of class RegularExpression

struct RegularExpressionMap : public Ast::AstMap<RegularExpression>
{
    RegularExpressionMap () : Ast::AstMap<RegularExpression>(AT_REGULAR_EXPRESSION_MAP) { }

    // this is the non-virtual, top-level Print method, not
    // to be confused with Ast::Base::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;
}; // end of class RegularExpressionMap

} // end of namespace Regex
} // end of namespace Barf

#endif // !defined(_BARF_REGEX_AST_HPP_)
