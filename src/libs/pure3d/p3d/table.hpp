/*===========================================================================
    table.hpp
    23-Dec-99 Created by Neall
    28-Jan-02 generisized

    Copyright (c)1999, 2000, 2001, 2002 Radical Entertainment, Ltd.
    All rights reserved.
===========================================================================*/

#ifndef _TABLE_HPP
#define _TABLE_HPP

#include <radload/utility/hashtable.hpp>

template <class T> class tTable : public RefHashTable<T>
{
};

#endif /* _TABLE_HPP */
