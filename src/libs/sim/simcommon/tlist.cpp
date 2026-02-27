
#include "simcommon/tlist.hpp"
#include <string.h> // memcpy
#include <radmemory.hpp>

namespace sim
{


VoidPList::VoidPList(int arraysize)
    : mLock(false),
    mSize(0),
    mArraySize(arraysize),
    mArray(NULL),
    mAllocator(RADMEMORY_ALLOC_DEFAULT)
{
    mAllocator = ::radMemoryGetCurrentAllocator ();

    if (mArraySize > 0)
        mArray = static_cast<voidp*>(::radMemoryAlloc (mAllocator, mArraySize * sizeof(voidp)));
}

VoidPList::~VoidPList()
{ 
    ::radMemoryFree( mArray ); 
}

bool VoidPList::Remove(const voidp el)
{
    bool removed = false;
    
    for (int i=mSize-1; i>=0; i--)
    {
        if (el == mArray[i])
        {
            RemoveAt(i);
            removed = true;
        }
    }
    return removed;
}

bool VoidPList::Contains(const voidp el) const
{
    for (int i=0; i<mSize; i++)
    {
        if (el == mArray[i])
            return true;
    }
    return false;
}

int VoidPList::Index(const voidp el) const
{
    for (int i=0; i<mSize; i++)
    {
        if (el == mArray[i])
            return i;
    }
    return -1;
}

void VoidPList::Add(const voidp el)
{
    if (mSize >= mArraySize)
    {
        Grow();
    }
    mArray[mSize] = el;
    SetSize(mSize+1);
    
}

void VoidPList::Grow()
{
    rTunePrintf( "simcommon/tlist.cpp - Growing Arrays is bad - please tell Ian\n" );
    ResizeArray(2*mArraySize);
}

void VoidPList::ResizeArray(int newarraysize)
{
    if (newarraysize == 0)
        newarraysize = TListMINSIZE;
    
    if (mArraySize < newarraysize)
    {
        voidp *array = static_cast<voidp*>(::radMemoryAlloc (mAllocator, sizeof(voidp)*newarraysize));
        
        rAssert(array != NULL);
        
        if (mSize > 0)
            memcpy(array, mArray, mSize*sizeof(voidp));
        
        ::radMemoryFree( mArray );
        mArray = array;
        mArraySize = newarraysize;
    }
}

void VoidPList::RemoveAll()
{
    SetSize(0);
}

void VoidPList::InsertAt(int nIndex, voidp newElement)
{
    rAssert( nIndex <= mSize );
    
    if ( mSize >= mArraySize)
    {
        ResizeArray(mSize*2);
    }
    for ( int i = mSize; i > nIndex; i-- )
    {
        mArray[i] = mArray[i-1];
    }
    mArray[nIndex] = newElement;
    
    SetSize(mSize+1);
}

void VoidPList::RemoveAt(int nIndex)
{
    if (nIndex < mSize)
    {
        for (int i=nIndex; i<mSize-1; i++)
        {
            mArray[i] = mArray[i+1];
        }
        SetSize(mSize-1);
    }
}

void VoidPList::SetAt(int nIndex, voidp newElement)
{
    rAssert(nIndex < mSize);
    mArray[nIndex] = newElement;
}

void VoidPList::Compact()
{
    if (mSize > 0)
    {
        int hole = -1;
        for (int i=0; i<mSize; i++)
        {
            if (mArray[i] == NULL)
            {
                if (hole == -1)
                    hole = i; // find the first hole
            }
            else if (hole != -1)
            {
                mArray[hole] = mArray[i];
                hole++;
                //mArray[i] = NULL; // to test
                //rAssert(mArray[hole] == NULL);
            }
        }
        if (hole != -1)
        {
            SetSize(hole);
        }
    }
}

// shallow copy the element of list into this
void VoidPList::Set(const VoidPList& list)
{
    RemoveAll();
    ResizeArray(list.GetSize());
    
    SetSize(list.GetSize());
    
    if (mSize > 0)
        memcpy(mArray, list.mArray, mSize*sizeof(voidp));
}

void VoidPList::Add(const VoidPList& list)
{
    if (list.mSize > 0)
    {
        if (mArraySize < mSize + list.mSize)
        {
            ResizeArray(mSize + list.mSize);
        }
        if (list.mSize > 0)
        {
            memcpy(&mArray[mSize], list.mArray, list.mSize*sizeof(voidp));
        }
        
        SetSize(mSize + list.GetSize());
    }
}

void VoidPList::Reset()
{
    for (int i=0; i<mArraySize; i++)
        mArray[i] = NULL;
}


//
//
//


ListLocker::ListLocker(VoidPList* list)
    : mList(list)
{
    rAssert(mList != NULL);
    rAssertMsg(mList->mLock == false, "List already locked, can't apply successive locks");
    mList->mLock = true;
}

ListLocker::~ListLocker()
{
    mList->mLock = false;
}

} // sim