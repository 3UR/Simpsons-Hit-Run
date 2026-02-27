//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _BMP_HPP
#define _BMP_HPP

#include <p3d/imagefactory.hpp>

class tBMPHandler : public tImageHandler
{
public:
    const char* GetExtension() { return "bmp"; }
    bool CanLoad()       { return true; }
    bool CanSave()       { return false; }

    bool CheckFormat(Format);
    void CreateImage(tFile* data, tImageHandler::Builder* builder);
    bool SaveImage(tImage*, char*);
};

#endif // _BMP_HPP

