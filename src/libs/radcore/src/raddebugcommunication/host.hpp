//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        host.Hpp
//
// Subsystem:	Radical Debug Communication
//
// Description:	This file contains all definitions and classes relevant to
//              the debug communicatio Host implementation.
//
// Revisions:	Mar 14, 2001 Creation
//
// Notes:       This component is only available under Windows 
//
//=============================================================================

#ifndef HOST_HPP
#define HOST_HPP

#ifdef RAD_WIN32

//=============================================================================
// Include Files
//=============================================================================

#include <windows.h>
#include <raddebugcommunication.hpp>

//=============================================================================
// Forward Class Declarations
//=============================================================================

class rDbgComHostChannel;

//=============================================================================
// Defintions
//=============================================================================


//=============================================================================
// Class Declarations
//=============================================================================

//
// This class implements the debug host interface. We only support a singleton
// instance of this object.
//
class rDbgComHost : public IRadDbgComTargetTable
{
    public:

    //
    // Contructor. Nothing too interesting.
    //
    rDbgComHost( void );

    //
    // Member functions implemented for the bIDebugHost interface.
    //
    virtual bool AddTargetDefinition( const radDbgComTargetName pName, 
                                      const radDbgComIPAddress pIpAddress,
                                      unsigned short port );

    virtual bool DeleteTargetDefinition( const radDbgComTargetName pName );
  
    virtual void EnumerateTargets( void* context, void (*pEnumerationCallback)( void* context, const radDbgComTargetName pName,
                                   const radDbgComIPAddress pIpAddress, unsigned short port, bool InUse ) );
 
    virtual bool SetDefaultTarget( const radDbgComTargetName pName );
    virtual bool GetDefaultTarget( radDbgComTargetName pName );
 
    //
    // Member functions implemented for bIRefCount
    //
    virtual void AddRef( void );
    virtual void Release( void );

    virtual void CreateChannel( const radDbgComTargetName pName, unsigned short protocol, 
                                IRadDbgComChannel** ppHostChannel );

    //
    // This member is used by the client host channels to construct a window
    // for them.
    //
    HWND CreateWindowHelper( rDbgComHostChannel* pChannel );
    void DestroyWindowHelper( HWND hWnd );

    //
    // Members used to retrieve the IP address and the port.
    //
    const char* GetIpAddress( unsigned int targetIndex );
    unsigned short GetPort( unsigned int targetIndex );

    //
    // Member used to release a protocol.
    //
    void FreeProtocol( unsigned int targetIndex, unsigned short protocol );

    void Service( void );

    private:
    
    //
    // Destruction done through release.
    //
    ~rDbgComHost( void );
 
    //
    // This static is used to field windows messages.
    //
    static LRESULT CALLBACK WindowProcedure( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

    //
    // This member maintains the reference count of this object.
    //
    unsigned int m_ReferenceCount;    

    //
    // This holds the mutex we use to serialize access to the target information table.
    // 
    HANDLE m_MutexHandle;
    
    //
    // Holds handle to shared memory object containing target information table.
    //
    HANDLE m_MemoryFileHandle;
    
    //
    // Holds the address of the shared memory image.
    //
    struct TargetInformation* m_TargetTable;

    //
    // Holds the module handle for this application.
    //
    HINSTANCE   m_hInstance;

    HWND        m_hWnd;

};

#endif
#endif


