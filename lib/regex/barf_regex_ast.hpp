// ///////////////////////////////////////////////////////////////////////////
// barf_regex_ast.hpp by Victor Dods, created 2006/01/27
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_REGEX_AST_HPP_)
#define BARF_REGEX_AST_HPP_

#include "barf_regex.hpp"

#include <bitset>
#include <vector>

#include "barf_ast.hpp"
#include "barf_graph.hpp"
#include "barf_util.hpp"

namespace Barf {
namespace Regex {

enum
{
    AST_BOUND = Ast::AST_START_CUSTOM_TYPES_HERE_,
    AST_BRACKET_CHAR_SET,
    AST_BRANCH,
    AST_CHAR,
    AST_CONDITIONAL_CHAR,
    AST_PIECE,
    AST_REGULAR_EXPRESSION,
    AST_REGULAR_EXPRESSION_MAP,

    AST_COUNT
};

string const &GetAstTypeString (AstType ast_type);

struct Atom : public Ast::Base
{
    Atom (AstType ast_type) : Ast::Base(FiLoc::ms_invalid, ast_type) { }

    // subclasses should return true iff the nfa transitions they generate
    // would not all be epsilon transitions (e.g. normal characters or conditionals)
    virtual bool RequiresNontrivialTransition () const = 0;
}; // end of struct Atom

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
        Ast::Base(FiLoc::ms_invalid, AST_BOUND),
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
}; // end of struct Bound

struct Piece : public Ast::Base
{
    Piece (Atom *atom, Bound *bound)
        :
        Ast::Base(FiLoc::ms_invalid, AST_PIECE),
        m_atom(atom),
        m_bound(bound)
    {
        assert(m_atom != NULL);
        assert(m_bound != NULL);
    }

    inline Atom const *GetAtom () const { return m_atom; }
    inline Bound const *GetBound () const { return m_bound; }

    void ReplaceBound (Bound *bound);

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Atom *m_atom;
    Bound *m_bound;

    friend bool NodesAreEqual (Ast::Base const *, Ast::Base const *);
}; // end of struct Piece

struct Branch : public Ast::AstList<Piece>
{
    Branch ()
        :
        Ast::AstList<Piece>(AST_BRANCH),
        m_last_modification_was_atom_addition(false)
    { }

    void AddAtom (Atom *atom);
    void AddBound (Bound *bound);

private:

    bool m_last_modification_was_atom_addition;

    friend bool NodesAreEqual (Ast::Base const *, Ast::Base const *);
}; // end of struct Branch

struct Char : public Atom
{
    Char (Uint8 ch)
        :
        Atom(AST_CHAR),
        m_char(ch)
    {
        assert(m_char != '\0' && "null char is not allowed");
    }

    inline Uint8 GetChar () const { return m_char; }

    Atom *Escaped ();
    void EscapeInsideBracketExpression ();

    virtual bool RequiresNontrivialTransition () const { return true; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    Uint8 m_char;

    friend bool NodesAreEqual (Ast::Base const *, Ast::Base const *);
}; // end of struct Char

struct ConditionalChar : public Atom
{
    ConditionalChar (ConditionalType conditional_type)
        :
        Atom(AST_CONDITIONAL_CHAR),
        m_conditional_type(conditional_type)
    {
        assert(m_conditional_type < CT_COUNT);
    }

    inline ConditionalType GetConditionalType () const { return m_conditional_type; }

    virtual bool RequiresNontrivialTransition () const { return true; }

    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    ConditionalType m_conditional_type;

    friend bool NodesAreEqual (Ast::Base const *, Ast::Base const *);
}; // end of struct ConditionalChar

struct BracketCharSet : public Atom
{
    BracketCharSet ()
        :
        Atom(AST_BRACKET_CHAR_SET)
    {
        m_char_set.reset();
    }
    BracketCharSet (Uint8 ch, bool negate)
        :
        Atom(AST_BRACKET_CHAR_SET)
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

    virtual bool RequiresNontrivialTransition () const { return true; }
    
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;

private:

    typedef bitset<256> CharSet;

    CharSet m_char_set;

    friend bool NodesAreEqual (Ast::Base const *, Ast::Base const *);
}; // end of BracketCharSet

struct RegularExpression : public Atom, public Ast::List<Branch>
{
    RegularExpression () : Atom(AST_REGULAR_EXPRESSION), Ast::List<Branch>() { }

    // this is the non-virtual, top-level Print method, not
    // to be confused with Ast::Base::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;

    virtual bool RequiresNontrivialTransition () const { return true; }
    
    virtual void Print (ostream &stream, StringifyAstType Stringify, Uint32 indent_level = 0) const;
}; // end of struct RegularExpression

struct RegularExpressionMap : public Ast::AstMap<RegularExpression>
{
    RegularExpressionMap () : Ast::AstMap<RegularExpression>(AST_REGULAR_EXPRESSION_MAP) { }

    // this is the non-virtual, top-level Print method, not
    // to be confused with Ast::Base::Print.
    void Print (ostream &stream, Uint32 indent_level = 0) const;
}; // end of struct RegularExpressionMap

bool NodesAreEqual (Ast::Base const *left, Ast::Base const *right);

} // end of namespace Regex
} // end of namespace Barf

#endif // !defined(BARF_REGEX_AST_HPP_)
