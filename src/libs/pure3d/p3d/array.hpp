//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _T_PURE3D_ARRAY_HPP_
#define _T_PURE3D_ARRAY_HPP_

#include <p3d/error.hpp>

//
// tArray: A simple semi-fixed size array, with size and bound checking 
// port most of code from p3dviewer. A empty array could be declared,
// and the size could be set by SetSize( ) for static array( tArray
// and tPtrArray). However, the size could be set only one time.
//
template <class T> class tArray
{
public:
   // construct an empty array    
   tArray( ) : size( 0 ), data(0){ /* void */ }   //we do allow to construct null array
   tArray(unsigned s) : size(s)
   {  
       //allocate memory only if size > 0
       if( size > 0 )
           data = new T[size];
       else
           data = 0;
   }

   // construct from a standard C array and size
   tArray(unsigned s, T* d) : size(s)
   { 
       //allocate memory only if size > 0
       if( size > 0 )
           data = new T[size];
       else
           data = 0;

       for(unsigned i = 0; i < size; i++)
           data[i] = d[i];
   }

   //since we allow null array to be automatically created, so
   //we also allow to set the size of the array after. However
   //this function does allow to be called once to set the real
   //size of the array. Any call afterward will be rejected
   //also, this function check if required memory space will 
   //be available or not. 
   virtual bool SetSize( int nSize )
   {
       if( size > 0 )
           return false;        //do nothing either because SetSize is called, or 
                                //already constructed with the right size;
       size = nSize;

       if( data )
       {
           delete [] data;
       }
       data = new T[ size ];
        
       if( data )
           return true;
       else
           return false;
   }

   // destruct
   virtual ~tArray() 
   { 
      delete [] data;
   }

   // get the size
   unsigned Size(void) const
   { 
      return size; 
   }

   // get the indexed element
   T& operator[](unsigned i) 
   { 
       P3DASSERT(i < size);
       return data[i]; 
   }

   // get the indexed element
   T& operator[](unsigned i)  const
   { 
       P3DASSERT(i < size);
       return data[i]; 
   }

   // get the raw data for passing to a function that takes a C array
   T* GetData() 
   { 
      return data;
   }

protected:
   unsigned size;
   T* data;
};


template <class T> class tDynamicArray : public tArray<T>
{
public:
    tDynamicArray( ) : tArray<T>( ){ /* void */ }
    tDynamicArray(unsigned s) : tArray<T>(s) {  /* */ }
    tDynamicArray(unsigned s, T* d) : tArray<T>(s,d) {  /* */ }

    using tArray<T>::size;
    using tArray<T>::data;

    void Shrink(unsigned newSize)
    {
        if(size > newSize)
            Resize(newSize);
    }

    void Grow(unsigned newSize)
    {
        if(size < newSize)
            Resize(newSize);
    }

    void Resize(unsigned newSize)
    {
        T* newData = new T[newSize];
        unsigned amountToCopy = (newSize > size) ? size : newSize;
        
        for(unsigned i = 0; i < amountToCopy; i++)
            newData[i] = data[i];

        delete [] data;
        data = newData;
        size = newSize;
    }

};

//
//  a static array template for pointer. Allocating a pointer array
//  will automatically initialize the array, and array compacting
//  and find-null-and-fill will be supported
//
template <class T> class tPtrArray : public tArray<T>
{
public:
    using tArray<T>::size;
    using tArray<T>::data;
    
    // construct an empty array    
    tPtrArray( ) : tArray<T>( ){ /* void */ }   //we do allow to construct null array
    tPtrArray(unsigned s) : tArray<T>( s ) 
    {  
        for( unsigned i = 0; i < size; ++i )
            data[ i ] = 0;
    }

    // construct from a standard C array and size
    tPtrArray(unsigned s, T* d) : tArray<T>( s, d ){  /* void */ }

    bool SetSize( int nSize )
    {
        bool result = tArray<T>::SetSize( nSize );

        if( result ){
            for( int i = 0; i < nSize; ++i )
                data[ i ] = 0;
        }

        return result;
    }

    // compact the list so that all unsed elements are at the end
    // only works on objects where !t is defined, and means that object is unused, i,e, pointers
    // returns number of elements in compacted list
    unsigned Compact(void)
    {
       unsigned compactDistance = 0;
       unsigned count = 0;

       for(unsigned i = 0; i < size; i++){
          if(!data[i])
             compactDistance++;

          else{
             count++;
             if(compactDistance){
                data[i - compactDistance] = data[i];
                data[i] = 0;
             }
          }
       }
       return count;
    }

    // add element to array in empty spot 
    // only works on objects where !t is defined, and means that object is unused, i,e, pointers
    unsigned Add(T t)
    {
       for(unsigned i = 0; i < size; i++){
          if(!data[i]){
             data[i] = t;
             return i;
          }
       }
       return -1;
       // assert(0);
    }
};

template <class T> class tPtrDynamicArray : public tPtrArray<T>
{
public:
    tPtrDynamicArray( ) : tPtrArray<T>( ){ /* void */ }
    tPtrDynamicArray(unsigned s) : tPtrArray<T>(s) {  /* */ }
    tPtrDynamicArray(unsigned s, T* d) : tPtrArray<T>(s,d) {  /* */ }

    using tPtrArray<T>::size;
    using tPtrArray<T>::data;

    void Shrink(unsigned newSize)
    {
        if(size > newSize)
            Resize(newSize);
    }

    void Grow(unsigned newSize)
    {
        if(size < newSize)
            Resize(newSize);
    }

    void Resize(unsigned newSize)
    {
        T* newData = new T[newSize];
        unsigned amountToCopy = (newSize > size) ? size : newSize;
        
        unsigned i;
        for( i = 0; i < amountToCopy; i++)
            newData[i] = data[i];

        // set the rest items to null
        for( i = amountToCopy;  i < newSize; ++i )
            newData[ i ] = 0;

        delete [] data;
        data = newData;
        size = newSize;
    }

    // add element to array in empty spot and resize if neccesary
    // only works on objects where !t is defined, and means that object is unused, i,e, pointers
    unsigned Add(T t)
    {
        unsigned oldSize = size;
        for(unsigned i = 0; i < oldSize; i++){
            if(!data[i]){
                data[i] = t;
                return i;
            }
        }
        Grow(this->Size()*2);
        data[oldSize] = t;
        return oldSize;

    }

    unsigned RealSize( ) const
    {
        unsigned count = 0;
        for(unsigned i = 0; i < size; i++){
            if(data[i]){
                ++count;     
            }
        }
        return count;
    }
};
#endif // #ifndef _TARRAY_HPP_

