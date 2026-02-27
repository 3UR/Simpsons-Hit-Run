//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include "pch.hpp"
#include "radsoundupdatableobject.hpp"

bool radSoundUpdatableObject::s_Updating = false;
bool radSoundUpdatableObject::s_ListWasChanged  = false;
radSoundUpdatableObject * radSoundUpdatableObject::s_pRadSoundUpdateHead = NULL;
unsigned int             radSoundUpdatableObject::s_UpdatableCount  = 0;
unsigned int              radSoundUpdatableObject::s_UpdatingCount = 0;
