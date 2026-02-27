//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        functionlist.cpp
//
// Subsystem:   Radical Remote Command Server
//
// Description:	This file contains the implementation of the FunctionList.
//
// Revisions:	06-Feb-2001    Creation
//
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <string.h>
#include <radmemorymonitor.hpp>
#include "remotecommand.hpp"

//=============================================================================
// Public Member Functions
//=============================================================================


//=============================================================================
// Function:    FunctionList::FunctionList
//=============================================================================
// Description: Constructor for the Function list
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------
FunctionList::FunctionList( radMemoryAllocator alloc )
	:
	radRefCount( 0 ),
	m_Head( NULL ),
	m_FunctionCount( 0 ),
	m_Current( NULL ),
    m_Allocator( alloc )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "FunctionList" );
}

//=============================================================================
// Function:    FunctionList::~FunctionList
//=============================================================================
// Description: Destructor for the function list
//              
// Returns:     N/A
//
// Notes:
//------------------------------------------------------------------------------
FunctionList::~FunctionList( void )
{
	//
	// Should not hang on to any unwanted references
	//
	Kill();
}

//=============================================================================
// Function:    FunctionList::Add
//=============================================================================
// Description: Add function to list
//              
// Parameters:  char*			functionName    - name of function
//				RemoteFunction	rfptr           - pointer to function
//				void*			userData        - some unknown user data
//
// Notes:
//------------------------------------------------------------------------------
void FunctionList::Add( const char* functionName, RemoteFunction rfptr, void* userData )
{
	rAssert( (functionName != "") && (functionName[0] != '\0') );
	rAssert( rfptr != NULL );
	//
	// Will need a new node
	FunctionNode* fNode = (FunctionNode*) radMemoryAlloc( m_Allocator, sizeof( FunctionNode ) );
	//
	// Enter the new data
	strcpy( fNode->m_FunctionName, functionName );
	fNode->m_Function = rfptr;
	fNode->m_userData = userData;
	fNode->m_Next = NULL;
	if( m_Head == NULL )
	{
		//
		// First time
		m_Head = fNode;
	}
	else
	{
		//
		// Not first time
		fNode->m_Next = m_Head;
		m_Head = fNode;
	}
	//
	// Keep track of how many
	++m_FunctionCount;
}	

//=============================================================================
// Function:    FunctionList::Remote
//=============================================================================
// Description: Remove function from the list
//              
// Parameters: char*			functionName    - name of function
//
// Notes:
//------------------------------------------------------------------------------
bool FunctionList::Remove( const char* functionName )
{
	//
	// helping pointers
	FunctionNode* current = m_Head;
	FunctionNode* prev = NULL;
	//
	// Try to find the requested functionNode
	while( current != NULL )
	{
		if( strcmp( current->m_FunctionName, functionName ) == 0 )
		{
			// We found the right one
			break;
		}
		//
		// No luck.  Move to the next node
		prev = current;
		current = current->m_Next;
	}
	//
	// If we went throught the whole list and found nothing or there
	// was nothing in the list to begin with. End with false
	if( current == NULL )
	{
		return false;
	}
	//
	// If node to be removed is at the head of the list we must move the head point
	// to the next element
	if( current == m_Head )
	{
		m_Head = m_Head->m_Next;
	}
	//
	// Otherwise we pull the current node out of the list
	else
	{
		prev->m_Next = current->m_Next;
	}
	// 
	// We kill the current node
	radMemoryFree( current );
	//
	// We decrement the function count
	--m_FunctionCount;
	return true;
}

//=============================================================================
// Function:    FunctionList::Get
//=============================================================================
// Description: Get function pointer from the list
//              
// Parameters:  char*			functionName    - name of function		 (IN)
//				RemoteFunction	rfptr           - pointer to function	 (OUT)
//				void*			userData        - some unknown user data (OUT)
//
// Notes:
//------------------------------------------------------------------------------
void FunctionList::Get( char* functionName, RemoteFunction* prfptr, void** userData )
{
	//
	// Helping pointer
	FunctionNode* current = m_Head;
	//
	// Go through the list to find the right node
	while( current != NULL )
	{
		if( strcmp( current->m_FunctionName, functionName ) == 0 )
		{
			//
			// We've got it.  Set the outward bound parameters and stop looking.
			*prfptr = current->m_Function;
			*userData = current->m_userData;
			return;
		}
		//
		// Try the next one
		current = current->m_Next;
	}
	//
	// Nothing of that description could be found
 	//
	*prfptr = NULL;
	*userData = NULL;
	return;
}

//=============================================================================
// Function:    FunctionList::GetNextName
//=============================================================================
// Description: Returns name of next function in list
//              
// Returns:     char*		- pointer to the name string
//
// Notes:       
//------------------------------------------------------------------------------
const char* FunctionList::GetNextName( void )
{
	//
	// If m_Current is pointing at something return a pointer to the name and return it
	if( m_Current != NULL )
	{
		char* name = m_Current->m_FunctionName;
		m_Current = m_Current->m_Next;
		return name;
	}

	//
	// Nothing to return.
	return "";
}

//=============================================================================
// Function:    FunctionList::ResetNextName
//=============================================================================
// Description: Resets the next name to the head of the list
//
// Notes:       
//------------------------------------------------------------------------------
void FunctionList::ResetNextName( void )
{
	//
	// Move the m_Current pointer to the head of the list
	m_Current = m_Head;
}

//=============================================================================
// Function:    FunctionList::Kill
//=============================================================================
// Description: Kills everything in the list
//
// Notes:       
//------------------------------------------------------------------------------
void FunctionList::Kill( void )
{
	// 
	// Kill everything in the list
	//
	FunctionNode* current = m_Head;
	while( current != NULL )
	{
		current = m_Head->m_Next;
		radMemoryFree( m_Head );
		m_Head = current;
	}
}

//=============================================================================
// Function:    FunctionList::NumFunctions
//=============================================================================
// Description: Returns the number of functions in the list
//
// Returns:		unsigned int  - the number of functions in the list
//
// Notes:       
//------------------------------------------------------------------------------
unsigned int FunctionList::NumFunctions( void )
{
	// 
	// Return the number of functions in the list
	//
	return m_FunctionCount;
}

