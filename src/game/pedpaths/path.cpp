//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        path.cpp
//
// Description: Implements Path class
//
// History:     09/18/2002 + Created -- Dusit Eakkachaichanvet
//
//=============================================================================


#include <pedpaths/path.h>
#include <pedpaths/pathsegment.h>

#include <raddebug.hpp>
#include <memory/srrmemory.h>

class PathSegment;


Path::Path() :
    mIsClosed( false ),
    mNumPathSegments( 0 ),
    mNumPeds( 0 ),
    mPathSegments( NULL )
{
}

Path::Path( bool isClosed, int nSegments )
{
    mIsClosed = isClosed;
    AllocateSegments( nSegments );
}

void Path::AllocateSegments( int nSegments )
{
MEMTRACK_PUSH_GROUP( "Path" );
    rAssert( nSegments > 0 );

    HeapMgr()->PushHeap( GMA_LEVEL_OTHER );

    if( mPathSegments != NULL )
    {
        // make sure we're only allocating ONCE, so we don't
        // fragment the memory.
        rAssert( false );
        delete[] mPathSegments;
    }

    mPathSegments = new PathSegment*[ nSegments ];
    int i;
    for( i=0; i<nSegments; i++ )
    {
        mPathSegments[i] = new PathSegment;
    }
    mNumPathSegments = nSegments;

    HeapMgr()->PopHeap( GMA_LEVEL_OTHER );

MEMTRACK_POP_GROUP( "Path" );
}

Path::~Path()
{
    if( mPathSegments != NULL )
    {
        delete [] mPathSegments;
    }
    mPathSegments = NULL;
}


PathSegment* Path::GetPathSegmentByIndex(int index) 
{
    rAssert( mPathSegments != NULL );
    rAssert( 0 <= index && index < mNumPathSegments );

    return mPathSegments[index];
        
}

bool Path::AddPedestrian()
{
    if( mNumPeds >= Path::MAX_PEDESTRIANS )
    {
        return false;
    }
    mNumPeds++;
    return true;
}
bool Path::RemovePedestrian()
{
    if( mNumPeds <= 0 )
    {
        return false;
    }
    mNumPeds--;
    return true;
}

bool Path::IsFull() const
{
    if( mNumPeds >= Path::MAX_PEDESTRIANS )
    {
        return true;
    }
    return false;
}
