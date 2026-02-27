/*===========================================================================
    entitytable.hpp
    28-Jan-02 Created by Neall

    Copyright (c) 2002 Radical Entertainment, Ltd.
    All rights reserved.
===========================================================================*/

#ifndef _ENTITYTABLE_HPP
#define _ENTITYTABLE_HPP

#include "p3d/table.hpp"
#include "p3d/entity.hpp"
#include <radload/utility/inventory.hpp>

class tEntityTable : public radLoadHashedStore
{
public:
    tEntityTable() : m_Name( "" )
    {
        //
    }

    void SetName(const char* name)
    {
        m_Name = name;
    }

    tName GetName()
    {
        return m_Name;
    }

    tUID GetUID()
    {
        return m_Name.GetUID();
    }

private:
    tName m_Name;
};

#endif /* _ENTITYTABLE_HPP */
