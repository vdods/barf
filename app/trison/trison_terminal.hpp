// ///////////////////////////////////////////////////////////////////////////
// trison_terminal.hpp by Victor Dods, created 2006/02/19
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(_TRISON_TERMINAL_HPP_)
#define _TRISON_TERMINAL_HPP_

#include "trison.hpp"

#include "barf_filoc.hpp"
#include "trison_ast.hpp"

namespace Trison {

class Terminal
{
public:

    Terminal (
        TokenIdentifier const *token_identifier,
        AstCommon::String const *assigned_type,
        bool this_must_delete_members)
        :
        m_token_identifier(token_identifier),
        m_assigned_type(assigned_type),
        m_this_must_delete_members(this_must_delete_members),
        m_filoc(token_identifier->GetFiLoc())
    {
        assert(m_token_identifier != NULL);
        // m_assigned_type may be NULL
    }
    ~Terminal ()
    {
        if (m_this_must_delete_members)
        {
            delete m_token_identifier;
            delete m_assigned_type;
        }
    }

    string GetImplementationFileIdentifier () const
    {
        assert(m_token_identifier != NULL);
        if (m_token_identifier->GetText() == "%error")
            return "ERROR_";
        else
            return m_token_identifier->GetText();
    }
    inline TokenIdentifier const *GetTokenIdentifier () const { return m_token_identifier; }
    inline AstCommon::String const *GetAssignedType () const { return m_assigned_type; }
    inline FiLoc const &GetFiLoc () const { return m_filoc; }

private:

    TokenIdentifier const *const m_token_identifier;
    AstCommon::String const *const m_assigned_type;
    bool const m_this_must_delete_members;
    FiLoc const m_filoc;
}; // end of class Terminal

} // end of namespace Trison

#endif // !defined(_TRISON_TERMINAL_HPP_)
