#ifndef _TLIST_HPP_
#define _TLIST_HPP_

#include <raddebug.hpp>
#include <radmemory.hpp>

namespace sim
{

#define TListMINSIZE 2
typedef void* voidp;

class ListLocker;

// list of void pointer class
class VoidPList
{
    friend class ListLocker;
    
public:
    VoidPList(int arraysize);
    virtual ~VoidPList();
    
    int GetSize() const { return mSize; }
    void RemoveAll();
    void RemoveAt(int i);
    void Reset();
    void Compact();
    
    void Add(const voidp el);
    bool Remove(const voidp el);
    void SetToNullAt(int i) { SetAt(i, NULL); }
    void ResizeArray(int arraysize);
    
protected:
    bool mLock;
    
    void SetAt(int i, voidp newElement);
    void InsertAt(int i, voidp newElement);
    voidp& GetAt(int i) const { rAssert(i < mSize);  return mArray[i]; }
    
    bool Contains(const voidp el) const;
    int Index(const voidp el) const;
    
    void Add(const VoidPList& list);
    void Set(const VoidPList& list);
    
    inline void SetSize(int s) { rAssert(!mLock); mSize = s; }
    
private:
    void Grow();
    
    int mSize;
    int mArraySize;
    voidp * mArray;
    radMemoryAllocator mAllocator;
};

// template class TList derived from the void* list
template <class T> class TList
    : public VoidPList
{
public:
    TList(int i = 0) : VoidPList(i) {}
    inline T& GetAt(int i) { return (T&) (VoidPList::GetAt(i)); }
    inline void SetAt(int i, T& newElement) { VoidPList::SetAt(i, voidp(newElement)); }
    inline void InsertAt(int i, T& newElement) { VoidPList::InsertAt(i, voidp(newElement)); }
    inline T& operator[] (int i) { return (T&) (VoidPList::GetAt(i)); }
    inline void Add(const T el) { VoidPList::Add(voidp(el)); }
    inline bool Contains(const T el) const { return VoidPList::Contains(voidp(el)); }
    inline int Index(const T el) const { return VoidPList::Index(voidp(el)); }
    inline bool Remove(const T el) { return VoidPList::Remove(voidp(el)); }
    
    //void Add(const TList<T>& list) { VoidPList::Add((VoidPList&) list); }
    //void Set(const TList<T>& list) { VoidPList::Set((VoidPList&) list); }
    inline TList<T>& operator+=(const TList<T>& list) { VoidPList::Add((VoidPList&) list); return *this; }
    inline TList<T>& operator=(const TList<T>& list) { VoidPList::Set((VoidPList&) list); return *this; }
};

class ListLocker
{
public:
    ListLocker(VoidPList* list);
    ~ListLocker();
    
private:
    VoidPList* mList;
};

} // sim

#endif


