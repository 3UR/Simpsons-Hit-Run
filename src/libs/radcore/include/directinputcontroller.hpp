//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        DirectInputController.hpp
//
// Subsystem:	Foundation Technologies - PS2 Controller System
//
// Description:	This file contains the class definitions for the different 
//              output points used in direct input force feedback.  These are
//              needed when working with wheels or rumbles.
//
// Date:    	July 14, 2003
//
// Author:      ziemek,
//              Neil Haran
//
//=============================================================================

#ifndef DICONTROLLER_HPP
#define DICONTROLLER_HPP

//=============================================================================
// Include Files
//=============================================================================

#include <radcontroller.hpp>

//============================================================================
// Interface: IRadControllerOutputPointDirectInput
//============================================================================

struct IRadControllerOutputPointDirectInput
    :
    public IRadControllerOutputPoint
{
    virtual const char * GetName( void ) = 0;
    virtual const char * GetType( void ) = 0;
    virtual float GetGain( void ) = 0;
    virtual void  SetGain( float value ) = 0;

    virtual void UpdateEffect( LPCDIEFFECT pEffect ) = 0;
    virtual void Start( void ) = 0;
    virtual void Stop( void ) = 0;

    virtual void Init( IDirectInputDevice8* pDevice ) = 0;
    virtual void SendOutput( void ) = 0;
    virtual void ReleaseEffect( void ) = 0;
};

//============================================================================
// Class: ControllerOutputPointDirectInput
//============================================================================
#define MAX_FORCES 8 // Maximum number of forces set for each force Type.

enum eForceType
{
    SPRING, 
    CONSTANT, 
    DAMPER, 
    SIDE_COLLISION, 
    FRONTAL_COLLISION, 
    DIRT_ROAD, BUMPY_ROAD, 
    SLIPPERY_ROAD, 
    SURFACE_EFFECT, 
    CAR_AIRBORNE,
    NUMBER_FORCE_EFFECTS
};

enum ePeriodicType
{
    SINE, 
    SQUARE, 
    TRIANGLE, 
    SAWTOOTHUP, 
    SAWTOOTHDOWN
};

enum eConditionForceNumber
{
    CONDITION_0, 
    CONDITION_1, 
    CONDITION_2, 
    CONDITION_3, 
    CONDITION_4, 
    CONDITION_5, 
    CONDITION_6, 
    CONDITION_7
};

enum ePeriodicForceNumber
{
    PERIODIC_0, 
    PERIODIC_1, 
    PERIODIC_2, 
    PERIODIC_3, 
    PERIODIC_4, 
    PERIODIC_5, 
    PERIODIC_6, 
    PERIODIC_7
};

enum eConstantForceNumber
{
    CONSTANT_0, 
    CONSTANT_1, 
    CONSTANT_2, 
    CONSTANT_3, 
    CONSTANT_4, 
    CONSTANT_5, 
    CONSTANT_6, 
    CONSTANT_7
};

enum eRampForceNumber
{
    RAMP_0, 
    RAMP_1, 
    RAMP_2, 
    RAMP_3, 
    RAMP_4, 
    RAMP_5, 
    RAMP_6, 
    RAMP_7
};

class ControllerOutputPointDirectInput
    :
    public IRadControllerOutputPoint,
    public radRefCount
{
public:

    IMPLEMENT_REFCOUNTED( "ControllerOutputPointDirectInput" )

    ControllerOutputPointDirectInput( LPCDIEFFECTINFO pdei, DWORD dataOffset );
    ~ControllerOutputPointDirectInput( void );

    virtual const char * GetName( void );
    virtual const char * GetType( void );
    virtual float GetGain( void );
    virtual void  SetGain( float value );
    virtual void UpdateEffect( LPCDIEFFECT pEffect );
    
    virtual void Start( void );
    virtual void Stop( void );
    
    virtual void Init( IDirectInputDevice8* pDevice );
    virtual void SendOutput( void );
    
    void ReleaseEffect( void );
    void CreateEffect( GUID guidEffect, LPCDIEFFECT effect, int forceNumber );
    void SetParams( LPCDIEFFECT effect, int forceNumber );

    long GetOffset() const { return m_diActuator; }
protected:
    HRESULT UnloadEffect( int forceNumber );
protected:
    LPDIRECTINPUTDEVICE8    m_pDIDevice;
    LPDIRECTINPUTEFFECT     m_pEffectID[MAX_FORCES];
    DIEFFECTINFO            m_diEffectInfo;
    DWORD                   m_diActuator;
    
    bool                    m_bEffectPlaying[MAX_FORCES];
    float                   m_fGain;
    int                     m_iForceNumber; // Not being used right now.. but if you wish to
                                            // have multiple forces per output point... use it.
};

//=============================================================================
// class rControllerInputPoint2
//=============================================================================

//=============================================================================
// class ConstantForceOutputPoint
//=============================================================================
class ConstantForceOutputPoint : public ControllerOutputPointDirectInput
{
public:
   
protected:

private:
};

//=============================================================================
// class ConditionalForceOutputPoint
//=============================================================================
class ConditionalForceOutputPoint : public ControllerOutputPointDirectInput
{
public:
protected:
private:
};

//=============================================================================
// class PeriodicForceOutputPoint
//=============================================================================
class PeriodicForceOutputPoint : public ControllerOutputPointDirectInput
{
public:
protected:
private:
};

#endif