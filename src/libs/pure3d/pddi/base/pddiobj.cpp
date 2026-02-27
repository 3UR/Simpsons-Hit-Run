/*===========================================================================
    pddiobj.cpp
    10-Oct-99 Created by Neall

    Copyright (c)1999  Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <pddi/pddi.hpp>

pddiObject::pddiObject()
{
    refCount = 0;
    lastError = PDDI_OK;
}

pddiObject::~pddiObject()
{
    //
}

void pddiObject::AddRef()
{
    refCount++;
}

void pddiObject::Release()
{
    refCount--;
    if(refCount < 1)
    {
        delete this;
    }
}

int pddiObject::GetLastError()
{
    return lastError;
}

