//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   MemoryCardManager										
//
// Description: Implementation of the MemoryCardManager class.
//
// Authors:     Tony Chu
//
// Revisions		Date			Author	    Revision
//                  2002/09/16      TChu        Created for SRR2
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include <data/memcard/memorycardmanager.h>
#include <data/gamedatamanager.h>
#include <data/savegameinfo.h>

#ifndef WORLD_BUILDER
#include <gameflow/gameflow.h>
#endif

#include <radfile.hpp>

#ifndef WORLD_BUILDER
#include <memory/srrmemory.h>
#else
#define MEMTRACK_PUSH_GROUP(x)
#define MEMTRACK_POP_GROUP(x)
#define GMA_PERSISTENT 0
#define GMA_DEFAULT 0
#define GMA_TEMP 0
#endif

#include <string.h>

// Static pointer to instance of singleton.
MemoryCardManager* MemoryCardManager::spInstance = NULL;

//===========================================================================
// Local Constants
//===========================================================================

const unsigned int MINIMUM_MEMCARD_CHECK_TIME = 0; // in msec
const int MAX_SAVED_GAME_TITLE_LENGTH = 32; // # chars

#if defined(RAD_WIN32) || defined(RAD_UWP)
char DEFAULT_GAME_DRIVE[radFileDrivenameMax+1]; // for win32, need to store the default.
#endif

const char* SAVE_GAME_DRIVE[] =
{
#if defined(RAD_WIN32) || defined(RAD_UWP)
    DEFAULT_GAME_DRIVE,
#endif
    "" // dummy terminator
};

const unsigned int NUM_SAVE_GAME_DRIVES =
    sizeof( SAVE_GAME_DRIVE ) / sizeof( SAVE_GAME_DRIVE[ 0 ] ) - 1;

//===========================================================================
// Public Member Functions
//===========================================================================

//==============================================================================
// MemoryCardManager::CreateInstance
//==============================================================================
//
// Description: - Creates the Game Data Manager.
//
// Parameters:	None.
//
// Return:      Pointer to the MemoryCardManager.
//
// Constraints: This is a singleton so only one instance is allowed.
//
//==============================================================================
MemoryCardManager* MemoryCardManager::CreateInstance()
{
    spInstance = new MemoryCardManager;
    rAssert( spInstance != NULL );

    return spInstance;
}

//==============================================================================
// MemoryCardManager::DestroyInstance
//==============================================================================
//
// Description: Destroy the Game Data Manager.
//
// Parameters:	None.
//
// Return:      None.
//
//==============================================================================
void MemoryCardManager::DestroyInstance()
{
    rAssert( spInstance != NULL );

    delete spInstance;
    spInstance = NULL;
}

//==============================================================================
// MemoryCardManager::GetInstance
//==============================================================================
//
// Description: - Access point for the MemoryCardManager singleton.  
//              - Creates the MemoryCardManager if needed.
//
// Parameters:	None.
//
// Return:      Pointer to the MemoryCardManager.
//
// Constraints: This is a singleton so only one instance is allowed.
//
//==============================================================================
MemoryCardManager* MemoryCardManager::GetInstance()
{
    rAssert( spInstance != NULL );

    return spInstance;
}

//===========================================================================
// MemoryCardManager::MemoryCardManager
//===========================================================================
// Description: 
//
// Constraints:	None.
//
// Parameters:	None.
//
// Return:      
//
//===========================================================================
MemoryCardManager::MemoryCardManager()
:   m_currentState( STATE_UNINITIALIZED ),
    m_numDrivesOpened( 0 ),
    m_memcardInfo( NULL ),
    m_memcardInfoLoadState( MEMCARD_INFO_NOT_LOADED ),
    m_memcardInfoLoadCallback( NULL ),
    m_radFile( NULL ),
    m_elapsedMemcardInfoLoadTime( 0 ),
    m_pDrives( NULL ),
    m_currentDrive( NULL ),
    m_radDriveErrorCallback( NULL ),
    m_mediaInfos( NULL ),
    m_currentMediaInfo( -1 ),
    m_nextMediaInfo( 0 ),
	m_formatDriveState( false ),
    m_formatCallback( NULL ),
    m_memcardCheckCallback( NULL ),
    m_memcardCheckingState( MEMCARD_CHECK_NOT_DONE ),
    m_elapsedMemcardCheckTime( 0 ),
    m_savedGameCreationSize( 0 )
{
    m_pDrives = new IRadDrive*[ NUM_SAVE_GAME_DRIVES ];
    rAssert( m_pDrives != NULL );

    m_mediaInfos = new IRadDrive::MediaInfo[ NUM_SAVE_GAME_DRIVES ];
    rAssert( m_mediaInfos != NULL );

    for( unsigned int i = 0; i < NUM_SAVE_GAME_DRIVES; i++ )
    {
        m_pDrives[ i ] = NULL;

        m_mediaInfos[ i ].m_FreeFiles = 0;
        m_mediaInfos[ i ].m_FreeSpace = 0;
        m_mediaInfos[ i ].m_MediaState = IRadDrive::MediaInfo::MediaNotPresent;
        m_mediaInfos[ i ].m_SectorSize = 0;
        m_mediaInfos[ i ].m_VolumeName[ 0 ] = '\0';
    }
}

//===========================================================================
// MemoryCardManager::~MemoryCardManager
//===========================================================================
// Description: 
//
// Constraints:	None.
//
// Parameters:	None.
//
// Return:      
//
//===========================================================================
MemoryCardManager::~MemoryCardManager()
{
    if( m_pDrives != NULL )
    {
        // release and un-mount drives
        //
        for( unsigned int i = 0; i < NUM_SAVE_GAME_DRIVES; i++ )
        {
            if( m_pDrives[ i ] != NULL )
            {
#if !defined(RAD_WIN32) && !defined(RAD_UWP)
                m_pDrives[ i ]->UnregisterErrorHandler( this );
#endif

                m_pDrives[ i ]->Release();
                m_pDrives[ i ] = NULL;

                m_numDrivesOpened--;
            }

#if !defined(RAD_WIN32) && !defined(RAD_UWP)
            radDriveUnmount( SAVE_GAME_DRIVE[ i ] );
#endif
        }

        rAssert( m_numDrivesOpened == 0 );

        delete [] m_pDrives;
        m_pDrives = NULL;
    }

    if( m_mediaInfos != NULL )
    {
        delete [] m_mediaInfos;
        m_mediaInfos = NULL;
    }

    m_currentState = STATE_UNINITIALIZED;
}

void
MemoryCardManager::Init( IRadDriveErrorCallback* radDriveErrorCallback )
{
    m_radDriveErrorCallback = radDriveErrorCallback;

#if defined(RAD_WIN32) || defined(RAD_UWP)
    radGetDefaultDrive( DEFAULT_GAME_DRIVE );
#endif

    // mount and open drives
    //
    for( unsigned int i = 0; i < NUM_SAVE_GAME_DRIVES; i++ )
    {
#if !defined(RAD_WIN32) && !defined(RAD_UWP)
        bool driveAlreadyMounted = radDriveMount( SAVE_GAME_DRIVE[ i ], GMA_PERSISTENT );
        rAssert( !driveAlreadyMounted );
#endif

        radDriveOpenAsync( &m_pDrives[ i ], SAVE_GAME_DRIVE[ i ] );
        rAssert( m_pDrives[ i ] );
        m_pDrives[ i ]->AddCompletionCallback( this, reinterpret_cast<void*>( (uintptr_t)i ) );

        // register error handler
        //
#if !defined(RAD_WIN32) && !defined(RAD_UWP)
        m_pDrives[ i ]->RegisterErrorHandler( this, NULL );
#endif
    }

    m_currentState = STATE_OPENING_DRIVES;
}

void
MemoryCardManager::Update( unsigned int elapsedTime )
{
    if( m_currentState == STATE_READY )
    {
        if( m_currentMediaInfo != m_nextMediaInfo )
        {
            m_currentMediaInfo = m_nextMediaInfo;

            // get the next media info asynchronously
            //
            rAssert( m_pDrives[ m_currentMediaInfo ] != NULL );
            m_pDrives[ m_currentMediaInfo ]->GetMediaInfoAsync( &m_mediaInfos[ m_currentMediaInfo ] );
            m_pDrives[ m_currentMediaInfo ]->AddCompletionCallback( this, NULL );
        }
    }
}

void
MemoryCardManager::StartMemoryCardCheck( IMemoryCardCheckCallback* callback )
{
    m_memcardCheckCallback = callback;
    m_elapsedMemcardCheckTime = 0;

    m_currentMediaInfo = -1;
    m_nextMediaInfo = 0;

    m_memcardCheckingState = MEMCARD_CHECK_IN_PROGRESS;
}

void
MemoryCardManager::UpdateMemoryCardCheck( unsigned int elapsedTime )
{
    m_elapsedMemcardCheckTime += elapsedTime;
    if( m_elapsedMemcardCheckTime > MINIMUM_MEMCARD_CHECK_TIME )
    {
        if( m_memcardCheckingState == MEMCARD_CHECK_COMPLETED )
        {
            if( this->IsMemcardInfoLoaded() )
            {
                this->OnMemoryCardCheckCompleted();
            }
            else
            {
                // memory card info must be loaded/loading during memory card check,
                // and should be unloaded when check is completed
                //
                rAssertMsg( m_memcardInfoLoadState != MEMCARD_INFO_NOT_LOADED,
                            "ERROR: *** Memory card info not loaded during memory card check!" );
            }
        }
    }

    if( m_memcardCheckingState == MEMCARD_CHECK_IN_PROGRESS )
    {
        this->Update( elapsedTime );
    }
}

const IRadDrive::MediaInfo*
MemoryCardManager::GetMediaInfo( unsigned int driveIndex ) const
{
    rAssert( driveIndex < NUM_SAVE_GAME_DRIVES );

    return &(m_mediaInfos[ driveIndex ] );
}

void
MemoryCardManager::LoadMemcardInfo( IMemoryCardInfoLoadCallback* callback )
{
#ifndef WORLD_BUILDER
    if( m_memcardInfoLoadState == MEMCARD_INFO_NOT_LOADED )
    {
MEMTRACK_PUSH_GROUP( "MemcardInfo" );
        bool isIngame = GetGameFlow()->GetCurrentContext() == CONTEXT_GAMEPLAY ||
                        GetGameFlow()->GetCurrentContext() == CONTEXT_PAUSE;

        GameMemoryAllocator heap = isIngame ? GMA_LEVEL_MISSION : GMA_LEVEL_MOVIE;

        HeapMgr()->PushHeap( heap );

        m_memcardInfoLoadCallback = callback;

        rAssert( m_memcardInfo == NULL );
        m_memcardInfo = new radMemcardInfo;
        rAssert( m_memcardInfo != NULL );

        rTunePrintf( ">> Loading memory card info ... ...\n" );

        m_elapsedMemcardInfoLoadTime = radTimeGetMilliseconds();

#if defined(RAD_WIN32) || defined(RAD_UWP)
        // go straight to the requests complete method; there is no
        // memcard info.
        OnProcessRequestsComplete( NULL );
#endif

        HeapMgr()->PopHeap( heap );
MEMTRACK_POP_GROUP( "MemcardInfo" );
    }
    else
    {
        rTuneWarningMsg( false, "WARNING: *** Memory card info already loaded! Ignoring request to load again.\n" );
    }
#endif
}

void
MemoryCardManager::UnloadMemcardInfo()
{
    if( m_memcardInfoLoadState == MEMCARD_INFO_LOAD_COMPLETED )
    {
        if( m_memcardInfo != NULL )
        {
            delete m_memcardInfo;
            m_memcardInfo = NULL;
        }

        rTunePrintf( ">> Memory card info unloaded.\n" );

        m_memcardInfoLoadState = MEMCARD_INFO_NOT_LOADED;
    }
    else
    {
        rTuneWarningMsg( false, "WARNING: *** Memory card info not completely loaded yet! Ignoring request to unload.\n" );
    }
}

bool
MemoryCardManager::IsMemcardInfoLoaded() const
{
    return (m_memcardInfoLoadState == MEMCARD_INFO_LOAD_COMPLETED);
}

void
MemoryCardManager::SetMemcardIconData( char* dataBuffer,
                                       unsigned int dataSize )
{
    rAssert( dataBuffer != NULL && dataSize > 0 );
    rAssert( m_memcardInfo != NULL );

    switch( m_memcardInfoLoadState )
    {
        case MEMCARD_INFO_NOT_LOADED:
        {
            rAssertMsg( false, "*** ERROR: Invalid MemcardInfo load state!" );

            break;
        }

        default:
        {
            break;
        }
    }
}

void
MemoryCardManager::OnProcessRequestsComplete( void* pUserData )
{
    m_memcardInfoLoadState++;

    if( m_memcardInfoLoadState == MEMCARD_INFO_LOAD_COMPLETED )
    {
        // determine amount of time it took to load memcard info
        //
        m_elapsedMemcardInfoLoadTime = radTimeGetMilliseconds() - m_elapsedMemcardInfoLoadTime;

        rTunePrintf( ">> Memory card info loaded. (%d msec)\n",
                     m_elapsedMemcardInfoLoadTime );

        if( m_memcardInfoLoadCallback != NULL )
        {
            m_memcardInfoLoadCallback->OnMemcardInfoLoadComplete();
            m_memcardInfoLoadCallback = NULL;
        }
    }
}

void
MemoryCardManager::SetCurrentDrive( unsigned int driveIndex )
{
    rAssert( driveIndex < NUM_SAVE_GAME_DRIVES );

    this->SetCurrentDrive( m_pDrives[ driveIndex ] );
}
void
MemoryCardManager::ClearCurrentDrive(  )
{
	m_currentDrive = NULL;
}

int
MemoryCardManager::GetCurrentDriveIndex() const
{
    return( this->GetDriveIndex( m_currentDrive ) );
}
const char *
MemoryCardManager::GetDriveName(unsigned int driveIndex) const
{
	rAssert(driveIndex < NUM_SAVE_GAME_DRIVES);
	rAssert(m_pDrives[ driveIndex ]);
	return m_pDrives[driveIndex]->GetDriveName();
}
const char *
MemoryCardManager::GetCurrentDriveVolumeName() const
{
	return m_mediaInfos[ GetCurrentDriveIndex() ].m_VolumeName;
}

bool
MemoryCardManager::IsCurrentDrivePresent( unsigned int currentDriveIndex )
{
	bool isMissing = true;
	if( static_cast<int>( currentDriveIndex ) != -1 )
	{
		isMissing = (m_mediaInfos[ currentDriveIndex ].m_MediaState == IRadDrive::MediaInfo::MediaNotPresent);
	}
	return !isMissing;
}
bool
MemoryCardManager::IsCurrentDriveReady( bool forceSyncUpdate, bool *unformatted , IRadDrive::MediaInfo::MediaState *errorOut)
{
    bool isReady = false;
    bool card_unformatted = false;

    int currentDriveIndex = this->GetCurrentDriveIndex();
	if (unformatted!=NULL)
		*unformatted = false;
    if( currentDriveIndex != -1 )
    {
        if( forceSyncUpdate )
        {
            // force synchronous update on current drive's media info
            //
            m_pDrives[ currentDriveIndex ]->GetMediaInfoSync( &m_mediaInfos[ currentDriveIndex ] );
        }
        if (errorOut)
            *errorOut = m_mediaInfos[ currentDriveIndex ].m_MediaState;

        if (  m_mediaInfos[ currentDriveIndex ].m_MediaState==IRadDrive::MediaInfo::MediaNotFormatted )
        {
            card_unformatted = true;
        }
        isReady = (m_mediaInfos[ currentDriveIndex ].m_MediaState == IRadDrive::MediaInfo::MediaPresent);
		if (unformatted)
			*unformatted = card_unformatted;
        
        if( !isReady 
			&& card_unformatted==false )
              // we need to keep track of unformatted drive so we can format it
		{
            // memory card must have been pulled out, reset current drive
            //
            m_currentDrive = NULL;
        }
    }

    return isReady;
}
 
int
MemoryCardManager::GetAvailableDrives( IRadDrive** pDrives,
                                       IRadDrive::MediaInfo** mediaInfos,
                                       IRadDrive **drive_mounted)
{
    int numAvailableDrives = 0;

    // query for all drives currently present
    //
    for( unsigned int i = 0; i < NUM_SAVE_GAME_DRIVES; i++ )
    {
        rAssert( m_pDrives[ i ] );

        if( m_mediaInfos[ i ].m_MediaState != IRadDrive::MediaInfo::MediaNotPresent )
        {
            if( pDrives != NULL )
            {
                pDrives[ numAvailableDrives ] = m_pDrives[ i ];
            }

            if( mediaInfos != NULL )
            {
                mediaInfos[ numAvailableDrives ] = &m_mediaInfos[ i ];
            }
            if (drive_mounted)
            {
                drive_mounted[i] = m_pDrives[ i ];
            }
            numAvailableDrives++;
        }
        else
        {
            if (drive_mounted)
            {
                drive_mounted[i] = NULL;
            }
        }

    }


    // terminate w/ NULL
    //
    if( pDrives != NULL )
    {
        pDrives[ numAvailableDrives ] = NULL;
    }

    return numAvailableDrives;
}

bool
MemoryCardManager::EnoughFreeSpace( unsigned int driveIndex ) const
{
    rAssert( driveIndex < NUM_SAVE_GAME_DRIVES );

    const unsigned int NUM_FREE_FILES_REQUIRED = 1;

    return( m_mediaInfos[ driveIndex ].m_FreeFiles >= NUM_FREE_FILES_REQUIRED &&
            m_mediaInfos[ driveIndex ].m_FreeSpace >= this->GetSavedGameCreationSize( driveIndex ) );
}

void MemoryCardManager::FormatDrive(unsigned int driveIndex, IMemoryCardFormatCallback *callback)
{ 
	SetCurrentDrive(driveIndex);
	m_formatDriveState = true;
	rAssert(callback);
	m_formatCallback = callback;
	m_currentDrive->FormatAsync();
	m_currentDrive->AddCompletionCallback( this , NULL);
}
void
MemoryCardManager::UpdateMemcardInfo( const char* savedGameTitle, int lineBreak )
{
    rAssert( m_memcardInfo != NULL );
}

void
MemoryCardManager::OnDriveOperationsComplete( void* pUserData )
{ 
	if ( m_formatDriveState )
	{
		m_formatCallback->OnFormatOperationComplete(Success);
		m_formatDriveState = false;
	}
    else if( m_currentState == STATE_OPENING_DRIVES )
    {
        unsigned int driveIndex = reinterpret_cast<uintptr_t>( pUserData );
        rAssert( driveIndex < NUM_SAVE_GAME_DRIVES );

        m_numDrivesOpened++;

        if( m_numDrivesOpened == NUM_SAVE_GAME_DRIVES )
        {
            // done opening all save game drives
            //
            m_currentState = STATE_READY;
        }
    }
    else // checking memory cards during bootup
    {
        rAssert( m_currentMediaInfo == m_nextMediaInfo );
        m_nextMediaInfo = (m_currentMediaInfo + 1) % NUM_SAVE_GAME_DRIVES;

        if( m_memcardCheckingState == MEMCARD_CHECK_IN_PROGRESS )
        {
            if( m_nextMediaInfo == 0 )
            {
                // ok, we finished querying media info from all drives
                m_memcardCheckingState = MEMCARD_CHECK_COMPLETED;
            }
        }
    }
}

bool 
MemoryCardManager::HasSaveGame(unsigned int driveIndex)
{
    rAssert( driveIndex < NUM_SAVE_GAME_DRIVES );
    return  GetGameDataManager()->DoesSaveGameExist( m_pDrives[ driveIndex ], false );
}

bool
MemoryCardManager::OnDriveError( radFileError error,
                                 const char* pDriveName,
                                 void* pUserData )
{

    rDebugPrintf( "*** MEMCARD: ERROR [%d] occurred on drive [%s]! ***\n",
                  error,
                  pDriveName );

	if ( m_formatDriveState )
	{
		m_formatCallback->OnFormatOperationComplete(error);
	}
	else if( m_radDriveErrorCallback != NULL )
	{
		m_radDriveErrorCallback->OnDriveError( error, pDriveName, pUserData );
	}

    // we should always return false, since we're always going to prompt
    // the user first before attempting to retry any operation
    //
    return false;
}

int
MemoryCardManager::GetDriveIndex( IRadDrive* pDrive ) const
{
    int driveIndex = -1;

    if( pDrive != NULL )
    {
        for( unsigned int i = 0; i < NUM_SAVE_GAME_DRIVES; i++ )
        {
            if( strcmp( pDrive->GetDriveName(), SAVE_GAME_DRIVE[ i ] ) == 0 )
            {
                driveIndex = static_cast<int>( i );

                break;
            }
        }
    }

    return driveIndex;
}

//===========================================================================
// Private Member Functions
//===========================================================================

void
MemoryCardManager::DetermineSavedGameCreationSize( unsigned int driveIndex )
{
    rAssert( driveIndex < NUM_SAVE_GAME_DRIVES );
    m_savedGameCreationSize = m_pDrives[ driveIndex ]->GetCreationSize( m_memcardInfo, GetGameDataManager()->GetGameDataSize() );

    rReleasePrintf( 
        "The Simpsons Hit & Run Saved Game File Size = %.2f KB (%d bytes)\n",
        m_savedGameCreationSize / 1024.0f, 
        m_savedGameCreationSize 
    );
}

void
MemoryCardManager::OnMemoryCardCheckCompleted()
{
    radFileError errorCode = Success;
    IRadDrive::MediaInfo::MediaState mediaState = IRadDrive::MediaInfo::MediaPresent;
    int driveIndex = -1;

    // search for a good memory card w/ enough free space to save games
    // 
    bool goodCardExists = false;
    bool fullCardExists = false;

    // and, at the same time, search for most recent save game among
    // all drives
    // 
    int mostRecentSaveGameDriveIndex = -1;
    int mostRecentSaveGameSlot = -1;
    radDate mostRecentTimeStamp;
    mostRecentTimeStamp.m_Year = 0;

    for( unsigned int i = 0; i < NUM_SAVE_GAME_DRIVES; i++ )
    {
        // determine saved game creation size
        //
        this->DetermineSavedGameCreationSize( i );

        if( m_mediaInfos[ i ].m_MediaState == IRadDrive::MediaInfo::MediaPresent )
        {
            unsigned int slot = 0;
            radDate timeStamp;
            bool saveGameExists = GetGameDataManager()->FindMostRecentSaveGame( m_pDrives[ i ],
                                                                                slot,
                                                                                timeStamp );
            if( saveGameExists )
            {
                if( SaveGameInfo::CompareTimeStamps( timeStamp, mostRecentTimeStamp ) > 0 )
                {
                    mostRecentSaveGameDriveIndex = static_cast<int>( i );
                    mostRecentSaveGameSlot = static_cast<int>( slot );
                    memcpy( &mostRecentTimeStamp, &timeStamp, sizeof( radDate ) );
                }
            }

            if( this->EnoughFreeSpace( i ) )
            {
                goodCardExists = true; // we found good card, continue looping to find the latest save game
            }
            else
            {
                if( saveGameExists )
                {
                    goodCardExists = true; // we found good card, continue looping to find the latest save game
                }
            }
        }
    }

#if defined(RAD_WIN32) || defined(RAD_UWP)
    rAssertMsg( m_mediaInfos[ 0 ].m_MediaState == IRadDrive::MediaInfo::MediaPresent,
                "ERROR: Default hard drive didn't mount." );
#endif // RAD_WIN32

    if( m_memcardCheckCallback != NULL )
    {
        m_memcardCheckCallback->OnMemoryCardCheckDone( errorCode,
                                                       mediaState,
                                                       driveIndex,
                                                       mostRecentSaveGameDriveIndex,
                                                       mostRecentSaveGameSlot );
        m_memcardCheckCallback = NULL;
    }
}