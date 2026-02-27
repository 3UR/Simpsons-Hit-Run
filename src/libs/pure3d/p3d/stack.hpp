//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _P3D_STACK_HPP_
#define _P3D_STACK_HPP_

#include <p3d/error.hpp>

// a simple templatized stack 
template <class T> class tStack
{
public:
    tStack(unsigned s) : size(s), top(0), buffer(new T[s])
    {
    }

    ~tStack()
    {
        delete [] buffer;
    }

    bool IsEmpty(void) const 
    { 
        return top == 0; 
    }

    void Push(T t) 
    { 
        P3DASSERT(top < size);
        buffer[top] = t;
        top++;
    }

    T Pop(void)
    {
        P3DASSERT(top != 0);
        top--;
        return buffer[top];
    }

protected:
    unsigned size;
    unsigned top;
    T* buffer;
};

#endif


