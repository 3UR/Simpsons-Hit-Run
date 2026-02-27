//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        directinputcontroller.cpp
//
// Subsystem:   Foundation Technologies - Controller System
//
// Description: This file contains the implementation of the Foundation 
//              Technologies direct input controller for win32
//
// Date:        Mar 16, 2001
//
//=============================================================================

//============================================================================
// Include Files
//============================================================================

#include "pch.hpp"

#if defined(RAD_WIN32)
#include <dinput.h>
#include <math.h>
#include <radcontroller.hpp>
#include <raddebug.hpp>
#include <radplatform.hpp>
#include <radobjectlist.hpp>
#include <radtime.hpp>
#include <radstring.hpp>
#include <radmemorymonitor.hpp>
#include <directinputcontroller.hpp>


//============================================================================
// Forward References
//============================================================================

class radControllerSystemDirectInput;

//============================================================================
// Statics
//============================================================================

static radControllerSystemDirectInput* s_pTheDirectInputControllerSystem = NULL;

static radMemoryAllocator g_ControllerSystemAllocator = RADMEMORY_ALLOC_DEFAULT;

// set to true before enumerating the keyboard
static bool s_EnumKeyboard = false;

static int s_VirtualKeyToIndex[ 256 ];
const int* VirtualKeyToIndex = &s_VirtualKeyToIndex[ -1 ]; // DIK_* starts at 1 not 0
const int* VirtualJoyKeyToIndex = &s_VirtualKeyToIndex[ -48 ]; // DIK_* starts at 1 not 0

//============================================================================
// Interface: IRadControllerInputPointDirectInput
//============================================================================

struct IRadControllerInputPointDirectInput
    :
    public IRadControllerInputPoint
{
    virtual unsigned int iGetDirectInput8Type( void ) = 0;
	virtual void		 iInitialize( unsigned int offset, unsigned int value ) = 0;
    virtual bool         iHandleEvent( unsigned int offset, unsigned int value, unsigned int virtualTime ) = 0;
    virtual void         iVirtualTimeChanged( unsigned int virtualTime ) = 0;
    virtual void         iVirtualTimeReMapped( unsigned int virtualTime ) = 0;
};

//============================================================================
// Interface: IRadControllerDirectInput
//============================================================================

struct IRadControllerDirectInput
    :
    public IRadController
{
    virtual void iSetBufferTime( unsigned int milliseconds, unsigned int pollingRate ) = 0;
    virtual void iVirtualTimeChanged( unsigned int virtualTime, unsigned int virtualTimAdjust ) = 0;
    virtual void iVirtualTimeReMapped( unsigned int virtualTime ) = 0;
    virtual void iPoll( void ) = 0;
};

//============================================================================
// Component: ControllerOutputPointDirectInput
//============================================================================
ControllerOutputPointDirectInput::ControllerOutputPointDirectInput( LPCDIEFFECTINFO pdei, DWORD dataOffset )
:   m_pDIDevice(NULL), 
    m_fGain(0.0f),
    m_iForceNumber(0),
    m_diActuator( dataOffset )
{
    radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "ControllerOutputPointDirectInput" );
    memcpy( &m_diEffectInfo, pdei, sizeof( m_diEffectInfo ) );
    m_diEffectInfo.dwSize = sizeof(DIEFFECTINFO);
    //memcpy( m_diEffectInfo.tszName, "Unknown", sizeof(m_diEffectInfo.tszName) );

    for( int i = 0; i < MAX_FORCES; i++ )
    {
        m_pEffectID[i] = NULL;
        m_bEffectPlaying[i] = false;
    }
}

ControllerOutputPointDirectInput::~ControllerOutputPointDirectInput()
{
    ReleaseEffect();
}

const char * ControllerOutputPointDirectInput::GetName( void )
{
    return m_diEffectInfo.tszName;
}

const char * ControllerOutputPointDirectInput::GetType( void )
{
    return "Analog";
}

float ControllerOutputPointDirectInput::GetGain( void )
{
    return m_fGain;
}

void ControllerOutputPointDirectInput::SetGain( float value )
{
    if ( value < 0.0f )
    {
        value = 0.0f;
    }
    else if ( value > 1.0f )
    {
        value = 1.0f;
    }

    m_fGain = value;
}

void ControllerOutputPointDirectInput::CreateEffect( GUID guidEffect, 
                                                     LPCDIEFFECT pEffect,
                                                     int forceNumber )
{
    HRESULT hResult = S_OK;

    // unload any effects still in memory.
    if( m_pEffectID[forceNumber] != NULL )
    {
        UnloadEffect(forceNumber);
    }

    hResult = m_pDIDevice->CreateEffect( guidEffect, pEffect, &m_pEffectID[forceNumber], NULL );
    m_diEffectInfo.guid = guidEffect;
    m_iForceNumber = forceNumber;
    rAssertMsg( hResult == S_OK, "Failed to create Effect!" );
}

void ControllerOutputPointDirectInput::UpdateEffect( LPCDIEFFECT pEffect )
{
    if( m_pEffectID[m_iForceNumber] == NULL )
    {
        CreateEffect( m_diEffectInfo.guid, pEffect, m_iForceNumber );
    }
    else
    {
        SetParams( pEffect, m_iForceNumber );
    }
}

void ControllerOutputPointDirectInput::SetParams( LPCDIEFFECT pEffect,
                                                  int forceNumber)
{
    if( m_bEffectPlaying[forceNumber] )
    {
        HRESULT hResult = S_OK;
        rAssertMsg( m_pEffectID[forceNumber] != NULL, "Attempt to set an uncreated force effect." );

        hResult = m_pEffectID[forceNumber]->SetParameters( pEffect, 
                                                           DIEP_DIRECTION |
                                                           DIEP_TYPESPECIFICPARAMS );
        m_iForceNumber = forceNumber;
    }
//    rAssertMsg( hResult == S_OK, "Failed to set Effect!" );
}

void ControllerOutputPointDirectInput::Start( void )
{
    HRESULT hResult = S_OK;

    rAssertMsg( m_pEffectID[m_iForceNumber] != NULL, "Attempt to start an uncreated force effect." );
    if (m_pEffectID[m_iForceNumber] != NULL)
    {
        // Make sure the device is acquired, if we are gaining focus.
        m_pDIDevice->Acquire();

        hResult = m_pEffectID[m_iForceNumber]->Start( 1, 0 ); // Start the effect

        if( SUCCEEDED(hResult) )
        {
            m_bEffectPlaying[m_iForceNumber] = true;
            rDebugPrintf("SUCCESS: Started force effect number %d\n", m_iForceNumber);
        }
        else if(FAILED(hResult))
        {
            rDebugPrintf("ERROR: Failed to start force effect number %d\n", m_iForceNumber);
        }
    }
}

void ControllerOutputPointDirectInput::Stop( void )
{
    HRESULT hResult = S_OK;

    if (m_pEffectID[m_iForceNumber] != NULL)
    {
        // Make sure the device is acquired, if we are gaining focus.
        //m_pDIDevice->Unacquire();
        m_pDIDevice->Acquire();

        hResult = m_pEffectID[m_iForceNumber]->Stop(); // Stop the effect

        if( SUCCEEDED(hResult) )
        {
            m_bEffectPlaying[m_iForceNumber] = false;
            rDebugPrintf("SUCCESS: Stopped force effect number %d\n", m_iForceNumber);
        }
        else if(FAILED(hResult))
        {
            rDebugPrintf("ERROR: Failed to stop force effect number %d\n", m_iForceNumber);
        }
    }
}

void ControllerOutputPointDirectInput::Init( IDirectInputDevice8* pDevice )
{
    rAssertMsg( pDevice != NULL, "ControllerOutputPointDirectInput::Initializing the DI device with a NULL pointer!" );
    m_pDIDevice = pDevice;
}

void ControllerOutputPointDirectInput::ReleaseEffect( void )
{
    for (int forceNumber = 0; forceNumber < MAX_FORCES; forceNumber++)
    {
        if (m_pEffectID[forceNumber])
        {
            m_pEffectID[forceNumber]->Release();
            m_pEffectID[forceNumber] = NULL;
        }
    }
}

void ControllerOutputPointDirectInput::SendOutput( void )
{

}

//=====================================================
// PRIVATE METHODS
//=====================================================
HRESULT ControllerOutputPointDirectInput::UnloadEffect( int forceNumber )
{
    HRESULT hResult = S_OK;

    if (m_pEffectID[forceNumber] != NULL)
    {
        // Make sure the device is acquired, if we are gaining focus.
        m_pDIDevice->Acquire();

        hResult = m_pEffectID[forceNumber]->Unload(); // Unload the effect

        if( SUCCEEDED(hResult) )
        {
            m_bEffectPlaying[forceNumber] = false;
            m_pEffectID[forceNumber] = NULL;
        }
    } 
    return hResult;
}

/*class ControllerOutputPointDirectInput
    :
    public IRadControllerOutputPoint,
    public radRefCount
{
public:

    IMPLEMENT_REFCOUNTED( "ControllerOutputPointDirectInput" )

    //========================================================================
    // ControllerOutputPointDirectInput::ControllerOutputPointDirectInput
    //========================================================================

    ControllerOutputPointDirectInput( LPCDIEFFECTINFO pdei, DWORD dataOffset )
        :
        radRefCount( 0 ),
        m_DirectInputEffect( NULL ),
        m_Actuator( dataOffset ),
        m_Gain( 0.0f ),
        m_LastGain( 0.0f )
    {
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "ControllerOutputPointDirectInput" );

        // clean up and store the force
        if( pdei->dwEffType & DIEFT_CONDITION )
        {
            m_ForceType = DIEFT_CONDITION;
        }
        else if( pdei->dwEffType & DIEFT_CONSTANTFORCE )
        {
            m_ForceType = DIEFT_CONSTANTFORCE;
        }
        else if( pdei->dwEffType & DIEFT_PERIODIC )
        {
            m_ForceType = DIEFT_PERIODIC;
        }
        else if( pdei->dwEffType & DIEFT_RAMPFORCE )
        {
            m_ForceType = DIEFT_RAMPFORCE;
        }

        ZeroMemory( &m_Effect, sizeof(m_Effect) );
        m_Effect.dwSize                  = sizeof(DIEFFECT);
        m_Effect.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
        m_Effect.dwDuration              = INFINITE;
        m_Effect.dwSamplePeriod          = 0;
        m_Effect.dwGain                  = DI_FFNOMINALMAX;
        m_Effect.rgdwAxes                = &m_Actuator;
        m_Effect.lpEnvelope              = 0;
        m_Effect.dwStartDelay            = 0;
    }

    //========================================================================
    // ControllerOutputPointDirectInput::~ControllerOutputPointDirectInput
    //========================================================================

    ~ControllerOutputPointDirectInput( void )
    {
        m_DirectInputEffect = NULL;
    }

    //========================================================================
    // ControllerOutputPointDirectInput::GetName
    //========================================================================

    virtual const char * GetName( void )
    {
        switch( m_ForceType )
        {
            case DIEFT_CONDITION:
                return "Condition";
            case DIEFT_CONSTANTFORCE:
                return "Constant";
            case DIEFT_PERIODIC:
                return "Periodic";
            case DIEFT_RAMPFORCE:
                return "Ramp";
            default:
                return "Unknown";
        }
    }

    //========================================================================
    // ControllerOutputPointDirectInput::GetType
    //========================================================================

    virtual const char * GetType( void )
    {
        return "Analog";
    }

    //========================================================================
    // ControllerOutputPointDirectInput::GetGain
    //========================================================================

    virtual float GetGain( void )
    {
        return m_Gain;
    }

    //========================================================================
    // ControllerOutputPointDirectInput::SetGain
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

    virtual HRESULT CreateEffect( const DIEFFECT* pEffect )
    {
        HRESULT hResult = S_OK;
        if( m_DirectInputEffect != NULL )
        {
            hResult = m_pDevice->CreateEffect( GUID_ConstantForce, pEffect, &m_DirectInputEffect, NULL );
        }
        return hResult;
    }

    virtual HRESULT SetParams( const DIEFFECT* pEffect )
    {
        HRESULT hResult = S_OK;
        if( m_DirectInputEffect != NULL )
        {
            hResult = m_DirectInputEffect->SetParameters( pEffect, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS );
        }
        return hResult;
    }

    virtual void UpdateEffect( const DIEFFECT* effect )
    {
        HRESULT hr;

        if( m_DirectInputEffect == NULL )
        {
            hr = pDevice->CreateEffect( GUID_ConstantForce, &m_Effect, &m_DirectInputEffect, NULL );
        }
        else
        {
            hr = m_DirectInputEffect->SetParameters( &m_Effect, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS );
        }

        rAssert( SUCCEEDED( hr ) );
        rAssert( m_DirectInputEffect != NULL );
    }

    virtual void Start( void )
    {
        rAssertMsg( m_DirectInputEffect != NULL, "Attempt to start an uncreated force effect." );

        HRESULT hr = m_DirectInputEffect->Start( 1, 0 );

        rAssert( SUCCEEDED( hr ) );
    }

    virtual void Stop( void )
    {
        rAssertMsg( m_DirectInputEffect != NULL, "Attempt to start an uncreated force effect." );

        HRESULT hr = m_DirectInputEffect->Stop();

        rAssert( SUCCEEDED( hr ) );
    }

    virtual void SendOutput( void )
    {
        if( m_ForceType == DIEFT_CONSTANTFORCE )
        {
            if ( m_Gain != m_LastGain )
            {
                if ( m_Gain < 0.00001f )
                {
                    m_DirectInputEffect->Stop( );
                }
                else
                {
                    DICONSTANTFORCE force;
                    force.lMagnitude = (LONG) ( m_Gain * DI_FFNOMINALMAX );
                    m_Effect.
                    HRESULT hr = m_DirectInputEffect->SetParameters( &m_Effect, DIEP_TYPESPECIFICPARAMS | DIEP_START );

                    rAssert( SUCCEEDED( hr ) );
                }
                m_LastGain = m_Gain;
            }
        }
    }

    //========================================================================
    // ControllerOutputPointDirectInput Data Members
    //========================================================================

    DIDEVICEOBJECTINSTANCE m_DirectInputDeviceObjectInstance;
    radRef< IDirectInputEffect > m_DirectInputEffect;
    DIEFFECT m_Effect;
    DWORD m_Actuator;
    DWORD m_ForceType;
    
    float m_Gain;
    float m_LastGain;

    bool m_Dirty;

    //DICONSTANTFORCE m_ForceEffect;
    //LONG m_Direction;
};*/

//============================================================================
// Component::radControllerInputPointDirectInput
//============================================================================

class radControllerInputPointDirectInput
    :
    public IRadControllerInputPointDirectInput,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerInputPointDirectInput" )

    //========================================================================
    // radControllerInputPointDirectInput::SetRange
    //========================================================================

    virtual void  SetRange( float min, float max )
    {
        m_MinRange = min;
        m_MaxRange = max;
    }

    //========================================================================
    // radControllerInputPointDirectInput::SetRange
    //========================================================================

    virtual void GetRange( float * pMin, float * pMax )
    {
        //
        // Either parameter can be null, but not both.
        //

        rAssert( pMax != NULL || pMin != NULL );

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
    // radControllerInputPointDirectInput::SetTolerance
    //========================================================================

    virtual void SetTolerance( float percentage )
    {
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
    // radControllerInputPointDirectInput::GetTolerance
    //========================================================================

    virtual float GetTolerance( void )
    {
        return m_Tolerance;
    }

    //========================================================================
    // radControllerInputPointDirectInput::CalculateNewValue
    //========================================================================

	float CalculateNewValue( float oldValue, unsigned int value )
	{
		//
		// The new value depends on the type of input point
		//

		float newValue;

		switch( DIDFT_GETTYPE(m_DirectInputDeviceObjectInstance.dwType) )
		{
			// Longs Axis / Slider

			case DIDFT_RELAXIS :
			{
				// cast to an int because the value can be negative.

				newValue = oldValue + (float)((int)( value ));
				break;
			}
			case DIDFT_ABSAXIS :
			{
				newValue = float( value ) / 65535.0f;

				if ( m_DirectInputDeviceObjectInstance.guidType == GUID_YAxis )
				{
					newValue = 1.0f - newValue;
				}

				break;
			}

			case DIDFT_BUTTON: 
			case DIDFT_PSHBUTTON : 
			case DIDFT_TGLBUTTON : 
			{
				newValue = ( value & 0x80 ) ? 1.0f : 0.0f;

				break; 
			}
			case DIDFT_POV:
			{
				//
				// The position is indicated in hundredths of a degree
				// clockwise from north (away from the user).
				// The center position is normally reported as -1
				// For indicators that have only five positions,
				// the value for a controller is -1, 0, 9,000, 18,000, or 27,000.
				//
            
				//
				// Map to degrees, or 1.0 if centered.
				//

				if ( (int) value == -1 )
				{
					newValue = 1.0f;
				}
				else
				{
					newValue = value / 36000.0f;
				}

				break;
			}

			case DIDFT_VENDORDEFINED:
			default:
			{
				newValue = 0.0f;
			}
		}

		return newValue;
	}

    //========================================================================
    // radControllerInputPointDirectInput::iHandleEvent
    //========================================================================

    virtual bool iHandleEvent( unsigned int offset, unsigned int value, unsigned int virtualTime )
    {
        if ( offset == m_Offset )
        {
			// Translate the value into a new input point value

			float newValue = CalculateNewValue( m_Value, value );
        
            //
            // Map our value between 0.0 and 1.0 to their range.
            //
      
            if ( newValue != m_Value )
            {
            
                //
                // Is the tollerance enough to call out
                //

                if ( fabsf( newValue - m_Value ) >= m_Tolerance )
                {
                    //
                    // Set our new value
                    //

                    m_Value = newValue;

                    //
                    // TimeInState will get set again later, but we must set
                    // it now for when we call out.
                    //

                    m_TimeInState = 0;
                    m_TimeOfStateChange = virtualTime;
            
                    //
                    // Callback Clients
                    //
                    
                    if ( m_xIOl_Callbacks != NULL )
                    {
                        AddRef( ); // We are calling out

                        IRadWeakCallbackWrapper * pIRwcw;

                        m_xIOl_Callbacks->Reset( );

                        while( pIRwcw = reinterpret_cast< IRadWeakCallbackWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
                        {
                            IRadControllerInputPointCallback* pCallback = (IRadControllerInputPointCallback *) pIRwcw->GetWeakInterface( );
                            
                            pCallback->OnControllerInputPointChange( (unsigned int) pIRwcw->GetUserData( ), m_Value ); 
                        }

                        Release( );
                    }
                }
            }
            
            return true; // we handled the event
        }       

        return false;
    }

    //========================================================================
    // radControllerInputPointDirectInput::iVirtualTimeChanged
    //========================================================================

    virtual void iVirtualTimeChanged( unsigned int virtualTime )
    {
        //
        // Time has passed, update the amount of time we have been in this
        // state.
        //

        m_TimeInState = virtualTime - m_TimeOfStateChange;
    }

    //========================================================================
    // radControllerInputPointDirectInput::iVirtualTimeReMapped
    //========================================================================

    virtual void iVirtualTimeReMapped( unsigned int virtualTime )
    {   
        //
        // If time has been remapped, we no longer have a reference point
        // for time in state, all we can do is set it to zero, having changed
        // at the start of time.
        //

        m_TimeInState = 0;
        m_TimeOfStateChange = virtualTime;
    }

    //========================================================================
    // radControllerInputPointDirectInput::RegisterControllerInputPointCallback
    //========================================================================

    virtual void RegisterControllerInputPointCallback
    (
        IRadControllerInputPointCallback * pCallback,
        unsigned int userData
    )
    {
        //
        // Wrap the weak interface with a callback wrapper so it can be
        // stored in our callback list
        //

        rAssert( pCallback != NULL );

        if ( pCallback != NULL && m_xIOl_Callbacks != NULL )
        {
            radRef< IRadWeakCallbackWrapper > xIWcr;

            ::radWeakCallbackWrapperCreate( & xIWcr, g_ControllerSystemAllocator );

            rAssert( xIWcr != NULL );

            if ( xIWcr != NULL )
            {
                xIWcr->SetWeakInterface( pCallback );
                xIWcr->SetUserData( (void*) userData );

                m_xIOl_Callbacks->AddObject( xIWcr );
            }
        }
    }

    //========================================================================
    // radControllerInputPointDirectInput::UnRegisterControllerInputPointCallback
    //========================================================================

    virtual void UnRegisterControllerInputPointCallback
    (
        IRadControllerInputPointCallback * pCallback
    )
    {
        //
        // Find all the callbacks in the last and remove them.
        //

        rAssert( pCallback != NULL );

        bool found = false;

        if ( pCallback != NULL && m_xIOl_Callbacks != NULL )
        {
            IRadWeakCallbackWrapper * pIRwcw;

            m_xIOl_Callbacks->Reset( );

            while ( pIRwcw = reinterpret_cast< IRadWeakCallbackWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
            {
                if ( pIRwcw->GetWeakInterface() == pCallback )
                {
                    m_xIOl_Callbacks->RemoveObject( pIRwcw );
                    found = true;
                }
            }
        }

        rAssertMsg( found, "Controller input point callback not registered." );
    }            

    //========================================================================
    // radControllerInputPointDirectInput::GetCurrentValue
    //========================================================================
    
    virtual float GetCurrentValue( unsigned int * pTime = NULL )
    {
        //
        // Report how long the point has been in this state, if the client
        // asked for it.
        //

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
    // radControllerInputPointDirectInput::GetName
    //========================================================================

    virtual const char * GetName( void )
    {
        //
        // Return the underlying driver's name for the input point.
        //

        return m_DirectInputDeviceObjectInstance.tszName;
    }     

    //========================================================================
    // rDirectIntputControllerPoint2::GetType
    //========================================================================

    virtual const char * GetType( void )
    {
        GUID & type = m_DirectInputDeviceObjectInstance.guidType;
        bool relative = ( DIDFT_GETTYPE(m_DirectInputDeviceObjectInstance.dwType) == DIDFT_RELAXIS );

        if ( type == GUID_XAxis )
        {
            return relative ? "RelXAxis" : "XAxis";
        }
        else if ( type == GUID_YAxis )
        {
            return relative ? "RelYAxis" : "YAxis";
        }
        else if ( type == GUID_ZAxis )
        {
            return relative ? "RelZAxis" : "ZAxis";
        }
        else if ( type == GUID_RxAxis )
        {
            return relative ? "RelRxAxis" : "RxAxis";
        }
        else if ( type == GUID_RyAxis )
        {
            return relative ? "RelRyAxis" : "RyAxis";
        }
        else if ( type == GUID_RzAxis )
        {
            return relative ? "RelRzAxis" : "RzAxis";
        }
        else if ( type == GUID_Slider )
        {
            return relative ? "RelSlider" : "Slider";
        }
        else if ( ( type == GUID_Button ) || ( type == GUID_Key ) )
        {
            return "Button";
        }
        else if ( type == GUID_POV )
        {
            return "POV";
        }

        return "Unknown";
    }

    //========================================================================
    // radControllerInputPointDirectInput::iGetDirectInput8Type
    //========================================================================

    virtual unsigned int iGetDirectInput8Type( void )
    {
        //
        // Called by the controller when setting up the data structure.
        //

        return m_DirectInputDeviceObjectInstance.dwType;
    }

    //========================================================================
    // radControllerInputPointDirectInput::iInitialization
    //========================================================================

	virtual void iInitialize( unsigned int offset, unsigned int value )
	{
		// Accept the new offset and value 
		
		m_Offset = offset;
		m_Value = CalculateNewValue( 0.0f, value );
	}

    //========================================================================
    // radControllerInputPointDirectInput::radControllerInputPointDirectInput
    //========================================================================

    radControllerInputPointDirectInput
    (
        class radControllerDirectInput* pIObject_Controller,
        const LPCDIDEVICEOBJECTINSTANCE pDoi,
        unsigned int offset
    )
        :
        radRefCount( 0 ),
        m_Offset( offset ),
        m_Value( 0.0 ),
        m_TimeInState( 0 ),
        m_TimeOfStateChange( 0 ),
        m_Tolerance( 0.0f ),
        m_MinRange( 0.0f ),
        m_MaxRange( 1.0f )
    {
       radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radControllerInputPointDirectInput" );
        m_DirectInputDeviceObjectInstance = *pDoi;

        ::radObjectListCreate( & m_xIOl_Callbacks, g_ControllerSystemAllocator );

        rAssert( m_xIOl_Callbacks != NULL );
    }

    //========================================================================
    // radControllerInputPointDirectInput::~radControllerInputPointDirectInput
    //========================================================================

    ~radControllerInputPointDirectInput( void )
    {
        if ( m_xIOl_Callbacks != NULL )
        {
            rAssertMsg( m_xIOl_Callbacks->GetSize() == 0, "Somebody forgot to unregister a controller input point callback" );
        }
    }

    //========================================================================
    // radControllerInputPointDirectInput::Data Members
    //========================================================================

    unsigned int m_Offset;

    float m_Value;
    unsigned int m_TimeInState;
    unsigned int m_TimeOfStateChange;
    float m_Tolerance;
    float m_MinRange;
    float m_MaxRange;

    DIDEVICEOBJECTINSTANCE m_DirectInputDeviceObjectInstance;
    unsigned int m_ActualOffset;

    radRef< IRadObjectList > m_xIOl_Callbacks;
};


//============================================================================
// Component::radControllerDirectInput
//============================================================================

class radControllerDirectInput
    :
    public IRadControllerDirectInput,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerDirectInput" )

    //========================================================================
    // radControllerDirectInput::IsConnected
    //========================================================================

    virtual bool IsConnected( void )
    {
        //
        // Just forward the call to the underlying hardware system's similar
        // call
        //

        HRESULT hr = m_xIDirectInput8->GetDeviceStatus
        (
            m_DirectInputDeviceInstance.guidInstance
        );

        rAssertMsg( SUCCEEDED( hr ), "IDirectInput8::GetDeviceStatus failed." );

        if ( hr == DI_OK )
        {
            return true;
        }

        return false;      
    }

    //========================================================================
    // radControllerDirectInput::GetName
    //========================================================================

    virtual const char * GetType( void )
    {
        //
        // Return the underlying direct input product name.
        //

        return m_DirectInputDeviceInstance.tszProductName;
    }

    //========================================================================
    // radControllerDirectInput::GetType
    //========================================================================

    virtual const char * GetClassification( void )
    {
        switch( m_DirectInputDeviceInstance.dwDevType & 0xFF )
        {
            case DI8DEVTYPE_MOUSE:
            {
                return "Mouse";
            }
            case DI8DEVTYPE_JOYSTICK:
            case DI8DEVTYPE_FLIGHT:
            case DI8DEVTYPE_1STPERSON:
            case DI8DEVTYPE_GAMEPAD:
            {
                return "Joystick";                      
            }
            case DI8DEVTYPE_KEYBOARD:
            {
                return "Keyboard";
            }
            case DI8DEVTYPE_DRIVING:
            {
                return "SteeringWheel";
            }
            case DI8DEVTYPE_DEVICE:
            default:
            {
                break;
            }                
        }

        return "Unknown";
    }

    //========================================================================
    // radControllerDirectInput::GetNumberOfInputPointsOfType
    //========================================================================

    virtual unsigned int GetNumberOfInputPointsOfType( const char * pType )
    {
        //
        // Just count up all of the input points in the list who's type
        // matches the requested type.
        //

        rAssert( pType != NULL );

        unsigned int count = 0;

        if ( m_xIOl_InputPoints != NULL && pType != NULL )
        {
            m_xIOl_InputPoints->Reset( );

            IRadControllerInputPoint * pIRadControllerInputPoint;

            while ( pIRadControllerInputPoint = reinterpret_cast< IRadControllerInputPoint * >( m_xIOl_InputPoints->GetNext( )  ))
            {
                if ( strcmp( pType, pIRadControllerInputPoint->GetType( ) ) == 0 )
                {
                    count++;
                }
            }
        }

        return count;
    }

    //========================================================================
    // radControllerDirectInput::GetNumberOfOutputPointsOfType
    //========================================================================

    unsigned int GetNumberOfOutputPointsOfType( const char * pType ) 
    {
        //
        // Count up the number of Output points of this time in the Output
        // point list
        //

        rAssert( pType != NULL );

        unsigned int count = 0;

        if ( m_xIOl_OutputPoints != NULL && pType != NULL )
        {
            m_xIOl_OutputPoints->Reset( );

            IRadControllerOutputPoint * pIRadControllerOutputPoint;

            while ( pIRadControllerOutputPoint = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) )
            {
                if ( strcmp( pIRadControllerOutputPoint->GetType( ), pType ) == 0 )
                {
                    count++;
                }        
            }
        }
        return count;
    }
    
    //========================================================================
    // radControllerDirectInput::GetInputPointByName
    //========================================================================

    virtual IRadControllerInputPoint * GetInputPointByName( const char * pName )
    {
        //
        // Just look for an input point that matches the name and return
        // the interface.
        //
        rAssert( pName != NULL );

        if ( m_xIOl_InputPoints != NULL  )
        {
            m_xIOl_InputPoints->Reset( );

            IRadControllerInputPoint * pIRadControllerInputPoint;

            while ( pIRadControllerInputPoint = reinterpret_cast< IRadControllerInputPoint * >( m_xIOl_InputPoints->GetNext( ) ) )
            {
                if ( strcmp( pIRadControllerInputPoint->GetName(), pName ) == 0 )
                {
                    return pIRadControllerInputPoint;
                }
            }          
        }

        return NULL;  
    }

    //========================================================================
    // radControllerDirectInput::GetOutputPointByName
    //========================================================================
    
    IRadControllerOutputPoint * GetOutputPointByName
    (
        const char * pName
    )
    {
        // Just loop through all of the Output points comparing each ones
        // name to the name passed in.
        //
        rAssert( pName != NULL );

        if ( m_xIOl_OutputPoints != NULL  )
        {
            m_xIOl_OutputPoints->Reset( );

            IRadControllerOutputPoint * pIRadControllerOutputPoint;

            while ( pIRadControllerOutputPoint = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) )
            {
                if ( strcmp( pName, pIRadControllerOutputPoint->GetName( ) ) == 0 )
                {
                    return pIRadControllerOutputPoint;
                }
            }
        }
        return NULL;
    }

    //========================================================================
    // radControllerDirectInput::GetInputPointByTypeAndIndex
    //========================================================================

    virtual IRadControllerInputPoint * GetInputPointByTypeAndIndex
    (
        const char * pType,
        unsigned int index
    )
    {
        //
        // Loop through the input point list counting each matching type.
        //

        rAssert( pType != NULL );

        if ( pType != NULL )
        {
            unsigned int count = 0;

            IRadControllerInputPoint * pIRadControllerInputPoint;

            m_xIOl_InputPoints->Reset( );

            while ( pIRadControllerInputPoint = reinterpret_cast< IRadControllerInputPoint * >( m_xIOl_InputPoints->GetNext( ) ) )
            {
                if ( strcmp( pIRadControllerInputPoint->GetType( ), pType ) == 0 )
                {
                    if ( count == index )
                    {
                        return pIRadControllerInputPoint;
                    }

                    count++;
                }
            }
        }

        return NULL;
    }

    //========================================================================
    // radControllerDirectInput::GetOutputPointByTypeAndIndex
    //========================================================================
    
    IRadControllerOutputPoint *  GetOutputPointByTypeAndIndex
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

        if ( pType != NULL )
        {
            unsigned int count = 0;

            m_xIOl_OutputPoints->Reset( );

            IRadControllerOutputPoint * pIRadControllerOutputPoint;

            while ( pIRadControllerOutputPoint = reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetNext( ) ) )
            {
                if ( strcmp( pIRadControllerOutputPoint->GetType( ), pType ) == 0 )
                {
                    if ( count == index )
                    {
                        return pIRadControllerOutputPoint;
                    }

                    count++;
                }
            }
        }
        return NULL;
    }

    //========================================================================
    // radControllerDirectInput::GetLocation
    //========================================================================

    virtual const char * GetLocation( void )
    {
        //
        // Just return our location string
        // 

        if ( m_xIString_Location != NULL )
        {
            return m_xIString_Location->GetChars( );
        }

        return "Unknown Location";
    }

    //========================================================================
    // radControllerDirectInput::iSetBufferTime
    //========================================================================

    virtual void iSetBufferTime( unsigned int milliseconds, unsigned int pollingRate )
    {
        //
        // Here the controller system is asking us to set our event buffer
        // size.
        //

        if ( m_xIOl_InputPoints != NULL && m_xIDirectInputDevice8 != NULL )
        {
            //
            // Can't set this unless the device is unaquired.
            //

            m_xIDirectInputDevice8->Unacquire( );

            //
            // Set to buffered mode.
            //

            DIPROPDWORD dipdw;

            dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
            dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            dipdw.diph.dwObj        = 0;
            dipdw.diph.dwHow        = DIPH_DEVICE;

            // we've made this big (the maximum) so that moving the mouse or
            // gamepad sticks around lots between polling events doesn't
            // overflow the buffer
            dipdw.dwData = 1023;

            HRESULT hr = m_xIDirectInputDevice8->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph );

            rAssertMsg( SUCCEEDED( hr ), "IDirectInputDevice8::SetProperty (buffer size) failed" );
        }
    }

    //========================================================================
    // radControllerDirectInput::iPoll
    //========================================================================

    virtual void iPoll( void )
    {
        //
        // Only do processing if sobody ( besides the controller manager is )
        // holding on to us.
        //

        if ( GetRefCount( ) > 1 )
        {
            //
            // Here the controller system is asking us to buffer another packet.
            // The buffering is internall to DirectInput, so we just pass
            // the poll call along.  Note that some devices don't require polling
            // but calling Poll( ) won't cause any ill side affects.
            //

            if ( m_xIDirectInputDevice8 != NULL )
            {
                //
                // We make sure the device is acquired every poll, this is an
                // extremely lightweight call if we are already aquired.
                //

                HRESULT hr = m_xIDirectInputDevice8->Acquire( );

                //
                // S_FALSE means we have aquired the device already, but this is
                // a successfull return value
                //
    
                if ( SUCCEEDED( hr ) )
                {
                    m_xIDirectInputDevice8->Poll( );
                }
            }

            //
            // Send over out rumble effect.
            //
            m_xIOl_OutputPoints->Reset( );
            IRadControllerOutputPointDirectInput* pIRadControllerOutputPoint;
            while ( pIRadControllerOutputPoint = reinterpret_cast< IRadControllerOutputPointDirectInput * >( m_xIOl_OutputPoints->GetNext( ) ) )
            {
                pIRadControllerOutputPoint->SendOutput( );
            }
        }
    }

    //========================================================================
    // radControllerDirectInput::iVirtualTimeChanged
    //========================================================================

    virtual void iVirtualTimeChanged
    (
        unsigned int virtualTime,
        unsigned int virtualTimeAdjust
    )
    {
        //
        // Only do processing if sobody ( besides the controller manager is )
        // holding on to us.
        //

        if ( GetRefCount( ) > 1 )
        {
            //
            // Here virtual time has changed, we must pull out all of the events
            // that happend before the new virtual time and broadcast all of
            // the events and set the new input point states.
            //

            if
            (
                m_xIDirectInputDevice8 != NULL && 
                m_xIOl_InputPoints != NULL
            )
            {
                DIDEVICEOBJECTDATA didod;

                DWORD size = 1;

                //
                // While there are still packets to remove from the hardware
                // buffer, we keep processing them.  If the next one's time
                // stap happened in the future we break the loop.
                //

                while ( size == 1 )
                {
                    //
                    // See if there is a packet that corresponds to the time
                    // period that we are running.
                    //

                    //
                    // Stupid in/out param - Get one packet, returns number of packets
                    // retreived! geez. 
                    //

                    size = 1;

                    //
                    // DirectX9 Debug Run-Time generates a debug message if we
                    // don't have the device aquired before we call GetDevice-
                    // Data. This is really dumb cause we can really test to
                    // see if it is aquired by calling GetDeviceData()
                    //

                    HRESULT hr = m_xIDirectInputDevice8->Acquire( );

                    if ( FAILED( hr ) )
                    {
                        break;
                    }
                    
                    hr = m_xIDirectInputDevice8->GetDeviceData
                    (
                        sizeof( DIDEVICEOBJECTDATA ),
                        & didod,
                        & size,     // in/out
                        DIGDD_PEEK
                    );

                    if ( FAILED( hr ) || size == 0)
                    {
                        break;
                    }

                    //
                    // What was the virtual time of this packet
                    //

                    unsigned int packetVirtualTime = didod.dwTimeStamp + virtualTimeAdjust;

                    //
                    // How old is this packet in virtualTime
                    //

                    unsigned int packetAgeInVirtualTime = virtualTime - packetVirtualTime;

                    //
                    // If the age is less then half an int, then the packet happened
                    // int last 25 virtual days.
                    //

                    if ( true )
                    {
                        //
                        // Yes, the event corresponds to the virtual time period that
                        // we are running.
                        //

                        size = 1;

                        hr = m_xIDirectInputDevice8->GetDeviceData
                        (
                            sizeof( DIDEVICEOBJECTDATA ),
                            & didod,
                            & size,
                            0 // dont' peek
                        );

                        if ( FAILED( hr ) || size == 0 )
                        {
                            break;
                        }
        
                        //
                        // Go through our input point list asking each input
                        // point to handle the event.  If the event is handled,
                        // break this while loop
                        //

                        if ( m_xIOl_InputPoints != NULL )
                        {
                            m_xIOl_InputPoints->Reset( );

                            IRadControllerInputPointDirectInput * pIDicip2;

                            while ( pIDicip2 = reinterpret_cast< IRadControllerInputPointDirectInput * >( m_xIOl_InputPoints->GetNext( ) ) )
                            {
                                if
                                (
                                    pIDicip2->iHandleEvent
                                    (
                                        didod.dwOfs,
                                        didod.dwData,
                                        packetVirtualTime
                                    )
                                )
                                {
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        //
                        // Event hasn't happened in virtual time
                        //

                        break; 
                    }
                }
            }

            //
            // Regardless of whether anything happened, notify all the input
            // points that time has passed
            //

            if ( m_xIOl_InputPoints != NULL )
            {
                m_xIOl_InputPoints->Reset( );

                IRadControllerInputPointDirectInput * pIDicip2;

                while ( pIDicip2 = reinterpret_cast< IRadControllerInputPointDirectInput * >( m_xIOl_InputPoints->GetNext( ) ) )
                {
                    pIDicip2->iVirtualTimeChanged( virtualTime );                           
                }
            }
        }
    }

    //========================================================================
    // radControllerDirectInput::iVirtualTimeReMapped
    //========================================================================

    virtual void iVirtualTimeReMapped( unsigned int virtualTime )
    {
        //
        // Here the controller system is telling us that virtual time
        // no relates to real time in a different way, we must flush
        // the event buffer, becuase previous events are meaningless.
        //

        if ( m_xIDirectInputDevice8 != NULL )
        {
            //
            // Flush the hardware device buffer
            //

            unsigned long dwItems = INFINITE;
         
            HRESULT hr = m_xIDirectInputDevice8->Acquire( );

            if ( SUCCEEDED( hr ) )
            {
                hr = m_xIDirectInputDevice8->GetDeviceData
                ( 
                    sizeof(DIDEVICEOBJECTDATA), 
                    NULL, 
                    & dwItems, 
                    0
                );
            }
        }

        //
        // Each input point holds the amount of time that it has
        // been in its current state, if virtualTime has changed,
        // they must be reset to zero.
        //

        if ( m_xIOl_InputPoints != NULL )
        {
            IRadControllerInputPointDirectInput * pIDicpip;

            m_xIOl_InputPoints->Reset( );

            while( pIDicpip = reinterpret_cast< IRadControllerInputPointDirectInput * >( m_xIOl_InputPoints->GetNext( ) ) )
            {
                pIDicpip->iVirtualTimeReMapped( virtualTime );
            }
        }
    }

    //========================================================================
    // radControllerDirectInput::GetNumberOfInputPoints
    //========================================================================

    virtual unsigned int GetNumberOfInputPoints( void )
    {
        return m_xIOl_InputPoints->GetSize( );
    }

    //========================================================================
    // radControllerDirectInput::GetInputPointByIndex
    //========================================================================

    virtual IRadControllerInputPoint * GetInputPointByIndex( unsigned int myindex)
    {
        return reinterpret_cast< IRadControllerInputPoint * >( m_xIOl_InputPoints->GetAt( myindex ) );
    }

    //========================================================================
    // rDirectOutputController2::GetNumberOfOutputPoints
    //========================================================================

    virtual unsigned int GetNumberOfOutputPoints( void )
    {
        return m_xIOl_OutputPoints->GetSize( );
    }

    //========================================================================
    // rDirectOutputController2::GetOutputPointByIndex
    //========================================================================

    virtual IRadControllerOutputPoint * GetOutputPointByIndex( unsigned int index)
    {
        return reinterpret_cast< IRadControllerOutputPoint * >( m_xIOl_OutputPoints->GetAt( index ) );
    }

    //========================================================================
    // radControllerDirectInput::EnumObjectsCallback
    //========================================================================

    static BOOL __stdcall EnumObjectsCallback
    (
          LPCDIDEVICEOBJECTINSTANCE lpddoi,
          LPVOID pVoidThis
    )
    {
        //
        // For each input point, create an object to represent it.
        //
        rAssert( pVoidThis != NULL );
        rAssert( lpddoi != NULL );

        if ( pVoidThis != NULL )
        {
            radControllerDirectInput * pThis = (radControllerDirectInput*) pVoidThis;

            if ( lpddoi->guidType != GUID_Unknown )
            {
                IRefCount * pObject = static_cast< IRefCount * >(
                    new( g_ControllerSystemAllocator ) radControllerInputPointDirectInput( pThis, lpddoi, 0 )
                                                                );
                //
                // Add this key to the virtual key table.
                //
                if ( s_EnumKeyboard )
                {
                    s_VirtualKeyToIndex[ lpddoi->dwOfs - 1 ] = pThis->m_xIOl_InputPoints->GetSize( );
                }

                pThis->m_xIOl_InputPoints->AddObject( pObject );

                //
                // If there is a motor associated with this object, add output
                // points for it.
                //
                if ( lpddoi->dwFlags & DIDOI_FFACTUATOR )
                {
                    HRESULT hr = pThis->m_xIDirectInputDevice8->EnumEffects( EnumEffectsCallback, pThis, DIEFT_ALL );
                    rAssertMsg( SUCCEEDED( hr ), "IDirectInputDevice8::EnumEffects Failed!" );
                }
            }
        }

        return DIENUM_CONTINUE;
    }

    //========================================================================
    // radControllerDirectInput::EnumEffectsCallback
    //========================================================================

    static BOOL __stdcall EnumEffectsCallback
    (
        LPCDIEFFECTINFO pdei,
        LPVOID pVoidThis
    )
    {
        //
        // For each type of effect, create an output point to access it
        //
        rAssert( pVoidThis != NULL );
        rAssert( pdei != NULL );

        if ( pVoidThis != NULL )
        {
            radControllerDirectInput * pThis = (radControllerDirectInput*) pVoidThis;
            unsigned int objectIndex = pThis->m_xIOl_InputPoints->GetSize( ) - 1;

            IRefCount * pObject = static_cast< IRefCount * >(
                new( g_ControllerSystemAllocator ) ControllerOutputPointDirectInput( pdei, objectIndex * 4 ) 
                );
            pThis->m_xIOl_OutputPoints->AddObject( pObject );
        }

        return DIENUM_CONTINUE;
    }

    //========================================================================
    // radControllerDirectInput::radControllerDirectInput
    //========================================================================

    radControllerDirectInput( LPCDIDEVICEINSTANCE pDidi, radRef< IDirectInput8 > xIDirectInput8 )
        :
        radRefCount( 0 ),
        m_InputBufferSize( 0 ),
        m_pInputBuffer( NULL ),
        m_xIDirectInput8( xIDirectInput8 )
    {
        ::ZeroMemory( & m_DirectInputDeviceInstance, sizeof( m_DirectInputDeviceInstance ) );

        rAssert( pDidi != NULL );
        rAssert( m_xIDirectInput8 != NULL );

        if ( m_xIDirectInput8 != NULL && pDidi != NULL )
        {
            m_DirectInputDeviceInstance = *pDidi;

            //
            // This list stores all of our intput point objects, make sure we
            // can get one ok...
            //

            ::radObjectListCreate( & m_xIOl_InputPoints, g_ControllerSystemAllocator );
            ::radObjectListCreate( & m_xIOl_OutputPoints, g_ControllerSystemAllocator );

            rAssert( m_xIOl_InputPoints != NULL );
            rAssert( m_xIOl_OutputPoints != NULL );

            if ( m_xIOl_InputPoints != NULL )
            {
                //
                // Create the underlying DirectInput8 device.
                //

                HRESULT hr = m_xIDirectInput8->CreateDevice
                (
                    pDidi->guidInstance,
                    & m_xIDirectInputDevice8,
                    NULL
                );
            
                rAssertMsg( SUCCEEDED( hr ), "IDirectInput8::CreateDeviceEx Failed" );
            
                if ( m_xIDirectInputDevice8 != NULL )
                {
                    //
                    // Ok we've got the device, enumerate all the input points
                    // that is has.
                    //
                    s_EnumKeyboard = strcmp( GetClassification(), "Keyboard" ) == 0;
                    hr = m_xIDirectInputDevice8->EnumObjects
                    (
                        EnumObjectsCallback,
                        this,
                        DIDFT_ALL
                    );
                    s_EnumKeyboard = false;

                    rAssertMsg( SUCCEEDED( hr ), "IDirectInputDevice8::EnumObjects Failed!" );

                    if ( SUCCEEDED( hr ) )
                    {
                        //
                        // Build the data format, creating the array based on our
                        // enumerated input points
                        //
                    
                        DIDATAFORMAT didf;
                        ::ZeroMemory( & didf, sizeof( didf ) );
                        didf.dwSize     = sizeof( didf );
                        didf.dwObjSize  = sizeof( DIOBJECTDATAFORMAT );
                        didf.dwFlags    = strcmp( GetClassification(), "Mouse" ) == 0 ? DIDF_RELAXIS : DIDF_ABSAXIS;
                        didf.dwDataSize = m_xIOl_InputPoints->GetSize( ) * 4;
                        didf.dwNumObjs  = m_xIOl_InputPoints->GetSize( );
                        didf.rgodf      = (LPDIOBJECTDATAFORMAT) radMemoryAlloc( RADMEMORY_ALLOC_TEMP, didf.dwNumObjs * sizeof( DIOBJECTDATAFORMAT ) );

                        //
                        // For each input object enumerated in the list, create the
                        // input point array descriptor for that point.
                        //

                        m_xIOl_InputPoints->Reset( );

                        IRadControllerInputPointDirectInput * pIDicip2;

                        unsigned int count = 0;

                        while ( pIDicip2 = reinterpret_cast< IRadControllerInputPointDirectInput * >(  m_xIOl_InputPoints->GetNext( ) ) )
                        {
                            rAssert( pIDicip2 != NULL );

                            didf.rgodf[ count ].pguid = NULL;
                            didf.rgodf[ count ].dwOfs = count * 4;
                            didf.rgodf[ count ].dwFlags = 0;
                            didf.rgodf[ count ].dwType = 0;
                            didf.rgodf[ count ].dwType = pIDicip2->iGetDirectInput8Type( );

                            count++;
                        }

                        //
                        // Set this data format on the device.
                        //

                        hr = m_xIDirectInputDevice8->SetDataFormat( & didf );

                        rAssertMsg( SUCCEEDED( hr ), "DirectInputDevice8::SetDataFormat Failed" );

                        //
                        // Create an input buffer based on the size that we specified
                        // the data format.
                        //

                        if ( SUCCEEDED( hr ) )
                        {
                            m_pInputBuffer = (char*) radMemoryAlloc( g_ControllerSystemAllocator, didf.dwDataSize );
                            m_InputBufferSize = didf.dwDataSize;
                        }                     

		                //
                        // Hopefully DirectInput8 makes a copy of this structure, or
                        // else we will blow up later because of this line!
                        //

                        radMemoryFree( RADMEMORY_ALLOC_TEMP, didf.rgodf );

                        //
                        // Set the cooperative level, we get the main window
                        // handle from the platform component.  The game sets this
                        // when it starts up.
                        //

                        radRef< IRadPlatform > xIWin32Platform;
                        xIWin32Platform = radPlatformGet(  );
                        rAssert( xIWin32Platform != NULL );

                        if ( xIWin32Platform != NULL )
                        {
                            //
                            // If we have rumble, we need to be in exclusive mode.
                            //
                            bool exclusive = false;
                            if ( m_xIOl_OutputPoints->GetSize( ) > 0 ) exclusive = true;

                            if ( !exclusive )
                            {
                                hr = m_xIDirectInputDevice8->SetCooperativeLevel
                                (
                                    xIWin32Platform->GetMainWindowHandle( ),
                                    DISCL_NONEXCLUSIVE | DISCL_FOREGROUND
                                );

                                rWarningMsg( SUCCEEDED( hr ), "IDirectInputDevice8::SetCooperativeLevel NONEXLUSIVE Failed, trying EXCLUSIVE" );
                                
                                if ( FAILED( hr ) ) exclusive = true;
                            }

                            if ( exclusive )
                            {
                                hr = m_xIDirectInputDevice8->SetCooperativeLevel
                                (
                                    xIWin32Platform->GetMainWindowHandle( ),
                                    DISCL_EXCLUSIVE | DISCL_FOREGROUND
                                );

                                rAssertMsg( SUCCEEDED( hr ), "IDirectInputDevice8::SetCooperativeLevel EXCLUSIVE failed too." );
                            }                            

                            if ( SUCCEEDED( hr ) )
                            {
                                //
                                // Turn off auto-center for force-feedback devices.
                                //
                                if ( m_xIOl_OutputPoints->GetSize( ) > 0 )
                                {
                                    DIPROPDWORD dipdw;
                                    dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
                                    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
                                    dipdw.diph.dwObj        = 0;
                                    dipdw.diph.dwHow        = DIPH_DEVICE;
                                    dipdw.dwData            = FALSE;

                                    HRESULT hr = m_xIDirectInputDevice8->SetProperty( DIPROP_AUTOCENTER, &dipdw.diph );
                                    rAssertMsg( SUCCEEDED( hr ), "IDirectInputDevice8::SetProperty Failed" );
                                }

                                //
                                // Get the location string for this device, for
                                // now we will just use the joystick number, this
                                // might fall apart for usb devices though.
                                //
                                // There is probably a better way to do this, but
                                // on the pc its hard to enumerate the locations.

                                ::radStringCreate( & m_xIString_Location, g_ControllerSystemAllocator );                            
                                rAssert( m_xIString_Location != NULL );
                                
                                if ( m_xIString_Location != NULL )
                                {

                                    if ( strcmp( GetClassification(), "Joystick" ) == 0 )
                                    {
                                        DIPROPDWORD dipw;
                                        dipw.diph.dwSize = sizeof( DIPROPDWORD );
                                        dipw.diph.dwHeaderSize = sizeof( DIPROPHEADER );
                                        dipw.diph.dwObj = 0; // DEVICE
                                        dipw.diph.dwHow = DIPH_DEVICE;
                                        dipw.dwData;

                                        HRESULT hr = m_xIDirectInputDevice8->GetProperty
                                        (
                                            DIPROP_JOYSTICKID,
                                            & (dipw.diph) // what utter crap this is!
                                        );
        
                                        rAssertMsg( SUCCEEDED( hr ), "IDirectInputDevice8::GetProperty (JoystickId) Failed" );

                                        m_xIString_Location->Append( "Joystick" );

                                        if ( SUCCEEDED( hr ) )
                                        {
                                            m_xIString_Location->Append( (int) dipw.dwData );
                                        }
                                        else
                                        {
                                            m_xIString_Location->Append( "?" );
                                        }
                                    }
                                    else if ( strcmp( GetClassification(), "Mouse" ) == 0 )
                                    {
                                        m_xIString_Location->Append( "Mouse0" );
                                    }
                                    else if ( strcmp( GetClassification(), "Keyboard" ) == 0 )
                                    {
                                        m_xIString_Location->Append( "Keyboard0" );
                                    }
                                    else if ( strcmp( GetClassification(), "SteeringWheel" ) == 0 )
                                    {
                                        m_xIString_Location->Append( "SteeringWheel0" );
                                    }
									else
									{
										m_xIString_Location->Append( "Unknown Location" );
									}
                                }

                                //
                                // Set the initialize event buffer size to 2 frames @ 16ms
                                // per frame.
                                //

                                iSetBufferTime
                                (
                                    32,
                                    16
                                ); 

                            }
                        }            
						

						//
						// Now that the data format and cooperative level
						// have been set.  We can acquire the device...
						//

                        do
                        {
    						hr = m_xIDirectInputDevice8->Acquire( );
                        }while (DIERR_OTHERAPPHASPRIO == hr);

                        rWarningMsg( SUCCEEDED( hr ), "radControllerDirectInput: Could not acquire device" );

						//
						// ... and grab its initial values.
						//
									                    
						m_xIOl_InputPoints->Reset( );

						unsigned int * pCurrentValues = ( unsigned int * ) radMemoryAlloc( 
							RADMEMORY_ALLOC_TEMP, m_xIOl_InputPoints->GetSize( ) * sizeof( unsigned int ) );

						// Current values are grabbed here

						hr = m_xIDirectInputDevice8->GetDeviceState( 
							m_xIOl_InputPoints->GetSize( ) * 4, pCurrentValues );
						
						rWarningMsg( SUCCEEDED( hr ), "radControllerDirectInput: Could not access controller to read initial values" );

						if( SUCCEEDED( hr ) )
						{
							//
							// Initialize each input point with a value and an offset
							//

							unsigned int count = 0;

							while ( pIDicip2 = reinterpret_cast< IRadControllerInputPointDirectInput * >(  m_xIOl_InputPoints->GetNext( ) ) )
							{
								rAssert( pIDicip2 != NULL );

								pIDicip2->iInitialize( count * sizeof( unsigned int ), pCurrentValues[ count ] );

								count++;
 							}
                        }

                        //
                        // Initialize all output points.
                        //
                        m_xIOl_OutputPoints->Reset( );
                        IRadControllerOutputPointDirectInput* pIRadControllerOutputPoint;
                        while ( pIRadControllerOutputPoint = reinterpret_cast< IRadControllerOutputPointDirectInput * >( m_xIOl_OutputPoints->GetNext( ) ) )
                        {
                            pIRadControllerOutputPoint->Init( m_xIDirectInputDevice8 );
                        }
						
						::radMemoryFree( RADMEMORY_ALLOC_TEMP, pCurrentValues );
                    }
                }
            }
        }
    }

    //========================================================================
    // radControllerDirectInput::~radControllerDirectInput
    //========================================================================

    ~radControllerDirectInput( void )
    {
        //
        // Free the dynamically allocated intput buffer.
        //
    
        if ( m_xIDirectInputDevice8 != NULL )
        {
            HRESULT hr = m_xIDirectInputDevice8->Unacquire( );
        }

        if ( m_pInputBuffer != NULL )
        {
            radMemoryFree( g_ControllerSystemAllocator, m_pInputBuffer );
            m_pInputBuffer = NULL;
        }
    }

    //========================================================================
    // Data Members
    //========================================================================
   
    unsigned int m_InputBufferSize;
    char * m_pInputBuffer;

    DIDEVICEINSTANCE m_DirectInputDeviceInstance;

    radRef< IDirectInput8 >       m_xIDirectInput8;
    radRef< IDirectInputDevice8 > m_xIDirectInputDevice8;
    radRef< IRadObjectList >        m_xIOl_InputPoints;
    radRef< IRadObjectList >        m_xIOl_OutputPoints;
    radRef< IRadString >            m_xIString_Location;
};

class radControllerSystemDirectInput
    :
    public IRadControllerSystem,
    public IRadTimerCallback,
    public radRefCount
{
    public:

    IMPLEMENT_REFCOUNTED( "radControllerDirectInput" )

    //========================================================================
    // radControllerDirectInput::Service
    //========================================================================

    void Service( void )
    {
        m_xITimerList->Service(  );   
    }

    //========================================================================
    // radControllerDirectInput::OnTimerDone
    //========================================================================

    virtual void OnTimerDone
    (
        unsigned int elapsedtime,
        void*        userData
    )
    {
        //
        // This timer "thread" drives the system to buffer controller states.
        // If the client is not using the virtual time system, we also
        // drive our internal virtual time from this function.
        //

        //
        // For each controller, tell it to buffer new state.
        //

        if ( m_xIOl_Controllers != NULL )
        {
            m_xIOl_Controllers->Reset( );

            IRadControllerDirectInput * pIDic;

            while ( pIDic = reinterpret_cast< IRadControllerDirectInput * >( m_xIOl_Controllers->GetNext( ) ) )
            {
                pIDic->iPoll( );
            }
        }

        //
        // If we are using virtual time, the client will call SetVirtualTime()
        // for us, if not we drive it ourself.  When the client is not
        // driving the time, we internally specify the time in timer manager
        // milliseconds
        //

        if( m_UsingVirtualTime == false )
        {
            SetVirtualTime( radTimeGetMilliseconds( ) );
        }
    }

    //========================================================================
    // radControllerDirectInput::RegisterConnectionChangeCallback2
    //========================================================================

    virtual void RegisterConnectionChangeCallback
    (
        IRadControllerConnectionChangeCallback * pCallback
    )
    {
        //
        // We will pretend we support the behavior but we don't because
        // querying the device state is a blocking call, as has been reported
        // to take an extremely long time.  We don't want to have to poll
        // this.
        //

        rWarningMsg( 0, "Controller Connection Change Callback Not Supported on PC" );

        //
        // Add the connection change callback to the list of connection
        // changes.  We can only store bIObjects in an object list.  The
        // weak interface wrapper just wraps a pointer with a bIObject
        // interface.
        //

        if ( m_xIOl_Callbacks != NULL )
        {
            radRef< IRadWeakInterfaceWrapper > xIWir;

            ::radWeakInterfaceWrapperCreate( & xIWir, g_ControllerSystemAllocator );

            rAssert( xIWir != NULL );

            if ( xIWir != NULL )
            {
                xIWir->SetWeakInterface( pCallback );

                m_xIOl_Callbacks->AddObject( xIWir );
            }
        }
    }

    //========================================================================
    // radControllerDirectInput::UnRegisterConnectionChangeCallback
    //========================================================================

    virtual void UnRegisterConnectionChangeCallback
    (
        IRadControllerConnectionChangeCallback * pCallback
    )
    {
        //
        // Search for the callback in the list and remove it when/if it is
        // found
        // 

        if ( m_xIOl_Callbacks != NULL )
        {
            IRadWeakInterfaceWrapper * pIRwiw;

            m_xIOl_Callbacks->Reset( );

            while ( pIRwiw = reinterpret_cast< IRadWeakInterfaceWrapper * >( m_xIOl_Callbacks->GetNext( ) ) )
            {
                if( pIRwiw->GetWeakInterface( ) == pCallback )
                {
                    m_xIOl_Callbacks->RemoveObject( pIRwiw );
                    return;
                }
            }
        }

        //
        // Oops, the client didn't register this callback they are trying
        // to remove
        //

        rAssertMsg( false, "Controller connection change callback no registered." );
    }

    //========================================================================
    // radControllerDirectInput::GetControllerAtLocation
    //========================================================================

    virtual IRadController * GetControllerAtLocation
    (
        const char * pLocation
    )
    {
        //
        // Just loop through each controller asking it for its location string
        // if we find a mach, return it.
        //

        rAssert( pLocation != NULL );

        if ( pLocation != NULL  )
        {
            if ( m_xIOl_Controllers != NULL )
            {
                m_xIOl_Controllers->Reset( );

                IRadController * pIRadController;

                while ( pIRadController = reinterpret_cast< IRadController * >( m_xIOl_Controllers->GetNext( ) ) )
                {
                    if ( strcmp( pIRadController->GetLocation( ), pLocation ) == 0 )
                    {
                        return pIRadController;
                    }
                }
            }
        }

        return NULL;
    }

    //========================================================================
    // radControllerDirectInput::GetControllerEnumerator2
    //========================================================================

    virtual void SetBufferTime( unsigned int milliseconds )
    {
        //
        // Here the client is setting the size of the buffer for each
        // controller.  If the event buffer size is zero, we are just
        // supposed to buffer current state.  By default we just
        // buffer the current state.
        //

        if ( milliseconds == 0 )
        {
            //
            // We are always buffering behind the scenes, so we set the
            // buffering time to one 60Hz frame even if the client thinks
            // we are just maintaining state.  Re map virtual time to
            // TimerManager milliseconds == game time.
            //

            m_UsingVirtualTime = false;

            MapVirtualTime( 0, 0 );

            milliseconds = 16;
        }
        else
        {
            m_UsingVirtualTime = true;
        }

        m_EventBufferSize = milliseconds;

        //
        // We need the current polling rate so each controller can
        // re-calculate its buffer size.
        //
        unsigned int pollingRate = 16;

        if ( m_xITimer != NULL )
        {
            pollingRate = m_xITimer->GetTimeout( );
        }

        //
        // For each controller pass the new buffer size an the polling rate so
        // the controller can re-calculate its actual buffers size.
        //

        if ( m_xIOl_Controllers != NULL )
        {
            m_xIOl_Controllers->Reset( );

            IRadControllerDirectInput * pIDipc2;

            while ( pIDipc2 = reinterpret_cast< IRadControllerDirectInput * >( m_xIOl_Controllers->GetNext( ) ) )
            {
                pIDipc2->iSetBufferTime( m_EventBufferSize, pollingRate );
            }
        }
    }

    //========================================================================
    // radControllerDirectInput::MapVirtualTime
    //========================================================================

    virtual void MapVirtualTime
    (
        unsigned int timerManagerTicks,
        unsigned int virtualTicks
    )
    {
        //
        // Here the client is telling us how timer manager ticks map to their
        // game ticks.  The function causes a re-sync (buffer flush)
        //

        //
        // How do we adjust timer manager ticks to get game ticks.
        //

        unsigned int gameAdjust = virtualTicks - timerManagerTicks;

        //
        // How do we adjust the tick count to get timer manager ticks
        //

        unsigned int timerManagerAdjust = radTimeGetMilliseconds() - ::GetTickCount( );

        //
        // Combine the two to adjust::GetTickCount() ticks to game time.
        //

        m_VirtualTimeAdjust = gameAdjust + timerManagerAdjust;

        if ( m_xIOl_Controllers != NULL )
        {
            //
            // Loop through the controllers, telling each one that time has
            // been re-mapped, pass in the new virtual time.
            //

            IRadControllerDirectInput * pIDic2;

            m_xIOl_Controllers->Reset( );

            while ( pIDic2 = reinterpret_cast< IRadControllerDirectInput * >( m_xIOl_Controllers->GetNext( ) ) )
            {
                pIDic2->iVirtualTimeReMapped( ::GetTickCount() + m_VirtualTimeAdjust );
            }
        }
    }
    
    //========================================================================
    // radControllerDirectInput::SetVirtualTime
    //========================================================================

    virtual void SetVirtualTime( unsigned int virtualTime )
    {
        //
        // Setting virtual time drives the system forward.  This could be
        // called internally if the clients buffers size is 0, or from
        // the outside world if the buffer size is > 0.
        //

        if ( m_xIOl_Controllers != NULL )
        {
            m_xIOl_Controllers->Reset( );

            IRadControllerDirectInput * pIDipc2;

            while ( pIDipc2 = reinterpret_cast< IRadControllerDirectInput * >( m_xIOl_Controllers->GetNext( ) ) )
            {
                pIDipc2->iVirtualTimeChanged( virtualTime, m_VirtualTimeAdjust );
            }
        }
    }

    //========================================================================
    // radControllerDirectInput::SetCaptureRate
    //========================================================================

    virtual void SetCaptureRate( unsigned int ms )
    {
        //
        // Simply set the timer to poll faster, this is bound by the frame
        // rate of the game, but hey, keep the frame rate up ...
        //

        if ( m_xITimer != NULL )
        {
            m_xITimer->SetTimeout( ms );
        }

        //
        // If we change the rate packets are being buffered we most adjust
        // the event buffer size for each controller, this call will force
        // a re-calculation using the above time
        //

        if ( m_UsingVirtualTime )
        {
            SetBufferTime( m_EventBufferSize );
        }
    }

    //========================================================================
    // radControllerDirectInput::GetNumberOfControllers
    //========================================================================

    virtual unsigned int GetNumberOfControllers( void )
    {
        return m_xIOl_Controllers->GetSize( );
    }

    //========================================================================
    // radControllerDirectInput::GetNumberOfControllers
    //========================================================================

    virtual IRadController * GetControllerByIndex( unsigned int index )
    {
        return reinterpret_cast< IRadController * >( m_xIOl_Controllers->GetAt( index ) );
    }  

    //========================================================================
    // radControllerDirectInput::CreateControllers
    //========================================================================

    static BOOL __stdcall EnumCreateControllers
    (
        LPCDIDEVICEINSTANCE pDidi, 
        VOID * pVoidThis
    ) 
    {
        //
        // Create a controller class to represent each controller
        // installed on the system.
        //

        rAssert( pDidi != NULL );
        rAssert( pVoidThis != NULL );

        if ( ( pDidi != NULL ) && ( pVoidThis != NULL ) )        
        {
            radControllerSystemDirectInput * pThis = (radControllerSystemDirectInput*) pVoidThis;

            if (pThis->m_xIOl_Controllers != NULL )
            {                
                IRefCount * pObject = static_cast< IRefCount * >(
                                new( g_ControllerSystemAllocator )radControllerDirectInput( pDidi, pThis->m_xIDirectInput8 )
                                                                );
                pThis->m_xIOl_Controllers->AddObject( pObject );
            }

            return DIENUM_CONTINUE;
        }

        //
        // Something went terribly (imposibly) wrong
        //

        return DIENUM_STOP;
    }

    //========================================================================
    // radControllerDirectInput::radControllerDirectInput
    //========================================================================

    radControllerSystemDirectInput
    (
        IRadControllerConnectionChangeCallback* pConnectionChangeCallback,
        radMemoryAllocator allocator
    )
        :
        m_UsingVirtualTime( false ),
        m_VirtualTimeAdjust( 0 ),
        m_EventBufferSize( 16 ),
        m_DefaultConnectionChangeCallback( NULL ),
        m_pWIControllerConnectionChangeCallback2( NULL )
    {
 
        radMemoryMonitorIdentifyAllocation( this, g_nameFTech, "radControllerSystemDirectInput" );
        //
        // Initialize Microsoft's COM, we might be the first system created
        // that uses it.
        //

        ::CoInitialize( NULL );

        //
        // Set the singleton to this one instance
        //
        rAssert( s_pTheDirectInputControllerSystem == NULL );
        s_pTheDirectInputControllerSystem = this;

        g_ControllerSystemAllocator = allocator;
   
        radTimeCreateList( &m_xITimerList, 1, g_ControllerSystemAllocator );

        //
        // We need the instance and main window handle from the platform
        //
        radRef< IRadPlatform > xIWin32Platform;
        xIWin32Platform = radPlatformGet(  );


        //
        // Our controller list
        //
        ::radObjectListCreate( & m_xIOl_Controllers, g_ControllerSystemAllocator );
        rAssert( m_xIOl_Controllers != NULL );

        //
        // Our callback list
        //
        ::radObjectListCreate( & m_xIOl_Callbacks, g_ControllerSystemAllocator );
        rAssert( m_xIOl_Callbacks != NULL );

        if ( xIWin32Platform != NULL )
        {
            //
            // Don't want to implicitly link
            //

            HRESULT hr = ::CoCreateInstance
            (
                CLSID_DirectInput8,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IDirectInput8,
                (void**) & m_xIDirectInput8
            );

            if ( SUCCEEDED( hr ) )
            {
                hr = m_xIDirectInput8->Initialize
                (
                    xIWin32Platform->GetInstanceHandle( ),
                    DIRECTINPUT_VERSION
                );
            }
        
            rAssertMsg( SUCCEEDED( hr ), "Couldn't Create Direct Input Object, this could be because of an invalid main window handle\n" );
            rDebugPrintf("Code: 0x%x, Handle: %x\n", hr, xIWin32Platform->GetInstanceHandle());

            if ( m_xIDirectInput8 != NULL )
            {
                hr = m_xIDirectInput8->EnumDevices
                (
                    DI8DEVCLASS_ALL,
                    EnumCreateControllers,
                    this,
                    DIEDFL_ATTACHEDONLY 
                );

                rAssertMsg( SUCCEEDED( hr ), "IDirectInput8::EnumDevices failed." );
            }

            //
            // Start a timer to poll, and start it now, phew! nasty.
            //
            m_xITimerList->CreateTimer( &m_xITimer, 1, this );

            rAssert( m_xITimer != NULL );
            
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
            // Make sure everything gets initialized with know states/times
            //
            MapVirtualTime( 0, 0 );
            SetBufferTime( 0 );
            SetCaptureRate( 10 );

            //
            // To keep with the current standard, the connection callback should be
            // activated when the controllers are first detected.  Since the controller
            // must already exists in WIN32, we must call the callback manually.
            //
            if( pConnectionChangeCallback && m_xIOl_Controllers != NULL)
            {
                m_xIOl_Controllers->Reset( );

                IRadController * pIRadController;

                while ( pIRadController = reinterpret_cast< IRadController * >( m_xIOl_Controllers->GetNext( ) ) )
                {
                    pConnectionChangeCallback->OnControllerConnectionStatusChange( pIRadController );
                }
            }
        }
    }

    //========================================================================
    // radControllerDirectInput::~radControllerDirectInput
    //========================================================================

    ~radControllerSystemDirectInput( void )
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
        // Make sure the client unregistered all of their callbacks
        //

        if ( m_xIOl_Callbacks != NULL )
        {            
            rAssertMsg( m_xIOl_Callbacks->GetSize() == 0, "Sombody forgot to unregister a controller connection change callback before the controller system was destroyed." );
        }

        //
        // Done with Microsoft's COM, we must distroy our COM objects BEFORE
        // we uninitialize COM or the Release() calls on them will explode.
        //

        m_xIDirectInput8 = NULL;
        m_xIOl_Controllers = NULL;
        m_xITimerList = NULL;

        ::CoUninitialize( );

        g_ControllerSystemAllocator = RADMEMORY_ALLOC_DEFAULT;
        rAssert( s_pTheDirectInputControllerSystem == this );
        s_pTheDirectInputControllerSystem = NULL;
    }

    //========================================================================
    // Data Members
    //========================================================================

    unsigned int m_RefCount;

    bool         m_UsingVirtualTime;
    unsigned int m_VirtualTimeAdjust;
    unsigned int m_EventBufferSize;

    IRadControllerConnectionChangeCallback* m_pWIControllerConnectionChangeCallback2;
    IRadControllerConnectionChangeCallback* m_DefaultConnectionChangeCallback;

    radRef< IDirectInput8 >    m_xIDirectInput8;
    radRef< IRadObjectList >   m_xIOl_Controllers;
    radRef< IRadObjectList >   m_xIOl_Callbacks;
    radRef< IRadTimer >        m_xITimer;
    radRef< IRadTimerList >    m_xITimerList;
};


//============================================================================
// Function: radControllerInitialize
//============================================================================

void radControllerInitialize
(
    IRadControllerConnectionChangeCallback* pConnectionChangeCallback,
    radMemoryAllocator alloc
)
{
    rAssert( s_pTheDirectInputControllerSystem == NULL );

    //
    // Start-up the virtual table.
    //
    for ( unsigned int i = 0; i < 256; i++ )
    {
        s_VirtualKeyToIndex[ i ] = -1;
    }

    new ( alloc ) radControllerSystemDirectInput( pConnectionChangeCallback, alloc );
}

void radControllerTerminate( void )
{
    radRelease( s_pTheDirectInputControllerSystem, NULL );
}

//============================================================================
// Function: radControllerSystemGet
//============================================================================
//
// Use this function to obtain an interface to the controller system object.
//

IRadControllerSystem* radControllerSystemGet( void )
{
    rAssert( s_pTheDirectInputControllerSystem != NULL );

    return( s_pTheDirectInputControllerSystem );

}

//============================================================================
// Function: radControllerSystemGet
//============================================================================
//
// Use this function to drive the processing of the controller system
//
void radControllerSystemService( void )
{
    if( s_pTheDirectInputControllerSystem != NULL )
    {
        s_pTheDirectInputControllerSystem->Service( );
    }
}

#endif // RAD_WIN32