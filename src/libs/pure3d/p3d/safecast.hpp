//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SAFECAST_HPP
#define _SAFECAST_HPP

#include <radload/radload.hpp>
typedef radLoadInventory::SafeCastBase tSafeEntityCasterBase;

template <class T> class tSafeEntityCaster : public tSafeEntityCasterBase
{
public:
    virtual IRefCount* safe_cast(IRefCount* c) { return (IRefCount*)dynamic_cast<T*>(c); }
};

#endif // _SAFECAST_HPP

