//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _TARGA_HPP
#define _TARGA_HPP

#include <p3d/imagefactory.hpp>

class tTargaHandler : public tImageHandler
{
public:
    const char* GetExtension() { return "tga"; }
    bool CanLoad()       { return true; }
    bool CanSave()       { return true; }

    bool CheckFormat(Format);
    void CreateImage(tFile* file, tImageHandler::Builder* builder);
    bool SaveImage(tImage* image, char* filename);
};

#endif // TARGA_HPP

