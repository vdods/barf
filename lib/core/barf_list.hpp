// ///////////////////////////////////////////////////////////////////////////
// barf_list.hpp by Victor Dods, created 2008/06/28
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_LIST_HPP_)
#define BARF_LIST_HPP_

#include "barf.hpp"

namespace Barf {

// ListNode is the superclass for elements contained in List.  the point
// of the List class (as opposed to just using an STL list) is that the
// ListNode elements are aware of being in the List (sort of like having
// STL list iterators to themselves).
template <typename ListNodeSubclass>
struct ListNode
{
public:

    ~ListNode ()
    {
        // remove this node from its list if it's a body
        if (NodeClass() == CT_BODY)
            Remove();
        // now ensure this node is a floaty or it is a head or tail of an empty list
        assert(NodeClass() == CT_FLOATY ||
               (NodeClass() == CT_HEAD && m_next->NodeClass() == CT_TAIL && m_next->m_prev == this) ||
               (NodeClass() == CT_TAIL && m_prev->NodeClass() == CT_HEAD && m_prev->m_next == this));
    }

    bool IsAnElement () const { return NodeClass() == CT_BODY; }
    ListNodeSubclass *Prev ()
    {
        assert(NodeClass() != CT_HEAD && "can't call Prev() on a head node");
        return m_prev->NodeClass() != CT_HEAD ? m_prev : NULL;
    }
    ListNodeSubclass const *Prev () const
    {
        assert(NodeClass() != CT_HEAD && "can't call Prev() on a head node");
        return m_prev->NodeClass() != CT_HEAD ? m_prev : NULL;
    }
    ListNodeSubclass *Next ()
    {
        assert(NodeClass() != CT_TAIL && "can't call Next() on a tail node");
        return m_next->NodeClass() != CT_TAIL ? m_next : NULL;
    }
    ListNodeSubclass const *Next () const
    {
        assert(NodeClass() != CT_TAIL && "can't call Next() on a tail node");
        return m_next->NodeClass() != CT_TAIL ? m_next : NULL;
    }

    void InsertBefore (ListNodeSubclass *node)
    {
        assert(node != NULL);
        assert(node->NodeClass() == CT_FLOATY && "can't insert a node that is already in a list");
        assert(NodeClass() == CT_BODY || (NodeClass() == CT_TAIL && "can't insert a node before a head or a floaty"));
        assert(node != this && "can't insert a node before itself");
        m_prev->m_next = node;
        node->m_prev = m_prev;
        node->m_next = static_cast<ListNodeSubclass *>(this);
        m_prev = node;
    }
    void InsertAfter (ListNodeSubclass *node)
    {
        assert(node != NULL);
        assert(node->NodeClass() == CT_FLOATY && "can't insert a node that is already in a list");
        assert(NodeClass() == CT_HEAD || (NodeClass() == CT_BODY && "can't insert a node after a tail or a floaty"));
        assert(node != this && "can't insert a node after itself");
        m_next->m_prev = node;
        node->m_next = m_next;
        node->m_prev = static_cast<ListNodeSubclass *>(this);
        m_next = node;
    }
    ListNodeSubclass *Remove ()
    {
        assert(NodeClass() == CT_BODY);
        assert(m_prev->m_next == this);
        assert(m_next->m_prev == this);
        m_prev->m_next = m_next;
        m_next->m_prev = m_prev;
        m_next = NULL;
        m_prev = NULL;
        assert(NodeClass() == CT_FLOATY);
        return static_cast<ListNodeSubclass *>(this);
    }

protected:

    ListNode () : m_prev(NULL), m_next(NULL) { assert(NodeClass() == CT_FLOATY); }

private:

    enum ClassType { CT_FLOATY = 0|0, CT_HEAD = 0|1, CT_TAIL = 2|0, CT_BODY = 2|1 };

    ClassType NodeClass () const { return ClassType((m_prev != NULL ? 2 : 0) | (m_next != NULL ? 1 : 0)); }

    ListNodeSubclass *m_prev;
    ListNodeSubclass *m_next;

    template <typename ElementType> friend struct List;
}; // end of struct ListNode<ListNodeSubclass>

template <typename ElementType>
struct List
{
    List ()
    {
        assert(sizeof(m_head) == 2*sizeof(void *)); // TODO: make into compile-time asserts
        assert(sizeof(m_tail) == 2*sizeof(void *));
        // these two casts are technically unsafe, but we'll be responsible.
        m_head.m_next = static_cast<ElementType *>(&m_tail);
        m_tail.m_prev = static_cast<ElementType *>(&m_head);
        assert(m_head.NodeClass() == ListNode<ElementType>::CT_HEAD);
        assert(m_tail.NodeClass() == ListNode<ElementType>::CT_TAIL);
    }
    ~List () { assert(IsEmpty() && "list not empty upon destruction"); }

    bool IsEmpty () const
    {
        assert(m_head.NodeClass() == ListNode<ElementType>::CT_HEAD);
        assert(m_tail.NodeClass() == ListNode<ElementType>::CT_TAIL);
        return m_head.m_next == &m_tail;
    }
    ElementType *Front () { return IsEmpty() ? NULL : m_head.Next(); }
    ElementType const *Front () const { return IsEmpty() ? NULL : m_head.Next(); }
    ElementType *Back () { return IsEmpty() ? NULL : m_tail.Prev(); }
    ElementType const *Back () const { return IsEmpty() ? NULL : m_tail.Prev(); }

    void Prepend (List<ElementType> &list)
    {
        assert(&list != this);
        if (list.IsEmpty())
            return;
        // put the contents of the source list before the first element
        list.m_head.m_next->m_prev = static_cast<ElementType *>(&m_head);
        list.m_tail.m_prev->m_next = m_head.m_next;
        m_head.m_next->m_prev = list.m_tail.m_prev;
        m_head.m_next = list.m_head.m_next;
        // empty the source list
        list.m_head.m_next = static_cast<ElementType *>(&list.m_tail);
        list.m_tail.m_prev = static_cast<ElementType *>(&list.m_head);
    }
    void Append (List<ElementType> &list)
    {
        assert(&list != this);
        if (list.IsEmpty())
            return;
        // put the contents of the source list after the last element
        list.m_tail.m_prev->m_next = static_cast<ElementType *>(&m_tail);
        list.m_head.m_next->m_prev = m_tail.m_prev;
        m_tail.m_prev->m_next = list.m_head.m_next;
        m_tail.m_prev = list.m_tail.m_prev;
        // empty the source list
        list.m_head.m_next = static_cast<ElementType *>(&list.m_tail);
        list.m_tail.m_prev = static_cast<ElementType *>(&list.m_head);
    }
    void Prepend (ElementType *node) { m_head.InsertAfter(node); }
    void Append (ElementType *node) { m_tail.InsertBefore(node); }
    ElementType *RemoveFront () { assert(Front() != NULL && "can't remove from an empty list"); return Front()->Remove(); }
    ElementType *RemoveBack () { assert(Back() != NULL && "can't remove from an empty list"); return Back()->Remove(); }

private:

    ListNode<ElementType> m_head;
    ListNode<ElementType> m_tail;
}; // end of struct List<ElementType>

} // end of namespace Barf

#endif // !defined(BARF_LIST_HPP_)
