//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        globalsettings.h
//
// Description: Declaration of globalSettings, which sets global sound values
//              in the game (e.g. master volume, sound defaults).  Created
//              using RadScript, hence the lower-case g.
//
// History:     07/08/2002 + Created -- Darren
//
//=============================================================================

#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

//========================================
// Nested Includes
//========================================
#include <radlinkedclass.hpp>

#include <sound/tuning/iglobalsettings.h>
#include <sound/soundrenderer/dasoundgroup.h>

//========================================
// Forward References
//========================================

//=============================================================================
//
// Synopsis:    globalSettings
//
//=============================================================================

class globalSettings : public IGlobalSettings,
                       public radLinkedClass< globalSettings >,
                       public radRefCount
{
    public:
        IMPLEMENT_REFCOUNTED( "globalSettings" );

        globalSettings();
        virtual ~globalSettings();

        //
        // Volume controls
        //
        IGlobalSettings& SetMasterVolume( float volume );

        IGlobalSettings& SetSfxVolume( float volume );
        float GetSfxVolume() { return( m_sfxVolume ); }

        IGlobalSettings& SetCarVolume( float volume );
        float GetCarVolume() { return( m_carVolume ); }

        IGlobalSettings& SetMusicVolume( float volume );
        float GetMusicVolume() { return( m_musicVolume ); }

        IGlobalSettings& SetDialogueVolume( float volume );
        float GetDialogueVolume() { return( m_dialogueVolume ); }

        IGlobalSettings& SetAmbienceVolume( float volume );
        float GetAmbienceVolume() { return( m_ambienceVolume ); }
        
        //
        // Ducking controls
        //
        float GetDuckVolume( Sound::DuckSituations situation, Sound::DuckVolumes volume ) { return( m_duckVolumes[situation].duckVolume[volume] ); }

        IGlobalSettings& SetPauseSfxVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_PAUSE, Sound::DUCK_SFX, volume ); return *this; }
        IGlobalSettings& SetPauseCarVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_PAUSE, Sound::DUCK_CAR, volume ); return *this; }
        IGlobalSettings& SetPauseMusicVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_PAUSE, Sound::DUCK_MUSIC, volume ); return *this; }
        IGlobalSettings& SetPauseDialogueVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_PAUSE, Sound::DUCK_DIALOG, volume ); return *this; }
        IGlobalSettings& SetPauseAmbienceVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_PAUSE, Sound::DUCK_AMBIENCE, volume ); return *this; }

        IGlobalSettings& SetMissionScreenSfxVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_MISSION, Sound::DUCK_SFX, volume ); return *this; }
        IGlobalSettings& SetMissionScreenCarVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_MISSION, Sound::DUCK_CAR, volume ); return *this; }
        IGlobalSettings& SetMissionScreenMusicVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_MISSION, Sound::DUCK_MUSIC, volume ); return *this; }
        IGlobalSettings& SetMissionScreenDialogueVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_MISSION, Sound::DUCK_DIALOG, volume ); return *this; }
        IGlobalSettings& SetMissionScreenAmbienceVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_MISSION, Sound::DUCK_AMBIENCE, volume ); return *this; }

        IGlobalSettings& SetLetterboxSfxVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_LETTERBOX, Sound::DUCK_SFX, volume ); return *this; }
        IGlobalSettings& SetLetterboxCarVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_LETTERBOX, Sound::DUCK_CAR, volume ); return *this; }
        IGlobalSettings& SetLetterboxMusicVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_LETTERBOX, Sound::DUCK_MUSIC, volume ); return *this; }
        IGlobalSettings& SetLetterboxDialogueVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_LETTERBOX, Sound::DUCK_DIALOG, volume ); return *this; }
        IGlobalSettings& SetLetterboxAmbienceVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_LETTERBOX, Sound::DUCK_AMBIENCE, volume ); return *this; }

        IGlobalSettings& SetDialogueSfxVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_DIALOG, Sound::DUCK_SFX, volume ); return *this; }
        IGlobalSettings& SetDialogueCarVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_DIALOG, Sound::DUCK_CAR, volume ); return *this; }
        IGlobalSettings& SetDialogueMusicVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_DIALOG, Sound::DUCK_MUSIC, volume ); return *this; }
        IGlobalSettings& SetDialogueDialogueVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_DIALOG, Sound::DUCK_DIALOG, volume ); return *this; }
        IGlobalSettings& SetDialogueAmbienceVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_DIALOG, Sound::DUCK_AMBIENCE, volume ); return *this; }

        IGlobalSettings& SetStoreSfxVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_STORE, Sound::DUCK_SFX, volume ); return *this; }
        IGlobalSettings& SetStoreCarVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_STORE, Sound::DUCK_CAR, volume ); return *this; }
        IGlobalSettings& SetStoreMusicVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_STORE, Sound::DUCK_MUSIC, volume ); return *this; }
        IGlobalSettings& SetStoreDialogueVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_STORE, Sound::DUCK_DIALOG, volume ); return *this; }
        IGlobalSettings& SetStoreAmbienceVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_STORE, Sound::DUCK_AMBIENCE, volume ); return *this; }

        IGlobalSettings& SetOnFootSfxVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_ONFOOT, Sound::DUCK_SFX, volume ); return *this; }
        IGlobalSettings& SetOnFootCarVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_ONFOOT, Sound::DUCK_CAR, volume ); return *this; }
        IGlobalSettings& SetOnFootMusicVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_ONFOOT, Sound::DUCK_MUSIC, volume ); return *this; }
        IGlobalSettings& SetOnFootDialogueVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_ONFOOT, Sound::DUCK_DIALOG, volume ); return *this; }
        IGlobalSettings& SetOnFootAmbienceVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_ONFOOT, Sound::DUCK_AMBIENCE, volume ); return *this; }

        IGlobalSettings& SetMinigameSfxVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_MINIGAME, Sound::DUCK_SFX, volume ); return *this; }
        IGlobalSettings& SetMinigameCarVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_MINIGAME, Sound::DUCK_CAR, volume ); return *this; }
        IGlobalSettings& SetMinigameMusicVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_MINIGAME, Sound::DUCK_MUSIC, volume ); return *this; }
        IGlobalSettings& SetMinigameDialogueVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_MINIGAME, Sound::DUCK_DIALOG, volume ); return *this; }
        IGlobalSettings& SetMinigameAmbienceVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_MINIGAME, Sound::DUCK_AMBIENCE, volume ); return *this; }

        IGlobalSettings& SetJustMusicSfxVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_JUST_MUSIC, Sound::DUCK_SFX, volume ); return *this; }
        IGlobalSettings& SetJustMusicCarVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_JUST_MUSIC, Sound::DUCK_CAR, volume ); return *this; }
        IGlobalSettings& SetJustMusicMusicVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_JUST_MUSIC, Sound::DUCK_MUSIC, volume ); return *this; }
        IGlobalSettings& SetJustMusicDialogueVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_JUST_MUSIC, Sound::DUCK_DIALOG, volume ); return *this; }
        IGlobalSettings& SetJustMusicAmbienceVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_JUST_MUSIC, Sound::DUCK_AMBIENCE, volume ); return *this; }

        IGlobalSettings& SetCreditsSfxVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_CREDITS, Sound::DUCK_SFX, volume ); return *this; }
        IGlobalSettings& SetCreditsCarVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_CREDITS, Sound::DUCK_CAR, volume ); return *this; }
        IGlobalSettings& SetCreditsMusicVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_CREDITS, Sound::DUCK_MUSIC, volume ); return *this; }
        IGlobalSettings& SetCreditsDialogueVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_CREDITS, Sound::DUCK_DIALOG, volume ); return *this; }
        IGlobalSettings& SetCreditsAmbienceVolume( float volume ) { setDuckVolume( Sound::DUCK_SIT_CREDITS, Sound::DUCK_AMBIENCE, volume ); return *this; }

        //
        // Car controls
        //
        IGlobalSettings& SetPeeloutMin( float min );
        float GetPeeloutMin() { return( m_peeloutMin ); }

        IGlobalSettings& SetPeeloutMax( float max );
        float GetPeeloutMax() { return( m_peeloutMax ); }

        IGlobalSettings& SetPeeloutMaxTrim( float trim );
        float GetPeeloutMaxTrim() { return( m_peeloutMaxTrim ); }

        IGlobalSettings& SetSkidRoadClipName( const char* clipName );
        const char* GetSkidRoadClipName() { return( m_roadSkidClip ); }

        IGlobalSettings& SetSkidDirtClipName( const char* clipName );
        const char* GetSkidDirtClipName() { return( m_dirtSkidClip ); }

        //
        // Footstep sounds
        //
        IGlobalSettings& SetFootstepRoadClipName( const char* clipName );
        const char* GetFootstepRoadClipName() { return( m_roadFootstepClip ); }

        IGlobalSettings& SetFootstepMetalClipName( const char* clipName );
        const char* GetFootstepMetalClipName() { return( m_metalFootstepClip ); }

        IGlobalSettings& SetFootstepWoodClipName( const char* clipName );
        const char* GetFootstepWoodClipName() { return( m_woodFootstepClip ); }

        //
        // Coin pitches
        //
        IGlobalSettings& SetCoinPitch( float pitch );
        float GetCoinPitch( unsigned int index );
        unsigned int GetNumCoinPitches() { return( m_coinPitchCount ); }

        //
        // Create a CarSoundParameters object
        //
        static globalSettings* ObjCreate( radMemoryAllocator allocator );

    private:

        //Prevent wasteful constructor creation.
        globalSettings( const globalSettings& original );
        globalSettings& operator=( const globalSettings& rhs );

        void setDuckVolume( Sound::DuckSituations situation, Sound::DuckVolumes volumeToSet, float volume );

        //
        // Ducking settings
        //
        Sound::DuckVolumeSet m_duckVolumes[Sound::NUM_DUCK_SITUATIONS];

        //
        // Car settings
        //
        float m_peeloutMin;
        float m_peeloutMax;
        float m_peeloutMaxTrim;

        char* m_roadSkidClip;
        char* m_dirtSkidClip;

        //
        // Footsteps
        //
        char* m_roadFootstepClip;
        char* m_metalFootstepClip;
        char* m_woodFootstepClip;

        //
        // Coin pitches
        //
        static const unsigned int s_maxCoinPitches = 10;
        float m_coinPitches[s_maxCoinPitches];
        unsigned int m_coinPitchCount;

        //
        // Hack!!
        //
        float m_ambienceVolume;
        float m_musicVolume;
        float m_sfxVolume;
        float m_dialogueVolume;
        float m_carVolume;
};

#endif // GLOBALSETTINGS_H

