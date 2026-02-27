//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        socket.hpp
//
// Subsystem:	Radical Debug Communication System
//
// Description:	This file contains the an abstract interface to sockets.
//				We use this interface so we can hide the underlying socket
//				implementation. Sometimes we use real sockets, othertimes
//				emulation.
//
// Author:		Peter Mielcarski
//
// Revisions:	V1.00	Mar 30, 2001
//
//=============================================================================

#ifndef	SOCKET_HPP
#define SOCKET_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radmemory.hpp>

//=============================================================================
// Defintions
//=============================================================================

//=============================================================================
// Interface
//=============================================================================

#if defined( SN_TCPIP ) || defined( RAD_WIN32 ) || defined( RAD_UWP )
//
// Game cubes socket is a pure abstract interface. The other platfroms should
// be as well and this should be cleaned up.
//
struct radSocket 
{
    virtual int socket( int af, int type, int protocol)
    {
        return( ::socket( af, type, protocol ) );
    }
    virtual int closesocket( int sock )
    {
        return( ::closesocket( sock ) );
    }
    virtual int setsockopt( int sock, int level, int optname, const char* optval, int optlen)
    {
        return( ::setsockopt( sock, level, optname, optval, optlen ) );
    }
    virtual int accept( int sock, struct sockaddr* addr, int* addrlen)
    {
        return( ::accept( sock, addr, addrlen) );
    }
    virtual int bind( int sock, struct sockaddr* addr, int addrlen)
    {
        return( ::bind( sock, addr, addrlen ) );
    }
    virtual int connect( int sock, const struct sockaddr* addr, int addrlen)
    {
        return( ::connect( sock, addr, addrlen ) );
    }
    virtual int listen( int sock, int backlog)
    {
        return( ::listen( sock, backlog) );
    }
    virtual int recv( int sock, char* buf, int len, int flags)
    {
        return( ::recv( sock, buf, len, flags) );
    }
    virtual int send( int sock, const char* buf, int len, int flags)
    {
        return( ::send( sock, buf, len, flags) );
    }
    virtual int lasterror( int sock )
    {
        #if defined( RAD_WIN32 ) || defined( RAD_UWP )
        return( WSAGetLastError( ) );
        #endif
    }
    virtual ~radSocket( void ) 
    {
        
    } 
    #if defined( RAD_WIN32 ) || defined( RAD_UWP )
    virtual int ioctlsocket( SOCKET s, long cmd, u_long *argp )
    {
        return( ::ioctlsocket( s,  cmd,  argp ) );
    }
    virtual int AsyncSelect( SOCKET s, HWND Wnd, unsigned int wMsg, long levent )
    {
        return( ::WSAAsyncSelect( s, Wnd, wMsg, levent ) );
    }
    #endif // RAD_WIN32 || RAD_UWP 

    //
    // This placement operator is used by the host side only
    //
    void* operator new( size_t size, void* p )
    {
        return( p );
    }    

    void* operator new( size_t size, radMemoryAllocator alloc )
    {
        return( radMemoryAlloc( alloc, size ) );
    }    

    void operator delete( void * pMemory )
    {
	    radMemoryFree( pMemory );
    }

};
#endif

#endif // SOCKET_HPP