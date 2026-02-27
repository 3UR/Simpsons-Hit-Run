#ifndef _TARRAY_HPP_
#define _TARRAY_HPP_

#include <string.h>
#include <raddebug.hpp>
#include <radmemory.hpp>

namespace sim
{

#define TARRAYMINSIZE 2

// template class TArray
template<class T> class TArray
{
public:
    TArray(int s = 0) : mSize(0), mArraySize(s), mArray(NULL), mLock(false), mDontUseMemCpy(false), mAllocator(RADMEMORY_ALLOC_DEFAULT)
    {
        if (mArraySize > 0)
            mArray = new T[mArraySize];
        mAllocator = ::radMemoryGetCurrentAllocator ();
    }

    ~TArray() { delete[] mArray; }

    int GetSize() const { return mSize; }
    void Clear() { SetSize(0); }

    T& GetAt(int i)             { rAssert (i < mSize);   return mArray[i]; }
    const T& GetAt(int i) const { rAssert (i < mSize);   return mArray[i]; }
    T& operator[] (int i)       { rAssert (i < mSize);   return mArray[i]; }
    T& NewElem();

    void Add(const T& el);
    void InsertAt(int i, const T& newElement);
    void RemoveAt(int i);
    void SetAt(int i, const T& newElement);
    void ResizeArray(int arraysize);
    int ArraySize() const { return mArraySize; }

    bool Contains(const T& inEl)
    {
        for (int i=0; i<mSize; i++)
        {
            if (mArray[i] == inEl)
                return true;
        }
        return false;
    }

    int Find(const T& inEl)
    {
        for (int i=0; i<mSize; i++)
        {
            if (mArray[i] == inEl)
                return i;
        }
        return -1;
    }

    bool Remove(const T& inEl)
    {
        for (int i=0; i<mSize; i++)
        {
            if (mArray[i] == inEl)
            {
                RemoveAt(i);
                return true;
            }
        }
        return false;
    }

    TArray<T>& operator=(const TArray<T>& inArray)
    {
        if (mArraySize < inArray.mArraySize)
            ResizeArray(inArray.mArraySize);
    
        mSize = inArray.mSize;
    
        for (int i=0; i<mSize; i++)
            mArray[i] = inArray.mArray[i];
    
        return *this;
    }

    void Lock() { rAssert(!mLock); mLock = true; }
    void Unlock() { rAssert(mLock); mLock = false; }

    void DontUseMemCpy() { mDontUseMemCpy = true; }


protected:
    inline void SetSize(int s) { rAssert(!mLock); mSize = s; }

private:
    void Grow();

    int mSize;
    int mArraySize;
    T* mArray;
    bool mLock;
    bool mDontUseMemCpy;
    radMemoryAllocator mAllocator;
};

//
//
//

template<class T> void TArray<T>::Add(const T& el)
{
    if (mSize >= mArraySize)
    {
        Grow();
    }
    mArray[mSize] = el;
    SetSize(mSize+1);
}

template<class T>
    void TArray<T>::InsertAt(int nIndex, const T& newElement)
{
    rAssert( nIndex <= mSize );
    
    if ( mSize >= mArraySize)
    {
        Grow();
    }
    for ( int i = mSize; i > nIndex; --i )
    {
        mArray[i] = mArray[i-1];
    }
    mArray[nIndex] = newElement;
    
    SetSize(mSize+1);
}

template<class T>
    void TArray<T>::RemoveAt(int nIndex)
{
 	rAssert(nIndex < mSize);

	if(mSize > 1)
	{
		mArray[nIndex] = mArray[mSize - 1];
	}
	SetSize(mSize-1);
}

template<class T>
    void TArray<T>::SetAt(int nIndex, const T& newElement)
{
    rAssert(nIndex < mSize);
    mArray[nIndex] = newElement;
}

template<class T>
    void TArray<T>::Grow()
{
    rAssertMsg( false, "TArray::Grow - automatic grows are a cause of fragmentation" );
    ResizeArray(2*mArraySize);
}

template<class T>
    void TArray<T>::ResizeArray(int newarraysize)
{
    if (newarraysize == 0)
        newarraysize = TARRAYMINSIZE;
    
    if (mArraySize < newarraysize)
    {
        radMemoryAllocator old = ::radMemorySetCurrentAllocator (mAllocator);
        T* array = new T[newarraysize];
        ::radMemorySetCurrentAllocator (old);
        
        rAssert(array != NULL);
        
        if (mSize > 0)
        {
            if (mDontUseMemCpy)
            {
                for ( int i = 0; i < mSize; ++i )
                {
                    array[i] = mArray[i];
                }
            }
            else
            {
                memcpy(array, mArray, mSize*sizeof(T));
            }
        }
        
        delete[] mArray;
        mArray = array;
        mArraySize = newarraysize;
    }
}

template<class T> T& TArray<T>::NewElem()
{
    if (mSize >= mArraySize)
    {
        Grow();
    }
    SetSize(mSize+1);
    return mArray[mSize-1];
}

} // sim

#endif


