//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// File:        soundrenderingmanager.hpp
//
// Subsystem:   Dark Angel - Sound Rendering Manager System
//
// Description: Description of the DA sound manager
//
// Revisions:
//  + Created October 2, 2001 -- breimer
//
//=============================================================================

#ifndef _SOUNDRENDERINGMANAGER_HPP
#define _SOUNDRENDERINGMANAGER_HPP

//=============================================================================
// Included Files
//=============================================================================

#include <radobject.hpp>
#include <radfile.hpp>
#include <Enums.h>
#include <radsound.hpp>
#include <radscript.hpp>

#include <sound/soundmanager.h>
#include <sound/soundrenderer/idasoundtuner.h>
#include <sound/soundrenderer/soundresourcemanager.h>
#include <sound/soundrenderer/playermanager.h>
#include <loading/loadingmanager.h>

//=============================================================================
// Global namespace forward declarations
//=============================================================================

//#define AUDIO_ENABLE_SCRIPTING

class SoundFileHandler;

//=============================================================================
// Define Owning Namespace
//=============================================================================

namespace Sound {

//=============================================================================
// Prototypes
//=============================================================================

class daSoundRenderingManager;
class daSoundDynaLoadManager;

//=============================================================================
// Class Declarations
//=============================================================================

static const unsigned int NUM_CHARACTER_NAMESPACES = 5;

//
// Language enumeration
//
enum DialogueLanguage
{
    DIALOGUE_LANGUAGE_ENGLISH,
    DIALOGUE_LANGUAGE_FRENCH,
    DIALOGUE_LANGUAGE_GERMAN,
    DIALOGUE_LANGUAGE_SPANISH
};

//
// The sound manger
//
class daSoundRenderingManager : public radRefCount, public LoadingManager::ProcessRequestsCallback
{
public:
    IMPLEMENT_REFCOUNTED( "daSoundManager" );

    //
    // Constructor and destructor
    //
    daSoundRenderingManager( );
    virtual ~daSoundRenderingManager( );

    //
    // Get the singleton
    //
    static daSoundRenderingManager* GetInstance( void );

    //
    // Terminate
    //
    void Terminate( void );
    
    void QueueCementFileRegistration();
    void QueueRadscriptFileLoads();
#ifdef AUDIO_ENABLE_SCRIPTING
    void LoadTypeInfoFile( const char* filename, SoundFileHandler* fileHandler );
    void LoadScriptFile( const char* filename, SoundFileHandler* fileHandler );
#endif

    void SetLanguage( Scrooby::XLLanguage language );

#ifdef AUDIO_ENABLE_SCRIPTING
    void ProcessTypeInfo( void* pUserData );
    void ProcessScript( void* pUserData );
#endif
    virtual void OnProcessRequestsComplete( void* pUserData );

    //
    // IDaSoundManager
    //
    void Initialize( void );
    bool IsInitialized( void );
    void Service( void );
    void ServiceOncePerFrame( unsigned int elapsedTime );
    void Render( void );

    IRadNameSpace* GetSoundNamespace( void );
    IRadNameSpace* GetTuningNamespace( void );
    IRadNameSpace* GetCharacterNamespace( unsigned int index );
    IRadSoundHalListener* GetTheListener( void );

    daSoundDynaLoadManager*         GetDynaLoadManager( void );
    IDaSoundTuner*                  GetTuner( void );
    daSoundResourceManager*         GetResourceManager( void );
    daSoundPlayerManager*           GetPlayerManager( void );

protected:

#ifdef AUDIO_ENABLE_SCRIPTING
    static void TypeInfoComplete( void* pUserData );
    static void ScriptComplete( void* pUserData );
    static void SoundObjectCreated( const char* objName, IRefCount* obj );
#else
    void RunApuSoundScripts( void );
    void RunBartSoundScripts( void );
    void RunHomerSoundScripts( void );
    void RunLisaSoundScripts( void );
    void RunMargeSoundScripts( void );
    void RunLevelSoundScripts( void );
    void RunSoundEffectScripts( void );
    void RunEnglishSoundScripts( void );
#ifdef PAL
    void RunFrenchSoundScripts( void );
    void RunGermanSoundScripts( void );
    void RunSpanishSoundScripts( void );
#endif
    void RunCarSoundScripts( void );
    void RunTuningSoundScripts( void );
#endif

private:

    static void FilePerformanceEvent( bool start, const char * pFile, unsigned int bytes );

    void registerDialogueCementFiles( const char* cementFilename );

#ifndef AUDIO_ENABLE_SCRIPTING
    template<class T>
    T& Create( const char* objName )
    {
        T* obj = T::ObjCreate( GMA_AUDIO_PERSISTENT );
        if( false == m_pCurrentNameSpace->AddInstance( objName, obj ) )
            rTunePrintf( "AUDIO: WARNING: Inventory Collision!: %s\n", objName );
        rReleaseAssert( GetSoundManager()->GetSoundLoader()->AddResourceToCurrentCluster( objName ) );
        return *obj;
    }

    void SetCurrentNameSpace( IRadNameSpace* pNameSpace ) { m_pCurrentNameSpace = pNameSpace; }
#endif
    
    // The singleton instance
    static daSoundRenderingManager*              s_Singleton;

    IRadScript*                         m_pScript;
    bool                                m_IsInitialized;

    //
    // Our namespaces
    //
    IRadNameSpace*                      m_pResourceNameSpace;
    IRadNameSpace*                      m_pTuningNameSpace;
    IRadNameSpace*                      m_pCurrentNameSpace;
    IRadNameSpace*                      m_pCharacterNameSpace[NUM_CHARACTER_NAMESPACES];

    //
    // Store the various related systems
    //
    daSoundDynaLoadManager*             m_pDynaLoadManager;
    IDaSoundTuner*                      m_pTuner;
    daSoundResourceManager*             m_pResourceManager;
    daSoundPlayerManager*               m_pPlayerManager;

    //
    // Loading system callback
    //
    SoundFileHandler* m_soundFileHandler;
    
    //
    // Cement file handles, in case we want to release them
    //
#if defined( RAD_UWP ) || defined( RAD_WIN32 )
    static const unsigned int NUM_SOUND_CEMENT_FILES = 12;
#endif
    unsigned int m_soundCementFileHandles[NUM_SOUND_CEMENT_FILES];

    //
    // Script loading count, so we can tell which namespace to put stuff in
    //
    unsigned int m_scriptLoadCount;
    
    unsigned int m_LastPerformanceEventTime;

    //
    // Language
    //
    DialogueLanguage m_currentLanguage;
    bool m_languageSelected;
};

} // Sound Namespace

#endif //_SOUNDRENDERINGMANAGER_HPP
