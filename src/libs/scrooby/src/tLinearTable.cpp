#include "stdafx.h"

#include "tLinearTable.h"
#include "FeEntity.h"
#include <p3d/error.hpp>

#include <string.h>



const int TABLE_SIZE = 1;        // initial table size (must be a power-of-2!)
const int RESIZE_PERCENTAGE = 80;   // table will resize if it gets this full
const int REPACK_PERCENTAGE = 60;   // table will repack if there are this many deleted items in the table




//-------------------------------------------------------------------
tLinearTable::RawIterator::RawIterator(tLinearTable* t) : table(t), index(0)
{
   table->AddRef();
}

tLinearTable::RawIterator::~RawIterator()
{
   table->Release();
}

FeEntity* tLinearTable::RawIterator::First()
{
   index = -1;
   mUID = 0;
   return Next();
}
FeEntity* tLinearTable::RawIterator::First(const tUID uid)
{
   index = -1;
   mUID = uid;
   return Next();
}

int tLinearTable::RawIterator::FirstFree( const tUID)
{
    index = 0;
    while( index < table->tableSize ) 
    {
       if (table->table[index]==0)
       {
             break;
       }
        if (table->table[index]==table->deleteMarker)
       {
             break;
       }
        index++;
   }
   return index;
}

FeEntity* tLinearTable::RawIterator::Next()
{
   for( index++; (index < table->tableSize) && !table->table[index]; index++ )
   {
       //nothing
   };
   
   while( index < table->tableSize )
   {
        if( table->table[index] ) 
        {
            if( ( mUID == static_cast< tUID >( 0 ) ) && (table->table[index] != table->deleteMarker) )
            {
                break;
            }
            else if( table->table[index]->GetUID() == mUID )
            {
                break;
            }
        }
    
        index++;
   }
   
   if( index < table->tableSize )
   {
       FeEntity* obj = table->table[index];
       return obj;
   }
   else
   {
       return NULL;
   }
}


int tLinearTable::RawIterator::FirstIndex(const tUID uid)
{
   index = 0;
   while (index < table->tableSize) {
       if (table->table[index]==0)
        return -1;// end of used table
    if (table->table[index]->GetUID()==uid)
        break;
    index++;
   }
   return index;
}

int tLinearTable::RawIterator::FirstIndex(FeEntity* entity)
{
   index = 0;
   while (index < table->tableSize) {
       if (table->table[index]==0)
        return -1;// end of used table
    if (table->table[index]==entity)
        break;
    index++;
   }
   return index;
}

FeEntity* tLinearTable::RawIterator::Last()
{
   index = table->tableSize;
   return Prev();
}

FeEntity* tLinearTable::RawIterator::Prev()
{
   for( --index ;(index >= 0) && !table->table[index]; --index )
   {
       //nothing
   }
   FeEntity* obj = (index >= 0) ? table->table[index] : NULL;
   return obj;
}

FeEntity* tLinearTable::RawIterator::Current()
{
   
   if( (index >= 0) || (index < table->tableSize) )
   {
       return( table->table[index] );
   }
   else
   {
       return( NULL );
   }
}

//-------------------------------------------------------------------
tLinearTable::tLinearTable() :
   tableSize(TABLE_SIZE), 
   table(NULL), 
   nElement(0), 
   resizeThreshhold(0),
   nDeleteMarker(0), 
   repackThreshhold(0)
{
   deleteMarker = new FeEntityDummy();
   deleteMarker->SetUID(~0);  // hopefully this uid will never be generated
   deleteMarker->AddRef();
}

tLinearTable::~tLinearTable()
{
   RemoveAll();
   delete[] table;
   deleteMarker->Release();
}

void tLinearTable::RemoveAll()
{
   if(table)
   {
      for(int i=0; i < tableSize; i++)
      {
         if(table[i] && (table[i] != deleteMarker))
         {
            table[i]->Release();
         }
         table[i] = NULL;
      }
   }
   nElement = 0;
   nDeleteMarker = 0;
}

//===========================================================================
// Grow
//===========================================================================
// Description: Grows the dynamically sizing array
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      None
//
//===========================================================================
void tLinearTable::Grow()
{
    rAssertMsg( false, "Automatic array growing often causes fragmentation - tell IAN" );
    Resize( tableSize * 2 );
}

// This function builds a new table with all the delete markers
// removed.  This will increase search performance on tables
// that have had a lot of items deleted from them.
void tLinearTable::Repack()
{
   if(table)
   {
      // build new table, rehash all elements
      nDeleteMarker = 0;
      FeEntity** oldTable = table;
      //table = new ( ScroobyPermPool ) FeEntity*[ tableSize ];
      table = new FeEntity*[ tableSize ];
      memset( table, 0, tableSize * sizeof( FeEntity* ) );
      Restore( oldTable, tableSize );
      delete[] oldTable;
   }
}

//===========================================================================
// Restore
//===========================================================================
// Description: takes a list of pointers and stores them in the current table
//
// Constraints:    None
//
// Parameters:    elements - the entities to store in the table
//              count - how many items are in the array
//
// Return:      None
//
//===========================================================================
void tLinearTable::Restore(FeEntity** elements, int count)
{
   for(int i=0; i < count; i++)
   {
      if(elements[i] && (elements[i] != deleteMarker))
      {
         StoreNoAddRef( elements[i] );
      }
   }
}

FeEntity* tLinearTable::Find(const tUID uid)
{
   RawIterator iterator(this);
   return iterator.First(uid);
}

//===========================================================================
// StoreNoAddRef
//===========================================================================
// Description: Stores the object without incrementing the reference count
//
// Constraints:    None
//
// Parameters:    obj - the entity to store in the table
//
// Return:      None
//
//===========================================================================
void tLinearTable::StoreNoAddRef( FeEntity* obj )
{
   if(nElement >= resizeThreshhold)
   {
      Grow();
   }

   // TODO: collision detection
   tUID uid = obj->GetUID();
   P3DASSERT(uid != static_cast< tUID >( ~0UL ) );   // this uid is reserved for deleted items
   RawIterator iterator(this);

   int where = iterator.FirstFree(uid);
   if(table[where] == deleteMarker)
   {
      nDeleteMarker--;
   }
   table[where] = obj;
   nElement++;
}

//===========================================================================
// Store
//===========================================================================
// Description: Stores the object incrementing the reference count
//
// Constraints:    None
//
// Parameters:    obj - the entity to store in the table
//
// Return:      None
//
//===========================================================================
void tLinearTable::Store( FeEntity* obj )
{
    StoreNoAddRef( obj );
    obj->AddRef();
}

void tLinearTable::Remove(FeEntity* obj)
{
   RawIterator iterator(this);
   int where = iterator.FirstIndex(obj->GetUID());
   if(where >= 0)
   {
      table[where]->Release();
      table[where] = deleteMarker;
      nElement--;
      if(++nDeleteMarker > repackThreshhold)
      {
         Repack();
      }
   }
}

void tLinearTable::Replace(FeEntity* obj, FeEntity* newObj)
{
   RawIterator iterator(this);
//   int where = iterator.FirstIndex(obj->GetUID());
   int where = iterator.FirstIndex(obj);
   if(where >= 0)
   {
      table[where]->Release();
      table[where] = newObj;
      newObj->AddRef();
   }
}

//===========================================================================
// tLinearTable::Resize
//===========================================================================
// Description: resizes the lineartable to the correct size
//
// Constraints: None
//
// Parameters:  size - the size to resize to
//
// Return:      None
//
//===========================================================================
void tLinearTable::Resize( const size_t size )
{
    if( table )
    {
        // build new table, rehash all elements
        FeEntity** oldTable = table;
        int oldSize = tableSize;
        nElement = 0;
        if( size > static_cast< size_t >( tableSize ) )
        {
            tableSize = size;
        }
        nDeleteMarker = 0;
        table = new FeEntity*[ tableSize ];
        memset( table, 0, tableSize * sizeof( FeEntity* ) );        //IAN IMPROVE: zero the array - is this necessary?
        Restore( oldTable, oldSize );
        delete[] oldTable;
    }
    else
    {
        tableSize = size;
        delete[] table;
        table = new FeEntity*[ tableSize ];
        memset( table, 0, tableSize * sizeof( FeEntity* ) );
    }

    resizeThreshhold = tableSize;
    repackThreshhold = (tableSize / 100) * REPACK_PERCENTAGE;
}