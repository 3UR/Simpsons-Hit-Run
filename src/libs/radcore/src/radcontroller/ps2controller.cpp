//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        PS2Controller.cpp
//
// Subsystem:	Foundation Technologies - PS2 Controller System
//
// Description:	This file contains the implementation of the Foundation
//              Technologies controller system. This module provides services
//				for enumerating and managing PS2 controllers and their functions.
//
// Date:    	November 14, 2000
//
// Author:      Michael Wu
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include <math.h>
#include <radmemorymonitor.hpp>
#include "ps2controller.hpp"

//============================================================================
// Function:    radControllerInitialize
// Paramters:   pConnectionChangeCallback - a callback that is called when the
//                  connection status of controllers changes.  It is called
//                  as soon as a controller is found and properly initialized.
//============================================================================

void radControllerInitialize
(
    IRadControllerConnectionChangeCallback* pConnectionChangeCallback,
    radMemoryAllocator alloc
)
{
    rAssert( g_theCS == NULL );

    g_CtlrAllocator = alloc;        
    new ( alloc ) rControllerSystem2( pConnectionChangeCallback );
}

void radControllerTerminate( void )
{
    //
    // Release the controller system
    //
    radRelease( g_theCS, NULL );
}


//============================================================================
// Function: radControllerSystemGet
//============================================================================
//
// Use this function to obtain an interface to the controller system object.
//
IRadControllerSystem* radControllerSystemGet
( 
    void
)
{
    rAssert( g_theCS != NULL );

    return( g_theCS );

}


//============================================================================
// Function: radControllerSystemServer
//============================================================================
//
// Use this function to drive the processing of the controller system
//
void radControllerSystemService( void )
{
    if( g_theCS != NULL )
    {
        g_theCS->Service( );
    }
}


//=============================================================================
// rControllerSystem2::rControllerSystem2
//=============================================================================

rControllerSystem2::rControllerSystem2( IRadControllerConnectionChangeCallback* pConnectionChangeCallback )
    :
    m_ReferenceCount( 1 ),
    m_StatusCallback( NULL )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rControllerSystem2" );

    g_theCS = this;

    for( int i=0; i<CL_MAXIMUM_SLOTS; i++ )
    {
        m_LocationTable[ i ] = NULL;
    }

    for ( int i=0; i<CL_MAX_WHEELS; i++ )
    {
        m_WheelDeviceDesc[ i ].desc.DevClass = 0;
        m_WheelDeviceDesc[ i ].desc.AdditionalControls = 0;
        m_WheelDeviceDesc[ i ].desc.Flags = 0;
        m_WheelDeviceDesc[ i ].desc.Topology[0] = 0x0;
        m_WheelDeviceDesc[ i ].desc.ExistingHandle = LG_INVALID_DEVICE;
        m_WheelDeviceDesc[ i ].controller = NULL;
    }

    //
    // Setup controller list for enumeration.
    //
    ::radObjectListCreate( & m_xIOl_Controllers , g_CtlrAllocator);

    //
    // Create a timer list to drive the processing of contollers
    //

    radTimeCreateList( &m_xITimerList, 1, g_CtlrAllocator );

	//
    // Create work timer. This must be done after the buffering system is set up
    // since we run right away.
	//
    m_xITimerList->CreateTimer( &m_xIWorkTimer, 16, this );

    rAssert( m_xIWorkTimer != NULL );

    m_VirtualTimeAdjust = 0;
    m_UsingVirtualTime = false;
    m_VirtualTime = 0;

    //
    // Initial buffering size is ( 16 ms polling / 16ms per frame) + 1
    //
    m_BufferingTime = 16;

	//
    // Initialize the SIF for loading the IOP modules we need.
	//
    sceSifInitRpc(0);

    //
    // Load IRX files.
    //
	ref< IRadPlatform > xIPs2Platform;
    xIPs2Platform = radPlatformGet( );
	rAssert( xIPs2Platform != NULL );

	if ( xIPs2Platform != NULL )
	{
		xIPs2Platform->LoadIrxModule( "sio2man.irx" );
		
		#ifdef RADCORE_PS2EE_MTAP
			xIPs2Platform->LoadIrxModule( "mtapman.irx" );
		#endif // RADCORE_PS2EE_MTAP

		xIPs2Platform->LoadIrxModule( "padman.irx" );

        //Load the steering wheel IRX
        xIPs2Platform->LoadIrxModule( "usbd.irx" );  //Need this too.
        xIPs2Platform->LoadIrxModule( "lgdevW.irx" );
	}

	//
    // Initialize the mtap library
	//
	#ifdef RADCORE_PS2EE_MTAP
		sceMtapInit();
	#endif

	//
    // Initialize the pad library
	//
    scePadInit( 0 );

    //Init the steeringwheel libs.
    int result = lgDevInit();
    rAssert( result == LG_SUCCESS );

    //
    // Register the connection change callback
    //
    if( pConnectionChangeCallback )
    {
        RegisterConnectionChangeCallback( pConnectionChangeCallback );
    }

    //
    // Initialize locations to prepare them for controllers.
    //
    InitializeLocations();

    //
    // Find all attached devices and start their initialization.
    // Notice that controllers will become ready asynchronously.
    //
    SetupControllers( );

    //
    // If there is no connection change callback, wait synchronously for the connection
    //
    rWarningMsg( pConnectionChangeCallback != NULL, "Spinning while initializing controllers synchronously." );
    if( pConnectionChangeCallback == NULL )
    {
        bool done = true;
        int s = 0;
        do
        {
            done = true;
            for( s = 0; s < CL_MAXIMUM_SLOTSONPORT; s++ )
            {
                if( m_LocationTable[ s ] != NULL )
                {
                    if
                    (
                        m_LocationTable[ s ]->GetControllerState( ) != iControllerLocation2::CL_STATE_DISCONNECTED &&
                        m_LocationTable[ s ]->GetControllerState( ) != iControllerLocation2::CL_STATE_READY
                    )
                    {
                        done = false;
                    }
                }
            }
            Service( );
        } while( !done );
    }
}

//=============================================================================
// rControllerSystem2::~rControllerSystem2
//=============================================================================

rControllerSystem2::~rControllerSystem2( void )
{
    if( m_StatusCallback != NULL )
    {
        UnRegisterConnectionChangeCallback( m_StatusCallback );
    }

    m_xIWorkTimer = NULL;

    m_xIOl_Controllers = NULL;
    m_xIWorkTimer = NULL;
    m_xITimerList = NULL;    

    for( int i=0; i<CL_MAXIMUM_SLOTS; i++ )
    {
        if( m_LocationTable[ i ] != NULL )
        {
            radRelease( m_LocationTable[ i ], this );
            m_LocationTable[ i ] = NULL;
        }
    }

    g_theCS = NULL;
}

//=============================================================================
// rControllerSystem2::Service
//=============================================================================

void rControllerSystem2::Service( void )
{
    m_xITimerList->Service(  );   
}

//=============================================================================
// rControllerSystem2::AddRef
//=============================================================================

void rControllerSystem2::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// rControllerSystem2::Release
//=============================================================================

void rControllerSystem2::Release( void )
{
    m_ReferenceCount--;

    if( m_ReferenceCount == 0 )
    {
        delete this;
    }
}

//=============================================================================
// rControllerSystem2::GetNumberOfControllers
//=============================================================================

unsigned int rControllerSystem2::GetNumberOfControllers( void )
{
    return m_xIOl_Controllers->GetSize( );
}

//=============================================================================
// rControllerSystem2::GetControllerByIndex
//=============================================================================

IRadController * rControllerSystem2::GetControllerByIndex( unsigned int indx )
{
    return reinterpret_cast< IRadController* >( m_xIOl_Controllers->GetAt( indx ) );
}

//=============================================================================
// rControllerSystem2::GetControllerAtLocation
//=============================================================================

IRadController * rControllerSystem2::GetControllerAtLocation( const char* pLocation )
{
    IRadController * pIRadController = NULL;

    //Use "USB0" or "USB1" for the wheels.  We only support two wheels regardless of 
    //the number of hubs and shit.
    if ( strncmp( "USB", pLocation, 3 ) == 0 )
    {
        //This is a wheel they're looking for.  I hope.
        if ( m_xIOl_Controllers != NULL )
        {
            m_xIOl_Controllers->Reset( );

            IRadController* pIRadController;

            while ( NULL != (pIRadController = reinterpret_cast<IRadController*>( m_xIOl_Controllers->GetNext( ) ) ) )
            {
                if ( strcmp( pIRadController->GetLocation(), pLocation ) == 0 )
                {
                    return pIRadController;
                }
            }
        }
        else
        {
            return NULL;
        }
    }

    bool found = false;
    int i = 0;
    for( i = 0; i < CL_MAXIMUM_SLOTS; i++ )
    {
        if( m_LocationTable[ i ] == NULL )
        {
            continue;
        }
        if( strcmp( pLocation, m_LocationTable[ i ]->GetName( ) ) == 0 )
        {
            found = true;
            break;
        }
    }
    if( found )
    {
		rController2* controller;
	    m_LocationTable[ i ]->GetAttachedController( &controller );

		if( controller != NULL ) 
		{
			pIRadController = static_cast< IRadController* >( controller );
            radRelease( controller, this );
		}
	}

    return pIRadController;
}

//=============================================================================
// rControllerSystem2::SetBufferTime
//=============================================================================

void rControllerSystem2::SetBufferTime( unsigned int milliseconds )
{
    if ( milliseconds == 0 )
    {
        //
        // We are always buffering behind the scenes, so we set the
        // buffering time to one 60Hz frame even if the client thinks
        // we are just maintaining state.
        //

        m_UsingVirtualTime = false;

        milliseconds = 16;
    }
    else
    {
        m_UsingVirtualTime = true;
    }

    //
    // Go through the list of controllers and reset the buffering size.
    //

    unsigned int pollingRate = 16;

    if ( m_xIWorkTimer != NULL )
    {
        pollingRate = m_xIWorkTimer->GetTimeout( );
    }

    m_BufferingTime = milliseconds;

    if ( m_xIOl_Controllers != NULL )
    {
        m_xIOl_Controllers->Reset( );

        iIController2 * pIController2;

        while ( NULL != ( pIController2 = reinterpret_cast< iIController2 * >( m_xIOl_Controllers->GetNext( ) ) ) )
        {
            pIController2->iSetBufferTime( ( milliseconds / pollingRate ) + 2 );
        }
    }
}

//=============================================================================
// rControllerSystem2::MapVirtualTime
//=============================================================================

void rControllerSystem2::MapVirtualTime( unsigned int timerManagerMs, unsigned int virtualTicks )
{
    if( m_xIWorkTimer != NULL )
    {
        m_VirtualTimeAdjust = virtualTicks - timerManagerMs;
    }
}

//=============================================================================
// rControllerSystem2::SetVirtualTime
//=============================================================================

void rControllerSystem2::SetVirtualTime( unsigned int virtualTicks )
{
    rAssertMsg( m_UsingVirtualTime == true, "Can't set virtual time if buffering 0 ms" );

    m_VirtualTime = virtualTicks;

    if ( m_xIOl_Controllers != NULL )
    {
        m_xIOl_Controllers->Reset( );

        iIController2 * pIController2;

        while ( NULL != ( pIController2 = reinterpret_cast< iIController2 * >( m_xIOl_Controllers->GetNext( ) ) ) )
        {
            pIController2->iVirtualTimeChanged( );
        }
    }
}

//=============================================================================
// rControllerSystem2::SetCaptureRate
//=============================================================================

void rControllerSystem2::SetCaptureRate( unsigned int timerManagerMs )
{
    if( m_xIWorkTimer != NULL )
    {
        m_xIWorkTimer->SetTimeout( timerManagerMs );
    }
}

//=============================================================================
// rControllerSystem2::RegisterConnectionChangeCallback2
//=============================================================================

void rControllerSystem2::RegisterConnectionChangeCallback( IRadControllerConnectionChangeCallback * pCallback )
{
    if( m_StatusCallback == NULL )
    {
        m_StatusCallback = pCallback;
    }
    else
    {
        rDebugString( "Multiple callbacks not currently supported" );        
    }
}

//=============================================================================
// rControllerSystem2::UnRegisterConnectionChangeCallback2
//=============================================================================

void rControllerSystem2::UnRegisterConnectionChangeCallback( IRadControllerConnectionChangeCallback * pCallback )
{
	rAssert( m_StatusCallback == pCallback );

    if ( m_StatusCallback == pCallback )
    {
        m_StatusCallback = NULL;
    }
}

//=============================================================================
// rControllerSystem2::OnTimerDone
//=============================================================================

void rControllerSystem2::OnTimerDone( unsigned int elapsedtime, void * pUserData )
{
    bool anyUSBChange = false;
    int lgEnumHint = 0;

    bool haveWheels = false;
    int i = 0;
    for( i = 0; i < CL_MAX_WHEELS; ++i )
    {
        if ( m_WheelDeviceDesc[ i ].desc.ExistingHandle != LG_INVALID_DEVICE )
        {
            haveWheels = true;
            break;
        }
    }

    //Reset for stuff below.
    i = 0;

    //Let's test the USB for wheels
    //First we setup the Async call to test the state of the USB
    //We're sneaking this in here because we wanna let these async just ride out.
    if ( !haveWheels )
    {
        // since we have no device open, we don't perform
        // any polls/reads on it. Instead we're queuing an
        // asynchronous call to retrieve a hint regarding newly
        // plugged devices.
        lgDevAEnumHint(&lgEnumHint);
    }
    else
    {
        // we use the synchronous call to retrieve the cached
        // hint of the lgDevRead() performed during the last
        // game loop iteration.
        lgDevEnumHint(&lgEnumHint);   
    }

    //Now we do other stuff.  At the end of this for loop, we'll check the status of the
    //Async calls.

    //
	// Go through each location and check the states of them to determine
    // if there is a connection change.
	//
	for( int p=0; p<CL_MAXIMUM_PORTS; p++ )
    {
		for( int s=0; s<CL_MAXIMUM_SLOTSONPORT; s++ )
        {
			if( m_LocationTable[ i ] != NULL )
			{
				int state = scePadGetState( m_LocationTable[ i ]->GetPort( ), m_LocationTable[ i ]->GetSlot( ) );

				//
                // There are two basic states, connected and disconnected.
                // For us, we do not want to hear about a controller unless
                // it is fully connected
				//
                if ( state == scePadStateDiscon )
				{
					rController2* controller = NULL;
					m_LocationTable[ i ]->GetAttachedController( &controller );
    				if( controller != NULL )
					{
						//
                        // Controller was connected.
						//
                        if ( controller->IsConnected( ) )
						{
							m_LocationTable[ i ]->SetControllerState( iControllerLocation2::CL_STATE_DISCONNECTED );

							//
                            // Don't delete the controller, just disconnect it.
                            // This way, we can reconnect that controller object if 
                            // the user plugs in a controller (of the same type) 
                            // in the same place.
							//
                            controller->Disconnect();
							
                            if ( m_StatusCallback != NULL )
							{
								IRadController* pController = static_cast< IRadController* >( controller );
								m_StatusCallback->OnControllerConnectionStatusChange( pController );
							}
						}
                        radRelease( controller, this );
					}
				}
				else if ( state == scePadStateStable || state == scePadStateFindCTP1 )
				{
					//
                    // Is the location object not created yet? If not, create it now.
					//
                    if( m_LocationTable[ i ] == NULL )
					{
						m_LocationTable[ i ] = new( g_CtlrAllocator ) iControllerLocation2( p, s );
					}

                    //
                    // Make sure the id of the attached controller is the same as the 
                    // id we are expecting.  This won't be the case if the controller
                    // was unplugged and plugged back in really quickly (e.g. a 
                    // CL_ID_DUALSHOCK2 will change to a CL_ID_DIGITAL
                    //

                    rController2* controller = NULL;
					m_LocationTable[ i ]->GetAttachedController( &controller );
    				if( controller != NULL )
					{
                        // Get the id of the actually attached controller using scePadRead

		                unsigned char buffer[32];
		                int len = scePadRead( m_LocationTable[ i ]->GetPort( ), m_LocationTable[ i ]->GetSlot( ), buffer );

                        // if the id is wrong, change the state to disconnected so it can be reconnected

                        if( len != 0 && buffer[ 1 ] != controller->GetId( ) )
                        {
                            // This code is copied above

							m_LocationTable[ i ]->SetControllerState( iControllerLocation2::CL_STATE_DISCONNECTED );

							//
                            // Don't delete the controller, just disconnect it.
                            // This way, we can reconnect that controller object if 
                            // the user plugs in a controller (of the same type) 
                            // in the same place.
							//
                            controller->Disconnect();
							
                            if ( m_StatusCallback != NULL )
							{
								IRadController* pController = static_cast< IRadController* >( controller );
								m_StatusCallback->OnControllerConnectionStatusChange( pController );
							}
                        }
                        radRelease( controller, this );
                    }

					//
                    // If this controller is not constructed yet OR if it not connected,
                    // we need to initialize whatever IS connected and see if it's either:
                    // something new, or an old controller.
					//
					// Controller IS connected
					if ( m_LocationTable[ i ]->GetControllerState( ) == iControllerLocation2::CL_STATE_DISCONNECTED )
					{
						m_LocationTable[ i ]->SetControllerState( iControllerLocation2::CL_STATE_STARTINIT );
					}
					//
                    // Check what connection state the controller is in for initialization.
					//
                    switch ( m_LocationTable[ i ]->GetControllerState( ) )
					{
						case iControllerLocation2::CL_STATE_STARTINIT:
						{
							_StartControllerInitialization( m_LocationTable[ i ], state );
							break;
						}

						case iControllerLocation2::CL_STATE_REQUESTMODE:
						{
							_RequestControllerMode( m_LocationTable[ i ] );
							break;
						}

						case iControllerLocation2::CL_STATE_REQUESTMODEDONE:
						{
							_RequestControllerModeDone( m_LocationTable[ i ], state );
							break;
						}

						case iControllerLocation2::CL_STATE_REQUESTACTUATOR:
						{
							_RequestControllerActuator( m_LocationTable[ i ] );
							break;
						}

						case iControllerLocation2::CL_STATE_REQUESTACTUATORDONE:
						{
							_RequestControllerActuatorDone( m_LocationTable[ i ], state );
							break;
						}

						case iControllerLocation2::CL_STATE_REQUESTPRESSURE:
						{
							_RequestControllerPressure( m_LocationTable[ i ] );
							break;
						}

						case iControllerLocation2::CL_STATE_REQUESTPRESSUREDONE:
						{
							_RequestControllerPressureDone( m_LocationTable[ i ], state );
							break;
						}

						case iControllerLocation2::CL_STATE_WAITASYNC:
						{
							_CheckControllerAsyncProcessing( m_LocationTable[ i ] );
							break;
						}

						case iControllerLocation2::CL_STATE_ALMOSTREADY:
						{
							_ControllerAlmostReady( m_LocationTable[ i ], state, elapsedtime );
							break;
						}

						case iControllerLocation2::CL_STATE_READY:
						{
							_ControllerReady( m_LocationTable[ i ], state, elapsedtime );
							break;
						}
                        
                        case iControllerLocation2::CL_STATE_DISCONNECTED:
                        {
                            break;
                        }
					}
				}
			}
			i++;
		}
	}

    //Hey!  Let's see what the Wheel async calls did.
    int asyncReturn; //We'll ignore this.
    lgDevASync(LGASYNC_MODE_WAIT, &asyncReturn);

    switch( lgEnumHint )
    {
    case LGHINT_ENUMNEEDED:
        {
            //Well, someone changed something.
            //Clean the checked flags on all the current devices.
            int deviceIndex;
            for ( deviceIndex = 0; deviceIndex < CL_MAX_WHEELS; ++deviceIndex )
            {
                m_WheelDeviceDesc[ deviceIndex ].desc.Flags &= ~CL_DEVICE_FLAG_CHECKED; //Clear the checked flag
            }
            
               
            int result = LG_SUCCESS;
            lgDevDeviceDesc deviceDesc;
            lgDevDeviceDesc newDevices[CL_MAX_CONNECTED_WHEELS];  //Arbitrary..  Let's only save 8.
            int savedDeviceCount = 0;

            //Test all devices and see what we get.
            for ( deviceIndex = 0; result != static_cast<int>(LGERR_NO_MORE_DEVICES) && 
                                   result != static_cast<int>(LG_ERROR); ++deviceIndex )
            {
                result = lgDevEnumerate( deviceIndex, &deviceDesc );
                switch( result )
                {
                case LGERR_DEVICE_LOST:
                    {
                        //This is supposed to be rare...  Try again!
                        deviceIndex = 0;
                        break;
                    }
                case LG_SUCCESS:
                    {
                        //Let's look at the device, if we have it, mark it checked.
                        //If we don't have it, save the device info.
                        int wheelIndex;
                        bool found = false;
                        for ( wheelIndex = 0; wheelIndex < CL_MAX_WHEELS; ++wheelIndex )
                        {
                            if ( m_WheelDeviceDesc[wheelIndex].desc.ExistingHandle != LG_INVALID_DEVICE &&
                                 memcmp( &deviceDesc.Topology, &(m_WheelDeviceDesc[wheelIndex].desc.Topology), 8 ) == 0 )
                            {
                                //we've got this one.
                                //Mark this one checked.
                                m_WheelDeviceDesc[wheelIndex].desc.Flags |= CL_DEVICE_FLAG_CHECKED;
                                found = true;
                            }
                        }

                        if ( !found )
                        {
                            //This is a new device.
                            memcpy( &newDevices[savedDeviceCount], &deviceDesc, sizeof(lgDevDeviceDesc) );
                            newDevices[savedDeviceCount].ExistingHandle = deviceIndex;  //This is to allow for lgDevOpen
                            savedDeviceCount++;
                        }
                        break;
                    }
                case LGERR_NO_MORE_DEVICES:
                    {
                        break;
                    }
                default:
                    {
                        rAssertMsg(false, "Problem polling the USB!");
                        break;
                    }
                }
            }

            //Test all the connected wheels for their checked flag.
            for ( deviceIndex = 0; deviceIndex < CL_MAX_WHEELS; ++deviceIndex )
            {
                if ( m_WheelDeviceDesc[ deviceIndex ].desc.ExistingHandle != LG_INVALID_DEVICE &&
                     !(m_WheelDeviceDesc[ deviceIndex ].desc.Flags & CL_DEVICE_FLAG_CHECKED) )
                {
                    //Remove this wheel.
                    char location[64];
                    sprintf( location, "USB%d", m_WheelDeviceDesc[ deviceIndex ].desc.Topology[0] - 0x01 );

                    IRadController* xIController = GetControllerAtLocation( location );
                    rAssert( xIController );

                    //Close the wheel device
                    int result = lgDevClose( m_WheelDeviceDesc[ deviceIndex ].desc.ExistingHandle );
                    m_WheelDeviceDesc[ deviceIndex ].desc.ExistingHandle = LG_INVALID_DEVICE;
                    rAssert( result == LG_SUCCESS );

                    //Get rid of the controller.
                    reinterpret_cast<iIController2*>(xIController)->Disconnect();
                    m_xIOl_Controllers->RemoveObject( xIController );  

                    if ( m_StatusCallback != NULL )
                    {
                        IRadController* pController = NULL;
                        pController = static_cast< IRadController* >( m_WheelDeviceDesc[ deviceIndex ].controller );
                        m_StatusCallback->OnControllerConnectionStatusChange( pController );
                    }
                    
                    m_WheelDeviceDesc[ deviceIndex ].controller = NULL;
                }
                else if ( m_WheelDeviceDesc[ deviceIndex ].desc.ExistingHandle != LG_INVALID_DEVICE )
                {
                    //Let's poll the controller.  Yay!
                    rAssert( m_WheelDeviceDesc[ deviceIndex ].controller != NULL );

                    m_WheelDeviceDesc[ deviceIndex ].controller->iPoll( radTimeGetMilliseconds( ) );
                    if ( !m_UsingVirtualTime )
                    {
                        m_WheelDeviceDesc[ deviceIndex ].controller->iVirtualTimeChanged( );
                    }
                }

                //Now let's fill it back up!
                if ( m_WheelDeviceDesc[ deviceIndex ].desc.ExistingHandle == LG_INVALID_DEVICE )
                {
                    //Pick one of the newDevices and use it.
                    for ( i = 0; i < savedDeviceCount; ++i )
                    {
                        if ( newDevices[i].ExistingHandle != LG_INVALID_DEVICE )
                        {
                            int newDeviceHandle = LG_INVALID_DEVICE;
                            //Let's try to open a new wheel.
                            result = lgDevOpen( newDevices[i].ExistingHandle, &newDeviceHandle );
                            if ( result == LG_SUCCESS )
                            {
                                //Shut off the default centering spring.
                                result = lgDevSetDeviceProperty(newDeviceHandle, LGPROP_CENTERINGENABLED, LG_FALSE);
                                rAssert( result == LG_SUCCESS );

                                //Copy the new deviceHandle to the newDevice.
                                newDevices[i].ExistingHandle = newDeviceHandle;

                                //Cool, make a new controller and store it in the buffer.
                                m_WheelDeviceDesc[ deviceIndex ].controller = new(g_CtlrAllocator) LGDevWheelController( &newDevices[i] );
                                rAssert( m_WheelDeviceDesc[ deviceIndex ].controller );

                                rDebugPrintf( "Found a new wheel on port: %d\n", newDevices[i].Topology[0] - 0x01 );

                                m_xIOl_Controllers->AddObject( m_WheelDeviceDesc[ deviceIndex ].controller );

                                //Copy this to our list of official wheels.
                                memcpy( &m_WheelDeviceDesc[ deviceIndex ].desc, &newDevices[i], sizeof(lgDevDeviceDesc) );
                                
                                //Invalidate for next search
                                newDevices[i].ExistingHandle = LG_INVALID_DEVICE;

                                if ( m_StatusCallback != NULL )
                                {
                                    IRadController* pController = NULL;
                                    pController = static_cast< IRadController* >( m_WheelDeviceDesc[ deviceIndex ].controller );
                                    m_StatusCallback->OnControllerConnectionStatusChange( pController );
                                }
                                break;
                            }
                        }
                    }                    
                }
            } 
            break;
        }
    case LGHINT_NOENUMNEEDED:
    case LGHINT_IKNOWNUTTIN:
    default:
        {
            for ( i = 0; i < CL_MAX_WHEELS; ++i )
            {
                if ( m_WheelDeviceDesc[ i ].desc.ExistingHandle != LG_INVALID_DEVICE )
                {
                    //Let's poll the controller.  Yay!
                    rAssert( m_WheelDeviceDesc[ i ].controller != NULL );

                    m_WheelDeviceDesc[ i ].controller->iPoll( radTimeGetMilliseconds( ) );
                    if ( !m_UsingVirtualTime )
                    {
                        m_WheelDeviceDesc[ i ].controller->iVirtualTimeChanged( );
                    }
                }
            }
            break;
        }
    }
}

//=============================================================================
// rControllerSystem2::InitializeLocations
//=============================================================================

void rControllerSystem2::InitializeLocations( void )
{
    for( int port = 0; port<CL_MAXIMUM_PORTS; port++ )
    {
	    //
        // Determine the number of slots on each port.
	    //

		#ifdef RADCORE_PS2EE_MTAP
			sceMtapPortOpen( port );
			sceMtapGetConnection( port );
		#endif

        int nSlots = CL_MAXIMUM_SLOTSONPORT;

        for( int slot = 0; slot<nSlots; slot++ )
        {
	        int ret = scePadPortOpen( port, slot, g_ControllerPadDMABuffer[ port * CL_MAXIMUM_SLOTSONPORT + slot ].m_PadDMABuffer );

            m_LocationTable[ port * CL_MAXIMUM_SLOTSONPORT + slot ] = new ( g_CtlrAllocator ) iControllerLocation2( port, slot );
        }
    }
}

//=============================================================================
// rControllerSystem2::CreateController
//=============================================================================

void rControllerSystem2::CreateController( int id, iControllerLocation2* location, rController2** pprController )
{
    IRadString* locationName = NULL;
    location->GetNameBString( &locationName );

    unsigned int pollingRate = 16;

    if ( m_xIWorkTimer != NULL )
    {
        pollingRate = m_xIWorkTimer->GetTimeout( );
    }

	*pprController = new ( g_CtlrAllocator ) rController2
        ( 
            location->GetPort(), 
            location->GetSlot(), 
            id, 
            locationName, 
            location->GetActAlignBuffer(), 
            location->GetActDirectBuffer(), 
            location->GetNumActuators(),
            ( m_BufferingTime / pollingRate ) + 2
        );

    radRelease( locationName, this );

	location->AttachController( *pprController );
    m_xIOl_Controllers->AddObject( reinterpret_cast< IRefCount* >( *pprController ) );
}

//=============================================================================
// rControllerSystem2::_StartControllerInitialization
//=============================================================================

void rControllerSystem2::_StartControllerInitialization( iControllerLocation2* location, int state )
{
	//
    // Start the initialization procedure.
    //
	
	//
    // Is the Controller not stable yet?
	//
    if ( state != scePadStateStable && state != scePadStateFindCTP1 )
	{
        return;
	}

	int p = location->GetPort( );
	int s = location->GetSlot( );

	int id = scePadInfoMode( p, s, InfoModeCurID, 0 );
	if ( id == 0 )
	{
		return;
	}

    //
	// Set the total number of modes for the controller.
    //
    location->SetTotalNumModes( scePadInfoMode( p, s, InfoModeIdTable, -1 ) );

	if ( location->GetTotalNumModes( ) == 0 )
	{
		//
        // Controller has no modes.
		//
        location->SetTotalNumModes( 1 );
		location->SetCurrentMode( 0 );
		location->SetModeQueryResult( scePadInfoMode( p, s, InfoModeCurID, 0 ),
									  location->GetCurrentMode( ) );
		location->SetControllerState( iControllerLocation2::CL_STATE_ALMOSTREADY );
	}
	else
	{
		//
        // Enumerate the available ids for the controller.
		//
        for ( int curid=0; curid<location->GetTotalNumModes( ); curid++ )
		{
			location->SetModeQueryResult( scePadInfoMode( p, s, InfoModeIdTable, curid ),
										  curid );
		}
		location->SetControllerState( iControllerLocation2::CL_STATE_REQUESTMODE );
	}
}

//=============================================================================
// rControllerSystem2::_RequestControllerMode
//=============================================================================

void rControllerSystem2::_RequestControllerMode( iControllerLocation2* location )
{
	int p = location->GetPort( );
	int s = location->GetSlot( );

	//
    // Select the mode we want. The modes start at 0, so we want maxmode-1 (last mode).
	//
    int desiredMode = location->GetTotalNumModes( ) - 1;

	//
    // Request for mode.
	//
    if ( scePadSetMainMode( p, s, desiredMode, 3 ) == 1 )
	{
		//
        // Go into asynchronous wait mode while waiting for our mode request.
		//
        location->SetCurrentMode( desiredMode );
		location->SetControllerPrevState( location->GetControllerState( ) );
		location->SetControllerState( iControllerLocation2::CL_STATE_WAITASYNC );
	}
	else
	{
		location->SetTotalNumModes( 1 );
		location->SetCurrentMode( 0 );
		location->SetModeQueryResult( scePadInfoMode( p, s, InfoModeCurID, 0 ),
									  location->GetCurrentMode( ) );
		location->SetControllerState( iControllerLocation2::CL_STATE_ALMOSTREADY );
	}
}

//=============================================================================
// rControllerSystem2::_RequestControllerModeDone
//=============================================================================

void rControllerSystem2::_RequestControllerModeDone( iControllerLocation2* location, int state )
{
	//
    // Are we waiting for mode request to end?
	//
    if ( state == scePadStateExecCmd )
    {
        return;
    }

    //
    // Mode successfully entered. Now try to get actuator to work.
    //
    location->SetControllerState( iControllerLocation2::CL_STATE_REQUESTACTUATOR );
}

//=============================================================================
// rControllerSystem2::_RequestControllerActuator
//=============================================================================


void rControllerSystem2::_RequestControllerActuator( iControllerLocation2* location )
{
	int p = location->GetPort( );
	int s = location->GetSlot( );

    //
    // Determine the number of actuators ( output points ) are available.
    //
	int infoact = scePadInfoAct( p, s, -1, 0 );
	if ( infoact == 0 )
	{
		//
        // Actuator not available, now request to see if pressure mode available.
		//
        location->SetControllerState( iControllerLocation2::CL_STATE_REQUESTPRESSURE );
	}
	else
	{
		//
        // Actuator available.
        //		

        // printf( "Found %d actuators\n", infoact );


        location->SetNumActuators( infoact );
        
        int i;

		for ( i=0; i<infoact; i++ )
		{
			location->SetActAlign( i, i );
            location->SetActDirect( 0x00, i );
		}

		for ( i=infoact; i<CL_MAXIMUM_ACTUATORS; i++ )
		{
			location->SetActAlign( 0xFF, i );
            location->SetActDirect( 0xFF, i );
		}

		//
        // Request actuator.
		//
        if ( scePadSetActAlign( p, s, location->GetActAlignBuffer( ) ) )
		{
			//
            // Requesting actuator successful.
			//
            location->SetControllerPrevState( location->GetControllerState( ) );
			location->SetControllerState( iControllerLocation2::CL_STATE_WAITASYNC );
    	}
		else
		{
			//
            // Requesting actuator was unsuccessful, Do nothing to continue the request.
            //		
        }
	}
}

//=============================================================================
// rControllerSystem2::_RequestControllerActuatorDone
//=============================================================================

void rControllerSystem2::_RequestControllerActuatorDone( iControllerLocation2* location, int state )
{
	//
    // Are we waiting for actuator request to complete?
	//
	if ( state == scePadStateExecCmd )
	{
        return;
	}

	//
    // Actuators successfully enabled. Now try get pressure sensitivity.
	//
    location->SetControllerState( iControllerLocation2::CL_STATE_REQUESTPRESSURE );
}


//=============================================================================
// rControllerSystem2::_RequestControllerPressure
//=============================================================================

void rControllerSystem2::_RequestControllerPressure( iControllerLocation2* location )
{
	int p = location->GetPort( );
	int s = location->GetSlot( );

    //
	// Does pressure mode exist?
	//
    if ( scePadInfoPressMode( p, s ) == 1 )
	{
        //
		// Pressure mode exists. Now request to enter pressure mode.
        //
		if ( scePadEnterPressMode( p, s ) == 1 )
		{
            //
			// Requested pressure mode successfully
            //
			location->SetControllerPrevState( location->GetControllerState( ) );
			location->SetControllerState( iControllerLocation2::CL_STATE_WAITASYNC );
		}
		else
		{
            //
			// Request for pressure mode did not succeed, Do nothing to continue the request.
            //
		}
	}
	else
	{
		//PrintMessage( "No pressure mode", p, s );
		location->SetControllerState( iControllerLocation2::CL_STATE_ALMOSTREADY );
	}
}

//=============================================================================
// rControllerSystem2::_RequestControllerPressureDone
//=============================================================================

void rControllerSystem2::_RequestControllerPressureDone( iControllerLocation2* location, int state )
{
    //
    // Wait till controller is finished command.
    //
	if ( state == scePadStateExecCmd )
	{
        //
		// Waiting for pressure mode request to end.
		//
        return;
	}

    //
	// Pressure mode successfully entered. Having gained pressure sensitivity, 
	// start reading.
    //
	location->SetControllerState( iControllerLocation2::CL_STATE_ALMOSTREADY );
}

//=============================================================================
// rControllerSystem2::_CheckControllerAsyncProcessing
//=============================================================================

void rControllerSystem2::_CheckControllerAsyncProcessing( iControllerLocation2* location )
{
	int p = location->GetPort( );
	int s = location->GetSlot( );

	int request = scePadGetReqState( p, s );

	if ( request == scePadReqStateFailed )
	{
        //
		// Request failed.  Return to previous state and try again.
        //
		location->SetControllerState( location->GetControllerPrevState( ) );
		location->SetControllerPrevState( iControllerLocation2::CL_STATE_WAITASYNC );
	}
	else if ( request == scePadReqStateComplete )
	{
        //
		// Request completed.
		//
        switch ( location->GetControllerPrevState( ) )
		{
			case iControllerLocation2::CL_STATE_REQUESTMODE:
				location->SetControllerState( iControllerLocation2::CL_STATE_REQUESTMODEDONE );
				break;

			case iControllerLocation2::CL_STATE_REQUESTPRESSURE:
				location->SetControllerState( iControllerLocation2::CL_STATE_REQUESTPRESSUREDONE );
				break;

			case iControllerLocation2::CL_STATE_REQUESTACTUATOR:
				location->SetControllerState( iControllerLocation2::CL_STATE_REQUESTACTUATORDONE );
				break;

			default:
				break;
		}
	}
}

//=============================================================================
// rControllerSystem2::_ControllerAlmostReady
//=============================================================================

void rControllerSystem2::_ControllerAlmostReady( iControllerLocation2* location, int state, int ltime )
{
	int p = location->GetPort( );
	int s = location->GetSlot( );

	if ( state == scePadStateStable || state == scePadStateFindCTP1 )
	{
		unsigned char buffer[32];
		int len = scePadRead( p, s, buffer );

    	//
        // Could not read anything.
		//
        if ( len == 0 )
		{
			return;
		}

		//
        // Unsuccessful Read.
	    //
        if ( buffer[0] != 0 )
		{
			return;
		}

        //
        // We successfully read information from the controller, now we can
		// create the new controller.
		//
        rController2* controller = NULL;

        //
        // If we disconnected a controller, the controller object is not necessarily 
        // removed. It is kept around and should somebody reconnect a controller of
        // the same type, the controller object is "reused". This way, we can avoid
        // some unnecessary object cleanup and instatiation.
        //
        // We have to update the controller with the current buffering size since
        // the controller was removed from the enumeration list.
        //

        unsigned int pollingRate = 16;

        if ( m_xIWorkTimer != NULL )
        {
            pollingRate = m_xIWorkTimer->GetTimeout( );
        }

        bool bConnectOld = location->ReconnectIfControllerMatches( buffer[1], &controller, ( m_BufferingTime / pollingRate ) + 2 );

        //
        // If we couldn't connect the old controller object (because maybe a different
        // kind of controller is plugged in), create the new one.
        //
        if( ! bConnectOld )
        {
            //
            // We have to update our list of controllers if we had added this controller
            // before.
            //
            if( controller != NULL )
            {
                m_xIOl_Controllers->RemoveObject( reinterpret_cast< IRefCount* >( controller ) );
            }

            //
            // Create the new controller.
            //
    		CreateController( buffer[1], location, &controller );
        }		    
        //
        // This callback is NULL while we do our call to SetupControllers. Thus,
        // we do not get any callbacks when we first do our controller initialization,
        // which is the expected behavior.
        //
        if ( m_StatusCallback != NULL )
        {
			IRadController* pController = NULL;
			pController = static_cast< IRadController* >( controller );
			m_StatusCallback->OnControllerConnectionStatusChange( pController );
        }

        // Controller is fully setup and CL_STATE_READY to go.
		location->SetControllerState( iControllerLocation2::CL_STATE_READY );
    
        if( controller != NULL )
        {
            radRelease( controller, this );
        }
    }
}

//=============================================================================
// rControllerSystem2::_ControllerReady
//=============================================================================

void rControllerSystem2::_ControllerReady( iControllerLocation2* location, int state, int ltime )
{
	int p = location->GetPort( );
	int s = location->GetSlot( );

	if ( state == scePadStateStable || state == scePadStateFindCTP1 )
	{
		rController2* controller = NULL;
		location->GetAttachedController( &controller );
		if( controller != NULL && controller->IsConnected( ) )
		{
            //
            // Poll each existing controller.
            //
            unsigned char* buffer = controller->GetDataBuffer();
			int len = scePadRead( p, s, buffer );

	    	// 
            // Could not read anything.
    		//
            if ( len == 0 )
			{
				return;
			}

			//
            // Unsuccessful read.
			//
            if ( buffer[0] != 0 )
			{
				return;
			}

			//
            // Put the read data into the controller
            //
            controller->iPoll( radTimeGetMilliseconds( ) );

            if ( ! m_UsingVirtualTime )
            {
                controller->iVirtualTimeChanged( );
            }

            radRelease( controller, this );
		}
		else
		{
			rDebugString( "We could not get the controller at the specified location" );
		}
	}
	else
	{
        //
		// Controller is in ready state but not stable yet. So we do nothing 
        // until it is in a stable state.
        //
	}
}

//=============================================================================
// rControllerSystem2::SetupControllers
//=============================================================================

//
// Initialize the controllers that are plugged in.  Control
// returns immediately after finding all controllers and starting
// their initialization.  The controllers will not be ready at this
// time.
//
void rControllerSystem2::SetupControllers( void )
{
    int ltime = 0;

    //
    // Get the callers thread priority, so we can rotatate threads at this priority.
    //
    ThreadParam threadInfo;
    ReferThreadStatus( GetThreadId( ), &threadInfo );
     
    //
    // Go through each port to find all controllers
    //
    int i = 0;
    for ( int p=0; p<CL_MAXIMUM_PORTS; p++ )
    {
        int nSlots = scePadGetSlotMax( p );
        //
        // Go through each available slot on each port
        //
        for ( int s=0; s<nSlots; s++ )
        {
            if ( m_LocationTable[ i ] != NULL )
            {
                RotateThreadReadyQueue( threadInfo.currentPriority );
                
                //
                // Update the controller
                //
                int state = scePadGetState( p, s );
                
                //
                // There are two basic states, connected and disconnected
                //
                if ( state == scePadStateDiscon )
                {
                    //
                    // Controller IS NOT connected
                    //
                    rController2* controller = NULL;
                    m_LocationTable[ i ]->GetAttachedController( &controller );
                    
                    if( controller != NULL )
                    {
                        //
                        // Controller was connected.
                        //
                        if ( controller->IsConnected( ) )
                        {
                            m_LocationTable[ i ]->SetControllerState( iControllerLocation2::CL_STATE_DISCONNECTED );
                            
                            //
                            // Don't delete the controller, just disconnect it ( to reconnect 
                            // if the user plugs in a controller in the same place with the 
                            // same type )
                            //
                            controller->Disconnect();
                        }
                        radRelease( controller, this );
                    }
                }
                else
                {
                    //
                    // Controller IS connected
                    //
                    if ( m_LocationTable[ i ]->GetControllerState( ) == iControllerLocation2::CL_STATE_DISCONNECTED )
                    {
                        m_LocationTable[ i ]->SetControllerState( iControllerLocation2::CL_STATE_STARTINIT );
                    }
                }
            }
        }
    }
}

//=============================================================================
// rControllerSystem2::iGetVirtualTime
//=============================================================================

unsigned int rControllerSystem2::iGetVirtualTime( void )
{
    if( ! m_UsingVirtualTime )
    {
        m_VirtualTime = radTimeGetMilliseconds( );
    }

    return m_VirtualTime;
}

//=============================================================================
// rControllerSystem2::iGetVirtualTimeAdjust
//=============================================================================

unsigned rControllerSystem2::iGetVirtualTimeAdjust( void )
{
    if( m_UsingVirtualTime )
    {
        return m_VirtualTimeAdjust;
    }
    else
    {
        return 0;
    }
}
























//=============================================================================
// iControllerLocation2::iControllerLocation2
//=============================================================================

iControllerLocation2::iControllerLocation2( int port, int slot )
	:
    m_ReferenceCount( 1 ),
    m_Port( port ),
	m_Slot( slot ),
    m_rController( NULL ),
	m_LocationName( NULL ),
    m_nState( CL_STATE_DISCONNECTED ),
    m_nPreviousState( CL_STATE_DISCONNECTED ),
	m_nNumModes( -1 ),
    m_nMode( -1 )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "iControllerLocation2" );

    radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
    ::radStringCreate( & m_LocationName, RADMEMORY_ALLOC_TEMP );                            

    m_LocationName->Append( "Port" );
    m_LocationName->Append( port );
    m_LocationName->Append( "\\Slot" );
    m_LocationName->Append( slot );
    ::radMemorySetCurrentAllocator( old );

    for( int i = 0; i < CL_MAXIMUM_CONTROLLERMODES; i++ )
	{
		m_nModes[ i ] = -1; 
	}
}

//=============================================================================
// iControllerLocation2::~iControllerLocation2
//=============================================================================

iControllerLocation2::~iControllerLocation2( void )
{
    if( m_LocationName != NULL )
    {
        radRelease( m_LocationName, this );
    }
    if( m_rController != NULL )
    {
        radRelease( m_rController, this );
    }
}

//=============================================================================
// iControllerLocation2::AddRef
//=============================================================================

void iControllerLocation2::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// iControllerLocation2::Release
//=============================================================================

void iControllerLocation2::Release( void )
{
    m_ReferenceCount--;

    if( m_ReferenceCount == 0 )
    {
        delete this;
    }
}
 
//=============================================================================
// iControllerLocation2::GetPort
//=============================================================================

int iControllerLocation2::GetPort( void )
{
    return m_Port;
}

//=============================================================================
// iControllerLocation2::GetSlot
//=============================================================================

int iControllerLocation2::GetSlot( void )
{
    return m_Slot;
}

//=============================================================================
// iControllerLocation2::GetName
//=============================================================================

const char* iControllerLocation2::GetName( void )
{
    if ( m_LocationName != NULL )
    {
        return m_LocationName->GetChars( );
    }
    return "Unknown Location";
}

//=============================================================================
// iControllerLocation2::GetNameBString
//=============================================================================

void iControllerLocation2::GetNameBString( IRadString** locationName )
{
    if( locationName != NULL )
    {
        radAddRef( m_LocationName, this );
        *locationName = m_LocationName;
    }
}

//=============================================================================
// iControllerLocation2::AttachController
//=============================================================================

//
// Called by the system to attach the created controller object.
//
void iControllerLocation2::AttachController( rController2* controller )
{
	// Detach the previous Controller
	if( m_rController != NULL )
	{
		m_rController->Disconnect( );
        radRelease( m_rController, this );
	}

	controller->Connect( );
    radAddRef( controller, this );
    m_rController = controller;
}

//=============================================================================
// iControllerLocation2::GetAttachedController
//=============================================================================

void iControllerLocation2::GetAttachedController( rController2** controller )
{
    if( m_rController != NULL )
    {
        if( m_rController->IsConnected( ) )
        {
            radAddRef( m_rController, this );
            *controller = m_rController;
            return;
        }
    }
    *controller = NULL;
}

//=============================================================================
// iControllerLocation2::SetControllerState
//=============================================================================

void iControllerLocation2::SetControllerState( LocationState state )
{
	m_nState = state;
}

//=============================================================================
// iControllerLocation2::GetControllerState
//=============================================================================

iControllerLocation2::LocationState iControllerLocation2::GetControllerState( void )
{
	return m_nState;
}

//=============================================================================
// iControllerLocation2::SetControllerPrevState
//=============================================================================

void iControllerLocation2::SetControllerPrevState( LocationState state )
{
	m_nPreviousState = state;
}

//=============================================================================
// iControllerLocation2::GetControllerPrevState
//=============================================================================

iControllerLocation2::LocationState iControllerLocation2::GetControllerPrevState( void )
{
	return m_nPreviousState;
}

//=============================================================================
// iControllerLocation2::SetTotalNumModes
//=============================================================================

void iControllerLocation2::SetTotalNumModes( int numModes )
{
	m_nNumModes = numModes;
}

//=============================================================================
// iControllerLocation2::GetTotalNumModes
//=============================================================================

int iControllerLocation2::GetTotalNumModes( void )
{
	return m_nNumModes;
}

//=============================================================================
// iControllerLocation2::SetCurrentMode
//=============================================================================

void iControllerLocation2::SetCurrentMode( int mode )
{
	m_nMode = mode;
}

//=============================================================================
// iControllerLocation2::GetCurrentMode
//=============================================================================

int iControllerLocation2::GetCurrentMode( void )
{
	return m_nMode;
}

//=============================================================================
// iControllerLocation2::SetModeQueryResult
//=============================================================================

void iControllerLocation2::SetModeQueryResult( int modeValue, int modeIndex )
{
	if( modeIndex < CL_MAXIMUM_CONTROLLERMODES )
	{
		m_nModes[ modeIndex ] = modeValue;
	}
}

//=============================================================================
// iControllerLocation2::GetModeQueryResult
//=============================================================================

int iControllerLocation2::GetModeQueryResult( int modeIndex )
{
	rAssert( modeIndex < CL_MAXIMUM_CONTROLLERMODES );
	return m_nModes[ modeIndex ];
}

//=============================================================================
// iControllerLocation2::SetActAlign
//=============================================================================

void iControllerLocation2::SetActAlign( int actValue, int lindex )
{
	rAssert( lindex < CL_MAXIMUM_ACTUATORS );
	m_cActuatorAlignment[ lindex ] = actValue;
}

//=============================================================================
// iControllerLocation2::SetActAlign
//=============================================================================

void iControllerLocation2::SetActDirect( int actValue, int lindex )
{
	rAssert( lindex < CL_MAXIMUM_ACTUATORS );
	m_cActuatorDirective[ lindex ] = actValue;
}

//=============================================================================
// iControllerLocation2::GetActAlignBuffer
//=============================================================================

unsigned char* iControllerLocation2::GetActAlignBuffer( void )
{
	return m_cActuatorAlignment;
}

//=============================================================================
// iControllerLocation2::GetActDirectBuffer
//=============================================================================

unsigned char* iControllerLocation2::GetActDirectBuffer( void )
{
	return m_cActuatorDirective;
}

//=============================================================================
// iControllerLocation2::ReconnectIfControllerMatches
//=============================================================================

bool iControllerLocation2::ReconnectIfControllerMatches( int id, rController2** controller, unsigned int bufferSize )
{
    if( m_rController != NULL )
    {
        if( m_rController->IsConnected( ) )
        {
            return true;
        }
        
        if( id == m_rController->GetId( ) )
        {
            radAddRef( m_rController, this );
            m_rController->Connect( );
            m_rController->iSetBufferTime( bufferSize );
            *controller = m_rController;
            return true;
        }

    }
    return false;
}

//=============================================================================
// iControllerLocation2::GetNumActuators
//=============================================================================

int iControllerLocation2::GetNumActuators( void )
{
    return m_NumActuators;
}

//=============================================================================
// iControllerLocation2::SetNumActuators
//=============================================================================

void iControllerLocation2::SetNumActuators( int numActuators )
{
    m_NumActuators = numActuators;
}

//=============================================================================
// rController2::rController2
//=============================================================================

rController2::rController2( int port, int slot, int id, IRadString* locationName, unsigned char* pActAlignBuffer, unsigned char* pActDirectBuffer, int numActuators, unsigned int bufferSize )
    :
    m_ReferenceCount( 1 ),
    m_Port( port ),
    m_Slot( slot ),
    m_Id( id ),
    m_Connected( false ),
    m_TypeId( id ),
    m_NumActuators( numActuators )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rController2" );

    unsigned int i = 0;

    if( pActAlignBuffer != NULL )
    {
        for( i = 0; i < CL_MAXIMUM_ACTUATORS; i++ )
        {
            m_cActuatorAlignment[ i ] = pActAlignBuffer[ i ];
        }
    }

    m_pOutputHandler = new( g_CtlrAllocator ) iControllerOutputHandler2
    ( 
        port, 
        slot, 
        pActDirectBuffer 
    );

    if( locationName != NULL )
    {
        radAddRef( locationName, this );
	    m_LocationName = locationName;
    }
    for( i=0; i<32; i++ )
    {
        m_DataBuffer[ i ] = 0;
    }

    //
    // Setup the input points list for enumeration.
    //
    ::radObjectListCreate( & m_xIOl_InputPoints, g_CtlrAllocator );
    rAssert( m_xIOl_InputPoints != NULL );

    //
    // Setup the output points list for enumeration.
    //
    ::radObjectListCreate( & m_xIOl_OutputPoints, g_CtlrAllocator );
    rAssert( m_xIOl_OutputPoints != NULL );

    if( m_xIOl_InputPoints  != NULL && m_xIOl_OutputPoints != NULL )
    {
        //
        // Create the predefined input points for this controller.
        //
        switch ( id )
        {
            case CL_ID_DIGITAL:
                for( i=0; i<14; i++ )
                {
                    //
                    // Create input point and add it to our list.
                    // 
                    IRadString* pName = NULL;
                    ::radStringCreate( & pName, g_CtlrAllocator);                            
                    rAssert( pName != NULL );
                    pName->Append( PsxDigitalInputPoints[ i ] );

                    IRadControllerInputPoint* ip = new( g_CtlrAllocator ) rControllerInputPoint2
                    ( 
                        reinterpret_cast< IRefCount* >( this ),
                        pName,
                        // button type index
                        PsxDigitalInputPointAttributes[ 0 ][ i ], 
                        // data offset into buffer, we include some space for the timestamp
                        sizeof( unsigned int ) + PsxDigitalInputPointAttributes[ 1 ][ i ],
                        // the mask that is applied to the offset byte in order to determine the value
                        PsxDigitalInputPointAttributes[ 2 ][ i ]
                    );

                    m_xIOl_InputPoints->AddObject( reinterpret_cast< IRefCount* >( ip ) );

                    radRelease( ip, this );
                    radRelease( pName, this );
                }
                break;

	        case CL_ID_DUALSHOCK:
                for( i=0; i<20; i++ )
                {
                    //
                    // Create input point and add it to our list.
                    // 
                    IRadString* pName = NULL;
                    ::radStringCreate( & pName, g_CtlrAllocator);                            
                    rAssert( pName != NULL );
                    pName->Append( PsxDualShockInputPoints[ i ] );

                    IRadControllerInputPoint* ip = new( g_CtlrAllocator ) rControllerInputPoint2
                    ( 
                        reinterpret_cast< IRefCount* >( this ),
                        pName,
                        PsxDualShockInputPointAttributes[ 0 ][ i ],
                        sizeof( unsigned int ) + PsxDualShockInputPointAttributes[ 1 ][ i ],
                        PsxDualShockInputPointAttributes[ 2 ][ i ]
                    );

                    m_xIOl_InputPoints->AddObject( reinterpret_cast< IRefCount* >( ip ) );

                    radRelease( ip, this );
                    radRelease( pName, this );
                }
                for( i=0; i<2; i++ )
                {
                    //
                    // Create output point and add it to our list.
                    // 
                    IRadString* pName = NULL;
                    ::radStringCreate( & pName, g_CtlrAllocator);                            
                    rAssert( pName != NULL );
                    pName->Append( PsxDualShockOutputPoints[ i ] );

                    IRadString* pType = NULL;
                    ::radStringCreate( & pType, g_CtlrAllocator);                            
                    rAssert( pType != NULL );
                    pType->Append( PsxDualShockOutputPointAttributes[ i ] );

                    IRadControllerOutputPoint* op = new( g_CtlrAllocator ) rControllerOutputPoint2
                    (
                        m_pOutputHandler,
                        pName,
                        pType
                    );

                    m_xIOl_OutputPoints->AddObject( reinterpret_cast< IRefCount* >( op ) );
                    
                    radRelease( op, this );
                    radRelease( pName, this );
                    radRelease( pType, this );
                }
			    scePadSetActAlign( m_Port, m_Slot, m_cActuatorAlignment );

                break;
       
	        case CL_ID_DUALSHOCK2:
                for( i=0; i<32; i++ )
                {
                    //
                    // Create input point and add it to our list.
                    // 
                    IRadString* pName = NULL;
                    ::radStringCreate( & pName, g_CtlrAllocator);                            
                    rAssert( pName != NULL );
                    pName->Append( PsxDualShock2InputPoints[ i ] );

                    IRadControllerInputPoint* ip = new( g_CtlrAllocator ) rControllerInputPoint2
                    ( 
                        reinterpret_cast< IRefCount* >( this ),
                        pName,
                        PsxDualShock2InputPointAttributes[ 0 ][ i ],
                        sizeof( unsigned int ) + PsxDualShock2InputPointAttributes[ 1 ][ i ],
                        PsxDualShock2InputPointAttributes[ 2 ][ i ]
                    );

                    m_xIOl_InputPoints->AddObject( reinterpret_cast< IRefCount* >( ip ) );

                    radRelease( ip, this );
                    radRelease( pName, this );
                }
                for( i=0; i<2; i++ )
                {
                    //
                    // Create output point and add it to our list. The DualShock2 output
                    // points are the same as the DualShock's.
                    // 
                    IRadString* pName = NULL;
                    ::radStringCreate( & pName, g_CtlrAllocator);                            
                    rAssert( pName != NULL );
                    pName->Append( PsxDualShockOutputPoints[ i ] );

                    IRadString* pType = NULL;
                    ::radStringCreate( & pType, g_CtlrAllocator);                            
                    rAssert( pType != NULL );
                    pType->Append( PsxDualShockOutputPointAttributes[ i ] );

                    IRadControllerOutputPoint* op = new( g_CtlrAllocator ) rControllerOutputPoint2
                    (
                        m_pOutputHandler,
                        pName,
                        pType                        
                    );

                    m_xIOl_OutputPoints->AddObject( reinterpret_cast< IRefCount* >( op ) );
                    
                    radRelease( op, this );
                    radRelease( pName, this );
                    radRelease( pType, this );
                }

			    scePadSetActAlign( m_Port, m_Slot, m_cActuatorAlignment );

                break;

            default:
                rDebugString( "Unrecognized Controller" );
                break;
        }

		// Initialize the input points with their current values

		int state = scePadGetState( m_Port, m_Slot );
		unsigned char sonyBuffer[32];

		int len = scePadRead( m_Port, m_Slot, (unsigned char*) & sonyBuffer );

		if( len > 0 )
		{
            unsigned char radBuffer[ sizeof(unsigned int) + 32 ];

			// Fill the buffer with a zero time value & and the controller values

			unsigned int zero = 0;
			memcpy( (void*)radBuffer, (void*) & zero, sizeof( unsigned int ) );
			memcpy( (void*)(radBuffer + sizeof( unsigned int )), & sonyBuffer, sizeof(unsigned int) + 32 );

			rAssert( m_xIOl_InputPoints != NULL );

            if ( m_xIOl_InputPoints != NULL )
            {
                m_xIOl_InputPoints->Reset( );

                iIControllerInputPoint2 * pIControllerInputPoint2;

                while ( NULL != ( pIControllerInputPoint2 = reinterpret_cast< iIControllerInputPoint2 * >( m_xIOl_InputPoints->GetNext( ) ) ) )
                {
                    pIControllerInputPoint2->iInitialize( (unsigned char*) & radBuffer );                                   
                }
            }
		}
		else
		{
			rDebugPrintf( "WARNING: Controller couldn't initialize with actual values\n" );
		}
    }

    //
    // Get a controller buffer to store controller states.
    //
    radControllerBufferCreate( &m_pIControllerBuffer2, g_CtlrAllocator );
    rAssert( m_pIControllerBuffer2 != NULL );
    m_pIControllerBuffer2->Initialize( bufferSize, 32 );
}

//=============================================================================
// rController2::~rController2
//=============================================================================

rController2::~rController2( void )
{
    m_pOutputHandler->Disconnect( );
    radRelease( m_pOutputHandler, this );
    m_xIOl_InputPoints = NULL;
    m_xIOl_OutputPoints = NULL;
    radRelease( m_LocationName, this );
    radRelease( m_pIControllerBuffer2, this );
}

//=============================================================================
// rController2::AddRef
//=============================================================================

void rController2::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// rController2::Release
//=============================================================================

void rController2::Release( void )
{
    m_ReferenceCount--;

    if( m_ReferenceCount == 0 )
    {
        delete this;
    }
}

//=============================================================================
// rController2::IsConnected
//=============================================================================

bool rController2::IsConnected( void )
{
    return m_Connected;
}

//=============================================================================
// rController2::GetClassification
//=============================================================================

const char* rController2::GetClassification( void )
{
    switch ( m_TypeId )
	{
		case CL_ID_DUALSHOCK2:
		case CL_ID_DUALSHOCK:
        case CL_ID_DIGITAL:
	        return "Joystick";
    
        default:
            return "Unknown";
    }
}

//=============================================================================
// rController2::GetType
//=============================================================================

const char* rController2::GetType( void )
{
    switch ( m_TypeId )
	{
        case CL_ID_DIGITAL:
            return "PsxDigital";

		case CL_ID_DUALSHOCK:
			return "PsxDualShock";
           
		case CL_ID_DUALSHOCK2:
            return "PsxDualShock2";

        default:
            return "Unknown";
    }
}

//=============================================================================
// rController2::GetNumberOfInputPointsOfType
//=============================================================================

unsigned int rController2::GetNumberOfInputPointsOfType( const char * pType )
{
    unsigned int count = 0;

    if ( m_xIOl_InputPoints != NULL )
    {
        m_xIOl_InputPoints->Reset( );

        IRadControllerInputPoint * pIRadControllerInputPoin;

        while ( NULL != ( pIRadControllerInputPoin = reinterpret_cast< IRadControllerInputPoint * >( m_xIOl_InputPoints->GetNext( ) ) ) )
        {
            if ( strcmp( pType, pIRadControllerInputPoin->GetType( ) ) == 0 )
            {
                count++;
            }
        }
    }

    return count;
}

//=============================================================================
// rController2::GetNumberOfOutputPointsOfType
//=============================================================================

unsigned int rController2::GetNumberOfOutputPointsOfType( const char * pType )
{
    unsigned int count = 0;

    if ( m_xIOl_OutputPoints != NULL )
    {
        m_xIOl_OutputPoints->Reset( );

        IRadControllerOutputPoint * pIRadControllerInputPoint;

        while ( NULL != ( pIRadControllerInputPoint = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) ) )
        {
            if ( strcmp( pType, pIRadControllerInputPoint->GetType( ) ) == 0 )
            {
                count++;
            }
        }
    }

    return count;
}

//=============================================================================
// rController2::GetInputPointByTypeAndIndex
//=============================================================================

IRadControllerInputPoint * rController2::GetInputPointByTypeAndIndex
(
    const char * pType,
    unsigned int lindex
)
{

    if ( m_xIOl_InputPoints != NULL )
    {
        unsigned int count = 0;

        IRadControllerInputPoint * pIRadControllerInputPoint;

        m_xIOl_InputPoints->Reset( );

        while ( NULL != ( pIRadControllerInputPoint = reinterpret_cast< IRadControllerInputPoint * >( m_xIOl_InputPoints->GetNext(  ) ) ) )
        {
            if ( strcmp( pIRadControllerInputPoint->GetType( ), pType ) == 0 )
            {
                if ( count == lindex )
                {
                    return pIRadControllerInputPoint;
                }

                count++;
            }
        }
    }

    return NULL;
}

//=============================================================================
// rController2::GetOutputPointByTypeAndIndex
//=============================================================================

IRadControllerOutputPoint * rController2::GetOutputPointByTypeAndIndex
(
    const char * pType,
    unsigned int lindex
)
{
    if ( m_xIOl_OutputPoints != NULL )
    {
        unsigned int count = 0;

        IRadControllerOutputPoint * pIRadControllerOutputPoint;

        m_xIOl_OutputPoints->Reset( );

        while ( NULL != ( pIRadControllerOutputPoint = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) ) )
        {
            if ( strcmp( pIRadControllerOutputPoint->GetType( ), pType ) == 0 )
            {
                if ( count == lindex )
                {
                    return pIRadControllerOutputPoint;
                }

                count++;
            }
        }
    }

    return NULL;
}

//=============================================================================
// rController2::GetInputPointByName
//=============================================================================

IRadControllerInputPoint * rController2::GetInputPointByName( const char * pName )
{
    if ( m_xIOl_InputPoints != NULL )
    {
        m_xIOl_InputPoints->Reset( );

        IRadControllerInputPoint * pIRadControllerInputPoint;

        while ( NULL != ( pIRadControllerInputPoint = reinterpret_cast< IRadControllerInputPoint * >( m_xIOl_InputPoints->GetNext( ) ) ) )
        {
            if ( strcmp( pIRadControllerInputPoint->GetName(), pName ) == 0 )
            {
                return pIRadControllerInputPoint;
            }
        }            
    }

    return NULL;
}

//=============================================================================
// rController2::GetOutputPointByName
//=============================================================================

IRadControllerOutputPoint * rController2::GetOutputPointByName( const char * pName )
{
    if ( m_xIOl_OutputPoints != NULL )
    {
        m_xIOl_OutputPoints->Reset( );

        IRadControllerOutputPoint * pIRadControllerOutputPoint;

        while ( NULL != ( pIRadControllerOutputPoint = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) ) )
        {
            if ( strcmp( pIRadControllerOutputPoint->GetName(), pName ) == 0 )
            {
                return pIRadControllerOutputPoint;
            }
        }
    }

    return NULL;
}

//=============================================================================
// rController2::GetLocation
//=============================================================================

const char* rController2::GetLocation( void )
{
    if ( m_LocationName != NULL )
    {
        return m_LocationName->GetChars( );
    }

    return "Unknown Location";
}

//=============================================================================
// rController2::GetNumberOfInputPoints
//=============================================================================

unsigned int rController2::GetNumberOfInputPoints( void )
{
    return m_xIOl_InputPoints->GetSize( );
}

//=============================================================================
// rController2::GetInputPointByIndex
//=============================================================================

IRadControllerInputPoint * rController2::GetInputPointByIndex( unsigned int indx )
{
    return reinterpret_cast< IRadControllerInputPoint * >( m_xIOl_InputPoints->GetAt( indx ) );
}


//=============================================================================
// rController2::GetNumberOfOutputPoints
//=============================================================================

unsigned int rController2::GetNumberOfOutputPoints( void )
{
    return m_xIOl_OutputPoints->GetSize( );
}    

//=============================================================================
// rController2::GetOutputPointByIndex
//=============================================================================

IRadControllerOutputPoint * rController2::GetOutputPointByIndex( unsigned int indx )
{
    return reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetAt( indx ) );
}

//=============================================================================
// rController2::GetDataBuffer
//=============================================================================

unsigned char* rController2::GetDataBuffer( void )
{
  	return m_DataBuffer;
}

//=============================================================================
// rController2::Disconnect
//=============================================================================

void rController2::Disconnect( void )
{
    m_Connected = false;

    if( m_pOutputHandler != NULL )
    {
        m_pOutputHandler->Disconnect( );
    }
}

//=============================================================================
// rController2::Connect
//=============================================================================

void rController2::Connect( void )
{
    m_Connected = true;
    
    //
    // Details of actuator parameters set in sony calls become ineffective if 
    // controller is disconnected or if the mode changes. So, reset the values
    // here.
    //
    switch ( m_TypeId )
	{
		case CL_ID_DUALSHOCK:
		case CL_ID_DUALSHOCK2:
            scePadSetActAlign( m_Port, m_Slot, m_cActuatorAlignment );
            break;
    }

    if( m_pOutputHandler != NULL )
    {
        m_pOutputHandler->Connect( );
    }
}

//=============================================================================
// rController2::iVirtualTimeChanged
//=============================================================================

void rController2::iVirtualTimeChanged( void )
{
    //
    // Only maintain refcounted controllers
    //
    if( m_ReferenceCount > 1 )
    {
        unsigned int timeStamp = 0;

        if( g_theCS == NULL )
        {
            return;
        }
        //
        // Look at the timestamp of the next packet.
        //
        while( m_pIControllerBuffer2->PeekNextPacket( (void*)&timeStamp, sizeof( timeStamp ) ) )
        {
            //
            // If the timestamp of the next packet is less than the current time, we want to
            // fire off events relating to that packet.
            //

            unsigned int packetAge = g_theCS->iGetVirtualTime( ) - ( timeStamp + g_theCS->iGetVirtualTimeAdjust( ) );

            if( packetAge < 0x7FFFFFFF )
            {
                //
                // Remove the packet from the buffering system as now we want to process it.
                //
                unsigned char tempBuffer[ sizeof(unsigned int) + 32 ];

                m_pIControllerBuffer2->DequeuePacket( (void*)tempBuffer );

                if ( m_xIOl_InputPoints != NULL )
                {
                    m_xIOl_InputPoints->Reset( );

                    iIControllerInputPoint2 * pIControllerInputPoint2;

                    while ( NULL != ( pIControllerInputPoint2 = reinterpret_cast< iIControllerInputPoint2 * >( m_xIOl_InputPoints->GetNext( ) ) ) )
                    {
                        pIControllerInputPoint2->iHandleEvent( tempBuffer, timeStamp );                                   
                    }
                }
            }
            else
            {
                break;
            }
        }
    }
}

//=============================================================================
// rController2::iPoll
//=============================================================================

void rController2::iPoll( unsigned int timerManagerMs )
{
    //
    // Only maintain refcounted controllers
    //
    if ( m_ReferenceCount > 1 )
    {
        //
        // Queue a packet onto the buffer.
        //
        unsigned char tempBuffer[ sizeof( unsigned int ) + 32 ];
        memcpy( (void*)tempBuffer, (void*)&timerManagerMs, sizeof( unsigned int ) );
        memcpy( (void*)(tempBuffer+sizeof( unsigned int )), GetDataBuffer( ), sizeof(unsigned int) + 32 );
        m_pIControllerBuffer2->EnqueuePacket( (void*)tempBuffer );
    }
}

//=============================================================================
// rController2::iSetBufferTime
//=============================================================================

void rController2::iSetBufferTime( unsigned int bufferSize )
{
    if( m_pIControllerBuffer2 != NULL )
    {
        m_pIControllerBuffer2->SetQueueSize( bufferSize );
    }
}

//=============================================================================
// rController2::GetId
//=============================================================================

int rController2::GetId( void )
{
    return m_Id;
}













//=============================================================================
// iControllerOutputHandler2::iControllerOutputHandler
//=============================================================================

iControllerOutputHandler2::iControllerOutputHandler2( int port, int slot, unsigned char* pActDirectBuffer )
    :
    m_ReferenceCount( 1 ),
    m_Connected( true ),
    m_Port( port ),
    m_Slot( slot )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "iControllerOutputHandler2" );

    if( pActDirectBuffer != NULL )
    {
        for( int i = 0; i < CL_MAXIMUM_ACTUATORS; i++ )
        {
            m_cActuatorDirective[ i ] = pActDirectBuffer[ i ];
        }
    }
}

//=============================================================================
// iControllerOutputHandler2::~iControllerOutput
//=============================================================================

iControllerOutputHandler2::~iControllerOutputHandler2( void )
{
}

//=============================================================================
// iControllerOutputHandler2::AddRef
//=============================================================================

void iControllerOutputHandler2::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// iControllerOutputHandler2::Release
//=============================================================================

void iControllerOutputHandler2::Release( void )
{
    m_ReferenceCount--;

    if( m_ReferenceCount == 0 )
    {
        delete this;
    }
}

//=============================================================================
// iControllerOutputHandler2::Connect
//=============================================================================

void iControllerOutputHandler2::Connect( void )
{
    m_Connected = true;
}

//=============================================================================
// iControllerOutputHandler2::Disconnect
//=============================================================================

void iControllerOutputHandler2::Disconnect( void )
{
    m_Connected = false;
}

//=============================================================================
// iControllerOutputHandler2::iOnControllerGainChange
//=============================================================================

void iControllerOutputHandler2::iOnControllerGainChange( IRadControllerOutputPoint* pOutputPoint )
{
    if( pOutputPoint != NULL && m_Connected )
    {
        if( strcmp( pOutputPoint->GetName( ), PsxDualShockOutputPoints[ 0 ] ) == 0 )
        {
            if( pOutputPoint->GetGain() == 1.0f )
            {
                m_cActuatorDirective[ 0 ] = 0xFF;
            } 
            else 
            {
                m_cActuatorDirective[ 0 ] = 0x00;
            }
        }
        else
        {
            // 
            // We only have 2 motors on DualShock controllers. This is tha large one.
            //
            m_cActuatorDirective[ 1 ] = (unsigned char)(pOutputPoint->GetGain() * 255.0f );
        }
        scePadSetActDirect( m_Port, m_Slot, m_cActuatorDirective );
    }
}

//=============================================================================
// rControllerOutputPoint2::rControllerOutputPoint2
//=============================================================================

rControllerOutputPoint2::rControllerOutputPoint2( iRIControllerOutputPointCallback2* handler, IRadString* name, IRadString* type )
    :
    m_ReferenceCount( 1 ),
    m_Value( 0.0f ),
    m_Name( name ),
    m_Type( type ),
    m_Callback( handler )
{
    radAddRef( m_Callback, this );
    radAddRef( name, this );
    radAddRef( type, this );
}

//=============================================================================
// rControllerOutputPoint2::~rControllerOutputPoint2
//=============================================================================

rControllerOutputPoint2::~rControllerOutputPoint2( void )
{
    radRelease( m_Callback, this );
    radRelease( m_Name, this );
    radRelease( m_Type, this );
}

//=============================================================================
// rControllerOutputPoint2::AddRef
//=============================================================================

void rControllerOutputPoint2::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// rControllerOutputPoint2::Release
//=============================================================================

void rControllerOutputPoint2::Release( void )
{
    m_ReferenceCount--;

    if( m_ReferenceCount == 0 )
    {
        delete this;
    }
}

//=============================================================================
// rControllerOutputPoint2::GetName
//=============================================================================

const char* rControllerOutputPoint2::GetName( void )
{
    if( m_Name != NULL )
    {
        return m_Name->GetChars( );
    }
    return NULL;
}

//=============================================================================
// rControllerOutputPoint2::GetType
//=============================================================================

const char* rControllerOutputPoint2::GetType( void )
{
    if( m_Type != NULL )
    {
        return m_Type->GetChars( );
    }
    return NULL;
}

//=============================================================================
// rControllerOutputPoint2::GetGain
//=============================================================================

float rControllerOutputPoint2::GetGain( void )
{
    return m_Value;
}

//=============================================================================
// rControllerOutputPoint2::SetGain
//=============================================================================

void rControllerOutputPoint2::SetGain( float value )
{
    if( value >= 1.0f )
    {
        m_Value = 1.0f;
    }
    else if( value <= 0.0f )
    {
        m_Value = 0.0f;
    }
    else
    {
        m_Value = value;
    }

    if( m_Callback != NULL )
    {
        m_Callback->iOnControllerGainChange( this );
    }
}


//=============================================================================
// rControllerInputPoint2::rControllerInputPoint2
//=============================================================================

rControllerInputPoint2::rControllerInputPoint2
( 
    IRefCount* pIObject_Controller, 
    IRadString* name,
    int inputPointTypeIndex,
    unsigned int dataOffset,
    unsigned int mask
)
    :
    m_ReferenceCount( 1 ),
    m_Offset( dataOffset ),
    m_Mask( mask ),
    m_Value( 0.0 ),
    m_Tolerance( 0.0f ),
    m_MinRange( 0.0f ),
    m_MaxRange( 1.0f ),
    m_pWIControllerInputPointCallback2( NULL ),
    m_UserData( 0 ),
    m_Name( name ),
    m_InputPointTypeIndex( inputPointTypeIndex )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "rControllerInputPoint2" );

    radAddRef( name, this );

    ::radStringCreate( & m_Type, g_CtlrAllocator );                            
    rAssert( m_Type != NULL );

    rAssert( inputPointTypeIndex >= 0 && inputPointTypeIndex <= 3 );
    m_Type->Append( InputPointTypes[ inputPointTypeIndex ] );
        
}

//=============================================================================
// rControllerInputPoint2::~rControllerInputPoint2
//=============================================================================

rControllerInputPoint2::~rControllerInputPoint2( void )
{
    radRelease( m_Name, this );
    radRelease( m_Type, this );
}

//=============================================================================
// rControllerInputPoint2::AddRef
//=============================================================================

void rControllerInputPoint2::AddRef( void )
{
    m_ReferenceCount++;
}

//=============================================================================
// rControllerInputPoint2::Release
//=============================================================================

void rControllerInputPoint2::Release( void )
{
    m_ReferenceCount--;

    if( m_ReferenceCount == 0 )
    {
        delete this;
    }
}

//=============================================================================
// rControllerInputPoint2::GetName
//=============================================================================

const char* rControllerInputPoint2::GetName( void )
{
    if( m_Name != NULL )
    {
        return m_Name->GetChars( );
    }
    return NULL;
}

//=============================================================================
// rControllerInputPoint2::GetType
//=============================================================================

const char* rControllerInputPoint2::GetType( void )
{
    if( m_Type != NULL )
    {
        return m_Type->GetChars( );
    }
    return NULL;
}

//=============================================================================
// rControllerInputPoint2::SetTolerance
//=============================================================================

void rControllerInputPoint2::SetTolerance( float percentage )
{
    rAssert( percentage >= 0.0f && percentage <= 1.0f );

    if( percentage < 0.0f )
    {
        percentage = 0.0f;
    }
    else if( percentage > 1.0f )
    {
        percentage = 1.0f;
    }
    m_Tolerance = percentage;
}

//=============================================================================
// rControllerInputPoint2::GetTolerance
//=============================================================================

float rControllerInputPoint2::GetTolerance( void )
{
	return m_Tolerance;
}

//=============================================================================
// rControllerInputPoint2::RegisterControllerInputPointCallback2
//=============================================================================

void rControllerInputPoint2::RegisterControllerInputPointCallback( IRadControllerInputPointCallback * pCallback, unsigned int userData )
{
    rAssert( pCallback != NULL );

    if( pCallback != NULL )
    {
        rAssertMsg( m_pWIControllerInputPointCallback2 == NULL, "Only one controller input point callback supported at this time" );
    }

    if( m_pWIControllerInputPointCallback2 == NULL )
    {
        m_UserData = userData;
        m_pWIControllerInputPointCallback2 = pCallback;
    }
}

//=============================================================================
// rControllerInputPoint2::UnRegisterControllerInputPointCallback2
//=============================================================================

void rControllerInputPoint2::UnRegisterControllerInputPointCallback( IRadControllerInputPointCallback * pCallback )
{
    rAssert( m_pWIControllerInputPointCallback2 == pCallback );

    if( m_pWIControllerInputPointCallback2 == pCallback )
    {
        m_pWIControllerInputPointCallback2 = NULL;
        m_UserData = 0;
    }
}

//=============================================================================
// rControllerInputPoint2::GetCurrentValue
//=============================================================================

float rControllerInputPoint2::GetCurrentValue( unsigned int* pTime )
{
    if( pTime != NULL && g_theCS != NULL )
    {
        rAssert( g_theCS != NULL );

        if( g_theCS != NULL )
        {
            *pTime = g_theCS->iGetVirtualTime( ) - m_Time;
        }
        else
        {
            *pTime = 0;
        }
    }

    //
    // Map it to the client's range here.
    //
    return(( m_MaxRange - m_MinRange ) * m_Value ) + m_MinRange;
}

//=============================================================================
// rControllerInputPoint2::SetRange
//=============================================================================

void rControllerInputPoint2::SetRange( float min, float max )
{
    m_MinRange = min;
    m_MaxRange = max;
}

//=============================================================================
// rControllerInputPoint2::GetRange
//=============================================================================

void rControllerInputPoint2::GetRange( float* pMin, float* pMax )
{
    rAssert( pMin != NULL );
    rAssert( pMax != NULL );
    
    if( pMin != NULL )
    {
        *pMin = m_MinRange;
    }

    if( pMax != NULL )
    {
        *pMax = m_MaxRange;
    }
}

//=============================================================================
// rControllerInputPoint2::CalculateNewValue
//=============================================================================

float rControllerInputPoint2::CalculateNewValue( unsigned char * buffer )
{
    int value;
    memcpy( &value, &buffer[ m_Offset ], 1 );
    value = value & m_Mask;

    switch( m_InputPointTypeIndex )
    {
        // Button
        case 0: 
            //
            // value cannot be negative since it is a digital button and the library
            // call will return us either 0 or 1.
            //
            return ( (unsigned int) value == m_Mask ) ? 0.0f : 1.0f;
            break;
        
        // AnalogButton
        case 1: 
            //
            // Under sony libraries, 1 means the button is pressed all the way down
            // but for us, we want 1 to mean that the button is in untouched state.
            // Invert the value reported by the library call.
            //
            return ( float( value ) / 255.0f );          
            break;

        // X Axis
        case 2:
        {
            return value  / 255.0f;
            break;
        }

        // Y Axis
        case 3:          
        {
            return 1.0f - ( value / 255.0f );
            break;
        }

        default:
			return 0.0f;
            break;
    }
}

//=============================================================================
// rControllerInputPoint2::iHandleEvent
//=============================================================================

void rControllerInputPoint2::iInitialize( unsigned char * buffer )
{
	//
	// Initialize the input point value
	//
	m_Value = CalculateNewValue( buffer );
}

//=============================================================================
// rControllerInputPoint2::iHandleEvent
//=============================================================================

bool rControllerInputPoint2::iHandleEvent( unsigned char* buffer, unsigned int ltime )
{
	float newValue = CalculateNewValue( buffer );

    if( newValue != m_Value )
    {
       if( (( newValue - m_Value ) >= m_Tolerance ) || (( m_Value - newValue ) >= m_Tolerance ) )
       {
            //
            // Only change the value when the tolerance threshold is surpassed.
            //
            m_Value = newValue;
            m_Time = ltime;

            if( m_pWIControllerInputPointCallback2 != NULL )
            {
                m_pWIControllerInputPointCallback2->OnControllerInputPointChange( m_UserData, m_Value );
            }
            return true;
        }
    }

    return false;
}

#ifdef RAD_DEBUG

void rControllerSystem2::Dump( char * pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [rControllerSystem2] At Memory Location:[%p]\n", this );
}
void iControllerLocation2::Dump( char * pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [iControllerLocation2] At Memory Location:[%p]\n", this );
}

void rController2::Dump( char * pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [rController2] At Memory Location:[%p]\n", this );
}
void iControllerOutputHandler2::Dump( char * pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [iControllerOutputHandler2] At Memory Location:[%p]\n", this );
}

void rControllerOutputPoint2::Dump( char * pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [rControllerOutputPoint2] At Memory Location:[%p]\n", this );
}

void rControllerInputPoint2::Dump( char * pStringBuffer, unsigned int bufferSize )
{
    sprintf( pStringBuffer, "Object: [rControllerInputPoint2] At Memory Location:[%p]\n", this );
}

#endif


//=============================================================================
// LGDevWheelInputPoint
//=============================================================================

//=============================================================================
// LGDevWheelInputPoint::LGDevWheelInputPoint
//=============================================================================
// Description: Comment
//
// Parameters:  ( const LGDevInputPointData* data )
//
// Return:      LGDevWheelInputPoint
//
//=============================================================================
LGDevWheelInputPoint::LGDevWheelInputPoint( const LGDevInputPointData* data ) :
    m_Value( 0.0f ),
    m_MinRange( 0.0f ),
    m_MaxRange( 1.0f ),
    m_Tolerance( 0.0f ),
    m_TimeInState( 0 ),
    m_TimeOfStateChange( 0 ),
    m_Data( data )
{
    //
    // Object list to store our callbacks
    //

    ::radObjectListCreate( & m_xIOl_Callbacks, g_CtlrAllocator );
    rAssert( m_xIOl_Callbacks != NULL );
}

//=============================================================================
// LGDevWheelInputPoint::~LGDevWheelInputPoint
//=============================================================================
// Description: Comment
//
// Parameters:  ( void )
//
// Return:      LGDevWheelInputPoint
//
//=============================================================================
LGDevWheelInputPoint::~LGDevWheelInputPoint( void )
{
    rAssertMsg( m_xIOl_Callbacks->GetSize() == 0, "Sombody forgot to UnRegister an input point callback" );
}

//=============================================================================
// LGDevWheelInputPoint::CalculateNewValue
//=============================================================================
// Description: Comment
//
// Parameters:  
//
// Return:      float 
//
//=============================================================================
float LGDevWheelInputPoint::CalculateNewValue( lgDevPosition* position )
{
    float newValue = 0.0f;

    if ( position != NULL )
    {
        if ( m_Data->m_pType == InputPointTypes[0] ) //Button
        {
            newValue = ( position->Buttons & m_Data->m_Mask ) ? 1.0f : 0.0f;
        }
        else if ( m_Data->m_pType == InputPointTypes[1] ) //Analogue Button
        {
            newValue = (float)((unsigned char)((char*)position)[m_Data->m_Offset]) / 255.0f;
        }
        else if ( m_Data->m_pType == InputPointTypes[2] || m_Data->m_pType == InputPointTypes[3] )//Some kind of axis.
        {
            newValue = (float)((unsigned char)((char*)position)[m_Data->m_Offset]) / 255.0f;  //They're 0 - 255
        }        
        else //POV
        {
            //TODO: Better way?
            unsigned int numberPos = ((unsigned int)((char*)position)[m_Data->m_Offset]);
            if ( numberPos == NEUTRAL )
            {
                newValue = 0.0f;
            }
            else if ( ( 1 << numberPos ) & m_Data->m_Mask ) 
            {
                newValue = 1.0f;
            } 
        }
    }
    
    return newValue;
}

//=============================================================================
// LGDevWheelInputPoint::Initialize
//=============================================================================
// Description: Comment
//
// Parameters:  ( lgDevPosition* position )
//
// Return:      
//
//=============================================================================
void LGDevWheelInputPoint::Initialize( lgDevPosition* position )
{
    //Calculate an initial value
    m_Value = CalculateNewValue( position );
}

//=============================================================================
// LGDevWheelInputPoint::HandleEvent
//=============================================================================
// Description: Comment
//
// Parameters:  ( lgDevPosition* position, unsigned int time )
//
// Return:      bool 
//
//=============================================================================
bool LGDevWheelInputPoint::HandleEvent( lgDevPosition* position, unsigned int time )
{
	float newValue = CalculateNewValue( position );

    if( newValue != m_Value )
    {
       if( (( newValue - m_Value ) >= m_Tolerance ) || (( m_Value - newValue ) >= m_Tolerance ) )
       {
            //
            // Only change the value when the tolerance threshold is surpassed.
            //
            m_Value = newValue;
            m_TimeOfStateChange = time;

            //
            // Notify callbacks
            //

            AddRef( ); // Don't want to self destruct while we're calling out

            IRadWeakCallbackWrapper * pIWcr;

            m_xIOl_Callbacks->Reset( );

            if ( NULL != (pIWcr = reinterpret_cast<IRadWeakCallbackWrapper*>( m_xIOl_Callbacks->GetNext( ) )) )
            {
                IRadControllerInputPointCallback* pCallback = (IRadControllerInputPointCallback*)pIWcr->GetWeakInterface( );
                unsigned int userData = (unsigned int) pIWcr->GetUserData( );
                pCallback->OnControllerInputPointChange( userData, m_Value );           
            }

            Release( );

            return true;
        }
        else
        {
            //
            // This input point has not changed value, or failed the tolerance
            // test, so count time in state.
            //

            m_TimeInState = time - m_TimeOfStateChange;
        }
    }

    return false;
}

//=============================================================================
// LGDevWheelInputPoint::SetTolerance
//=============================================================================
// Description: Comment
//
// Parameters:  
//
// Return:      void 
//
//=============================================================================
void LGDevWheelInputPoint::SetTolerance( float percentage )
{
    //
    // Set tolerance in range.
    //

    rAssert( percentage >= 0.0f && percentage <= 1.0f );

    if ( percentage < 0.0f )
    {
        percentage = 0.0f;
    }
    else if ( percentage > 1.0f )
    {
        percentage = 1.0f;
    }

    m_Tolerance = percentage;
}

//=============================================================================
// LGDevWheelInputPoint::RegisterControllerInputPointCallback
//=============================================================================
// Description: Comment
//
// Parameters:  ( IRadControllerInputPointCallback * pCallback, unsigned int userData )
//
// Return:      void 
//
//=============================================================================
void LGDevWheelInputPoint::RegisterControllerInputPointCallback( IRadControllerInputPointCallback * pCallback, 
                                                                 unsigned int userData )
{
    //
    // Wrap the weak interface we a callback wrapper so we can store it
    // in our object list.
    //

    rAssert( pCallback != NULL );

    ref< IRadWeakCallbackWrapper > xIWcr;

    radWeakCallbackWrapperCreate( &xIWcr, g_CtlrAllocator ); 

    rAssert( xIWcr != NULL );

    if ( xIWcr != NULL )
    {
        xIWcr->SetWeakInterface( pCallback );
        xIWcr->SetUserData( (void*) userData );
    }

    m_xIOl_Callbacks->AddObject( xIWcr );
}

//=============================================================================
// LGDevWheelInputPoint::UnRegisterControllerInputPointCallback
//=============================================================================
// Description: Comment
//
// Parameters:  ( IRadControllerInputPointCallback * pCallback )
//
// Return:      void 
//
//=============================================================================
void LGDevWheelInputPoint::UnRegisterControllerInputPointCallback( IRadControllerInputPointCallback * pCallback )
{
    //
    // Simply look for the callback in the list and delete it, if found.
    //
    rAssert( pCallback != NULL );

    IRadWeakCallbackWrapper * pIWcr;

    m_xIOl_Callbacks->Reset( );

    if ( NULL != (pIWcr = reinterpret_cast< IRadWeakCallbackWrapper * >( m_xIOl_Callbacks->GetNext( ) )) )
    {
        if ( pIWcr->GetWeakInterface( ) == pCallback )
        {
            m_xIOl_Callbacks->RemoveObject( pIWcr );
            return;
        }
    }

    rAssertMsg( false, "Controller Input Point Callback Not Registered." );
}

//=============================================================================
// LGDevWheelInputPoint::GetCurrentValue
//=============================================================================
// Description: Comment
//
// Parameters:  ( unsigned int* pTime )
//
// Return:      
//
//=============================================================================
float LGDevWheelInputPoint::GetCurrentValue( unsigned int* pTime )
{
    if ( pTime != NULL )
    {
        *pTime = m_TimeInState;
    }                
    
    //
    // we map it to the client's range here, this solves a bunch of
    // problems with the client remapping in mid-game.
    //

    return (( m_MaxRange - m_MinRange ) * m_Value ) + m_MinRange;
}

//=============================================================================
// LGDevWheelInputPoint::GetRange
//=============================================================================
// Description: Comment
//
// Parameters:  ( float * pMin, float * pMax )
//
// Return:      void 
//
//=============================================================================
void LGDevWheelInputPoint::GetRange( float * pMin, float * pMax )
{
    //
    // Either param can be null, but not both!
    //

    rAssert( pMin != NULL || pMax != NULL );

    if ( pMin != NULL )
    {
        *pMin = m_MinRange;
    }

    if ( pMax != NULL )
    {
        *pMax = m_MaxRange;
    }
}


//=============================================================================
// LGDevWheelController
//=============================================================================

//=============================================================================
// LGDevWheelController::LGDevWheelController
//=============================================================================
// Description: Comment
//
// Parameters:  lgDevDeviceDesc desc
//
// Return:      LGDevWheelController
//
//=============================================================================
LGDevWheelController::LGDevWheelController( lgDevDeviceDesc* desc ) :
    radRefCount( 0 )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "LGDevWheelController" );

    // Get a controller buffering system, this is a shared component
    // used by other implementations on other platforms
    //
    radControllerBufferCreate( &m_xIControllerBuffer, g_CtlrAllocator );
    
    //
    // Get an object list to store our input points
    //
    ::radObjectListCreate( &m_xIOl_InputPoints, g_CtlrAllocator );
    ::radObjectListCreate( &m_xIOl_OutputPoints, g_CtlrAllocator );
    
	//
    // Get a string to store our location
    //
    ::radStringCreate( &m_xIString_Location, g_CtlrAllocator );

    memcpy( &m_DeviceDesc, desc, sizeof( lgDevDeviceDesc ) );

    //
    // Initialize the controller buffer to store game pad sized packets.
    // The queue size will get set later.
    m_xIControllerBuffer->Initialize( 3, sizeof(LGDevControllerPacket) );


    //
    // Create our location name based
    //
    m_xIString_Location->SetSize(4);
    m_xIString_Location->Append( "USB" );
	m_xIString_Location->Append( m_DeviceDesc.Topology[0] - 0x01 );


    LGDevInputPointData* points = NULL;
    unsigned int pointSize = 0;
    
    //What kind of wheel is this?  Init the input / output points.
    if ( m_DeviceDesc.DevClass == LGCLASS_WHEEL_A )
    {
        points = LGDevWheelAInputPoints;
        pointSize = sizeof( LGDevWheelAInputPoints );
    }
    else if ( m_DeviceDesc.DevClass == LGCLASS_WHEEL_B )
    {
        points = LGDevWheelBInputPoints;
        pointSize = sizeof( LGDevWheelBInputPoints );
    }
    else
    {
        rAssertMsg(false, "Unknown wheel type!\n" );
    }

    for ( unsigned int input = 0; input < ( pointSize / sizeof( LGDevInputPointData ) ); input++ )
    {
        IRefCount* pObject = static_cast<IRefCount*>(new( g_CtlrAllocator ) LGDevWheelInputPoint( &points[input] ));
        m_xIOl_InputPoints->AddObject( pObject );
    }

	//
	// Create the output points
	//
    if ( m_xIOl_OutputPoints != NULL )
    {
        LGDevWheelOutputPoint * pSpring = new( g_CtlrAllocator ) LGDevWheelOutputPoint( "CenterSpring", LGTYPE_SPRING );

        m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pSpring ) );

        LGDevWheelOutputPoint * pDamper = new( g_CtlrAllocator ) LGDevWheelOutputPoint( "BaseDamper", LGTYPE_DAMPER );

        m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pDamper ) );

        LGDevWheelOutputPoint * pConstant = new( g_CtlrAllocator ) LGDevWheelOutputPoint( "Constant", LGTYPE_CONSTANT );

        m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pConstant ) );

        LGDevWheelOutputPoint * pRumble = new( g_CtlrAllocator ) LGDevWheelOutputPoint( "Rumble", LGTYPE_TRIANGLE );

        m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pRumble ) );

        LGDevWheelOutputPoint * pHeavyRumble = new( g_CtlrAllocator ) LGDevWheelOutputPoint( "HeavyRumble", LGTYPE_SQUARE );

        m_xIOl_OutputPoints->AddObject( static_cast< IRefCount * >( pHeavyRumble ) );
    }

}

//=============================================================================
// LGDevWheelController::iPoll
//=============================================================================
// Description: Comment
//
// Parameters:  ( unsigned int timerManagerMs )
//
// Return:      
//
//=============================================================================
void LGDevWheelController::iPoll( unsigned int timerManagerMs )
{
    int result = LG_SUCCESS;
    lgDevPosition position;
    result = lgDevRead( m_DeviceDesc.ExistingHandle, LGREADMODE_IMMEDIATE, &position );

    if ( result == LG_SUCCESS )
    {
    	LGDevControllerPacket packet;
		packet.time = timerManagerMs;
        memcpy( &packet.position, &position, sizeof(lgDevPosition) );
								
		//
		// Store the data packet in buffer
		//
		m_xIControllerBuffer->EnqueuePacket( &packet );
		
		// Send our output point data to the device here
		//
		m_xIOl_OutputPoints->Reset( );

        LGDevWheelOutputPoint* pICop;

        while( NULL != (pICop = reinterpret_cast< LGDevWheelOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) )) )
        {
            //Pass in the device we're updating.
            pICop->Update( m_DeviceDesc.ExistingHandle );
		}
    }
}

//=============================================================================
// LGDevWheelController::iVirtualTimeChanged
//=============================================================================
// Description: Comment
//
// Parameters:  ( void )
//
// Return:      void 
//
//=============================================================================
void LGDevWheelController::iVirtualTimeChanged( void )
{
    unsigned int timeStamp = 0;

    if( g_theCS == NULL )
    {
        return;
    }
    //
    // Look at the timestamp of the next packet.
    //
    while( m_xIControllerBuffer->PeekNextPacket( (void*)&timeStamp, sizeof( timeStamp ) ) )
    {
        //
        // If the timestamp of the next packet is less than the current time, we want to
        // fire off events relating to that packet.
        //

        unsigned int packetAge = g_theCS->iGetVirtualTime( ) - ( timeStamp + g_theCS->iGetVirtualTimeAdjust( ) );

        if( packetAge < 0x7FFFFFFF )
        {
            //
            // Remove the packet from the buffering system as now we want to process it.
            //
            LGDevControllerPacket packet;

            bool gotPacket = m_xIControllerBuffer->DequeuePacket( &packet );

            if ( gotPacket )
            {
                if ( m_xIOl_InputPoints != NULL )
                {
                    m_xIOl_InputPoints->Reset( );

                    LGDevWheelInputPoint* pLGDevWheelInputPoint;

                    while ( NULL != (pLGDevWheelInputPoint = reinterpret_cast<LGDevWheelInputPoint*>( m_xIOl_InputPoints->GetNext( ) ) ) )
                    {
                        pLGDevWheelInputPoint->HandleEvent( &(packet.position), timeStamp );                                   
                    }
                }
            }
            else
            {
                break;
            }

        }
        else
        {
            break;
        }
    }
}

//=============================================================================
// LGDevWheelController::iSetBufferTime
//=============================================================================
// Description: Comment
//
// Parameters:  ( unsigned int bufferSize )
//
// Return:      void 
//
//=============================================================================
void LGDevWheelController::iSetBufferTime( unsigned int bufferSize )
{
    if ( m_xIControllerBuffer != NULL )
    {
        m_xIControllerBuffer->SetQueueSize( bufferSize );
    }
}

//=============================================================================
// LGDevWheelController::Disconnect
//=============================================================================
// Description: Comment
//
// Parameters:  ( void )
//
// Return:      void 
//
//=============================================================================
void LGDevWheelController::Disconnect( void )
{
    m_DeviceDesc.ExistingHandle = LG_INVALID_DEVICE;
}

//=============================================================================
// LGDevWheelController::Connect
//=============================================================================
// Description: Comment
//
// Parameters:  ( void )
//
// Return:      void 
//
//=============================================================================
void LGDevWheelController::Connect( void )
{

}

//=============================================================================
// LGDevWheelController::IsConnected
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      bool 
//
//=============================================================================
bool LGDevWheelController::IsConnected()
{
    return m_DeviceDesc.ExistingHandle != LG_INVALID_DEVICE;
}

//=============================================================================
// LGDevWheelController::GetNumberOfInputPointsOfType
//=============================================================================
// Description: Comment
//
// Parameters:  ( const char * pType )
//
// Return:      unsigned 
//
//=============================================================================
unsigned int LGDevWheelController::GetNumberOfInputPointsOfType( const char* pType )
{
    //
    // Count up the number of input points of this time in the input
    // point list
    //

    rAssert( pType != NULL );

    unsigned int count = 0;

    m_xIOl_InputPoints->Reset( );

    IRadControllerInputPoint* pICip;

    while ( NULL != (pICip = reinterpret_cast<IRadControllerInputPoint*>(m_xIOl_InputPoints->GetNext( ))) )
    {
        if ( strcmp( pICip->GetType( ), pType ) == 0 )
        {
            count++;
        }        
    }

    return count;    
}

//=============================================================================
// LGDevWheelController::GetNumberOfOutputPointsOfType
//=============================================================================
// Description: Comment
//
// Parameters:  ( const char* pType )
//
// Return:      
//
//=============================================================================
unsigned int LGDevWheelController::GetNumberOfOutputPointsOfType( const char* pType )
{
    //
    // Count up the number of Output points of this time in the Output
    // point list
    //

    rAssert( pType != NULL );

    unsigned int count = 0;

    m_xIOl_OutputPoints->Reset( );

    IRadControllerOutputPoint* pICop;

    while ( NULL != (pICop = reinterpret_cast<IRadControllerOutputPoint*>(m_xIOl_OutputPoints->GetNext( ))) )
    {
        if ( strcmp( pICop->GetType( ), pType ) == 0 )
        {
            count++;
        }        
    }

    return count;
}

//=============================================================================
// LGDevWheelController::GetInputPointByTypeAndIndex
//=============================================================================
// Description: Comment
//
// Parameters:  ( const char * pType, unsigned int index )
//
// Return:      IRadControllerInputPoint
//
//=============================================================================
IRadControllerInputPoint* LGDevWheelController::GetInputPointByTypeAndIndex( const char * pType, unsigned int index )
{
    //
    // Just loop through all of the input points counting each one of
    // that time
    //

    rAssert( pType != NULL );

    unsigned int count = 0;

    m_xIOl_InputPoints->Reset( );

    IRadControllerInputPoint * pICip;

    while ( NULL != (pICip = reinterpret_cast<IRadControllerInputPoint*>(m_xIOl_InputPoints->GetNext( ))) )
    {
        if ( strcmp( pICip->GetType( ), pType ) == 0 )
        {
            if ( count == index )
            {
                return pICip;
            }

            count++;
        }
    }
    
    return NULL;
}

//=============================================================================
// LGDevWheelController::GetOutputPointByTypeAndIndex
//=============================================================================
// Description: Comment
//
// Parameters:  ( const char * pType, unsigned int index )
//
// Return:      IRadControllerOutputPoint
//
//=============================================================================
IRadControllerOutputPoint* LGDevWheelController::GetOutputPointByTypeAndIndex( const char * pType, unsigned int index ) 
{
    //
    // Just loop through all of the Output points counting each one of
    // that time
    //

    rAssert( pType != NULL );

    unsigned int count = 0;

    m_xIOl_OutputPoints->Reset( );

    IRadControllerOutputPoint * pICop;

    while ( NULL != (pICop = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) )) )
    {
        if ( strcmp( pICop->GetType( ), pType ) == 0 )
        {
            if ( count == index )
            {
                return pICop;
            }

            count++;
        }
    }

    return NULL;
}

//=============================================================================
// LGDevWheelController::GetInputPointByName
//=============================================================================
// Description: Comment
//
// Parameters:  ( const char * pName )
//
// Return:      IRadControllerInputPoint
//
//=============================================================================
IRadControllerInputPoint* LGDevWheelController::GetInputPointByName( const char * pName )
{
    //
    // Just loop through all of the input points comparing each ones
    // name to the name passed in.
    //

    rAssert( pName != NULL );

    m_xIOl_InputPoints->Reset( );

	IRadControllerInputPoint * pICip;

    while ( NULL != (pICip = reinterpret_cast< IRadControllerInputPoint * >( m_xIOl_InputPoints->GetNext( ) )) )
    {
        if ( strcmp( pName, pICip->GetName( ) ) == 0 )
        {
            return pICip;
        }
    }

    return NULL;
}

//=============================================================================
// LGDevWheelController::GetOutputPointByName
//=============================================================================
// Description: Comment
//
// Parameters:  ( const char * pName )
//
// Return:      IRadControllerOutputPoint
//
//=============================================================================
IRadControllerOutputPoint* LGDevWheelController::GetOutputPointByName( const char * pName )
{
    //
    // Just loop through all of the Output points comparing each ones
    // name to the name passed in.
    //

    rAssert( pName != NULL );

    m_xIOl_OutputPoints->Reset( );

    IRadControllerOutputPoint * pICop;

    while ( NULL != (pICop = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) )) )
    {
        if ( strcmp( pName, pICop->GetName( ) ) == 0 )
        {
            return pICop;
        }
    }

    return NULL;
}

//=============================================================================
// LGDevWheelController::GetInputPointByIndex
//=============================================================================
// Description: Comment
//
// Parameters:  ( unsigned int indx )
//
// Return:      IRadControllerInputPoint
//
//=============================================================================
IRadControllerInputPoint* LGDevWheelController::GetInputPointByIndex( unsigned int indx )
{ 
    return reinterpret_cast< IRadControllerInputPoint * >( m_xIOl_InputPoints->GetAt( indx ) ); 
};

//=============================================================================
// LGDevWheelController::GetOutputPointByIndex
//=============================================================================
// Description: Comment
//
// Parameters:  ( unsigned int indx )
//
// Return:      IRadControllerOutputPoint
//
//=============================================================================
IRadControllerOutputPoint* LGDevWheelController::GetOutputPointByIndex( unsigned int indx ) 
{ 
    return reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetAt( indx ) ); 
};

//=============================================================================
// LGDevWheelOutputPoint::LGDevWheelOutputPoint
//=============================================================================
// Description: Comment
//
// Parameters:  ( const char * pName, unsigned char type )
//
// Return:      LGDevWheelOutputPoint
//
//=============================================================================
LGDevWheelOutputPoint::LGDevWheelOutputPoint( const char * pName, unsigned char type ) :
    radRefCount( 0 ),
    m_pName( pName ),
    m_uEffectID( -1 ),
    m_bDirty( false ),
    m_eCommand( IDLE )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "LGDevWheelOutputPoint" );

    m_Effect.Type = type;
}

//=============================================================================
// LGDevWheelOutputPoint::~LGDevWheelOutputPoint
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      LGDevWheelOutputPoint
//
//=============================================================================
LGDevWheelOutputPoint::~LGDevWheelOutputPoint()
{
}

//=============================================================================
// LGDevWheelOutputPoint::GetGain
//=============================================================================
// Description: Comment
//
// Parameters:  ( void )
//
// Return:      float 
//
//=============================================================================
float LGDevWheelOutputPoint::GetGain( void )
{
    rAssert( false );
    return 0.0f;
}

//=============================================================================
// LGDevWheelOutputPoint::SetGain
//=============================================================================
// Description: Comment
//
// Parameters:  ( float value )
//
// Return:      void 
//
//=============================================================================
void LGDevWheelOutputPoint::SetGain( float value )
{
    rAssert( false );
}

//=============================================================================
// LGDevWheelOutputPoint::GetEffect
//=============================================================================
// Description: Comment
//
// Parameters:  ()
//
// Return:      const 
//
//=============================================================================
const LGForceEffect* LGDevWheelOutputPoint::GetEffect() const
{
    return &m_Effect;
}

//=============================================================================
// LGDevWheelOutputPoint::UpdateEffect
//=============================================================================
// Description: Comment
//
// Parameters:  ( const LGForceEffect* effect )
//
// Return:      void 
//
//=============================================================================
void LGDevWheelOutputPoint::UpdateEffect( const LGForceEffect* effect )
{
    rAssert( effect->Type == m_Effect.Type );
    
    m_Effect.Duration = effect->Duration;
    m_Effect.StartDelay = effect->StartDelay;
    m_Effect.p = effect->p;

    m_bDirty = true;
}

//=============================================================================
// LGDevWheelOutputPoint::Update
//=============================================================================
// Description: Comment
//
// Parameters:  ( unsigned int device )
//
// Return:      void 
//
//=============================================================================
void LGDevWheelOutputPoint::Update( unsigned int device )
{
    if ( m_uEffectID == -1 && m_bDirty )
    {
        int ret = LG_SUCCESS;
        //Let's download the effect to the wheel
        lgDevDownloadForceEffect( device, (int*)(&m_uEffectID), &m_Effect );
        rAssert( ret == LG_SUCCESS );

        m_bDirty = false;
    } 
    else if ( m_bDirty )
    {
        int ret = LG_SUCCESS;

        lgDevAStartForceEffect( m_uEffectID );
        lgDevASync(LGASYNC_MODE_WAIT, &ret);

        rAssert( ret == LG_SUCCESS );

        int result = lgDevAUpdateForceEffect( m_uEffectID, &m_Effect );
        rAssert( result != LGERR_INVALID_PARAMETER );

        ret = LG_SUCCESS;
        lgDevASync(LGASYNC_MODE_WAIT, &ret);

        rAssert( ret == LG_SUCCESS );

        m_bDirty = false;
    }

    switch ( m_eCommand )
    {
    case START:
        {
            lgDevStartForceEffect( m_uEffectID );
            break;
        }
    case STOP:
        {
            lgDevStopForceEffect( m_uEffectID );
            break;
        }
    default:
        {
            //Must be idle
            break;
        }
    }

    m_eCommand = IDLE;
}
