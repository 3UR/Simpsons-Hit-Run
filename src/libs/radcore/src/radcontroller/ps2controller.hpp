//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        PS2Controller.hpp
//
// Subsystem:	Foundation Technologies - PS2 Controller System
//
// Description:	This file contains the header description of the Foundation
//              Technologies controller system. This module provides services
//				for enumerating and managing PS2 controllers and their functions.
//
// Date:    	November 14, 2000
//
// Author:      Michael Wu
//
//=============================================================================

#ifndef PS2CONTROLLER_HPP
#define PS2CONTROLLER_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <eekernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <eeregs.h>
#include <sifdev.h>
#include <sifrpc.h>
#include <libpad.h>
#include <libgraph.h>
#include <libpkt.h>
#include <libdma.h>
#include <libdev.h>
#include <string.h>
#include <radplatform.hpp>
#include <radobject.hpp>
#include <radobjectlist.hpp>
#include <raddebug.hpp>
#include <radtime.hpp>
#include <radmemory.hpp>
#include <radcontroller.hpp>
#include <radstring.hpp>
#include "radcontrollerbuffer.hpp"

#include <liblgdev.h>

#define RADCORE_PS2EE_MTAP

#ifdef RADCORE_PS2EE_MTAP
	#include <libmtap.h>
#endif // RADCORE_PS2EE_MTAP

//=============================================================================
// Definitions
//=============================================================================

#define CL_MAXIMUM_PORTS            2
#define CL_MAXIMUM_SLOTSONPORT      4
#define CL_MAXIMUM_SLOTS            ( CL_MAXIMUM_PORTS * CL_MAXIMUM_SLOTSONPORT )

#define CL_MAXIMUM_CONTROLLERMODES  5
#define CL_MAXIMUM_ACTUATORS        6

#define CL_ID_DUALSHOCK2			0x79
#define CL_ID_DUALSHOCK				0x73
#define CL_ID_DIGITAL				0x41

//Wheel defines
#define CL_MAX_WHEELS               2
#define CL_MAX_CONNECTED_WHEELS     8
#define CL_DEVICE_FLAG_CHECKED      0x80000000

//=============================================================================
// Forward declarations
//=============================================================================

class rControllerSystem2;
class iControllerLocation2;
class rController2;
class iControllerOutputHandler2;
class rControllerOutputPoint2;
class rControllerInputPoint2;
class LGDevWheelController;

//=============================================================================
// Static variables
//=============================================================================

static const char* ControllerNames[]     = { "PsxDigital", "PsxDualShock", "PsxDualShock2" };
static const char* ClassificationTypes[] = { "Joystick" };
static const char* InputPointTypes[]     = { "Button", "AnalogButton", "XAxis", "YAxis", "POV" };

//
// Digital Input Points
//
static const char* PsxDigitalInputPoints[] =
{
    "Select", "Start", "DPadUp", "DPadRight", "DPadDown", "DPadLeft", "L2", "R2", "L1", "R1", "Triangle", "Circle", "X", "Square"
};

static const int PsxDigitalInputPointAttributes[][14] = 
{
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0}, // Button Type index
    {   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3}, // Data Offset into buffer (byte)
    {0x01,0x08,0x10,0x20,0x40,0x80,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80}, // Mask 
};

//
// DualShock Input Points
//
static const char* PsxDualShockInputPoints[] =
{
    "Select", "Start", "DPadUp", "DPadRight", "DPadDown", "DPadLeft", "L2", "R2", "L1", "R1", "Triangle", "Circle", "X", "Square",
    "L3", "R3", "RightStickX", "RightStickY", "LeftStickX", "LeftStickY"
};

static const int PsxDualShockInputPointAttributes[][20] =
{
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   3,   2,   3},
    {   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   2,   2,   4,   5,   6,   7},
    {0x01,0x08,0x10,0x20,0x40,0x80,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x02,0x04,0xFF,0xFF,0xFF,0xFF}
};

//
// DualShock Output Points
//
static const char* PsxDualShockOutputPoints[] =
{
    "SmallMotor", "LargeMotor"
};

static const char* PsxDualShockOutputPointAttributes[] =
{
    "Digital", "Analog"
};

//
// DualShock2 Input Points
//
static const char* PsxDualShock2InputPoints[] =
{
    "Select", "Start", "DPadUp", "DPadRight", "DPadDown", "DPadLeft", "L2", "R2", "L1", "R1", "Triangle", "Circle", "X", "Square",
    "L3", "R3", "RightStickX", "RightStickY", "LeftStickX", "LeftStickY",
    "AnalogDPadRight", "AnalogDPadLeft", "AnalogDPadUp", "AnalogDPadDown", "AnalogTriangle", "AnalogCircle", "AnalogX", "AnalogSquare", "AnalogL1", "AnalogR1", "AnalogL2", "AnalogR2"
};

static const int PsxDualShock2InputPointAttributes[][32] =
{
    {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   2,   3,   2,   3,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1},
    {   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   2,   2,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19},
    {0x01,0x08,0x10,0x20,0x40,0x80,0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x02,0x04,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}
};

struct LGDevInputPointData
{
    const char * m_pType;
    const char * m_pName;
    char m_Offset;
    unsigned int m_Mask;
};

//These correspond to offsets in the lgDevPosition structure
const unsigned char X1 = 0x01;
const unsigned char Y1 = 0x02;
const unsigned char Accelerator = 0x07;
const unsigned char Brake = 0x08;
const unsigned char Buttons = 0x0B;
const unsigned char POV0 = 0x09;

const unsigned int NEUTRAL     = 0xFFFFFFFF;
const unsigned int UP          = ( 1 << 0 );
const unsigned int UP_RIGHT    = ( 1 << 1 );
const unsigned int RIGHT       = ( 1 << 2 );
const unsigned int DOWN_RIGHT  = ( 1 << 3 );
const unsigned int DOWN        = ( 1 << 4 );
const unsigned int DOWN_LEFT   = ( 1 << 5 );
const unsigned int LEFT        = ( 1 << 6 );
const unsigned int UP_LEFT     = ( 1 << 7 );

static LGDevInputPointData LGDevWheelAInputPoints[] =
{
    { InputPointTypes[3], "Wheel"   , X1, 0 },
    { InputPointTypes[2], "Combined", Y1, 0 },
    { InputPointTypes[1], "Gas"     , Accelerator, 0 },
    { InputPointTypes[1], "Brake"   , Brake, 0 },
    { InputPointTypes[0], "LGL1"      , Buttons, LGBUTTON_BUTTON0 },
    { InputPointTypes[0], "LGR1"      , Buttons, LGBUTTON_BUTTON1 },
    { InputPointTypes[0], "LGX"       , Buttons, LGBUTTON_BUTTON2 },
    { InputPointTypes[0], "LGY"       , Buttons, LGBUTTON_BUTTON3 },
    { InputPointTypes[0], "LGA"       , Buttons, LGBUTTON_BUTTON4 },
    { InputPointTypes[0], "LGB"       , Buttons, LGBUTTON_BUTTON5 }
};

static LGDevInputPointData LGDevWheelBInputPoints[] =
{
    { InputPointTypes[3], "Wheel"   , X1, 0 },
    { InputPointTypes[2], "Combined", Y1, 0 },
    { InputPointTypes[1], "Gas"     , Accelerator, 0 },
    { InputPointTypes[1], "Brake"   , Brake, 0 },
    { InputPointTypes[0], "Square"  , Buttons, LGBUTTON_SQUARE },
    { InputPointTypes[0], "X"       , Buttons, LGBUTTON_CROSS },
    { InputPointTypes[0], "Circle"  , Buttons, LGBUTTON_CIRCLE },
    { InputPointTypes[0], "Triangle", Buttons, LGBUTTON_TRIANGLE },
    { InputPointTypes[0], "Start"   , Buttons, LGBUTTON_START },
    { InputPointTypes[0], "Select"  , Buttons, LGBUTTON_SELECT },
    { InputPointTypes[0], "L1"      , Buttons, LGBUTTON_L1 },
    { InputPointTypes[0], "R1"      , Buttons, LGBUTTON_R1 },
    { InputPointTypes[0], "L2"      , Buttons, LGBUTTON_L2 },
    { InputPointTypes[0], "R2"      , Buttons, LGBUTTON_R2 },
    { InputPointTypes[4], "DPadUp"  , POV0, UP | UP_LEFT | UP_RIGHT},
    { InputPointTypes[4], "DPadDown", POV0, DOWN | DOWN_LEFT | DOWN_RIGHT },
    { InputPointTypes[4], "DPadLeft" , POV0, LEFT | UP_LEFT | DOWN_LEFT },
    { InputPointTypes[4], "DPadRight", POV0, RIGHT | UP_RIGHT | DOWN_RIGHT }
};

struct LGDevControllerPacket
{
    unsigned int time;
    lgDevPosition position;
};

//
// This static is used to manage the single instance of the controller system.
//
static rControllerSystem2* g_theCS = NULL;

//
// For memory allocation.
//
static radMemoryAllocator g_CtlrAllocator;

/*
//
// This structure and proceeding declaration is the buffer used for
// communicating with the IOP.  The buffer itself is 16 bytes (128 bits)
// but each one needs to be 64-byte aligned.  Hence the padding.
//
struct sPadDMABuffer
{
    u_long128 m_PadDMABuffer;		// This is the DMA memory used to communicate with the IOP
    long unusedBuffer1 : 64;		// 48-byte pad to make this struct 64-byte aligned
    long unusedBuffer2 : 64;
    long unusedBuffer3 : 64;
    long unusedBuffer4 : 64;
    long unusedBuffer5 : 64;
    long unusedBuffer6 : 64;
};
*/

struct sPadDMABuffer
{
    u_long128 m_PadDMABuffer[scePadDmaBufferMax] __attribute__((aligned (64)));
};

static sPadDMABuffer g_ControllerPadDMABuffer[ CL_MAXIMUM_SLOTS ] __attribute__ ((aligned(64)));

//=============================================================================
// Internal Interface iIControllerLocation2
//=============================================================================

struct iIControllerLocation2
    :
    public IRefCount
{
    //
    // These are the states a controller can be in during initialization ( used by
    // iControllerLocation2 and rControllerSystem2 ).
    //
    enum LocationState {
        CL_STATE_DISCONNECTED,
        CL_STATE_STARTINIT,
        CL_STATE_ALMOSTREADY,           
        CL_STATE_READY,                 
        CL_STATE_WAITASYNC,             
        CL_STATE_REQUESTMODE,           
        CL_STATE_REQUESTMODEDONE,       
        CL_STATE_REQUESTPRESSURE,       
        CL_STATE_REQUESTPRESSUREDONE,   
        CL_STATE_REQUESTACTUATOR,       
        CL_STATE_REQUESTACTUATORDONE   
    };

    virtual int GetPort( void ) = 0;
    virtual int GetSlot( void ) = 0;
    virtual const char* GetName( void ) = 0;
    virtual void GetNameBString( IRadString** locationName ) = 0;
	virtual void AttachController( rController2* controller ) = 0;
    virtual void GetAttachedController( rController2** controller ) = 0;
	virtual void SetControllerState( LocationState state ) = 0;
	virtual LocationState GetControllerState( void ) = 0;
	virtual void SetControllerPrevState( LocationState state ) = 0;
	virtual LocationState GetControllerPrevState( void ) = 0;
	virtual void SetTotalNumModes( int numModes ) = 0;
	virtual int GetTotalNumModes( void ) = 0;
	virtual void SetCurrentMode( int mode ) = 0;
	virtual int GetCurrentMode( void ) = 0;
	virtual void SetModeQueryResult( int modeValue, int modeIndex ) = 0; 
	virtual int GetModeQueryResult( int modeIndex ) = 0;
	virtual void SetActAlign( int actValue, int indx ) = 0;
	virtual void SetActDirect( int actValue, int indx ) = 0;
	virtual unsigned char* GetActAlignBuffer( void ) = 0;
	virtual unsigned char* GetActDirectBuffer( void ) = 0;
    virtual bool ReconnectIfControllerMatches( int id, rController2** controller, unsigned int bufferSize ) = 0;    
    virtual int GetNumActuators( void ) = 0;
    virtual void SetNumActuators( int numActuators ) = 0;
};

//=============================================================================
// Internal Interface iIController2
//=============================================================================

struct iIController2
    :
    public IRadController
{
    virtual void iVirtualTimeChanged( void ) = 0;
    virtual void iPoll( unsigned int timerManagerMs ) = 0;
    virtual void iSetBufferTime( unsigned int bufferSize ) = 0;
    virtual void Disconnect( void ) = 0;
    virtual void Connect( void ) = 0;
};

//=============================================================================
// Internal Callback Interface iIControllerOutputCallback2
//=============================================================================

struct iRIControllerOutputPointCallback2
    :
    public IRefCount
{
    virtual void iOnControllerGainChange( IRadControllerOutputPoint* pOutputPoint ) = 0;
};

//=============================================================================
// Internal Interface iIControllerInputPoint2
//=============================================================================

struct iIControllerInputPoint2
    :
    public IRadControllerInputPoint
{
	virtual void iInitialize( unsigned char * buffer ) = 0;
    virtual bool iHandleEvent( unsigned char* buffer, unsigned int time ) = 0;
};


struct LGDevDescription
{
    lgDevDeviceDesc desc;
    LGDevWheelController* controller;
};

//=============================================================================
// class rControllerSystem2
//
// The controller system maintains a set of locations (which have a one-to-one 
// relationship with controllers). It manages polling of the devices between
// intervals.
//=============================================================================

class rControllerSystem2 
    : 
    public IRadControllerSystem, 
    public IRadTimerCallback,
    public radObject
{
    public:
		rControllerSystem2( IRadControllerConnectionChangeCallback* pConnectionChangeCallback );
		virtual ~rControllerSystem2( void );

		void AddRef( void );
		void Release( void ); 
        #ifdef RAD_DEBUG
        void Dump( char* pStringBuffer, unsigned int bufferSize );
        #endif
        
        unsigned int GetNumberOfControllers( void );
        IRadController * GetControllerByIndex( unsigned int indx );
        IRadController * GetControllerAtLocation( const char* pLocation );
        void SetBufferTime( unsigned int milliseconds );
        void MapVirtualTime( unsigned int timerManagerMs, unsigned int virtualTicks );
        void SetVirtualTime( unsigned int virtualTicks );
        void SetCaptureRate( unsigned int timerManagerMs );
        void RegisterConnectionChangeCallback( IRadControllerConnectionChangeCallback* pCallback );
        void UnRegisterConnectionChangeCallback( IRadControllerConnectionChangeCallback* pCallback );
        void OnTimerDone( unsigned int elapsedTime, void * pUserData );
        unsigned int iGetVirtualTime( void );
        unsigned int iGetVirtualTimeAdjust( void );

        void Service( void );

    private:
        void InitializeLocations( void );
        void SetupControllers( void );
		void CreateController( int id, iControllerLocation2* location, rController2** pprController );
        
        //
        // Internal states that a controller can be in.
        //
        void _StartControllerInitialization( iControllerLocation2* location, int state );
        void _RequestControllerMode( iControllerLocation2* location );
        void _RequestControllerModeDone( iControllerLocation2* location, int state );
        void _RequestControllerActuator( iControllerLocation2* location );
        void _RequestControllerActuatorDone( iControllerLocation2* location, int state );
        void _RequestControllerPressure( iControllerLocation2* location );
        void _RequestControllerPressureDone( iControllerLocation2* location, int state );
        void _CheckControllerAsyncProcessing( iControllerLocation2* location );
        void _ControllerAlmostReady( iControllerLocation2* location, int state, int time );
        void _ControllerReady( iControllerLocation2* location, int state, int time );

		int m_ReferenceCount;

        //
        // Set by client to alert of connection change.
        //
        IRadControllerConnectionChangeCallback* m_StatusCallback;

        //
        // The internal list of locations. Each location can be mapped to one
		// controller. The iControllerLocation2 tells us what the state of
		// the controller is in, which is useful during initialization of the
		// controller. At that time, we generate requests to determine what 
		// features the controller device has.
        //
        iControllerLocation2* m_LocationTable[ CL_MAXIMUM_SLOTS ];

        //This is where we're going to store USB wheel controller info.
        //We're going to add some data to the flags field to mark info we need to know.
        //CL_DEVICE_FLAG_CHECKED tells us if during an enumeration we checked this guy
        LGDevDescription m_WheelDeviceDesc[ CL_MAX_WHEELS ];

        //
        // The controller system also maintains a list of controllers so that it
        // can provide enumeration easily.
        //
        ref< IRadObjectList > m_xIOl_Controllers;
        
        //
        // Used for polling.
        //
        ref< IRadTimerList > m_xITimerList;
		ref< IRadTimer > m_xIWorkTimer;

        //
        // The difference between timer manager time and virtual time.
        //
        int m_VirtualTimeAdjust;

        //
        // Whether or not we are buffering.
        //
        bool m_UsingVirtualTime;

        //
        // The current virtual time.
        //
        int m_VirtualTime;

        //
        // The size of the buffer that each controller manages. This is related
        // to the time of polling.
        //
        unsigned int m_BufferingTime;
};

//=============================================================================
// class rControllerLocation2
//
// This class has a one-to-one relationship with a controller. It has a bunch
// of member variables to hold the state of the controller during the 
// controller's initialization.  The variables are then used when the controller
// is finally ready to be created.  The idea of the location is a logical
// division between the controller system and controller.
//=============================================================================

class iControllerLocation2 
    : 
    public iIControllerLocation2, 
    public radObject
{
    public:
        iControllerLocation2( int port, int slot ); 
        virtual ~iControllerLocation2( void );

		void AddRef( void );
		void Release( void ); 

        #ifdef RAD_DEBUG
        void Dump( char* pStringBuffer, unsigned int bufferSize );
        #endif
 
        int GetPort( void );
        int GetSlot( void );
        const char* GetName( void );
        void GetNameBString( IRadString** locationName );
        void AttachController( rController2* controller );
        void GetAttachedController( rController2** controller );
		void SetControllerState( LocationState state );
		LocationState GetControllerState( void );
		void SetControllerPrevState( LocationState state );
		LocationState GetControllerPrevState( void );
		void SetTotalNumModes( int numModes );
		int GetTotalNumModes( void );
		void SetCurrentMode( int mode );
		int GetCurrentMode( void );
		void SetModeQueryResult( int modeValue, int modeIndex ); 
		int GetModeQueryResult( int modeIndex );
		void SetActAlign( int actValue, int indx );
		void SetActDirect( int actValue, int indx );
		unsigned char* GetActAlignBuffer( void );
		unsigned char* GetActDirectBuffer( void );
        bool ReconnectIfControllerMatches( int id, rController2** controller, unsigned int bufferSize );   
        int GetNumActuators( void );
        void SetNumActuators( int numActuators );

    private:
        int m_ReferenceCount;
        int m_Port;
        int m_Slot;
    	rController2* m_rController;
        IRadString* m_LocationName;

        //
        // The current state of initialization of a contoller before the object is created.
		// This state is used by our state machine which determines what kind of controller
		// is attached..
        //
        LocationState m_nState;

		//
        // The previous state is used as a fallback when we move into a controller 
		// initialization state that fails.
        //
        LocationState m_nPreviousState;
		
        //
		// The total number of modes for the connected controller.
        //
        int m_nNumModes; 
		
        //
		// The current mode that the controller is in.
        //
        int m_nMode;

		//
        // The result values from querying what the connected controller supports.
        //
        int m_nModes[ CL_MAXIMUM_CONTROLLERMODES ]; 
		
        //
        // The number of output points available.
        //
        int m_NumActuators;

		//
        // The following are used as feedback ( output points ) buffers for calls to
		// the sony libraries.
		//
        unsigned char m_cActuatorAlignment[ CL_MAXIMUM_ACTUATORS ];
		unsigned char m_cActuatorDirective[ CL_MAXIMUM_ACTUATORS ];
};
    
//=============================================================================
// class rController2
//=============================================================================

class rController2 
    : 
    public iIController2,
    public radObject
{
    public:
		rController2(int port, int slot, int id, IRadString* locationName, unsigned char* pActAlignBuffer, unsigned char* pActDirectBuffer, int numActuators, unsigned int bufferSize );
		virtual ~rController2( void );

		void AddRef( void );
		void Release( void ); 

        #ifdef RAD_DEBUG
        void Dump( char* pStringBuffer, unsigned int bufferSize );
        #endif

        void Disconnect( void );
        void Connect( void );
        bool IsConnected( void );
	    const char* GetClassification( void );
        const char* GetType( void );
        unsigned int GetNumberOfInputPointsOfType( const char* pType );
        unsigned int GetNumberOfOutputPointsOfType( const char* pType );
        IRadControllerInputPoint  * GetInputPointByTypeAndIndex( const char* pType, unsigned int indx );
        IRadControllerOutputPoint * GetOutputPointByTypeAndIndex( const char* pType, unsigned int indx );
        IRadControllerInputPoint  * GetInputPointByName( const char* pName );
        IRadControllerOutputPoint * GetOutputPointByName( const char* pName );
	    const char* GetLocation( void );

        unsigned int GetNumberOfInputPoints( void );
        IRadControllerInputPoint * GetInputPointByIndex( unsigned int indx );
        unsigned int GetNumberOfOutputPoints( void );
        IRadControllerOutputPoint * GetOutputPointByIndex( unsigned int indx );

        unsigned char* GetDataBuffer( void );

        void iVirtualTimeChanged( void );
        void iPoll( unsigned int timerManagerMs );
        void iSetBufferTime( unsigned int bufferSize );

        int GetId( void );

    private:
		int m_ReferenceCount;
        
        int m_Port;
        int m_Slot;
        int m_Id;
        bool m_Connected;
        int m_TypeId;
        int m_NumActuators;

        //
        // The description of which output points are available for this
        // controller.
        //
        unsigned char m_cActuatorAlignment[ CL_MAXIMUM_ACTUATORS ];

        //
        // The data buffer is a chunk of memory that contains the state of each
        // input point in raw format. (used in the sony function calls)
        //
		unsigned char m_DataBuffer[32];

        //
        // This is the name of the location object to which we are mapped.
        //
        IRadString* m_LocationName;

        //
        // Each controller has a buffer that holds states which. This allows us
        // to buffer controller data to handle later.
        //
        IRadControllerBuffer* m_pIControllerBuffer2;

        //
        // The list of input points.
        //
        ref< IRadObjectList > m_xIOl_InputPoints;

        //
        // This list of output points.
        //
        ref< IRadObjectList > m_xIOl_OutputPoints;

        //
        // Used to prevent circular referencing between controller and
        // input points.
        //
        iControllerOutputHandler2* m_pOutputHandler;
};

//=============================================================================
// class rControllerOutputHandler2
//
// This is used to prevent circular referencing problems between the controller 
// and the output points.
//=============================================================================

class iControllerOutputHandler2
    :
    public iRIControllerOutputPointCallback2,
    public radObject
{
    public:
        iControllerOutputHandler2( int port, int slot, unsigned char* pActDirectBuffer );
        virtual ~iControllerOutputHandler2( void );

	    void AddRef( void );
	    void Release( void ); 

        #ifdef RAD_DEBUG
        void Dump( char* pStringBuffer, unsigned int bufferSize );
        #endif

        void Connect( void );
        void Disconnect( void );
        void iOnControllerGainChange( IRadControllerOutputPoint* pOutputPoint );

    private:
        int m_ReferenceCount;
        bool m_Connected;
        int m_Port;
        int m_Slot;

        unsigned char m_cActuatorDirective[ CL_MAXIMUM_ACTUATORS ];
};

//=============================================================================
// class rControllerOutputPoint2
//=============================================================================
  
class rControllerOutputPoint2 
    : 
    public IRadControllerOutputPoint, 
    public radObject
{
    public:
		rControllerOutputPoint2( iRIControllerOutputPointCallback2* handler, IRadString* name, IRadString* type );
		virtual ~rControllerOutputPoint2( void );

		void AddRef( void );
		void Release( void ); 

        #ifdef RAD_DEBUG
        void Dump( char* pStringBuffer, unsigned int bufferSize );
        #endif

        const char* GetName( void );
	    const char* GetType( void );
        float GetGain( void );
        void SetGain( float value );

    private:
		int m_ReferenceCount;
        float m_Value;

        IRadString* m_Name;
        IRadString* m_Type;

        iRIControllerOutputPointCallback2* m_Callback;
};

//=============================================================================
// class rControllerInputPoint2
//=============================================================================
  
class rControllerInputPoint2 
    : 
    public iIControllerInputPoint2,
    public radObject
{
    public:
		rControllerInputPoint2( IRefCount* pIObject_Controller, IRadString* name, int inputPointTypeIndex, unsigned int dataOffset, unsigned int mask );
		virtual ~rControllerInputPoint2( void );

		void AddRef( void );
		void Release( void ); 

        #ifdef RAD_DEBUG
        void Dump( char* pStringBuffer, unsigned int bufferSize );
        #endif

        const char* GetName( void );
	    const char* GetType( void );
        void SetTolerance( float percentage ); 
        float GetTolerance( void );
        void RegisterControllerInputPointCallback( IRadControllerInputPointCallback* pCallback, unsigned int userData );
        void UnRegisterControllerInputPointCallback( IRadControllerInputPointCallback* pCallback );
        float GetCurrentValue( unsigned int *pTime );
        void SetRange( float min, float max );
        void GetRange( float* pMin, float* pMax );
        
        bool iHandleEvent( unsigned char* buffer, unsigned int time );
		void iInitialize( unsigned char * buffer );

    private:

		//
		// Internal translation from buffer value to input point value
		//
		float CalculateNewValue( unsigned char * buffer );

		int m_ReferenceCount;
        //
        // The offset byte in the buffer that we can find our data.
        //
        unsigned int m_Offset;
        //
        // The mask that, once applied to the offset byte, produces the value.
        // 
        unsigned int m_Mask;

        float m_Value;
        unsigned int m_Time;
        float m_Tolerance;
        float m_MinRange;
        float m_MaxRange;

        IRadControllerInputPointCallback* m_pWIControllerInputPointCallback2;
        unsigned int m_UserData;

        IRadString* m_Name;
        
        //
        // The type of this input point. Holding an int is a little more efficient
        // then holding strings and using string comparisons.
        //
        unsigned int m_InputPointTypeIndex;

        IRadString* m_Type;
};

class LGDevWheelInputPoint : public IRadControllerInputPoint, public radRefCount
{
public:

    IMPLEMENT_REFCOUNTED( "LGDevWheelInputPoint" )

    LGDevWheelInputPoint( const LGDevInputPointData* data );
    ~LGDevWheelInputPoint( void );

    void Initialize( lgDevPosition* position );
    bool HandleEvent( lgDevPosition* position, unsigned int time );

    //
    // This is the name of the input point, this is controller specific.
    //

    const char * GetName( void ) { return m_Data->m_pName; };
    
    //
    // See table (below) for a list of valid input point types.
    //
 
    const char * GetType( void ) { return m_Data->m_pType; };

    //
    // Allows you to set what percentage of the input point's total range
    // is considered enough of a change to warrant calling back.
    //

    void  SetTolerance( float percentage ); // 0.0 --> 1.0
    float /* percentage */ GetTolerance( void ) { return m_Tolerance; };

    //
    // Register a notification callback to receive notification that
    // the input point value has changed.
    // 
           
    void RegisterControllerInputPointCallback
    (
        IRadControllerInputPointCallback * pCallback,
        unsigned int userData = 0
    );

    void UnRegisterControllerInputPointCallback
    (
        IRadControllerInputPointCallback * pCallback
    );

    //
    // The current value of the input point.  This depends, somewhat
    // on what the device is, and the symantics of the point, but,
    // you can treat all input points as a value from some min to some
    // max.
    //
    // pTime represents the length of time that this input point has
    // been in this state, int can be null if you do not need this
    // value

    float GetCurrentValue( unsigned int * pTime = NULL );

    //
    // This allows you to re-map the range of the device to a more
    // convenient range for game calculations.
    //

    void  SetRange( float min, float max ) { m_MinRange = min; m_MaxRange = max; };
    void  GetRange( float * pMin, float * pMax );

private:
    
    float CalculateNewValue( lgDevPosition* position );
    
    float m_Value;
    float m_MinRange;
    float m_MaxRange;
    float m_Tolerance;

    unsigned int m_TimeInState;
    unsigned int m_TimeOfStateChange;

    const LGDevInputPointData* m_Data;

    ref< IRadObjectList > m_xIOl_Callbacks;
};

class LGDevWheelOutputPoint : public IRadControllerOutputPoint, public radRefCount
{
public:
    enum Command { IDLE, START, STOP };

    IMPLEMENT_REFCOUNTED( "LGDevWheelOutputPoint" )
	LGDevWheelOutputPoint(  const char * pName, unsigned char type  );
	virtual ~LGDevWheelOutputPoint( void );

    const char* GetName( void ) { return m_pName; };
    const char* GetType( void ) { return "ForceEffect"; };
    float GetGain( void );
    void SetGain( float value );

    const LGForceEffect* GetEffect() const;
    void UpdateEffect( const LGForceEffect* effect );
    void Start() { m_eCommand = START; };
    void Stop() { m_eCommand = STOP; };
    void Update( unsigned int device );

private:
    const char * m_pName;
    int m_uEffectID;

    LGForceEffect m_Effect;
    bool m_bDirty;
    
    Command m_eCommand;
};

class LGDevWheelController : public iIController2, public radRefCount
{
public:

    IMPLEMENT_REFCOUNTED( "LGDevWheelController" )

    //Contructor
    LGDevWheelController( lgDevDeviceDesc* desc );
    ~LGDevWheelController( void ) {};

    void iVirtualTimeChanged( void );
    void iPoll( unsigned int timerManagerMs );
    void iSetBufferTime( unsigned int bufferSize );

    //
    // Is this controller currently connected to the system.
    //

    void Connect( void );
    void Disconnect( void );


    bool IsConnected( void );

    //
    // See table (below) for known controller types on various platforms.
    //

    const char * GetType( void ) { return "Logitech"; };       
    
    // 
    // See table (below) for known classifications.
    //

    const char * GetClassification( void ) { return "Wheel"; };

    //
    // These functions determine the capability of the controller device,
    // See table (below) for known input/output point types.  Use thes functions
    // to treat controllers in a generic way, or to display mapping
    // options to the user.
    //

    unsigned int GetNumberOfInputPointsOfType( const char * pType );

    //
    // Using the above function call you know that there are say, 5 "Analog
    // Button"s on the controller.  So call this function to get the input
    // point of that index.  This alows you to treat controllers in a generic
    // way.  For example, if you just whant the defualt X axis no matter what
    // device is in use you can call GetInputPointByIndex( "X-Axis", 0,
    // & pIControllerInputPoint2 )...
    //

    unsigned int GetNumberOfOutputPointsOfType( const char * pType );


    IRadControllerInputPoint * GetInputPointByTypeAndIndex
    (
        const char * pType,
        unsigned int indx
    );

    IRadControllerOutputPoint * GetOutputPointByTypeAndIndex
    (
        const char * pType,
        unsigned int indx
    );

    //
    // If you know exactly what you are looking for: "Button1", "Triangle",
    // "R1", etc. You can get the point directly, without enumerating.  This
    // is the same name returned by InputPoint::GetName( ) or 
    // OutputPoint::GetName( ).  This is useful for higher level logical systems 
    // that will have controller templates and event mapping.
    //

    IRadControllerInputPoint * GetInputPointByName
    (
        const char * pName
    );

    virtual IRadControllerOutputPoint * GetOutputPointByName
    (
        const char * pName
    );

    //
    // Call this function to get the pysical location to which this controller
    // is connected. See below for valid location names of the different platforms.
    //

    const char * GetLocation( void ) { return m_xIString_Location->GetChars(); };

    //
    // If you want a complete picture of everything on the device.
    // This is useful for displaying mapping options to the user.
    //

    IRadControllerInputPoint* GetInputPointByIndex( unsigned int indx );
    unsigned int GetNumberOfInputPoints( void ) { return m_xIOl_InputPoints->GetSize( ); };

    IRadControllerOutputPoint* GetOutputPointByIndex( unsigned int indx );
    unsigned int GetNumberOfOutputPoints( void ) { return m_xIOl_OutputPoints->GetSize( ); };

private:
    lgDevDeviceDesc                   m_DeviceDesc;

    ref< IRadObjectList >             m_xIOl_InputPoints;
    ref< IRadObjectList >             m_xIOl_OutputPoints;

    ref< IRadString >                 m_xIString_Location;
    ref< IRadControllerBuffer >       m_xIControllerBuffer;

    const char*                       m_Type;
    const char*                       m_Classification;
};

#endif // _RPS2_CONTROLLER2_HPP_