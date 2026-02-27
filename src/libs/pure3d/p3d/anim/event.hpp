//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _EVENT_HPP_
#define _EVENT_HPP_

#include <p3d/entity.hpp>
#include <p3d/error.hpp>

class tEvent;
class tBaseEvent;
class tGenericEvent;
class tEventOrigin;
class tEventRecipient;

const int EVENT_ORIGIN_UNDEFINED = 0;
const int EVENT_ORIGIN_SEQUENCER = 1;
const int EVENT_ORIGIN_NIS       = 2;

class tEvent : public tRefCounted
{
public:
    virtual tUID GetName(void);

    virtual unsigned GetParameter(void);
    virtual void SetParameter(unsigned);
};

class tBaseEvent : public tEvent
{
public:
    tBaseEvent(void) {;}
    tBaseEvent(char* n, char* r, unsigned u = 0);

    tUID GetName(void);
    void SetName(char* name);

    unsigned GetParameter(void);
    void SetParameter(unsigned);

    const char* GetNameFull(void)
    {
#ifdef P3D_ALLOW_ENTITY_GETNAME
        return(name.GetText());
#else
        P3DASSERT(0);
        return NULL;
#endif
    }

protected:
    tName name;
    unsigned param;
};

class tGenericEvent  : public tBaseEvent
{
public:
    tGenericEvent(void) {;}
    tGenericEvent(char* name, char* recip, unsigned len, void* data, bool del, unsigned param = 0);
    virtual ~tGenericEvent();

    unsigned GetDataLength(void);
    void SetDataLength(unsigned);

    void* GetData(void);
    void SetData(void* data, bool del);

protected:
    unsigned dataLen;
    void* data;
    bool delData;
};

class tEventOrigin 
{
public:
    virtual void AddRef(void) = 0;
    virtual void Release(void) = 0;

    virtual unsigned Origin(void) {return EVENT_ORIGIN_UNDEFINED;}
};

class tEventRecipient 
{
public:
    virtual void AddRef(void) = 0;
    virtual void Release(void) = 0;

    virtual void Dispatch(tEvent* event, tEventOrigin* origin, const tName& address) = 0;
    virtual bool HandlesEvents(const tName& address) = 0;
};

#endif

