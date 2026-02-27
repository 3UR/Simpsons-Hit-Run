#ifndef __Singleton__
#define __Singleton__

#include <raddebug.hpp>

#ifndef NULL
#define NULL 0
#endif

template <class TYPE>
class Singleton
{
public:
    virtual ~Singleton();
    static TYPE* Instance();

protected:
    Singleton();

private:
    static TYPE* sInstance;
};

template <class TYPE>
inline Singleton<TYPE>::Singleton() 
{
    rAssert( sInstance == NULL );
    sInstance = (TYPE*)this;
}

template <class TYPE>
inline Singleton<TYPE>::~Singleton()
{
//    P3TASSERT(sInstance != NULL);
    rAssert( sInstance != NULL );
    sInstance = NULL;
}

template <class TYPE>
inline TYPE* Singleton<TYPE>::Instance()
{
    rAssert( sInstance != NULL );
    return sInstance;
}

// use this definition in the Type.cpp file
#define InitSingleton(TYPE) TYPE *Singleton<TYPE>::sInstance = NULL

#endif // __Singleton__
