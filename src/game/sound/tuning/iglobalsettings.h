//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        iglobalsettings.h
//
// Description: Declaration of interface class IGlobalSettings, which sets 
//              global sound values in the game (e.g. master volume, 
//              sound defaults).  Created using RadScript.
//
// History:     07/08/2002 + Created -- Darren
//
//=============================================================================

#ifndef IGLOBALSETTINGS_H
#define IGLOBALSETTINGS_H

//========================================
// Nested Includes
//========================================
#include <radobject.hpp>

//========================================
// Forward References
//========================================

//=============================================================================
//
// Synopsis:    IGlobalSettings
//
//=============================================================================

class IGlobalSettings : public IRefCount
{
    public:
        //
        // Volume controls
        //
        virtual IGlobalSettings& SetMasterVolume( float volume ) = 0;

        virtual IGlobalSettings& SetSfxVolume( float volume ) = 0;
        virtual IGlobalSettings& SetCarVolume( float volume ) = 0;
        virtual IGlobalSettings& SetMusicVolume( float volume ) = 0;
        virtual IGlobalSettings& SetDialogueVolume( float volume ) = 0;
        virtual IGlobalSettings& SetAmbienceVolume( float volume ) = 0;

        //
        // Ducking controls
        //
        virtual IGlobalSettings& SetPauseSfxVolume( float volume ) = 0;
        virtual IGlobalSettings& SetPauseCarVolume( float volume ) = 0;
        virtual IGlobalSettings& SetPauseMusicVolume( float volume ) = 0;
        virtual IGlobalSettings& SetPauseDialogueVolume( float volume ) = 0;
        virtual IGlobalSettings& SetPauseAmbienceVolume( float volume ) = 0;

        virtual IGlobalSettings& SetMissionScreenSfxVolume( float volume ) = 0;
        virtual IGlobalSettings& SetMissionScreenCarVolume( float volume ) = 0;
        virtual IGlobalSettings& SetMissionScreenMusicVolume( float volume ) = 0;
        virtual IGlobalSettings& SetMissionScreenDialogueVolume( float volume ) = 0;
        virtual IGlobalSettings& SetMissionScreenAmbienceVolume( float volume ) = 0;

        virtual IGlobalSettings& SetLetterboxSfxVolume( float volume ) = 0;
        virtual IGlobalSettings& SetLetterboxCarVolume( float volume ) = 0;
        virtual IGlobalSettings& SetLetterboxMusicVolume( float volume ) = 0;
        virtual IGlobalSettings& SetLetterboxDialogueVolume( float volume ) = 0;
        virtual IGlobalSettings& SetLetterboxAmbienceVolume( float volume ) = 0;

        virtual IGlobalSettings& SetDialogueSfxVolume( float volume ) = 0;
        virtual IGlobalSettings& SetDialogueCarVolume( float volume ) = 0;
        virtual IGlobalSettings& SetDialogueMusicVolume( float volume ) = 0;
        virtual IGlobalSettings& SetDialogueDialogueVolume( float volume ) = 0;
        virtual IGlobalSettings& SetDialogueAmbienceVolume( float volume ) = 0;

        virtual IGlobalSettings& SetStoreSfxVolume( float volume ) = 0;
        virtual IGlobalSettings& SetStoreCarVolume( float volume ) = 0;
        virtual IGlobalSettings& SetStoreMusicVolume( float volume ) = 0;
        virtual IGlobalSettings& SetStoreDialogueVolume( float volume ) = 0;
        virtual IGlobalSettings& SetStoreAmbienceVolume( float volume ) = 0;

        virtual IGlobalSettings& SetOnFootSfxVolume( float volume ) = 0;
        virtual IGlobalSettings& SetOnFootCarVolume( float volume ) = 0;
        virtual IGlobalSettings& SetOnFootMusicVolume( float volume ) = 0;
        virtual IGlobalSettings& SetOnFootDialogueVolume( float volume ) = 0;
        virtual IGlobalSettings& SetOnFootAmbienceVolume( float volume ) = 0;

        virtual IGlobalSettings& SetMinigameSfxVolume( float volume ) = 0;
        virtual IGlobalSettings& SetMinigameCarVolume( float volume ) = 0;
        virtual IGlobalSettings& SetMinigameMusicVolume( float volume ) = 0;
        virtual IGlobalSettings& SetMinigameDialogueVolume( float volume ) = 0;
        virtual IGlobalSettings& SetMinigameAmbienceVolume( float volume ) = 0;

        virtual IGlobalSettings& SetJustMusicSfxVolume( float volume ) = 0;
        virtual IGlobalSettings& SetJustMusicCarVolume( float volume ) = 0;
        virtual IGlobalSettings& SetJustMusicMusicVolume( float volume ) = 0;
        virtual IGlobalSettings& SetJustMusicDialogueVolume( float volume ) = 0;
        virtual IGlobalSettings& SetJustMusicAmbienceVolume( float volume ) = 0;

        virtual IGlobalSettings& SetCreditsSfxVolume( float volume ) = 0;
        virtual IGlobalSettings& SetCreditsCarVolume( float volume ) = 0;
        virtual IGlobalSettings& SetCreditsMusicVolume( float volume ) = 0;
        virtual IGlobalSettings& SetCreditsDialogueVolume( float volume ) = 0;
        virtual IGlobalSettings& SetCreditsAmbienceVolume( float volume ) = 0;

        //
        // Car controls
        //
        virtual IGlobalSettings& SetPeeloutMin( float min ) = 0;
        virtual IGlobalSettings& SetPeeloutMax( float max ) = 0;
        virtual IGlobalSettings& SetPeeloutMaxTrim( float trim ) = 0;

        virtual IGlobalSettings& SetSkidRoadClipName( const char* clipName ) = 0;
        virtual IGlobalSettings& SetSkidDirtClipName( const char* clipName ) = 0;

        //
        // Footstep sounds
        //
        virtual IGlobalSettings& SetFootstepRoadClipName( const char* clipName ) = 0;
        virtual IGlobalSettings& SetFootstepMetalClipName( const char* clipName ) = 0;
        virtual IGlobalSettings& SetFootstepWoodClipName( const char* clipName ) = 0;

        //
        // Coin pitches
        //
        virtual IGlobalSettings& SetCoinPitch( float pitch ) = 0;
};


#endif // IGLOBALSETTINGS_H

