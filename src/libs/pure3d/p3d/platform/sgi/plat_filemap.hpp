//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _PLAT_FILEMAP_HPP
#define _PLAT_FILEMAP_HPP

#include <p3d/file.hpp>

class tLinuxFileMap : public tFileMem
{
public:
    tLinuxFileMap(char* filename);

    bool IsOpen(void) { return buf != NULL;}

protected:
    virtual ~tLinuxFileMap();

    void Open(char* filename);
    void Close(void);

    void* buf;
    int size;
    int fh;
};

#endif /*_FILEMAP_HPP*/

