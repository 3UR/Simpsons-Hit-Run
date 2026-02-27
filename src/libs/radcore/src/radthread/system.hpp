//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        system.hpp
//
// Subsystem:	Radical Threading Services - System Services
//
// Description:	This file contains all definitions and classes relevant to
//              the initialization and management of the threading system.
//
// Revisions:	January 7, 2002  PGM    Creation
//
// Notes:       
//=============================================================================

#ifndef	SYSTEM_HPP
#define SYSTEM_HPP

//=============================================================================
// Include Files
//=============================================================================

//=============================================================================
// Defintions
//=============================================================================

//
// To manage our threading objects in a thread safe manner we need to perform
// exclusion operations. These two functions provide platform independant
// ways of doing this. 
//
void radThreadInternalLock( void );
void radThreadInternalUnlock( void );

#endif


