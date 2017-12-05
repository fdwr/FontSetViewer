//+---------------------------------------------------------------------------
//
//  Copyright (c) Microsoft, 2008. All rights reserved.
//
//  Contents:   Smart pointers.
//
//----------------------------------------------------------------------------
#pragma once


// Helper to return multiple supported interfaces.
//
// Example:
//
//  STDMETHOD(QueryInterface)(IID const& iid, __out void** object) OVERRIDE
//  {
//      COM_BASE_RETURN_INTERFACE(iid, IUnknown, object);
//      COM_BASE_RETURN_INTERFACE(iid, IDWriteInlineObject, object);
//      COM_BASE_RETURN_NO_INTERFACE(object);
//  }
//
#define COM_BASE_RETURN_INTERFACE(iid, U, object) \
    if (iid == UUIDOF(U)) \
    { \
        U* p = static_cast<U*>(this); \
        p->AddRef(); \
        *object = p; \
        return S_OK; \
    }

// For those cases when diamond inheritance causes the ambiguous cast compilation error.
#define COM_BASE_RETURN_INTERFACE_AMBIGUOUS(iid, U, object, subthis) \
    if (iid == UUIDOF(U)) \
    { \
        U* p = static_cast<U*>(subthis); \
        p->AddRef(); \
        *object = p; \
        return S_OK; \
    }

#define COM_BASE_RETURN_NO_INTERFACE(object) \
        *object = nullptr; \
        return E_NOINTERFACE;


// RefCountBase implementation for local reference-counted objects.
class RefCountBase
{
public:
    explicit RefCountBase() throw()
    :   refValue_()
    { }

    explicit RefCountBase(ULONG refValue) throw()
    :   refValue_(refValue)
    { }

    unsigned long IncrementRef() throw()
    {
        return InterlockedIncrement(&refValue_);
    }

    unsigned long DecrementRef() throw()
    {
        ULONG newCount = InterlockedDecrement(&refValue_);
        if (newCount == 0)
            delete this;
        return newCount;
    }

    // Ensure we have a v-table pointer and that the destructor is always
    // called on the most derived class.
    virtual ~RefCountBase()
    { }

protected:
    unsigned long refValue_;
};


class RefCountBaseStatic : public RefCountBase
{
public:
    typedef RefCountBase Base;

    explicit RefCountBaseStatic() throw()
    :   Base()
    { }

    explicit RefCountBaseStatic(ULONG refValue) throw()
    :   Base(refValue)
    { }

    // Just use inherited IncrementRef.

    // Do not delete the reference.
    unsigned long DecrementRef() throw()
    {
        return InterlockedDecrement(&refValue_);
    }
};


// COM base implementation for IUnknown.
//
// Example:
//
//  class RenderTarget : public ComBase<IDWriteTextRenderer>
//
template <typename T = IUnknown, typename RCB = RefCountBase>
class ComBase : public RCB, public T
{
public:
    typedef RCB Base;
    typedef T BaseInterface;

/*
** Leave the definition of QI to the subclass.
**
    // IUnknown interface
    STDMETHOD(QueryInterface)(IID const& iid, __out void** object) OVERRIDE
    {
        COM_BASE_RETURN_INTERFACE(iid, IUnknown, object);
        COM_BASE_RETURN_NO_INTERFACE(object);
    }
*/

    IFACEMETHODIMP_(unsigned long) AddRef()
    {
        return RCB::IncrementRef();
    }

    IFACEMETHODIMP_(unsigned long) Release()
    {
        return RCB::DecrementRef();
    }
};
