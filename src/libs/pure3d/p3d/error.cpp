//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/error.hpp>
#include <p3d/p3dtypes.hpp>
#include <p3d/utility.hpp>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

class AdaptPDDI : public pddiDevice::MessageCallback
{
    void Info(const char* message)
    {
        p3d::print(message);
    }
    
    void Warn(const char* message)
    {
        p3d::print(message);
    }

    bool Fatal(const char* message, const char* file, int line)
    {
        // we are using ftt asserts, call their error handling function
        return rDebugAssertFail_Implementation(message, file, line);
    }
};

void tDebug::CapturePDDIMessages(pddiDevice* dev)
{
    static AdaptPDDI adapt;
    dev->SetMessageCallback(&adapt);
}


// Like assert, but not compiled out in release mode
void P3DVERIFY(int cond, const char *msg1, const char *msg2, const char *msg3, const char *msg4, const char *msg5)
{
    if(!cond)
    {   
        char buf[255];
        buf[0] = 0;
        if(msg1) strcat(buf, msg1);
        if(msg2) strcat(buf, msg2);
        if(msg3) strcat(buf, msg3);
        if(msg4) strcat(buf, msg4);
        if(msg5) strcat(buf, msg5);
        strcat(buf, "\n");
        rDebugAssertFail_Implementation(buf, "", 0);

    }
}
