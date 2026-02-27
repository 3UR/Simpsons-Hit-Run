// tLinearTable.h
// Created by wng on May 18, 2000 @ 11:48 AM.

#ifndef __tLinearTable__
#define __tLinearTable__

#include <p3d/refcounted.hpp>
#include <p3d/p3dtypes.hpp>
#include "utility/memory.h"

class FeEntity;

class tLinearTable 
: 
    public tRefCounted
    //public FeAllocatesMemory
{
public:

   tLinearTable();


   // raw iterator - finds all objects in table - returns NULL when done
   class RawIterator
//   :
//   public FeAllocatesMemory
   {
   public:
      RawIterator(tLinearTable* table);
      ~RawIterator();
      FeEntity* First();
      FeEntity* Last();
      FeEntity* Next();
      FeEntity* Prev();
      FeEntity* Current();
      FeEntity* First(const tUID uid);
   
   private:
      tLinearTable* table;
      int index;
      tUID mUID;
      int FirstFree(const tUID uid);
      int FirstIndex(const tUID uid);
      int FirstIndex(FeEntity* entity);
   friend class tLinearTable;
   };

   friend class RawIterator;
   
   void AddRef()
   {
       tRefCounted::AddRef();
   };
   void Release()
   {
       tRefCounted::Release();
   };
   FeEntity* Find(const tUID uid);
   void Store(FeEntity* obj);
   void StoreNoAddRef( FeEntity* obj );
   void Remove(FeEntity* obj);
   void Replace(FeEntity* obj, FeEntity* newObj);
   void RemoveAll();
   int  GetElementCount() { return nElement; }
   void Resize( const size_t size );

private:
   virtual ~tLinearTable();

   void Grow();
   void Repack();
   void Restore(FeEntity** elements, int count);

   int tableSize;
   FeEntity** table;
   int nElement;
   int resizeThreshhold;
   int nDeleteMarker;
   int repackThreshhold;
   FeEntity* deleteMarker;
};
#endif
