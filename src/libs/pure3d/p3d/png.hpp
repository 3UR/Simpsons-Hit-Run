//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PNG_HPP
#define _PNG_HPP

#include <p3d/imagefactory.hpp>

class tPNGHandler : public tImageHandler
{
public:
    const char* GetExtension() { return "png"; }
    bool CanLoad()       { return true; }
    bool CanSave()       { return false; }

    bool CheckFormat(Format);
    void CreateImage(tFile* data, tImageHandler::Builder* builder);
    bool SaveImage(tImage* image, char* filename);
};

#endif // _PNG_HPP

