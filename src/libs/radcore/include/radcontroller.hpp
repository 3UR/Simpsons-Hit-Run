//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        radcontroller.hpp
//
// Subsystem:	Foundation Technologies - Controller
//
// Description:	This file contains all definitions and interfaces required to
//              interact with the controller system.
//              The Controller System manages a collection of Controller devices.
//              Each Controller device contains a set of Input (ie buttons) or 
//              Output (ie rumble pack) Points.
//
//
// Revisions:   Mar 4, 2001        Creation
//
//=============================================================================

#ifndef	RADCONTROLLER_HPP
#define RADCONTROLLER_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radobject.hpp>                     
#include <radmemory.hpp>                     

//=============================================================================
// Forward Declarations
//=============================================================================

struct IRadControllerSystem;
struct IRadControllerInputPointCallback;
struct IRadControllerInputPoint;
struct IRadControllerOutputPoint;
struct IRadController;
struct IRadControllerConnectionChangeCallback;

#ifdef RAD_WIN32
struct DIEFFECT;
#endif
//=============================================================================
// Functions
//=============================================================================

//
// Use these functions to initialize and terminate the controller system.
// Must frame all other calls.
//
void radControllerInitialize( IRadControllerConnectionChangeCallback* pConnectionChangeCallback = NULL, radMemoryAllocator alloc = RADMEMORY_ALLOC_DEFAULT ); 
void radControllerTerminate( void );

//
// Use this function to obtain an interface to the controller system object. Remeber
// to add ref if you hold onto the interface.
//
IRadControllerSystem* radControllerSystemGet( void );

//
// In order for the controller system to operate, this function must be periodically
// invoked by the games main loop.
//
void radControllerSystemService( void );

//=============================================================================
// Interface: IRadControllerSystem
//=============================================================================
//
// Description: The Controller System manages all of the controllers. The user
//              can register connection callbacks which indicate when a 
//              controller is physically plugged or removed from the console 
//              system. The Controller System also allows the user to buffer 
//              controller inputs. This is useful in cases where the game 
//              is busy with something and isn't able to poll for an input 
//              change. The game can later "catch" up on the changes, by
//              requesting from the Controller System for input states that 
//              were buffered.
//
//              While being created, the Controller System will also initialize 
//              all attached controllers synchronously.
//
//=============================================================================

struct IRadControllerSystem
    :
    public IRefCount
{
    //
    // Enumerate through the attached controllers
    //

    virtual unsigned int GetNumberOfControllers( void ) = 0;
    virtual IRadController * GetControllerByIndex( unsigned int indx) = 0;


    //
    // Call this function to get a controller at a specific location.
    // If there is no controller connected at the requested location
    // during time of request, NULL is returned through ppIController2.
    //
    // XBox :"Port0\Slot0"  --> "Port3\Slot2"
    // PC  : "Joystick0"    --> "Joystick[n]" | "Mouse0" | "Keyboard0"
    //

    virtual IRadController * GetControllerAtLocation( const char * pLocation ) = 0;

    //
    // You can use buffered input, in which case the controller system will
    // run in "simulation time".  SetEventBufferTime sets how long the
    // controller system will buffer events for.  By default this value
    // is zero, meaning the controller system will not buffer events other
    // than the current state.
    //
    // If you set this value to 16, the controller will buffer events for
    // at least 16ms.
    //
    // Making a call to this will reset the buffer, thus, invalidating 
    // any states currently stored.
    //

    virtual void SetBufferTime( unsigned int milliseconds ) = 0;

    //
    // Use this function to map the timer manager ticks to your own
    // virtual measure.
    //

    virtual void MapVirtualTime( unsigned int timerManagerMs, unsigned int virtualTicks  ) = 0;

    // 
    // Calling this function will cause the controller system to report its
    // state and times based on this virtual time.
    //
    // The controller system may have buffered events in real time that have
    // not yet been broadcast or reported because the game simulation hasn't
    // caught up to real time.
    //

    virtual void SetVirtualTime( unsigned int virtualTicks ) = 0;

    //
    // Set the capture rate in milliseconds
    //
    
    virtual void SetCaptureRate( unsigned int ms ) = 0;

    //
    // Register to be notified when a controller is connected or
    // disconnected.
    //

    virtual void RegisterConnectionChangeCallback
    (
        IRadControllerConnectionChangeCallback * pCallback
    ) = 0;

    //
    // UnRegister the notification made in the above call.
    //

    virtual void UnRegisterConnectionChangeCallback
    (
        IRadControllerConnectionChangeCallback * pCallback
    ) = 0;
};

//=============================================================================
// Interface: IRadControllerConnectionChnageCallback
//=============================================================================
//
// Description: This callback interface is used with the ControllerSystem to
//              determine when/if a controller is physically connected or 
//              removed.
//
//=============================================================================

struct IRadControllerConnectionChangeCallback
{
    //
    // Implement this interface to be called back when a controller is
    // connected or disconnected from the system.  Call functions on the
    // controller to dermine a course of action.  For example the following
    // is an example something you might do as a result of this callback.
    //
    // if ( ( pIController == pIPlayer1Controller ) && 
    //      ( pIController->IsConnected( ) == false )
    // {
    //      Display( "Please plug controller back into: %s\n", 
    //               pIController->GetLocation() );
    // }
    //

    virtual void OnControllerConnectionStatusChange( IRadController * pIController ) = 0;
};


//=============================================================================
// Interface: IRadController
//=============================================================================
// 
// Description: The Controller contains a set of input points. The Controller
//              can be:     - queried for these input points
//                          - queried for the physical location of where it it
//                            attached on the console
//                          - queried for its information (type, classification)
//
//              Controller values will not be updated unless clients of the 
//              controller hold a reference to it and have called AddRef()
//
//=============================================================================

struct IRadController
    :
    public IRefCount
{
    //
    // Is this controller currently connected to the system.
    //

    virtual bool IsConnected( void ) = 0;

    //
    // See table (below) for known controller types on various platforms.
    //

    virtual const char * GetType( void ) = 0;       
    
    // 
    // See table (below) for known classifications.
    //

    virtual const char * GetClassification( void ) = 0;

    //
    // These functions determine the capability of the controller device,
    // See table (below) for known input/output point types.  Use thes functions
    // to treat controllers in a generic way, or to display mapping
    // options to the user.
    //

    virtual unsigned int GetNumberOfInputPointsOfType( const char * pType ) = 0;

    //
    // Using the above function call you know that there are say, 5 "Analog
    // Button"s on the controller.  So call this function to get the input
    // point of that index.  This alows you to treat controllers in a generic
    // way.  For example, if you just whant the defualt X axis no matter what
    // device is in use you can call GetInputPointByIndex( "X-Axis", 0,
    // & pIControllerInputPoint2 )...
    //

    virtual unsigned int GetNumberOfOutputPointsOfType( const char * pType ) = 0;


    virtual IRadControllerInputPoint * GetInputPointByTypeAndIndex
    (
        const char * pType,
        unsigned int indx
    ) = 0;

    virtual IRadControllerOutputPoint * GetOutputPointByTypeAndIndex
    (
        const char * pType,
        unsigned int indx
    ) = 0;

    //
    // If you know exactly what you are looking for: "Button1", "Triangle",
    // "R1", etc. You can get the point directly, without enumerating.  This
    // is the same name returned by InputPoint::GetName( ) or 
    // OutputPoint::GetName( ).  This is useful for higher level logical systems 
    // that will have controller templates and event mapping.
    //

    virtual IRadControllerInputPoint * GetInputPointByName
    (
        const char * pName
    ) = 0;

    virtual IRadControllerOutputPoint * GetOutputPointByName
    (
        const char * pName
    ) = 0;

    //
    // Call this function to get the pysical location to which this controller
    // is connected. See below for valid location names of the different platforms.
    //

    virtual const char * GetLocation( void ) = 0;

    //
    // If you want a complete picture of everything on the device.
    // This is useful for displaying mapping options to the user.
    //

    virtual unsigned int GetNumberOfInputPoints( void ) = 0;
    virtual IRadControllerInputPoint * GetInputPointByIndex( unsigned int indx ) = 0;

    virtual unsigned int GetNumberOfOutputPoints( void ) = 0;
    virtual IRadControllerOutputPoint * GetOutputPointByIndex( unsigned int indx ) = 0;
};

//=============================================================================
// Interface: IRadControllerOutputPoint
//=============================================================================

struct IRadControllerOutputPoint
    :
    public IRefCount
{
    //
    // This is the name of the input point, this is controller specific.
    //
    virtual const char * GetName( void ) = 0;

    //
    // See table (below) for a list of valid output point types.
    //
    virtual const char * GetType( void ) = 0;

    //
    // Gets the the current output value.
    //
    virtual float GetGain( void ) = 0;

    //
    // Sets the percentage of output. For analog output points, this value should 
    // be between 0.0 and 1.0.  For digital output points, this value is either 
    // 0.0 or 1.0.
    //
    virtual void SetGain( float value ) = 0;

    //
    // Force feedback effects for win32
    //
#if defined( RAD_WIN32 )
    virtual long GetOffset( void ) const = 0;
    virtual void UpdateEffect( const DIEFFECT* effect ) = 0;
    virtual void Start( void ) = 0;
    virtual void Stop( void ) = 0;
    virtual void ReleaseEffect( void ) = 0;
#endif
};

//=============================================================================
// Interface: rIControllerInputPoint
//=============================================================================
//
// Description: The Controller Input Point can be thought of as just a button.
//              The user can register a callback with the Input Point that will 
//              invoked when a button is pressed or released. Input points have 
//              a tolerance setting that allows sensitive adjusting for whether 
//              or not the callback is to be issued.
//
//=============================================================================

struct IRadControllerInputPoint
    :
    public IRefCount
{
    //
    // This is the name of the input point, this is controller specific.
    //

    virtual const char * GetName( void ) = 0;
    
    //
    // See table (below) for a list of valid input point types.
    //
 
    virtual const char * GetType( void ) = 0;

    //
    // Allows you to set what percentage of the input point's total range
    // is considered enough of a change to warrant calling back.
    //

    virtual void  SetTolerance( float percentage ) = 0; // 0.0 --> 1.0
    virtual float /* percentage */ GetTolerance( void ) = 0;

    //
    // Register a notification callback to receive notification that
    // the input point value has changed.
    // 
           
    virtual void RegisterControllerInputPointCallback
    (
        IRadControllerInputPointCallback * pCallback,
        unsigned int userData = 0
    ) = 0;

    virtual void UnRegisterControllerInputPointCallback
    (
        IRadControllerInputPointCallback * pCallback
    ) = 0;

    //
    // The current value of the input point.  This depends, somewhat
    // on what the device is, and the symantics of the point, but,
    // you can treat all input points as a value from some min to some
    // max.
    //
    // pTime represents the length of time that this input point has
    // been in this state, int can be null if you do not need this
    // value

    virtual float GetCurrentValue( unsigned int * pTime = NULL ) = 0;

    //
    // This allows you to re-map the range of the device to a more
    // convenient range for game calculations.
    //

    virtual void  SetRange( float min, float max ) = 0;
    virtual void  GetRange( float * pMin, float * pMax ) = 0;
};

//=============================================================================
// Interface: bWIControllerInputPointCallback
//=============================================================================
//
// Description: This callback is used with by Controller Input Point to notify
//              when an input point has changed its value ( ie when a button is 
//              pressed down ).
//
//=============================================================================

struct IRadControllerInputPointCallback
{
    //
    // This will call you back when the input point for which you
    // registered this callback has changed value.
    //

    virtual void OnControllerInputPointChange( unsigned int userData, float newValue ) = 0;
};

#ifdef RAD_WIN32
//
// Virtual key table. Pass in one of the DIK_* defines from dinput.h, and this will return the
// index of the input point from the system keyboard (keyboard0), or -1 if the key doesn't exist.
//
extern const int* VirtualKeyToIndex;
extern const int* VirtualJoyKeyToIndex;
#endif

//
// InputPoint Types, OutputPoint Types Controller Types, and Input/Output Point Names.
//
// Names are stored as strings to facilitate their representation as data
// in higher level objects without having to write historically problematic
// enum parsers.
//
//
// Controller Categories:
// ---------------------
//
// "Joystick"
// "Mouse" 
// "Keyboard" 
//
//
// InputPoint Types:
// ----------------
//
// Name:            Default Range:  Description:
//
// "Button"          0.0 or 1.0     Digital button down or up
// "AnalogButton"    0.0 to 1.0     Button with pressure
// "XAxis"          -1.0 to 1.0     X-Axis left (-1.0) to right (1.0)
// "YAxis"          -1.0 to 1.0     Y-Axis up (-1.0) to down (1.0)
//
// OutputPoint Types:
// -----------------
//
// Name:            Range:          Description:
//
// "Digital"        0.0 or 1.0      Digital motor with on/off setting
// "Analog"         0.0 to 1.0      Analog motor
//
// Known Controller Types and Input/Output Point Names:
// ---------------------------------------------------
//
// PLAYSTATION2:
//
// Controller Name : Classification
//
// "PsxDigital" : "Joystick"
//
//      Input Point Name:   Input Point Type:   Output Point Name:  OutputPoint Type:
//
//      "Start"             "Button"            
//      "Select"            "Button"            
//      "DPadLeft"          "Button"
//      "DPadUp"            "Button"
//      "DPadRight"         "Button"
//      "DPadDown"          "Button"
//      "X"                 "Button"
//      "Square"            "Button"
//      "Circle"            "Button"
//      "Triangle"          "Button"
//      "L1"                "Button"
//      "L2"                "Button"
//      "R1"                "Button"
//      "R2"                "Button"
//
// "PsxDualShock" : "Joystick" Adds
//
//      "L3"                "Button"            "SmallMotor"        "Digital"
//      "R3"                "Button"            "LargeMotor"        "Analog"
//
//      "LeftStickX"        "XAxis"
//      "LeftStickY"        "YAxis"
//      "RightStickX"       "XAxis"
//      "RightStickY"       "YAxis"
//
// "PsxDualShock2" : "Joystick" Adds
//
//      "AnalogDPadLeft"    "AnalogButton"
//      "AnalogDPadUp"      "AnalogButton"
//      "AnalogDPadRight"   "AnalogButton"
//      "AnalogDPadDown"    "AnalogButton"
//      "AnalogX"           "AnalogButton"
//      "AnalogSquare"      "AnalogButton"
//      "AnalogCircle"      "AanlogButton"
//      "AnalogTriangle"    "AnalogButton"
//      "AnalogL1"          "AnalogButton"
//      "AnalogL2"          "AnalogButton"
//      "AnalogR1"          "AnalogButton"
//      "AnalogR2"          "AnalogButton"
//
// XBOX:
//
// "XBoxStandard" : "Joystick"

//      "DPadUp"            "Button"            "LeftMotor"         "Analog"
//      "DPadDown"          "Button"            "RightMotor"        "Analog"
//      "DPadLeft"          "Button"
//      "DPadRight"         "Button"
//      "Start"             "Button"
//      "Back"              "Button"
//      "LeftThumb"         "Button"
//      "RightThumb"        "Button"
//      "A"                 "AnalogButton"
//      "B"                 "AnalogButton"
//      "X"                 "AnalogButton"
//      "Y"                 "AnalogButton"
//      "Black"             "AnalogButton"
//      "White"             "AnalogButton"
//      "LeftTrigger"       "AnalogButton"
//      "RightTrigger"      "AnalogButton"
//
//      "LeftStickX"        "XAxis"
//      "LeftStickY"        "YAxis"
//      "RightStickX"       "XAxis"
//      "RightStickY"       "YAxis"
//
// PC:
//
// Use enumeration, and or query by name/type/category, etc.
// to determine capablities.
//
// Standard types will be defined in the future, but this is of limited
// use on the PC.

#endif
