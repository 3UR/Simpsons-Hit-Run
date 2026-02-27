//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PLAT_FILEMAP_HPP
#define _PLAT_FILEMAP_HPP

#include <p3d/file.hpp>

class tLinuxFileMap : public tFileMem
{
public:
    tLinuxFileMap(const char* filename);

    bool IsOpen(void) { return GetMemory() != NULL; }

protected:
    virtual ~tLinuxFileMap();

    void Open(const char* filename);
    void Close(void);

    int fh;
};

#endif /*_FILEMAP_HPP*/

