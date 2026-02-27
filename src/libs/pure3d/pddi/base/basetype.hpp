/*===========================================================================
    basecontext.hpp
    10-Oct-99 Created by Neall

    Copyright (c)1999  Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _BASETYPE_HPP
#define _BASETYPE_HPP

#include <pddi/base/debug.hpp>

template <class T> class pddiStack
{
public:
    pddiStack(int d)     { stack = new T[d]; top = 0; depth = d; }
    virtual ~pddiStack() { delete[] stack; }
    T* Top()             { return &stack[top]; }
    void Push()          { PDDIASSERT(top < (depth - 1)); top++; stack[top] = stack[top-1]; }
    void Pop()           { PDDIASSERT(top > 0); top--; }

protected:
    int depth;
    int top;
    T* stack;
};

#endif /* _BASETYPE_HPP */

