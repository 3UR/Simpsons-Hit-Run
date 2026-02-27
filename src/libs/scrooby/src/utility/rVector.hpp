//******************************************************************************
//
//  rVector.hpp
//
//******************************************************************************
#ifndef R_VECTOR_HPP
#define R_VECTOR_HPP

#include <raddebug.hpp>
#include <radmemory.hpp>
#include <stdlib.h>

template <class T> class rVector
{
    public:
        // Constructors, Destructors, and Operators
        rVector( void );
        virtual ~rVector( void );
        const T& operator[](long location) const;
        T& operator[](long location);

        // Public Methods
        void Clear();
        void ClearAndDestroy();
        long Capacity() const;
        bool Empty() const;        
        void Erase(long location);
        void Erase(T& item);
        void Insert(T& item);
        void Insert(T& item, long location);
        void Reserve(long number);
        long Size() const;
        long GrowSize() const;
        void SetGrowSize(long size);

#ifdef RAD_DEBUG
        // Public Attributes
        static long s_VectorsCreated;
        static long s_TotalVectorSize;
#endif

    private:
        // Unused Constructors, Destructors, and Operators
        rVector( const rVector& aVector );
        rVector& operator=( const rVector& aVector );
        rVector* operator&( void );
        const rVector* operator&( void ) const;

        // Private Methods
        T* m_Array;
        long m_Size;
        long m_Capacity;
        long m_GrowSize;
        radMemoryAllocator m_Allocator;
};


//===========================================================================
// rVector::rVector()
//===========================================================================
// Description: constructor
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
template <class T> inline rVector<T>::rVector() :
    m_Array(0),
    m_Size(0),
    m_Capacity(0),
    m_GrowSize(10),
    m_Allocator(RADMEMORY_ALLOC_DEFAULT)
{
}

//===========================================================================
// rVector::~rVector()
//===========================================================================
// Description: destructor
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
template <class T> inline rVector<T>::~rVector()
{
    this->ClearAndDestroy();
}

//===========================================================================
// rVector::operator[]
//===========================================================================
// Description: [] operator
//
// Constraints:    NONE
//
// Parameters:    location
//
// Return:      const reference to the object at that index
//
//===========================================================================
template <class T> inline const T& rVector<T>::operator[](long location) const
{
    //rAssert( location < mSize );

    return m_Array[location];
}

//===========================================================================
// rVector::operator[]
//===========================================================================
// Description: [] operator
//
// Constraints:    NONE
//
// Parameters:    location
//
// Return:      reference to the object at that index
//
//===========================================================================
template <class T> inline T& rVector<T>::operator[](long location)
{
    //rAssert( location < mSize );

    return m_Array[location];
}

//===========================================================================
// rVector::Clear
//===========================================================================
// Description: ???
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
template <class T> inline void rVector<T>::Clear()
{
    ClearAndDestroy();
//    m_Size = 0;
//    rAssert( m_Capacity > 0 );
//
//    for( int i = 0; i < m_Capacity; i++ )
//    {
//        m_Array[ i ] = NULL;
//    }
//    m_Size = 0;
}

//===========================================================================
// rVector::ClearAndDestroy
//===========================================================================
// Description: Deletes the array
//
// Constraints:    NONE
//
// Parameters:    NONE
//
// Return:      NONE
//
//===========================================================================
template <class T> inline void rVector<T>::ClearAndDestroy()
{
    //rAssert( m_Capacity > 0 );

    if( m_Array != NULL )
    {
        delete [] m_Array;
        m_Array = NULL;
    }

    m_Size = 0;
    m_Capacity = 0;
}

template <class T> inline long rVector<T>::Capacity() const
{
    return m_Capacity;
}

template <class T> inline bool rVector<T>::Empty() const
{
    return (m_Size == 0);
}

template <class T> inline void rVector<T>::Erase(T& item)
{
    int i;
    for (i=0; i<m_Size; i++)
    {
        if (item == m_Array[i]) break;
    }

    //rAssert( i == m_Size );

    this->Erase(i);
}

template <class T> inline void rVector<T>::Erase(long location)
{
    //rAssert( location < m_Size );

    for (int i=location; i<m_Size-1; i++)
    {
        m_Array[i] = m_Array[i+1];
    }
    m_Size--;
}

template <class T> inline void rVector<T>::Insert(T& item)
{
    this->Insert(item,m_Size);
}

template <class T> inline void rVector<T>::Insert(T& item, long location)
{
    //rAssert( (m_Size + 1) > m_Capacity );

    if ( (m_Size + 1) > m_Capacity )
    {
        this->Reserve(m_Size + m_GrowSize);
    }

    for (int i=m_Size; i>location; i--)
    {
        m_Array[i+1] = m_Array[i];
    }

    m_Array[location] = item;
    m_Size++;
}

template <class T> inline void rVector<T>::Reserve(long number)
{
    // Check for first insert.  We assume m_Capacity never shrinks.
    //
    if (m_Capacity == 0)
    {
        m_Allocator = ::radMemoryGetCurrentAllocator ();
    }

    T* newArray;

    radMemoryAllocator old = ::radMemorySetCurrentAllocator (m_Allocator);
    newArray = new T[number];
    ::radMemorySetCurrentAllocator (old);

    //rAssert( m_Size < number );

    if (m_Size > number) m_Size = number;

    for(int i=0; i<m_Size; i++)
    {
        newArray[i] = m_Array[i];
    }

    if( m_Array != NULL )
    {
        delete[] m_Array;
    }

    m_Array = newArray;
    m_Capacity = number;
}

template <class T> inline long rVector<T>::Size() const
{
    return m_Size;
}

template <class T> inline long rVector<T>::GrowSize() const
{
    return m_GrowSize;
}

template <class T> inline void rVector<T>::SetGrowSize(long size)
{
    m_GrowSize = size;
}

#endif //R_VECTOR_HPP
