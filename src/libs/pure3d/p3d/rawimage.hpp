//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _RAWIMAGE_HPP
#define _RAWIMAGE_HPP

#include <p3d/imagefactory.hpp>

class tRawImageHandler : public tImageHandler
{
public:
    const char* GetExtension() { return "raw"; }
    bool CanLoad()       { return true; }
    bool CanSave()       { return false; }

    bool CheckFormat(Format);
    void CreateImage(tFile* file, tImageHandler::Builder* builder);
};

#endif // RAWIMAGE_HPP

