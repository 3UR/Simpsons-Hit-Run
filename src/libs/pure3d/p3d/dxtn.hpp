//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _DXTN_HPP
#define _DXTN_HPP

#include <p3d/imagefactory.hpp>

class tDXTNHandler : public tImageHandler
{
public:
    const char* GetExtension() { return "dds"; }
    bool CanLoad()       { return true; }
    bool CanSave()       { return false; }

    bool CheckFormat(Format);
    void CreateImage(tFile* data, tImageHandler::Builder* builder);
    bool SaveImage(tImage*, char*);
};

#endif // _DXTN_HPP

