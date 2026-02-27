//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/platform/sgi/plat_filemap.hpp>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

tLinuxFileMap::tLinuxFileMap(char* filename)
{
    buf = NULL;
    size = 0;
    Open(filename);
    SetFilename(filename);
}

tLinuxFileMap::~tLinuxFileMap()
{
    Close();
}

void tLinuxFileMap::Open(char* filename)
{
    fh = open(filename, O_RDONLY);
    if(fh == -1)
    {
        return;
    }
    
    size = lseek(fh, 0, SEEK_END);
    if(size < 0)
    {
        return;
    }

    buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fh, 0);
    if(!buf)
    {
        close(fh);
        return;
    }
}

void tLinuxFileMap::Close()
{
    munmap(static_cast<caddr_t>(buf),size);
    close(fh);
}

