// 2008.06.28 - Copyright Victor Dods - Licensed under Apache 2.0

#if !defined(BARF_WEAKREFERENCE_HPP_)
#define BARF_WEAKREFERENCE_HPP_

#include "barf.hpp"

namespace Barf {

// the point of class WeakReference is to be able to track the reference
// count for a particular pointer value.  it's not a "smart" pointer in that
// it doesn't delete the reference counted pointer.
template <typename T>
class WeakReference
{
public:

    WeakReference () : m_instance(NULL) { }
    WeakReference (T *pointer) : m_instance(new Instance<T>(pointer)) { m_instance->IncrementReferenceCount(); }
    WeakReference (WeakReference<T> const &r) { m_instance = r.m_instance; if (m_instance != NULL) m_instance->IncrementReferenceCount(); }
    ~WeakReference () { if (m_instance != NULL) m_instance->DecrementReferenceCount(); }
    WeakReference &operator = (WeakReference<T> const &r)
    {
        if (m_instance != NULL)
            m_instance->DecrementReferenceCount();
        m_instance = r.m_instance;
        if (m_instance != NULL)
            m_instance->IncrementReferenceCount();
        return *this;
    }

    T &operator * () { assert(m_instance != NULL); return **m_instance; }
    T const &operator * () const { assert(m_instance != NULL); return **m_instance; }

    T *operator -> () { assert(m_instance != NULL); return &**m_instance; }
    T const *operator -> () const { assert(m_instance != NULL); return &**m_instance; }

    bool IsValid () const { return m_instance != NULL; }
    void Release () { if (m_instance != NULL) { m_instance->DecrementReferenceCount(); m_instance = NULL; } }

    bool InstanceIsValid () const { assert(m_instance != NULL); return m_instance->IsValid(); }
    void InstanceRelease () { assert(m_instance != NULL); m_instance->Release(); }
    Uint32 ReferenceCount () const { assert(m_instance != NULL); return m_instance->ReferenceCount(); }

private:

    template <typename U>
    class Instance
    {
    public:

        Instance (U *pointer) : m_pointer(pointer), m_reference_count(0) { assert(m_pointer != NULL && "invalid (NULL) pointer"); }
        ~Instance () { assert(m_reference_count == 0); }

        U &operator * () { assert(m_pointer != NULL && "dereferenced a NULL pointer"); return *m_pointer; }
        U const &operator * () const { assert(m_pointer != NULL && "dereferenced a NULL pointer"); return *m_pointer; }

        bool IsValid () const { return m_pointer != NULL; }
        void Release () { m_pointer = NULL; }
        Uint32 ReferenceCount () const { return m_reference_count; }

        void IncrementReferenceCount () { assert(m_reference_count < Uint32(-1)); ++m_reference_count; }
        void DecrementReferenceCount () { assert(m_reference_count > 0); if (--m_reference_count == 0) delete this; }

    private:

        Instance (Instance const &) { assert(false && "this should never be called"); }
        void operator = (Instance const &) { assert(false && "this should never be called"); }

        U *m_pointer;
        Uint32 m_reference_count;
    }; // end of class WeakReference<T>::Instance<U>

    Instance<T> *m_instance;
}; // end of class WeakReference<T>

} // end of namespace Barf

#endif // !defined(BARF_WEAKREFERENCE_HPP_)
