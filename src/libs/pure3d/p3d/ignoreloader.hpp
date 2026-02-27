//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _IGNORELOADER_HPP
#define _IGNORELOADER_HPP

//-------------------------------------------------------------------------
// This loader is setup to ignore certain chunk IDs that are OK to have
// in a file, but are not used by the Pure3D runtime.  Unrecognized
// chunks that aren't handled by this loader will cause the load manager
// to print out a warning.
class tIgnoreLoader : public tChunkHandler
{
public:
    virtual tLoadStatus Load(tChunkFile* file, tEntityStore*)
    {
        while(file->ChunksRemaining())
        {
            file->BeginChunk();
            file->EndChunk();
        }
        return LOAD_OK;
    }

    virtual bool CheckChunkID(unsigned id)
    {
        bool handled = false;
        
        switch(id)
        {
            case P3D_HISTORY:
            case P3D_ALIGN:
                handled = true;
            break;
            
            default:
                handled = false;
            break;
        }
        
        return handled;
    }
	unsigned GetChunkID() { return P3D_HISTORY; }
};

#endif
