//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/anim/event.hpp>

#include<string.h>

tUID tEvent::GetName(void)
{
    return tUID(0);
}

unsigned tEvent::GetParameter(void)
{
    return 0;
}

void tEvent::SetParameter(unsigned)
{
}

tBaseEvent::tBaseEvent(char* n, char* r, unsigned u)
{
    SetName(n);
    SetParameter(u);
}

tUID tBaseEvent::GetName(void)
{
    return name.GetUID();
}

void tBaseEvent::SetName(char* n)
{
    name.SetText(n);
}

unsigned tBaseEvent::GetParameter(void)
{
    return param;
}

void tBaseEvent::SetParameter(unsigned p)
{
    param = p;
}

tGenericEvent::tGenericEvent(char* name, char* recip, unsigned len, void* d, bool del, unsigned param) : tBaseEvent(name, recip, param), dataLen(len), data(d), delData(del)
{
}

tGenericEvent::~tGenericEvent()
{
    if(delData)
        delete [] (char*)data;
}

unsigned tGenericEvent::GetDataLength(void)
{
    return dataLen;
}

void tGenericEvent::SetDataLength(unsigned l)
{
    dataLen = l;
}

void* tGenericEvent::GetData(void)
{
    return data;
}

void tGenericEvent::SetData(void* d, bool del)
{
    data = d;
    delData = del;
}

