//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================
//
// File: system.hpp
//
//=============================================================================

#ifndef SOUND_HAL_WIN32_SYSTEM_HPP
#define SOUND_HAL_WIN32_SYSTEM_HPP

//============================================================================
// Include Files
//============================================================================

#include <radsound_hal.hpp>
#include "radsoundwin.hpp"
#include "../../common/radsoundupdatableobject.hpp"

//============================================================================
// Definitiona
//============================================================================

#define RSD_SYSTEM_MAX_AUX_SENDS 2

//============================================================================
// Component: radSoundHalSystem
//============================================================================

class radSoundHalSystem
	:
	public IRadSoundHalSystem,
	public radSoundObject
{
    public: 

	    IMPLEMENT_REFCOUNTED( "radSoundHalSystem" )

        radSoundHalSystem( radMemoryAllocator allocator );
        ~radSoundHalSystem( void );
        virtual void Initialize( const SystemDescription & systemDescription );
	    virtual IRadSoundHalMemoryRegion * GetRootMemoryRegion( void );
        virtual unsigned int GetNumAuxSends( );
	    virtual void SetOutputMode( radSoundOutputMode mode );
	    virtual radSoundOutputMode GetOutputMode( void );
        virtual void Service( void );
        virtual void ServiceOncePerFrame( void );
        virtual void GetStats( IRadSoundHalSystem::Stats * pStats );
        virtual void SetAuxEffect( unsigned int auxNumber, IRadSoundHalEffect * pIRadSoundHalEffect );
        virtual IRadSoundHalEffect * GetAuxEffect( unsigned int auxNumber );
        virtual void SetAuxGain( unsigned int aux, float gain );
        virtual float GetAuxGain( unsigned int aux );
        ALCdevice * GetOpenALDevice( void );
        ALCcontext * GetOpenALContext( void );
        ALuint GetOpenALAuxSlot( unsigned int aux );

        static radSoundHalSystem * GetInstance( void );

        static radSoundHalSystem * s_pRsdSystem;

    private:

        void *          m_pSoundMemory;
        ALCint          m_NumAuxSends;
        ALCuint         m_AuxSlots[RSD_SYSTEM_MAX_AUX_SENDS];

        ALCdevice *     m_pDevice;
        ALCcontext *    m_pContext;

        radRef< IRadSoundHalEffect >      m_refIRadSoundHalEffect[ RSD_SYSTEM_MAX_AUX_SENDS ];

        unsigned int    m_LastServiceTime;

        LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
        LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
        LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
        LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
};

#endif // SOUND_HAL_WIN32_SYSTEM_HPP