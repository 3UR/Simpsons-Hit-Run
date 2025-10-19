//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   CGuiScreenSaveGame
//
// Description: Implementation of the CGuiScreenSaveGame class.
//
// Authors:     Tony Chu
//
// Revisions		Date			Author	    Revision
//                  2002/07/04      TChu        Created for SRR2
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include <presentation/gui/ingame/guiscreensavegame.h>
#include <presentation/gui/guimanager.h>
#include <presentation/gui/guimenu.h> 
#include <presentation/gui/guitextbible.h>
#include <presentation/gui/guiscreenmessage.h>
#include <presentation/gui/guiscreenprompt.h>

#include <data/gamedatamanager.h>
#include <data/savegameinfo.h>
#include <data/memcard/memorycardmanager.h>
#include <memory/srrmemory.h>
#include <gameflow/gameflow.h>

#include <raddebug.hpp>     // Foundation
#include <group.h>
#include <layer.h>
#include <page.h>
#include <screen.h>
#include <text.h>

//===========================================================================
// CGuiScreenSaveGame::CGuiScreenSaveGame
//===========================================================================
// Description: Constructor.
//
// Constraints:	None.
//
// Parameters:	None.
//
// Return:      N/A.
//
//===========================================================================
CGuiScreenSaveGame::CGuiScreenSaveGame
(
	Scrooby::Screen* pScreen,
	CGuiEntity* pParent
)
:
	CGuiScreen( pScreen, pParent, GUI_SCREEN_ID_SAVE_GAME ),
    CGuiScreenLoadSave( pScreen ),
    m_pMenu( NULL ),
	m_pFullText( NULL ),
    m_StatusPromptShown(false),
    m_nonEmptySlots( 0 )
{
MEMTRACK_PUSH_GROUP( "CGUIScreenSaveGame" );
    // Retrieve the Scrooby drawing elements.
    //
    Scrooby::Page* pPage = m_pScroobyScreen->GetPage( "GameSlots" );
    rAssert( pPage );

    Scrooby::Group* menu = pPage->GetGroup( "Menu" );
    rAssert( menu != NULL );

	m_pFullText = pPage->GetText( "FullMessage" );
	rAssert( m_pFullText != NULL );
	m_pFullText->SetVisible(false);
    m_pFullText->SetTextMode( Scrooby::TEXT_WRAP );
    // Create a menu.
    //
    m_pMenu = new(GMA_LEVEL_HUD) CGuiMenu( this, NUM_GAME_SLOTS );
    rAssert( m_pMenu != NULL );

    // Add menu items
    //
    for( unsigned int i = 0; i < NUM_GAME_SLOTS; i++ )
    {
        char objectName[ 32 ];
        sprintf( objectName, "Slot%d", i );

        m_pMenu->AddMenuItem( menu->GetText( objectName ) );
    }

#if defined(RAD_WIN32) || defined(RAD_UWP)
    Scrooby::Text* pText = pPage->GetText( "LoadSaveMessage" );
    if( pText != NULL )
    {
        pText->SetIndex( 1 );
    }
#else
    pPage = m_pScroobyScreen->GetPage( "SelectMemoryDevice" );
    rAssert( pPage != NULL );
    Scrooby::Layer* foreground = pPage->GetLayer( "Foreground" );
    rAssert( foreground != NULL );

    Scrooby::Text* pText = foreground->GetText( "LoadSave" );
    if( pText != NULL )
    {
        pText->SetIndex( 1 );
    }
#endif

    this->AutoScaleFrame( m_pScroobyScreen->GetPage( "BigBoard" ) );
MEMTRACK_POP_GROUP("CGUIScreenSaveGame");
}


//===========================================================================
// CGuiScreenSaveGame::~CGuiScreenSaveGame
//===========================================================================
// Description: Destructor.
//
// Constraints:	None.
//
// Parameters:	None.
//
// Return:      N/A.
//
//===========================================================================
CGuiScreenSaveGame::~CGuiScreenSaveGame()
{
    if( m_pMenu != NULL )
    {
        delete m_pMenu;
        m_pMenu = NULL;
    }
}


//===========================================================================
// CGuiScreenSaveGame::HandleMessage
//===========================================================================
// Description: 
//
// Constraints:	None.
//
// Parameters:	None.
//
// Return:      N/A.
//
//===========================================================================
void CGuiScreenSaveGame::HandleMessage
(
	eGuiMessage message, 
	unsigned int param1,
	unsigned int param2 
)
{
	if (message == GUI_MSG_MESSAGE_UPDATE)
	{
		if (m_formatState)
		{
			m_elapsedFormatTime += param1;
			if (m_elapsedFormatTime > m_minimumFormatTime && m_formatDone)
            {   
                m_StatusPromptShown = true;
                m_formatState = false;

				if( m_formatResult == Success )
				{
                    m_guiManager->DisplayPrompt(PROMPT_FORMAT_SUCCESS_GC + PLATFORM_TEXT_INDEX, this, PROMPT_TYPE_CONTINUE); // format success
                }
				else 
				{
					m_guiManager->DisplayPrompt(PROMPT_FORMAT_FAIL_GC + PLATFORM_TEXT_INDEX, this, PROMPT_TYPE_CONTINUE); // format fail
				}
			}
		}
	} 
	else if (message == GUI_MSG_PROMPT_UPDATE)
	{
 		GetMemoryCardManager()->Update( param1 ); // update so we know the status

        if (m_StatusPromptShown==false) { // check for user unplugging memcard if not showing status
		    int current_drive = GetMemoryCardManager()->GetCurrentDriveIndex();

		    if( !GetMemoryCardManager()->IsCurrentDrivePresent(current_drive) )
			    ReloadScreen();
        }
    }
	else if( message == GUI_MSG_ON_DISPLAY_MESSAGE )
    {
		if( m_operation == SAVE )
		{
			// start the save game process
			//
			rAssert( m_currentSlot != -1 );
            GetGameDataManager()->SaveGame( m_currentSlot, this );
		}
        else if( m_operation == DELETE_GAME )
        {
            // get the filename
            char filename[ radFileFilenameMax + 1 ];
            GetGameDataManager()->FormatSavedGameFilename( filename,
                sizeof( filename ),
                m_currentSlot );
            radFileError err = GetGameDataManager()->DeleteGame( filename, true, this );
        }
		else 
		{
			FormatCurrentDrive();
		}
    }
    else if ( message==GUI_MSG_PROMPT_START_RESPONSE )
    {
        m_pParent->HandleMessage( GUI_MSG_UNPAUSE_INGAME );
    }
	else if (message == GUI_MSG_ERROR_PROMPT_RESPONSE )
	{
		 this->HandleErrorResponse( static_cast<CGuiMenuPrompt::ePromptResponse>( param2 ) );
    }
	else if( message == GUI_MSG_MENU_PROMPT_RESPONSE )
    {
        switch( param1 )
        {

            case PROMPT_FORMAT_CONFIRM_GC: 
		    case PROMPT_FORMAT_CONFIRM_PS2: 
		    case PROMPT_FORMAT_CONFIRM_XBOX: // do you really want to format
            {	
                if (param2==CGuiMenuPrompt::RESPONSE_YES) 
				    m_guiManager->DisplayPrompt(PROMPT_FORMAT_CONFIRM2_GC + PLATFORM_TEXT_INDEX,this);
			    else
			    {
                    this->GotoMemoryCardScreen( true );
			    }
			    break;
            }
		    case PROMPT_FORMAT_CONFIRM2_GC:
		    case PROMPT_FORMAT_CONFIRM2_PS2:
		    case PROMPT_FORMAT_CONFIRM2_XBOX: // really format
            {
			    if (param2==CGuiMenuPrompt::RESPONSE_YES) 
			    {
				    m_operation = FORMAT;
				    m_guiManager->DisplayMessage(CGuiScreenMessage::MSG_ID_FORMATTING_GC + PLATFORM_TEXT_INDEX, this);
			    }
			    else
			    {
                    this->GotoMemoryCardScreen( true );
			    }
			    break;
            }
            case PROMPT_LOAD_DELETE_CORRUPT_GC:
            case PROMPT_LOAD_DELETE_CORRUPT_PS2:
            case PROMPT_LOAD_DELETE_CORRUPT_XBOX:
            case PROMPT_LOAD_DELETE_CORRUPT_XBOX_HD:
            {
                if (param2==CGuiMenuPrompt::RESPONSE_NO)
                {
                    this->ReloadScreen();
                }
                else if (param2==CGuiMenuPrompt::RESPONSE_YES)
                {
                    // get the filename
                    char filename[ radFileFilenameMax + 1 ];
                    GetGameDataManager()->FormatSavedGameFilename( filename,
                        sizeof( filename ),
                        m_currentSlot );
                    radFileError err = GetGameDataManager()->DeleteGame(filename);
                    this->OnDeleteGameComplete( err );
                }
                else
                {
                    rTuneAssert(!"not reached");
                }

                break;
            }    

		    case PROMPT_FORMAT_SUCCESS_GC: 
		    case PROMPT_FORMAT_SUCCESS_PS2: 
		    case PROMPT_FORMAT_SUCCESS_XBOX: // format ok
            {
            // m_currentSlot = 0; on ps2 we don't come here
            // this->SaveGame(); 
			    this->ReloadScreen(); 
			    break;
            }

		    case PROMPT_FORMAT_FAIL_GC:
		    case PROMPT_FORMAT_FAIL_PS2:
		    case PROMPT_FORMAT_FAIL_XBOX: // format fail
            {
			    GetMemoryCardManager()->ClearCurrentDrive();
                this->GotoMemoryCardScreen( true );

			    break;
            }
		
            case PROMPT_DELETE_CORRUPT_SUCCESS_GC:
            case PROMPT_DELETE_CORRUPT_SUCCESS_PS2:
            case PROMPT_DELETE_CORRUPT_SUCCESS_XBOX:

            case PROMPT_DELETE_CORRUPT_FAIL_GC:
            case PROMPT_DELETE_CORRUPT_FAIL_PS2:
            case PROMPT_DELETE_CORRUPT_FAIL_XBOX:
            {
                this->ReloadScreen();
                break;	
            }

            case PROMPT_SAVE_CONFIRM_OVERWRITE_GC:
            {
                if( param2 == CGuiMenuPrompt::RESPONSE_YES )
                {
                    m_guiManager->DisplayPrompt( PROMPT_SAVE_CONFIRM_GC, this, PROMPT_TYPE_SAVE );
                }
                else
                {
                    rAssert( param2 == CGuiMenuPrompt::RESPONSE_NO );

                    this->ReloadScreen();
                }

                break;
            }
            case PROMPT_SAVE_CONFIRM_GC:
            case PROMPT_SAVE_CONFIRM_PS2:
            case PROMPT_SAVE_CONFIRM_XBOX:
            case PROMPT_SAVE_CONFIRM_OVERWRITE_PS2:
            case PROMPT_SAVE_CONFIRM_OVERWRITE_XBOX:
            {
                if( param2 == CGuiMenuPrompt::RESPONSE_YES ||
                    param2 == CGuiMenuPrompt::RESPONSE_SAVE )
                {
                    this->SaveGame();
                }
                else
                {
                    rAssert( param2 == CGuiMenuPrompt::RESPONSE_NO );

                    this->ReloadScreen();
                }

                break;
            }

            case PROMPT_SAVE_SUCCESSFUL:
            {
                m_pParent->HandleMessage( GUI_MSG_ON_SAVE_GAME_COMPLETE );

                break;
            }

            default:
            {
                // handle normal error response, "continue", etc
                this->HandleErrorResponse( static_cast<CGuiMenuPrompt::ePromptResponse>( param2 ) );

                break;
            }
        }
    }

    if( m_state == GUI_WINDOW_STATE_RUNNING )
    {
        if( message == GUI_MSG_CONTROLLER_SELECT &&
            !GetMemoryCardManager()->IsMemcardInfoLoaded() )
        {
            // ignore user select inputs until memcard info is loaded
            //
            return;
        }

        switch( message )
        {
            case GUI_MSG_MENU_SELECTION_MADE:
            {
                m_currentSlot = param1; //  // param1 = slot

                SaveGameInfo saveGameInfo;
                bool corrupt = false;
                IRadDrive* currentDrive = GetMemoryCardManager()->GetCurrentDrive();
                bool saveGameExists = GetGameDataManager()->GetSaveGameInfo( currentDrive, m_currentSlot, &saveGameInfo, &corrupt );
                
                if (corrupt)
                {
                    int plat_index = PLATFORM_TEXT_INDEX;
                    m_guiManager->DisplayPrompt( PROMPT_LOAD_DELETE_CORRUPT_GC + plat_index, this );
                }
                else if( (m_nonEmptySlots & (1 << m_currentSlot)) > 0 )
                {
                    // saved game exists in current slot; prompt w/ overwrite
                    // confirmation message
                    //
                    m_guiManager->DisplayPrompt( PROMPT_SAVE_CONFIRM_OVERWRITE_XBOX, this );
                }
                else
                {
                    m_guiManager->DisplayPrompt( PROMPT_SAVE_CONFIRM_XBOX, this );
                }

                break;
            }
            case GUI_MSG_CONTROLLER_BACK:
            {
                break;
            }
            default:
            {
                break;
            }
        }

        // relay message to menu
        if( m_pMenu != NULL )
        {
            m_pMenu->HandleMessage( message, param1, param2 );
        }

        CGuiScreenLoadSave::HandleMessage( message, param1, param2 );
    }

	// Propogate the message up the hierarchy.
	//
	CGuiScreen::HandleMessage( message, param1, param2 );
}

void
CGuiScreenSaveGame::OnSaveGameComplete( radFileError errorCode )
{
    m_lastError = errorCode;
    m_StatusPromptShown = true;

    if( errorCode == Success )
    {
        m_guiManager->DisplayPrompt( PROMPT_SAVE_SUCCESSFUL, this, PROMPT_TYPE_CONTINUE );
    }
    else
    {
        int errorMessage = GetErrorMessageIndex( errorCode, ERROR_DURING_SAVING );

        switch( errorCode )
        {
            case Success:
            {
                rAssert( false );
                break;
            }
            default:
            {
                m_guiManager->DisplayErrorPrompt( errorMessage, this,
                                                  ERROR_RESPONSE_CONTINUE );
                break;
            }
        }
    }
}

void
CGuiScreenSaveGame::OnDeleteGameComplete( radFileError errorCode )
{
    m_operation = SCREEN_OP_IDLE;
    m_StatusPromptShown = true;

    if( errorCode == Success )
    {
        m_guiManager->DisplayPrompt( PROMPT_DELETE_CORRUPT_SUCCESS_GC + PLATFORM_TEXT_INDEX,
                                     this,
                                     PROMPT_TYPE_CONTINUE );
    }
    else
    {
        m_guiManager->DisplayPrompt( PROMPT_DELETE_CORRUPT_FAIL_GC + PLATFORM_TEXT_INDEX,
                                     this,
                                     PROMPT_TYPE_CONTINUE );
    }
}

void
CGuiScreenSaveGame::HandleErrorResponse( CGuiMenuPrompt::ePromptResponse response )
{
    switch( response )
    {
        case (CGuiMenuPrompt::RESPONSE_CONTINUE):
        case (CGuiMenuPrompt::RESPONSE_CONTINUE_WITHOUT_SAVE):
        {
            if( m_operation == SAVE )
            {
                this->ReloadScreen();
            }
            else if( m_operation == FORMAT )
            {
                this->ReloadScreen();
            }
            else
            {
                this->GotoMemoryCardScreen( true );
            }

            break;
        }
        case (CGuiMenuPrompt::RESPONSE_RETRY):
        {
            if( m_operation == SAVE )
            {
                this->SaveGame();
            }
            else if( m_operation == FORMAT )
            {
                m_guiManager->DisplayMessage(CGuiScreenMessage::MSG_ID_FORMATTING_GC + PLATFORM_TEXT_INDEX, this);
            }
            else
            {
                this->ReloadScreen();
            }

            break;
        }

        case (CGuiMenuPrompt::RESPONSE_FORMAT_GC):
        case (CGuiMenuPrompt::RESPONSE_FORMAT_XBOX):
        case (CGuiMenuPrompt::RESPONSE_FORMAT_PS2):
        {
            m_guiManager->DisplayPrompt(PROMPT_FORMAT_CONFIRM2_GC + PLATFORM_TEXT_INDEX,this);
            break;
        }
        default:
        {
            rTunePrintf( "*** WARNING: Unhandled response for error [%d]!\n", m_lastError );
            rAssert( false );

            this->ReloadScreen();

            break;
        }
    }

    CGuiScreenLoadSave::HandleErrorResponse( response );
}


//===========================================================================
// CGuiScreenSaveGame::InitIntro
//===========================================================================
// Description: 
//
// Constraints:	None.
//
// Parameters:	None.
//
// Return:      N/A.
//
//===========================================================================
void CGuiScreenSaveGame::InitIntro()
{
	unsigned int num_empty_slots = 0;
	bool unformatted = false;
    IRadDrive::MediaInfo::MediaState mediaState;

    m_StatusPromptShown = false;
    m_operation = SCREEN_OP_IDLE;
    
	if( s_forceGotoMemoryCardScreen || !GetMemoryCardManager()->IsCurrentDriveReady( true, &unformatted, &mediaState ))
    {
		if (unformatted && !s_forceGotoMemoryCardScreen)
		{
			m_guiManager->DisplayPrompt(PROMPT_FORMAT_CONFIRM_GC+PLATFORM_TEXT_INDEX,this);
			m_numTransitionsPending = -1; // disable all transitions
		}
		else
		{
			this->GotoMemoryCardScreen();
			m_numTransitionsPending = -1; // disable all transitions
		}
        return;
    }

    m_nonEmptySlots = 0; // reset non-empty slots bitmask

    this->UpdateCurrentMemoryDevice();

    rAssert( m_pMenu );
    m_pMenu->Reset();

    IRadDrive* currentDrive = GetMemoryCardManager()->GetCurrentDrive();

    int currentDriveIndex = GetMemoryCardManager()->GetCurrentDriveIndex();
    bool enoughFreeSpace = GetMemoryCardManager()->EnoughFreeSpace( currentDriveIndex );

    radDate mostRecentTimestamp;
    mostRecentTimestamp.m_Year = 0;

    // update all save game slots display info
    //
    for( unsigned int i = 0; i < NUM_GAME_SLOTS; i++ )
    {
        SaveGameInfo saveGameInfo;
        bool corrupt;
        bool saveGameExists = GetGameDataManager()->GetSaveGameInfo( currentDrive, i, &saveGameInfo, &corrupt );
//        saveGameExists = saveGameExists && saveGameInfo.CheckData();

        Scrooby::Text* slotText = dynamic_cast<Scrooby::Text*>( m_pMenu->GetMenuItem( i )->GetItem() );
        rAssert( slotText != NULL );

        HeapMgr()->PushHeap( GMA_LEVEL_HUD );
       if( saveGameExists )
        {
            if (corrupt)
            {
                UnicodeString corruptSlot;
                corruptSlot.ReadUnicode( GetTextBibleString( "CORRUPT_SLOT_(XBOX)" ) );
                slotText->SetString(0,corruptSlot);
            }
            else
            {
                slotText->SetString( 0, saveGameInfo.m_displayFilename );
            }

            // default to slot with most recent saved game file
            //
            const SaveGameInfoData* pData = saveGameInfo.GetData();
            rAssert( pData != NULL );
            if( SaveGameInfo::CompareTimeStamps( pData->m_timeStamp, mostRecentTimestamp ) > 0 )
            {
                memcpy( &mostRecentTimestamp, &pData->m_timeStamp, sizeof( radDate ) );

                m_pMenu->Reset( i );
            }

            // update non-empty slots bitmask
            //
            m_nonEmptySlots |= (1 << i);
        }
        else
        {
            UnicodeString emptySlot;
            if (enoughFreeSpace)
                emptySlot.ReadUnicode( GetTextBibleString( "EMPTY_SLOT" ) );
            else
                emptySlot.ReadUnicode( GetTextBibleString( "FULL_SLOT" ) );

            slotText->SetString( 0, emptySlot );
			num_empty_slots++;
        }
        HeapMgr()->PopHeap(GMA_LEVEL_HUD);

        // enable slot selection only if save game exists or there's enough
        // free space to save a new game
        //
        m_pMenu->SetMenuItemEnabled( i, saveGameExists || enoughFreeSpace );
    }

	/* display/hide full message */
	if (!enoughFreeSpace)
	{
		int message_index = 0; // no existing slot
		if (num_empty_slots < NUM_GAME_SLOTS )
			message_index = 1; // has existing slot
		// we have 2 group of per platform messages, gc,ps2,xbox_mu, xbox_hd
		message_index = message_index * 4 + PLATFORM_TEXT_INDEX;
#if defined(RAD_WIN32) || defined(RAD_UWP)
        message_index = 9;
#endif
		m_pFullText->SetIndex(message_index);
		m_pFullText->SetVisible(true);
	}
	else
	{
		m_pFullText->SetVisible(false);
	}
	this->SetButtonVisible( BUTTON_ICON_ACCEPT, m_pMenu->GetSelection() != -1 );
}


//===========================================================================
// CGuiScreenSaveGame::InitRunning
//===========================================================================
// Description: 
//
// Constraints:	None.
//
// Parameters:	None.
//
// Return:      N/A.
//
//===========================================================================
void CGuiScreenSaveGame::InitRunning()
{
//    rAssertMsg( GetMemoryCardManager()->IsMemcardInfoLoaded(),
//                "WARNING: *** Memory card info not loaded yet!" );
}


//===========================================================================
// CGuiScreenSaveGame::InitOutro
//===========================================================================
// Description: 
//
// Constraints:	None.
//
// Parameters:	None.
//
// Return:      N/A.
//
//===========================================================================
void CGuiScreenSaveGame::InitOutro()
{
}


void
CGuiScreenSaveGame::GotoMemoryCardScreen( bool isFromPrompt )
{
#if defined(RAD_WIN32) || defined(RAD_UWP)
    m_pParent->HandleMessage( GUI_MSG_BACK_SCREEN );
#else
    if( isFromPrompt )
    {
        s_forceGotoMemoryCardScreen = true;
        this->ReloadScreen();
    }
    else
    {
        m_pParent->HandleMessage( GUI_MSG_GOTO_SCREEN, GUI_SCREEN_ID_MEMORY_CARD );
    }
#endif // RAD_WIN32 || RAD_UWP
}


//---------------------------------------------------------------------
// Private Functions
//---------------------------------------------------------------------

void
CGuiScreenSaveGame::SaveGame()
{
	m_operation = SAVE;
#if defined(RAD_UWP)
    m_guiManager->DisplayMessage( CGuiScreenMessage::MSG_ID_SAVING_GAME_XBOX, this );
#elif defined(RAD_WIN32)
    m_guiManager->DisplayMessage( CGuiScreenMessage::MSG_ID_SAVING_GAME_PC, this );
#endif
}