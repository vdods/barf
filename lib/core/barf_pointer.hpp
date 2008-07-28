// ///////////////////////////////////////////////////////////////////////////
// barf_pointer.hpp by Victor Dods, created 2006/10/17
// ///////////////////////////////////////////////////////////////////////////
// Unless a different license was explicitly granted in writing by the
// copyright holder (Victor Dods), this software is freely distributable under
// the terms of the GNU General Public License, version 2.  Any works deriving
// from this work must also be released under the GNU GPL.  See the included
// file LICENSE for details.
// ///////////////////////////////////////////////////////////////////////////

#if !defined(BARF_POINTER_HPP_)
#define BARF_POINTER_HPP_

#include "barf.hpp"

#include <iostream>

namespace Barf {

// "big smart pointer": non-invasive but larger-than-a-single-pointer
// (2 pointers, plus a heap-allocated Uint32) smart pointer class.
template <typename T>
class Bsp
{
public:

    inline Bsp ()
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        m_real_pointer = NULL;
        m_ref_count = NULL;
    }
    inline Bsp (Bsp const &bsp)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        operator=(bsp);
    }
    template <typename U>
    inline Bsp (Bsp<U> const &bsp)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        operator=<U>(bsp);
    }
    inline explicit Bsp (T *p)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        m_real_pointer = p;
        if (m_real_pointer != NULL)
            m_ref_count = new Uint32(1);
        else
            m_ref_count = NULL;
    }
    template <typename U>
    inline explicit Bsp (U *p)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        m_real_pointer = Dsc<T *>(p);
        if (m_real_pointer != NULL)
            m_ref_count = new Uint32(1);
        else
            m_ref_count = NULL;
    }

    inline ~Bsp ()
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        Release();
    }

    inline void operator = (Bsp const &bsp)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        Release();
        m_real_pointer = bsp.m_real_pointer;
        m_ref_count = bsp.m_ref_count;
        if (m_real_pointer != NULL)
            ++*m_ref_count;
    }
    template <typename U>
    inline void operator = (Bsp<U> const &bsp)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        Release();
        m_real_pointer = Dsc<T *>(bsp.m_real_pointer);
        m_ref_count = bsp.m_ref_count;
        if (m_real_pointer != NULL)
            ++*m_ref_count;
    }
    inline T const &operator * () const
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        assert(m_real_pointer != NULL && "you dereferenced a NULL Bsp<>");
        return *m_real_pointer;
    }
    inline T &operator * ()
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        assert(m_real_pointer != NULL && "you dereferenced a NULL Bsp<>");
        return *m_real_pointer;
    }
    inline T const *operator -> () const
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        assert(m_real_pointer != NULL && "you dereferenced a NULL Bsp<>");
        return m_real_pointer;
    }
    inline T *operator -> ()
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        assert(m_real_pointer != NULL && "you dereferenced a NULL Bsp<>");
        return m_real_pointer;
    }

    inline bool IsValid () const { return m_real_pointer != NULL; }
    inline Uint32 GetRefCount () const { return (m_ref_count != NULL) ? *m_ref_count : 0; }

    inline void Release ()
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        if (m_ref_count != NULL)
        {
            assert(m_real_pointer != NULL);
            assert(*m_ref_count > 0);
            if (*m_ref_count == 1)
            {
                delete m_ref_count;
                delete m_real_pointer;
            }
            else
                --*m_ref_count;
            m_ref_count = NULL;
            m_real_pointer = NULL;
        }
    }

private:

    T *m_real_pointer;
    Uint32 *m_ref_count;

    template <typename U> friend class Bsp;
}; // end of class Bsp<T>

// "invasive smart pointer": invasive but same-size-as-a-single-pointer
// (1 pointer, plus a Uint32 in a baseclass which the pointed-to object
// instance must derive from) smart pointer class.
template <typename T>
class Isp
{
public:

    inline Isp ()
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        m_real_pointer = NULL;
    }
    inline Isp (Isp const &bsp)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        operator=(bsp);
    }
    template <typename U>
    inline Isp (Isp<U> const &bsp)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        operator=<U>(bsp);
    }
    inline explicit Isp (T *p)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        m_real_pointer = p;
        assert(m_real_pointer == NULL || m_real_pointer->m_ref_count == 1);
    }
    template <typename U>
    inline explicit Isp (U *p)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        m_real_pointer = Dsc<T *>(p);
        assert(m_real_pointer == NULL || m_real_pointer->m_ref_count == 1);
    }

    inline ~Isp ()
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        Release();
    }

    inline void operator = (Isp const &bsp)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        Release();
        m_real_pointer = bsp.m_real_pointer;
        if (m_real_pointer != NULL)
            ++m_real_pointer->m_ref_count;
    }
    template <typename U>
    inline void operator = (Isp<U> const &bsp)
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        Release();
        m_real_pointer = Dsc<T *>(bsp.m_real_pointer);
        if (m_real_pointer != NULL)
            ++m_real_pointer->m_ref_count;
    }
    inline T const &operator * () const
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        assert(m_real_pointer != NULL && "you dereferenced a NULL Isp<>");
        return *m_real_pointer;
    }
    inline T &operator * ()
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        assert(m_real_pointer != NULL && "you dereferenced a NULL Isp<>");
        return *m_real_pointer;
    }
    inline T const *operator -> () const
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        assert(m_real_pointer != NULL && "you dereferenced a NULL Isp<>");
        return m_real_pointer;
    }
    inline T *operator -> ()
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        assert(m_real_pointer != NULL && "you dereferenced a NULL Isp<>");
        return m_real_pointer;
    }

    inline bool IsValid () const { return m_real_pointer != NULL; }
    inline Uint32 GetRefCount () const { return (m_real_pointer != NULL) ? m_real_pointer->m_ref_count : 0; }

    inline void Release ()
    {
        cerr << __PRETTY_FUNCTION__ << endl;
        if (m_real_pointer != NULL)
        {
            assert(m_real_pointer->m_ref_count > 0);
            if (m_real_pointer->m_ref_count == 1)
                delete m_real_pointer;
            else
                --m_real_pointer->m_ref_count;
            m_real_pointer = NULL;
        }
    }

private:

    T *m_real_pointer;

    template <typename U> friend class Isp;
}; // end of class Isp<T>

// this is the class your objects must inherit from to use Isp<>
class RefCounted
{
public:

    RefCounted () : m_ref_count(1) { }
    ~RefCounted () { assert(m_ref_count == 1); }

    inline Uint32 GetRefCount () const { return m_ref_count; }

private:

    Uint32 m_ref_count;

    template <typename T> friend class Isp;
}; // end of class RefCounted

} // end of namespace Barf

#endif // !defined(BARF_POINTER_HPP_)
