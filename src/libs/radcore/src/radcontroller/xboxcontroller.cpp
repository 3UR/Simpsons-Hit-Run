//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        xboxcontroller.cpp
//
// Subsystem:	Foundation Technologies - Controller System
//
// Description:	This file contains the implementation of the Foundation 
//              Technologies xbox contoller
//
// Date:    	
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"
#include <stdio.h>
#include <math.h>
#include <radobject.hpp>
#include <radcontroller.hpp>
#include <raddebug.hpp>
#include <radstring.hpp>
#include <radobjectlist.hpp>
#include <radtime.hpp>
#include <radmemorymonitor.hpp>
#include "radcontrollerbuffer.hpp"

#include <Windows.h>
#include <xinput.h>

//============================================================================
// Internal Interfaces
//============================================================================

struct IRadControllerInputPointXBox
    :
    public IRadControllerInputPoint
{
	virtual void iInitialize( char * pData ) = 0;
    virtual void iVirtualTimeReMapped( unsigned int virtualTime ) = 0;
    virtual void iVirtualTimeChanged( unsigned int virtualTime, char * pData ) = 0;
};

struct IRadControllerXBox
    :
    public IRadController
{
    virtual void iPoll( unsigned int virtualTime ) = 0;
    virtual void iVirtualTimeReMapped( unsigned int virtualTime ) = 0;
    virtual void iVirtualTimeChanged( unsigned int virtualTime ) = 0;
    virtual void iSetBufferTime( unsigned int milliseconds, unsigned int pollingRate ) = 0;
};

//============================================================================
// Globals
//============================================================================

struct XBoxInputPoint
{
    char * m_pType;
    char * m_pName;
    unsigned int m_Mask;
};

static char * g_Xbipt[] =
{
    "Button",
    "AnalogButton",
    "XAxis",
    "YAxis"
};

static XBoxInputPoint g_XBoxPoints[] =
{
    { g_Xbipt[ 0 ], "DPadUp",           XINPUT_GAMEPAD_DPAD_UP },
    { g_Xbipt[ 0 ], "DPadDown",         XINPUT_GAMEPAD_DPAD_DOWN },
    { g_Xbipt[ 0 ], "DPadLeft",         XINPUT_GAMEPAD_DPAD_LEFT },
    { g_Xbipt[ 0 ], "DPadRight",        XINPUT_GAMEPAD_DPAD_RIGHT },
    { g_Xbipt[ 0 ], "Start",            XINPUT_GAMEPAD_START },
    { g_Xbipt[ 0 ], "Back",             XINPUT_GAMEPAD_BACK },
    { g_Xbipt[ 0 ], "LeftThumb",        XINPUT_GAMEPAD_LEFT_THUMB },
    { g_Xbipt[ 0 ], "RightThumb",       XINPUT_GAMEPAD_RIGHT_THUMB },
    { g_Xbipt[ 0 ], "A",                XINPUT_GAMEPAD_A },
    { g_Xbipt[ 0 ], "B",                XINPUT_GAMEPAD_B },
    { g_Xbipt[ 0 ], "X",                XINPUT_GAMEPAD_X },
    { g_Xbipt[ 0 ], "Y",                XINPUT_GAMEPAD_Y },
    { g_Xbipt[ 0 ], "Black",            XINPUT_GAMEPAD_LEFT_SHOULDER },
    { g_Xbipt[ 0 ], "White",            XINPUT_GAMEPAD_RIGHT_SHOULDER },
    { g_Xbipt[ 1 ], "LeftTrigger",      sizeof ( WORD ) + ( sizeof( BYTE ) * 0 ) },
    { g_Xbipt[ 1 ], "RightTrigger",     sizeof ( WORD ) + ( sizeof( BYTE ) * 1 ) },
    { g_Xbipt[ 2 ], "LeftStickX",       sizeof ( WORD ) + ( sizeof( BYTE ) * 2 ) + ( sizeof( SHORT ) * 0 ) },
    { g_Xbipt[ 3 ], "LeftStickY",       sizeof ( WORD ) + ( sizeof( BYTE ) * 2 ) + ( sizeof( SHORT ) * 1 ) },
    { g_Xbipt[ 2 ], "RightStickX",      sizeof ( WORD ) + ( sizeof( BYTE ) * 2 ) + ( sizeof( SHORT ) * 2 ) },
    { g_Xbipt[ 3 ], "RightStickY",      sizeof ( WORD ) + ( sizeof( BYTE ) * 2 ) + ( sizeof( SHORT ) * 3 ) }
};

static class radControllerSystemXBox* s_pTheXBoxControllerSystem2 = NULL;
static radMemoryAllocator g_ControllerSystemAllocator = RADMEMORY_ALLOC_DEFAULT;

//============================================================================
// Component: radControllerOutputPointXBox
//============================================================================

class radControllerOutputPointXBox
    :
    public IRadControllerOutputPoint,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerOutputPointXBox" )

    //========================================================================
    // radControllerOutputPointXBox::rXBoxControllerOutputPoint
    //========================================================================

    radControllerOutputPointXBox( const char * pName )
        :
        radRefCount( 0 ),
        m_pName( pName ),
        m_Gain( 0.0f )
    {
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radControllerOutputPointXBox" );
    }

    //========================================================================
    // radControllerOutputPointXBox::~rXBoxControllerOutputPoint
    //========================================================================

    ~radControllerOutputPointXBox( void )
    {
    }

    //========================================================================
    // radControllerOutputPointXBox::GetName
    //========================================================================

    virtual const char * GetName( void )
    {
        return m_pName;
    }

    //========================================================================
    // radControllerOutputPointXBox::GetType
    //========================================================================

    virtual const char * GetType( void )
    {
        return "Analog";
    }

    //========================================================================
    // radControllerOutputPointXBox::GetGain
    //========================================================================

    virtual float GetGain( void )
    {
        return m_Gain;
    }

    //========================================================================
    // radControllerOutputPointXBox::SetGain
    //========================================================================

    virtual void SetGain( float value )
    {
        if ( value < 0.0f )
        {
            value = 0.0f;
        }
        else if ( value > 1.0f )
        {
            value = 1.0f;
        }

        m_Gain = value;
    }

    //========================================================================
    // radControllerOutputPointXBox Data Members
    //========================================================================

    const char * m_pName;
    float m_Gain;
};

//============================================================================
// Component: radControllerInputPointXBox
//============================================================================

class radControllerInputPointXBox
    :
    public IRadControllerInputPointXBox,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerInputPointXBox" )

    //========================================================================
    // radControllerInputPointXBox::iIVirtualTimeReMapped
    //========================================================================

    virtual void iVirtualTimeReMapped( unsigned int virtualTime )
    {
        //
        // The client has done a re-sync to game time, all we can do is
        // set our changed-state time to "now".
        //
        m_TimeInState = 0;
        m_TimeOfStateChange = virtualTime;
    }

    //========================================================================
    // radControllerInputPointXBox::CalculateNewValue
    //========================================================================

	float CalculateNewValue( char * pData )
	{
        //
        // Calculate the current value of the input point according to the
        // data structure passed in.  We get initialized with the offset
        // into this data array to get our data.  Knowing this and our type
        // we can determine our new floating point value.
        //

        float newValue = 0.0f;

        if ( pData != NULL )
        {
            if ( m_pType == g_Xbipt[ 0 ] ) // Button
            {
                newValue = (((unsigned int*)(pData))[ 0 ] & m_Offset) ? 1.0f : 0.0f;
            }
            else if ( m_pType == g_Xbipt[ 1 ] ) // Analog Button
            {
                newValue = ((unsigned char*)pData)[ m_Offset ];
                newValue /= 255.0f;
            }
            else if ( ( m_pType == g_Xbipt[ 2 ] ) || ( m_pType == g_Xbipt[ 3 ] ) ) // X/Y Axis
            {
                SHORT* pShort = (SHORT*)( pData + m_Offset );
                newValue = (float)(*pShort);
                if ( newValue > 0.0f )
                {
                    newValue /= 65534.0f;
                }
                else
                {
                    newValue /= 65536.0f;
                }

                newValue += 0.5f;
            }
            else if ( m_pType == g_Xbipt[ 4 ] ) //This is an analog button being used digitally.
            {
                newValue = ((unsigned char*)pData)[ m_Offset ];
                newValue /= 255.0f;
                newValue = ( newValue > 0.01f ) ? 1.0f : 0.0f;
            }
            else
            {
                rAssert( 0 );
            }
        }

		return newValue;
	}

    //========================================================================
    // radControllerInputPointXBox::iIVirtualTimeChanged
    //========================================================================

    virtual void iVirtualTimeChanged( unsigned int virtualTime, char * pData )
    {
        rAssert( pData != NULL );

		// Get a new value from the pData structure

		float newValue = CalculateNewValue( pData );
        
        //
        // Check tolerance
        //

        if
        (
            ( newValue != m_Value ) && 
            ( fabsf( newValue - m_Value ) >= m_Tolerance )
        )
        {
            //
            // The input point has changed and we are in tolerance.
            //

            m_Value = newValue;

            m_TimeOfStateChange = virtualTime;
            m_TimeInState = 0; // Just changed

            //
            // Notify callbacks
            //

            AddRef( ); // Don't want to self destruct while we're calling out

            IRadWeakCallbackWrapper * pIWcr;

            m_xIOl_Callbacks->Reset( );

            if ( pIWcr = reinterpret_cast< IRadWeakCallbackWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
            {
                IRadControllerInputPointCallback * pCallback = ( IRadControllerInputPointCallback* ) pIWcr->GetWeakInterface( );
                unsigned int userData = (unsigned int) pIWcr->GetUserData( );

                pCallback->OnControllerInputPointChange( userData, m_Value );           
            }

            Release( );
        }
        else
        {
            //
            // This input point has not changed value, or failed the tolerance
            // test, so count time in state.
            //

            m_TimeInState = virtualTime - m_TimeOfStateChange;
        }
    }

    //========================================================================
    // radControllerInputPointXBox::iInitialize
    //========================================================================

	virtual void iInitialize( char * pData )
	{
		// Set the new value

		m_Value = CalculateNewValue( pData );
	}

    //========================================================================
    // radControllerInputPointXBox::GetName
    //========================================================================

    virtual const char * GetName( void )
    {
        //
        // This points to a string in the global controller definition array
        // (see top of file)
        //

        return m_pName;
    }
    
    //========================================================================
    // radControllerInputPointXBox::GetType
    //========================================================================
    
    virtual const char * GetType( void )
    {
        //
        // This points to a string in the global controller definition array
        // (see top of file)
        //

        return m_pType;      
    }

    //========================================================================
    // radControllerInputPointXBox::SetTolerance
    //========================================================================

    virtual void  SetTolerance( float percentage )
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

    //========================================================================
    // radControllerInputPointXBox::GetTolerance
    //========================================================================

    virtual float /* percentage */ GetTolerance( void )
    {
        //
        // Simply retur the tolerance
        // 

        return m_Tolerance;
    }

    //========================================================================
    // radControllerInputPointXBox::RegisterControllerInputPointCallback
    //========================================================================
           
    virtual void RegisterControllerInputPointCallback
    (
        IRadControllerInputPointCallback * pCallback,
        unsigned int userData = 0
    )
    {
        //
        // Wrap the weak interface we a callback wrapper so we can store it
        // in our object list.
        //

        rAssert( pCallback != NULL );

        radRef< IRadWeakCallbackWrapper > xIWcr;

        radWeakCallbackWrapperCreate( &xIWcr, g_ControllerSystemAllocator ); 

        rAssert( xIWcr != NULL );

        if ( xIWcr != NULL )
        {
            xIWcr->SetWeakInterface( pCallback );
            xIWcr->SetUserData( (void*) userData );
        }

        m_xIOl_Callbacks->AddObject( xIWcr );
    }

    //========================================================================
    // radControllerInputPointXBox::UnRegisterControllerInputPointCallback
    //========================================================================

    virtual void UnRegisterControllerInputPointCallback
    (
        IRadControllerInputPointCallback * pCallback
    )
    {
        //
        // Simply look for the callback in the list and delete it, if found.
        //
        rAssert( pCallback != NULL );

        IRadWeakCallbackWrapper * pIWcr;

        m_xIOl_Callbacks->Reset( );

        if ( pIWcr = reinterpret_cast< IRadWeakCallbackWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
        {
            if ( pIWcr->GetWeakInterface( ) == pCallback )
            {
                m_xIOl_Callbacks->RemoveObject( pIWcr );
                return;
            }
        }

        rAssertMsg( false, "Controller Input Point Callback Not Registered." );
    }

    //========================================================================
    // radControllerInputPointXBox::GetCurrentValue
    //========================================================================

    virtual float GetCurrentValue( unsigned int * pTime = NULL )
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

    //========================================================================
    // radControllerInputPointXBox::SetRange
    //========================================================================

    virtual void  SetRange( float min, float max )
    {
        //
        // Note that max range CAN be less than min range, if this is what
        // the client wants.
        //

        m_MinRange = min;
        m_MaxRange = max;
    }

    //========================================================================
    // radControllerInputPointXBox::GetRange
    //========================================================================
    
    virtual void GetRange( float * pMin, float * pMax )
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

    //========================================================================
    // radControllerInputPointXBox::radControllerInputPointXBox
    //========================================================================

    radControllerInputPointXBox( const char * pType, const char * pName, unsigned int offset )
        :
        radRefCount( 0 ),
        m_Value( 0.0f ),
        m_MinRange( 0.0f ),
        m_MaxRange( 1.0f ),
        m_Tolerance( 0.0f ),
        m_TimeInState( 0 ),
        m_TimeOfStateChange( 0 ),
        m_pType( pType ),
        m_pName( pName ),
        m_Offset( offset )
    {
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radControllerInputPointXBox" );
        
        //
        // Object list to store our callbacks
        //

        ::radObjectListCreate( & m_xIOl_Callbacks, g_ControllerSystemAllocator );
    }

    //========================================================================
    // radControllerInputPointXBox::~radControllerInputPointXBox
    //========================================================================
    
    ~radControllerInputPointXBox( void )
    {        
        rAssertMsg( m_xIOl_Callbacks->GetSize() == 0, "Sombody forgot to UnRegister an input point callback" );
    }

    //========================================================================
    // radControllerInputPointXBox Data Members
    //========================================================================

    float m_Value;
    float m_MinRange;
    float m_MaxRange;
    float m_Tolerance;

    unsigned int m_TimeInState;
    unsigned int m_TimeOfStateChange;

    const char * m_pType;
    const char * m_pName;

    unsigned int m_Offset;

    radRef< IRadObjectList > m_xIOl_Callbacks;
};

//============================================================================
// Component: radControllerXBox
//============================================================================

class radControllerXBox
    :
    public IRadControllerXBox,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerXBox" )

    //========================================================================
    // radControllerXBox::iPoll
    //========================================================================

    virtual void iPoll( unsigned int virtualTime )
    {
        //
        // Query the hardware for current state and store it in the
        // controller buffer, it will be pulled out by virtual time
        // changing.
        //

        if ( GetRefCount( ) > 1 )
        {
            if ( m_iController != -1 )
            {
                XINPUT_STATE xInputState{ 0 };

                DWORD rval = XInputGetState( m_iController, & xInputState );

                rAssert( rval == ERROR_SUCCESS || rval == ERROR_DEVICE_NOT_CONNECTED );

                if ( rval == ERROR_SUCCESS )
                {
                    xInputState.dwPacketNumber = virtualTime;

                    m_xIControllerBuffer2->EnqueuePacket( & xInputState );
                }
            }

            //
            // Send our output point data to the device here
            //

            {                
                IRadControllerOutputPoint * pICop2_Left  = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetAt( 0 ) );
                IRadControllerOutputPoint * pICop2_Right = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetAt( 1 ) );

                WORD newLeftGain  = (WORD) ( pICop2_Left->GetGain( ) * 65535.0f );
                WORD newRightGain = (WORD) ( pICop2_Right->GetGain( ) * 65535.0f );
        
                if
                (
                    ( newLeftGain  != m_XInputFeedback.wLeftMotorSpeed ) ||
                    ( newRightGain != m_XInputFeedback.wRightMotorSpeed )
                )
                {
                    m_XInputFeedback.wLeftMotorSpeed =  newLeftGain;
                    m_XInputFeedback.wRightMotorSpeed = newRightGain;

					rAssert(m_iController != 0);

                    DWORD result = 0;
					if(m_iController != 0)
					{
						result = XInputSetState( m_iController, & m_XInputFeedback );
					}

                    //
                    // Old Controllers don't support output and this will
                    // fail
                    //

                    // rAssert( result == ERROR_IO_PENDING );
                }
            }
        }                  
    }

    //========================================================================
    // radControllerXBox::iVirtualTimeReMapped
    //========================================================================

    virtual void iVirtualTimeReMapped( unsigned int virtualTime )
    {
        m_xIControllerBuffer2->Flush( );

        IRadControllerInputPointXBox * pICip2;

        m_xIOl_InputPoints->Reset( );

        while ( pICip2 = reinterpret_cast< IRadControllerInputPointXBox * >( m_xIOl_InputPoints->GetNext( ) ) )
        {
            pICip2->iVirtualTimeReMapped( virtualTime );
        }
    }

    //========================================================================
    // radControllerXBox::iVirtualTimeChanged
    //========================================================================

    virtual void iVirtualTimeChanged( unsigned int virtualTime )
    {
        if( GetRefCount( ) > 1 )
        {
            unsigned int time = 0;

            //
            // Packets are stamped with virtual time.
            //

            //
            // While packets and packets happened in the past, let each
            // input point handle its own data from the structure.
            //

            while
            (
                m_xIControllerBuffer2->PeekNextPacket( & time, sizeof( time ) )
            )
            {
                unsigned int packetAge = virtualTime - time;

                //
                // Virtual time wraps every 50 daty, so test if the packet
                // is less than 25 virtual days (half an int) old.
                //

                if ( packetAge >= 0x7FFFFFFF )
                {
                    //
                    // Packet is in the future, so we are done.
                    //

                    break;
                }

                //
                // Pull this packet from the buffering system
                //

                XINPUT_STATE xInputState;

                bool packet = m_xIControllerBuffer2->DequeuePacket( & xInputState );

                rAssertMsg( packet == true, "InternalError" );

                //
                // If we got the packet, which we better have, update all of our input points
                //

                if ( packet == true )
                {
                    m_xIOl_InputPoints->Reset( );

                    IRadControllerInputPointXBox * pIXbcip2;

                    while ( pIXbcip2 = reinterpret_cast< IRadControllerInputPointXBox * >( m_xIOl_InputPoints->GetNext( ) ) )
                    {                                          
                        pIXbcip2->iVirtualTimeChanged
                        (
                            time, // virtual time of the packet.
                            (char*) & ( xInputState.Gamepad )
                        );
                    }
                }
                else
                {
                    //
                    // We should never get here, but break the while loop
                    // just incase.
                    //

                    break;
                }
            }
        }
    }

    //========================================================================
    // radControllerXBox::iSetBufferTime
    //========================================================================

    virtual void iSetBufferTime
    (
        unsigned int milliseconds,
        unsigned int pollingRate
    )
    {
        //
        // This will flush the queue
        //

        m_xIControllerBuffer2->SetQueueSize( ( milliseconds / pollingRate ) + 2 );
    }

    //========================================================================
    // radControllerXBox::IsConnection
    //========================================================================

    virtual bool IsConnected( void )
    {
        //
        // A null handle is the flag for being disconnected.
        //
        XINPUT_STATE state{ 0 };
        return XInputGetState(m_iController, &state) != ERROR_DEVICE_NOT_CONNECTED;
    }

    //========================================================================
    // radControllerXBox::GetType
    //========================================================================

    virtual const char * GetType( void )
    {
        //
        // Allways an XBoxStandard joystick
        //
        return "XBoxStandard";
    }
    
    //========================================================================
    // radControllerXBox::GetClassification
    //========================================================================

    virtual const char * GetClassification( void )
    {
        //
        // Always a joystick
        //

        return "Joystick";
    }

    //========================================================================
    // radControllerXBox::GetNumberOfInputPointsOfType
    //========================================================================

    virtual unsigned int GetNumberOfInputPointsOfType
    (
        const char * pType
    )
    {
        //
        // Count up the number of input points of this time in the input
        // point list
        //

        rAssert( pType != NULL );

        unsigned int count = 0;

        m_xIOl_InputPoints->Reset( );

        IRadControllerInputPoint * pICip2;

        while ( pICip2 = reinterpret_cast< IRadControllerInputPointXBox * >( m_xIOl_InputPoints->GetNext( ) ) )
        {
            if ( strcmp( pICip2->GetType( ), pType ) == 0 )
            {
                count++;
            }        
        }

        return count;
    }

    //========================================================================
    // radControllerXBox::GetNumberOfOutputPointsOfType
    //========================================================================

    unsigned int GetNumberOfOutputPointsOfType( const char * pType )
    {
        //
        // Count up the number of Output points of this time in the Output
        // point list
        //

        rAssert( pType != NULL );

        unsigned int count = 0;

        m_xIOl_OutputPoints->Reset( );

        IRadControllerOutputPoint * pICip2;

        while ( pICip2 = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) )
        {
            if ( strcmp( pICip2->GetType( ), pType ) == 0 )
            {
                count++;
            }        
        }

        return count;
    }

    //========================================================================
    // radControllerXBox::GetInputPointByTypeAndIndex
    //========================================================================

    virtual IRadControllerInputPoint * GetInputPointByTypeAndIndex
    (
        const char * pType,
        unsigned int index
    )
    {
        //
        // Just loop through all of the input points counting each one of
        // that time
        //

        rAssert( pType != NULL );

        unsigned int count = 0;

        m_xIOl_InputPoints->Reset( );

        IRadControllerInputPoint * pICip2;

        while ( pICip2 = reinterpret_cast< IRadControllerInputPointXBox * >( m_xIOl_InputPoints->GetNext( ) ) )
        {
            if ( strcmp( pICip2->GetType( ), pType ) == 0 )
            {
                if ( count == index )
                {
                    return pICip2;
                }

                count++;
            }
        }
        
        return NULL;
    }

    //========================================================================
    // radControllerXBox::GetOutputPointByTypeAndIndex
    //========================================================================

    IRadControllerOutputPoint * GetOutputPointByTypeAndIndex
    (  
        const char * pType,
        unsigned int index
    ) 
    {
        //
        // Just loop through all of the Output points counting each one of
        // that time
        //

        rAssert( pType != NULL );

        unsigned int count = 0;

        m_xIOl_OutputPoints->Reset( );

        IRadControllerOutputPoint * pICip2;

        while ( pICip2 = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) )
        {
            if ( strcmp( pICip2->GetType( ), pType ) == 0 )
            {
                if ( count == index )
                {
                    return pICip2;
                }

                count++;
            }
        }

        return NULL;
    }

    //========================================================================
    // radControllerXBox::GetInputPointByName
    //========================================================================

    virtual IRadControllerInputPoint * GetInputPointByName
    (
        const char * pName
    )
    {
        //
        // Just loop through all of the input points comparing each ones
        // name to the name passed in.
        //

        rAssert( pName != NULL );

        m_xIOl_InputPoints->Reset( );

        IRadControllerInputPoint * pICip2;

        while ( pICip2 = reinterpret_cast< IRadControllerInputPointXBox * >( m_xIOl_InputPoints->GetNext( ) ) )
        {
            if ( strcmp( pName, pICip2->GetName( ) ) == 0 )
            {
                return pICip2;
            }
        }

        return NULL;
    }

    //========================================================================
    // radControllerXBox::GetOutputPointByName
    //========================================================================

    IRadControllerOutputPoint * GetOutputPointByName
    (
        const char * pName
    )
    {
        //
        // Just loop through all of the Output points comparing each ones
        // name to the name passed in.
        //

        rAssert( pName != NULL );

        m_xIOl_OutputPoints->Reset( );

        IRadControllerOutputPoint * pICip2;

        while ( pICip2 = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) )
        {
            if ( strcmp( pName, pICip2->GetName( ) ) == 0 )
            {
                return pICip2;
            }
        }

        return NULL;
    }

    //========================================================================
    // radControllerXBox::GetLocation
    //========================================================================

    virtual const char * GetLocation( void )
    {
        //
        // Just return the location string;
        //

        return m_xIString_Location->GetChars( );
    }

    //========================================================================
    // radControllerXBox::GetNumberOfInputPoints
    //========================================================================

    virtual unsigned int GetNumberOfInputPoints( void )
    {
        return m_xIOl_InputPoints->GetSize( );
    }

    //========================================================================
    // radControllerXBox::GetInputPointByIndex
    //========================================================================

    virtual IRadControllerInputPoint * GetInputPointByIndex( unsigned int index )
    {
        return reinterpret_cast< IRadControllerInputPointXBox * >( m_xIOl_InputPoints->GetAt( index ) );
    }

    //========================================================================
    // radControllerXBox::GetNumberOfInputPoints
    //========================================================================

    virtual unsigned int GetNumberOfOutputPoints( void )
    {
        return m_xIOl_OutputPoints->GetSize( );
    }

    //========================================================================
    // radControllerXBox::GetOutputPointByIndex
    //========================================================================

    virtual IRadControllerOutputPoint * GetOutputPointByIndex( unsigned int index )
    {
        return reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetAt( index ) );
    }

    //========================================================================
    // radControllerXBox::radControllerXBox
    //========================================================================

    radControllerXBox::radControllerXBox
    (
        unsigned int thisAllocator,
        DWORD iController,
        unsigned int virtualTime,
        unsigned int bufferTime,
        unsigned int pollingRate
    )
        :
        radRefCount( 0 ),
        m_iController( iController )
    {
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radControllerXBox" );

        ::ZeroMemory( & m_XInputFeedback, sizeof( m_XInputFeedback ) );

        //
        // Get a controller buffering system, this is a shared component
        // used by other implementations on other platforms
        //
        radControllerBufferCreate( &m_xIControllerBuffer2, g_ControllerSystemAllocator );
        
        //
        // Get an object list to store our input points
        //

        ::radObjectListCreate( & m_xIOl_InputPoints, g_ControllerSystemAllocator );
        ::radObjectListCreate( & m_xIOl_OutputPoints, g_ControllerSystemAllocator );
        //
        // Get a string to store our location
        //

        ::radStringCreate( & m_xIString_Location, g_ControllerSystemAllocator );

        //
        // Initialize the controller buffer to store XINPUT_STATE sized packets.
        //

        //
        // The queue size will get set later.
        //

        m_xIControllerBuffer2->Initialize( 1, sizeof( XINPUT_STATE ) /* buffer size */);

        //
        // Create our location name based on our port and slot
        //

		m_xIString_Location->SetSize( 12 );
        m_xIString_Location->Append( "Port" );
        m_xIString_Location->Append( (unsigned int) m_iController );
        m_xIString_Location->Append( "\\Slot0" );

        //
        // Create all of our intput points, this is always the same for every
        // x box controller.  See static array above.
        //

        for ( unsigned int button = 0; button < ( sizeof( g_XBoxPoints ) / sizeof( XBoxInputPoint ) ); button++ )
        {
			radRef< radControllerInputPointXBox > pInputPoint = new( g_ControllerSystemAllocator ) radControllerInputPointXBox
			(
                g_XBoxPoints[ button ].m_pType, 
                g_XBoxPoints[ button ].m_pName,
                g_XBoxPoints[ button ].m_Mask
            );

            m_xIOl_InputPoints->AddObject( pInputPoint );

			//
			// Hand the point its first value
			//

            if ( m_iController != -1 )
            {
                XINPUT_STATE xInputState;

                ::ZeroMemory( & xInputState, sizeof( xInputState ) );

                DWORD rval = XInputGetState( m_iController, & xInputState );

                rAssert( rval == ERROR_SUCCESS );

				pInputPoint->iInitialize( (char *) & xInputState.Gamepad );
            }
        }

        if ( m_xIOl_OutputPoints != NULL )
        {
            radControllerOutputPointXBox * pLeft = new( g_ControllerSystemAllocator ) radControllerOutputPointXBox( "LeftMotor" );

            radControllerOutputPointXBox * pRight = new( g_ControllerSystemAllocator ) radControllerOutputPointXBox( "RightMotor" );

            m_xIOl_OutputPoints->AddObject( reinterpret_cast< IRefCount * >( pLeft ) );
            m_xIOl_OutputPoints->AddObject( reinterpret_cast< IRefCount * >( pRight ) );
        }

        //
        // Set everything to a know state using our regular runtime functions.
        // Note that the controller might get created during gameplay after
        // the controller system has been run for a while.
        // 

        iSetBufferTime( bufferTime, pollingRate );
        iVirtualTimeReMapped( virtualTime );
    }

    //========================================================================
    // radControllerXBox::
    //========================================================================

    ~radControllerXBox( void )
    {
    }

    //========================================================================
    // radControllerXBox Data Members
    //========================================================================

    DWORD m_iController;

    radRef< IRadObjectList >             m_xIOl_InputPoints;
    radRef< IRadObjectList >             m_xIOl_OutputPoints;

    radRef< IRadString >                 m_xIString_Location;
    radRef< IRadControllerBuffer >       m_xIControllerBuffer2;
    
    XINPUT_VIBRATION                  m_XInputFeedback;
};

//============================================================================
// Component: radControllerSystemXBox
//============================================================================

class radControllerSystemXBox
    :
    public IRadControllerSystem,
    public IRadTimerCallback,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerSystemXBox" )

     //========================================================================
    // radControllerSystemXBox::CheckDeviceConnectionStatus
    //========================================================================

    void CheckDeviceConnectionStatus( void )
    {
        AddRef( );

        //
        // Check if devices have been inserted or removed
        //
        {
            for ( unsigned int i = 0; i < 4; i ++ )
            {
                {

                    //
                    // Find the controller in question (may not exist though)
                    //
                    radRef< IRadController > xIController2;
                    radRef< IRadControllerXBox > xIXBoxController2;

                    char location[ 255 ];

                    sprintf( location, "Port%d\\Slot0", i );

                    xIController2 = GetControllerAtLocation( location );

                    xIXBoxController2 = (IRadControllerXBox*) xIController2.m_pInterface;


                    // Is there a controller currently plugged in?
                    XINPUT_STATE state{ 0 };
                    DWORD ret = XInputGetState(i, &state);

                    if ( ret == ERROR_SUCCESS )
                    {
                        //
                        // Here a device has been inserted, so open it
                        //

                        if ( xIXBoxController2 == NULL )
                        {
                            //
                            // Here the controller at this location has not yet been 
                            // constructed, so construct a new controller
                            //

                            unsigned int virtualTime = 0;
                            unsigned int pollingRate = 10;

                            virtualTime = radTimeGetMilliseconds() + m_VirtualTimeAdjust;
                            
                            if ( m_xITimer != NULL )
                            {
                                pollingRate = m_xITimer->GetTimeout( );
                            }

                            xIController2 = new ( g_ControllerSystemAllocator ) radControllerXBox
                            (
                                g_ControllerSystemAllocator,
                                i,
                                virtualTime,
                                m_EventBufferTime,
                                pollingRate
                            );

                            m_xIOl_Controllers->AddObject
                            (
                                xIController2
                            );
                        }
                        else
                        {
                            Release();
                            return;
                        }
                    }


                    IRadWeakInterfaceWrapper * pIWir;

                    m_xIOl_Callbacks->Reset( );

                    while( pIWir = reinterpret_cast< IRadWeakInterfaceWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
                    {

                        IRadControllerConnectionChangeCallback * pCallback = (IRadControllerConnectionChangeCallback *) pIWir->GetWeakInterface( );
                        pCallback->OnControllerConnectionStatusChange( xIController2 );
                    }
                }
            }
        }

        Release( );
    }

    //========================================================================
    // radControllerSystemXBox::OnTimerDone
    //========================================================================

    virtual void OnTimerDone( unsigned int elapsedtime, void* pUserData )
    {
        //
        // Check for device connections, removals
        //

        CheckDeviceConnectionStatus( );
       
        //
        // Now, update all of our controllers
        //

        m_xIOl_Controllers->Reset( );

        IRadControllerXBox * pIXbc2;

        while ( pIXbc2 = reinterpret_cast< IRadControllerXBox * >( m_xIOl_Controllers->GetNext( ) ) )
        {
            //
            // The controller stamps packets with virtual time, so
            // we pass in virtual time.
            //

            pIXbc2->iPoll( radTimeGetMilliseconds( ) + m_VirtualTimeAdjust );
        }

        //
        // If the client is not driving us, we drive ourselves from this timer
        //

        if ( m_UsingVirtualTime == false )
        {
            SetVirtualTime( radTimeGetMilliseconds( ) );
        }
    }

    //========================================================================
    // radControllerSystemXBox::GetNumberOfControlelrs
    //========================================================================

    virtual unsigned int GetNumberOfControllers( void )
    {
        return m_xIOl_Controllers->GetSize( );
    }

    //========================================================================
    // radControllerSystemXBox::GetControllerByIndex
    //========================================================================

    virtual IRadController * GetControllerByIndex( unsigned int myindex )
    {
        return reinterpret_cast< IRadControllerXBox * >( m_xIOl_Controllers->GetAt( myindex ) );
    }

    //========================================================================
    // radControllerSystemXBox::GetControllerAtLocation
    //========================================================================

    virtual IRadController * GetControllerAtLocation
    (
        const char * pLocation
    )
    {
        //
        // Just loop through all of the controllers asking it for its location
        // if we find a match, return it.
        //

        rAssert( pLocation != NULL );

        m_xIOl_Controllers->Reset( );

        IRadController * pIC2;

        while ( pIC2 = reinterpret_cast< IRadControllerXBox * >( m_xIOl_Controllers->GetNext( ) ) )
        {
            if ( strcmp( pIC2->GetLocation(), pLocation ) == 0 )
            {
                return pIC2;
            }
        }

        return NULL;
    }

    //========================================================================
    // radControllerSystemXBox::SetBufferTime
    //========================================================================

    virtual void SetBufferTime( unsigned int milliseconds )
    {
        if ( milliseconds == 0 )
        {
            //
            // We are always buffering behind the scenes, so we set the
            // buffering time to one 60Hz frame even if the client thinks
            // we are just maintaining state.
            //

            m_UsingVirtualTime = false;

            MapVirtualTime( 0, 0 );

            milliseconds = 10;
        }
        else
        {
            m_UsingVirtualTime = true;
        }

        
        unsigned int pollingRate = 10;

        pollingRate = m_xITimer->GetTimeout( );

        m_EventBufferTime = milliseconds;

        m_xIOl_Controllers->Reset( );

        IRadControllerXBox * pIDipc2;

        while ( pIDipc2 = reinterpret_cast< IRadControllerXBox * >( m_xIOl_Controllers->GetNext( ) ) )
        {
            pIDipc2->iSetBufferTime( milliseconds, pollingRate );
        }
    }

    //========================================================================
    // radControllerSystemXBox::MapVirtualTime
    //========================================================================

    virtual void MapVirtualTime
    (
        unsigned int systemTicks,
        unsigned int virtualTicks
    )
    {
        //
        // Here the client has requested a (re)mapping of virtual time to
        // timer manager time.
        //

        //
        // Recalculate the adjustment factor.  This number represents the
        // number to add to timer manager ticks to get game ticks.
        //
        m_VirtualTimeAdjust = virtualTicks - systemTicks;

        //
        // Inform each controller that virtual time has been remapped.
        //

        m_xIOl_Controllers->Reset( );

        IRadControllerXBox * pIXbc2;

        while ( pIXbc2 = reinterpret_cast< IRadControllerXBox * >( m_xIOl_Controllers->GetNext( ) ) )
        {
            pIXbc2->iVirtualTimeReMapped( radTimeGetMilliseconds() + m_VirtualTimeAdjust );
        }       
    }

    //========================================================================
    // radControllerSystemXBox::SetVirtualTime
    //========================================================================

    virtual void SetVirtualTime( unsigned int virtualTime )
    {
        //
        // Inform each controller that virtual time has changed.
        //

        m_xIOl_Controllers->Reset( );

        IRadControllerXBox * pIXbc2;

        while ( pIXbc2 = reinterpret_cast< IRadControllerXBox * >( m_xIOl_Controllers->GetNext( ) ) )
        {
            pIXbc2->iVirtualTimeChanged( virtualTime );
        }
    }

    //========================================================================
    // radControllerSystemXBox::SetCaptureRate
    //========================================================================

    virtual void SetCaptureRate( unsigned int ms )
    {
        m_xITimer->SetTimeout( ms );

        //
        // Reset event queue size given new filling rate
        //

        SetBufferTime( m_EventBufferTime );
    }

    //========================================================================
    // radControllerSystemXBox::RegisterConnectionChangeCallback2
    //========================================================================

    virtual void RegisterConnectionChangeCallback
    (
        IRadControllerConnectionChangeCallback * pCallback
    )
    {
        //
        // Store the callback in a weak interface wrapper so we can store
        // it in an object list.
        //

        rAssert( pCallback != NULL );

        radRef< IRadWeakInterfaceWrapper > xIWir;

        ::radWeakInterfaceWrapperCreate( & xIWir, g_ControllerSystemAllocator );

        xIWir->SetWeakInterface( pCallback );

        m_xIOl_Callbacks->AddObject( xIWir );
    }

    //========================================================================
    // radControllerSystemXBox::UnRegisterConnectionChagneCallback2
    //========================================================================

    virtual void UnRegisterConnectionChangeCallback
    (
        IRadControllerConnectionChangeCallback * pCallback
    )
    {
        //
        // Look for the callback in the list and remove it if found.
        //

        rAssert( pCallback != NULL );

        IRadWeakInterfaceWrapper * pIWir;

        m_xIOl_Callbacks->Reset( );

        while ( pIWir = reinterpret_cast< IRadWeakInterfaceWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
        {
            if ( pIWir->GetWeakInterface() == pCallback )
            {
                m_xIOl_Callbacks->RemoveObject( pIWir  );
                return;
            }
        }
        
        rAssertMsg( false, "Controller connection change callback not registered." );            
    }

    //========================================================================
    // radControllerSystemXBox::Service
    //========================================================================

    void Service( void )
    {
        m_xITimerList->Service(  );   
    }

    //========================================================================
    // radControllerSystemXBox::radControllerSystemXBox
    //========================================================================

    radControllerSystemXBox
    (
        IRadControllerConnectionChangeCallback* pConnectionChangeCallback,
        radMemoryAllocator thisAllocator
    )
        :
        m_UsingVirtualTime( false ),
        m_VirtualTimeAdjust( 0 ),
        m_EventBufferTime( 0 ),
        m_DefaultConnectionChangeCallback( NULL )
    {
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radControllerSystemXBox" );

        //
        // Set our singleton to "this"
        //
        rAssert( s_pTheXBoxControllerSystem2 == NULL );
        s_pTheXBoxControllerSystem2 = this;
        
        g_ControllerSystemAllocator = thisAllocator;

		//
        // Create a timer to poll with, this will be changed to a thread if
        // people start complaining about latencies.
        //

        //
        // Create a timer list to drive the processing of contollers
        //
    
        radTimeCreateList( &m_xITimerList, 1, g_ControllerSystemAllocator );

        m_xITimerList->CreateTimer( & m_xITimer, 10, this );

        //
        // Object list to hold controllers
        //
        ::radObjectListCreate( & m_xIOl_Controllers, g_ControllerSystemAllocator );
        rAssert( m_xIOl_Controllers != NULL );

        //
        // Object list to hold callbacks
        //
        ::radObjectListCreate( & m_xIOl_Callbacks, g_ControllerSystemAllocator );
        rAssert( m_xIOl_Callbacks != NULL );

        //
        // Register the default connection state callback
        //
        m_DefaultConnectionChangeCallback = pConnectionChangeCallback;
        if( pConnectionChangeCallback )
        {
            m_DefaultConnectionChangeCallback = pConnectionChangeCallback;
            RegisterConnectionChangeCallback( pConnectionChangeCallback );
        }

        //
        // If there is no connection change callback, wait synchronously for the connection
        //
        rWarningMsg( pConnectionChangeCallback != NULL, "Spinning while initializing controllers synchronously." );
        if( pConnectionChangeCallback == NULL )
        {
            while( m_xIOl_Controllers->GetSize( ) == 0 )
            {
                CheckDeviceConnectionStatus( );
            }
        }


        //
        // Set everything to know state
        //        
        SetCaptureRate( 10 );
        MapVirtualTime( 0, 0 );
        SetBufferTime( 0 );
    }

    //========================================================================
    // radControllerSystemXBox::~radControllerSystemXBox
    //========================================================================
        
    ~radControllerSystemXBox( void )
    {
        //
        // Unregister the default connection change callback
        //
        if( m_DefaultConnectionChangeCallback != NULL )
        {
            UnRegisterConnectionChangeCallback( m_DefaultConnectionChangeCallback );
            m_DefaultConnectionChangeCallback = NULL;
        }

        //
        // Make sure the client(s) unregistered all of their callbacks.
        //
        
        rAssertMsg( m_xIOl_Callbacks->GetSize() == 0, "Somebody forgot to unregister a controller connection change callback" );

        g_ControllerSystemAllocator = RADMEMORY_ALLOC_DEFAULT;

        //
        // Set the singleton back to null.
        //

        rAssert( s_pTheXBoxControllerSystem2 == this );
        s_pTheXBoxControllerSystem2 = NULL;

    }


    //========================================================================
    // Data Members
    //========================================================================
    
    bool m_UsingVirtualTime;
    unsigned int m_VirtualTimeAdjust;
    unsigned int m_EventBufferTime;

    IRadControllerConnectionChangeCallback* m_DefaultConnectionChangeCallback;

    radRef< IRadObjectList >     m_xIOl_Callbacks;

    radRef< IRadObjectList >     m_xIOl_Controllers;
    radRef< IRadTimer >          m_xITimer;
    radRef< IRadTimerList >      m_xITimerList;

};

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
    rAssert( s_pTheXBoxControllerSystem2 == NULL );

    new ( alloc ) radControllerSystemXBox( pConnectionChangeCallback, alloc );
}

void radControllerTerminate( void )
{
    radRelease( s_pTheXBoxControllerSystem2, NULL );
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
    rAssert( s_pTheXBoxControllerSystem2 != NULL );

    return( s_pTheXBoxControllerSystem2 );
}

//============================================================================
// Function: radControllerSystemGet
//============================================================================
//
// Use this function to drive the processing of the controller system
//
void radControllerSystemService( void )
{
    if( s_pTheXBoxControllerSystem2 != NULL )
    {
        s_pTheXBoxControllerSystem2->Service( );
    }
}