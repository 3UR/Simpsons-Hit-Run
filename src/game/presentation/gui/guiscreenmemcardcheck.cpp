//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   CGuiScreenMemCardCheck
//
// Description: Implementation of the CGuiScreenMemCardCheck class.
//
// Authors:     Tony Chu
//
// Revisions		Date			Author	    Revision
//                  2002/07/23      TChu        Created for SRR2
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include <presentation/gui/guiscreenmemcardcheck.h>
#include <presentation/gui/guiscreenmemorycard.h>
#include <presentation/gui/guimanager.h>
#include <presentation/gui/frontend/guiscreenloadgame.h>
#include <presentation/gui/guiscreenprompt.h>

#include <main/game.h>
#include <main/platform.h>
#include <memory/createheap.h>

#include <raddebug.hpp> // Foundation
#include <layer.h>
#include <page.h>
#include <screen.h>
#include <text.h>

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

#ifndef RAD_E3
    // This will enable auto-loading the most recent saved game immediately
    // after the memory card check.
    //
    #define ENABLE_AUTOLOAD_AFTER_MEMCHECK
#endif

enum eMemCardCheckMessage
{
    CHECKING_MEMCARDS_GC,
    CHECKING_MEMCARD_B_GC,
    CHECKING_MEMCARDS_PS2,
    CHECKING_MEMCARD_2_PS2,
    CHECKING_HARDDISK_XBOX,
    CHECKING_HARDDISK_PC,

    NUM_MEMCARD_CHECK_MESSAGES
};

//===========================================================================
// Public Member Functions
//===========================================================================

//===========================================================================
// CGuiScreenMemCardCheck::CGuiScreenMemCardCheck
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
CGuiScreenMemCardCheck::CGuiScreenMemCardCheck
(
	Scrooby::Screen* pScreen,
	CGuiEntity* pParent
)
:   CGuiScreen( pScreen, pParent, GUI_SCREEN_ID_MEMORY_CARD_CHECK ),
    m_messageText( NULL ),
	m_minimumFormatTime(1000),
	m_formatState(false),
    m_StatusPromptShown(false),
    m_isAutoLoadPending( false )
{
    Scrooby::Page* pPage = m_pScroobyScreen->GetPage( "MemCardCheck" );
    rAssert( pPage );

    Scrooby::Layer* foreground = pPage->GetLayer( "Foreground" );

    m_messageText = foreground->GetText( "CheckingMemoryCards" );
    rAssert( m_messageText );
    m_messageText->SetTextMode( Scrooby::TEXT_WRAP );

#if defined(RAD_WIN32) || defined(RAD_UWP)
    m_messageText->SetIndex( CHECKING_HARDDISK_PC );
    // no need to display checking message on WIN32
    m_messageText->SetVisible( false );
    this->SetFadingEnabled( false );
#endif
}


//===========================================================================
// CGuiScreenMemCardCheck::~CGuiScreenMemCardCheck
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
CGuiScreenMemCardCheck::~CGuiScreenMemCardCheck()
{
}


//===========================================================================
// CGuiScreenMemCardCheck::HandleMessage
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
void CGuiScreenMemCardCheck::HandleMessage
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
				if( m_formatResult == Success )
				{
					m_guiManager->DisplayPrompt(PROMPT_FORMAT_SUCCESS_GC + PLATFORM_TEXT_INDEX, this, PROMPT_TYPE_CONTINUE); // format success
					m_formatState = false;
				}
				else if (m_formatResult)
				{
					m_guiManager->DisplayPrompt(PROMPT_FORMAT_FAIL_GC + PLATFORM_TEXT_INDEX, this, PROMPT_TYPE_CONTINUE); // format fail
					m_formatState = false;
				}
			}
		}
	}
	else if ( message == GUI_MSG_MENU_PROMPT_RESPONSE || message == GUI_MSG_ERROR_PROMPT_RESPONSE)
    {
		switch ( param1 )
		{
			case PROMPT_FORMAT_SUCCESS_GC: 
			case PROMPT_FORMAT_SUCCESS_PS2: 
			case PROMPT_FORMAT_SUCCESS_XBOX: // format ok
                this->OnContinue();
				break;

			case PROMPT_FORMAT_FAIL_GC:
			case PROMPT_FORMAT_FAIL_PS2:
			case PROMPT_FORMAT_FAIL_XBOX: // format fail
				this->ReloadScreen(); // user has to retry

				GetMemoryCardManager()->ClearCurrentDrive();
				break;
			case PROMPT_FORMAT_CONFIRM2_GC:
			case PROMPT_FORMAT_CONFIRM2_PS2:
			case PROMPT_FORMAT_CONFIRM2_XBOX: // really format
				if (param2==CGuiMenuPrompt::RESPONSE_YES) 
				{
					m_guiManager->DisplayMessage(CGuiScreenMessage::MSG_ID_FORMATTING_GC + PLATFORM_TEXT_INDEX, this);
					m_formatState = true;
					m_elapsedFormatTime = 0;
					m_formatDone = false;
					GetMemoryCardManager()->FormatDrive(CGuiScreenMemoryCard::s_currentMemoryCardSlot, this);
				}
				else
				{
                    this->ReloadScreen(); // user has to retry
					GetMemoryCardManager()->ClearCurrentDrive();
				}
				break;

			default:
			switch( param2 )
			{
				case (CGuiMenuPrompt::RESPONSE_YES):
				case (CGuiMenuPrompt::RESPONSE_CONTINUE):
				case (CGuiMenuPrompt::RESPONSE_CONTINUE_WITHOUT_SAVE):
				{
					this->ReloadScreen();
					break;
				}
                case (CGuiMenuPrompt::RESPONSE_FORMAT_GC):
                case (CGuiMenuPrompt::RESPONSE_FORMAT_XBOX):
                case (CGuiMenuPrompt::RESPONSE_FORMAT_PS2):
				{
                    m_StatusPromptShown = false;
					m_guiManager->DisplayPrompt(PROMPT_FORMAT_CONFIRM2_GC + PLATFORM_TEXT_INDEX,this);
					break;
				}
				case (CGuiMenuPrompt::RESPONSE_MANAGE_MEMCARDS):
				{
                    IRadMemoryAllocator* allocator = GetAllocator( GMA_PERSISTENT );
                    IRadMemoryHeap* staticHeap = dynamic_cast< IRadMemoryHeap* >( allocator );
                    staticHeap->AllowFreeing( true );
                    GetGame()->GetPlatform()->LaunchDashboard();
					break;
				}
				default:
				{
					break;
				}
			}
		}
    }

    if( m_state == GUI_WINDOW_STATE_RUNNING )
    {
        switch( message )
        {
            case GUI_MSG_UPDATE:
            {
                GetMemoryCardManager()->UpdateMemoryCardCheck( param1 );

                break;
            }
            default:
            {
                break;
            }
        }
    }

	// Propogate the message up the hierarchy.
	//
	CGuiScreen::HandleMessage( message, param1, param2 );
}
void
CGuiScreenMemCardCheck::OnFormatOperationComplete( radFileError errorCode )
{
	if (m_formatDone != true) // check if we already received an error or not
		m_formatResult = errorCode;
	m_formatDone = true;
}

void
CGuiScreenMemCardCheck::OnMemoryCardCheckDone( radFileError errorCode,
                                               IRadDrive::MediaInfo::MediaState mediaState,
                                               int driveIndex,
                                               int mostRecentSaveGameDriveIndex,
                                               int mostRecentSaveGameSlot )
{
#if defined(RAD_WIN32) || defined(RAD_UWP)
    GetMemoryCardManager()->SetCurrentDrive( static_cast<unsigned int>( 0 ) );
#endif

#ifdef ENABLE_AUTOLOAD_AFTER_MEMCHECK
    if( mostRecentSaveGameSlot != -1 )
    {
        CGuiScreenMemoryCard::s_currentMemoryCardSlot = mostRecentSaveGameDriveIndex;
        GetMemoryCardManager()->SetCurrentDrive( static_cast<unsigned int>( mostRecentSaveGameDriveIndex ) );

        CGuiScreenAutoLoad::SetGameSlot( mostRecentSaveGameSlot );

        m_isAutoLoadPending = true;
    }
#endif

    if( errorCode == Success && mediaState == IRadDrive::MediaInfo::MediaPresent )
    {
        this->OnContinue();
    }
    else
    {
        CGuiScreenMemoryCard::s_currentMemoryCardSlot = driveIndex;

        m_StatusPromptShown = true;

        int errorMessage = -1;
        if( mediaState != IRadDrive::MediaInfo::MediaPresent )
        {
            errorMessage = GetErrorMessageIndex( mediaState );
        }
        else if( errorCode != Success )
        {
            errorMessage = GetErrorMessageIndex( errorCode, ERROR_DURING_CHECKING );
        }

		int format_response = 0;

        rAssert( errorMessage != -1 );
        
        m_guiManager->DisplayErrorPrompt( errorMessage, this, ERROR_RESPONSE_CONTINUE );
    }
}

//===========================================================================
// CGuiScreenMemCardCheck::InitIntro
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
void CGuiScreenMemCardCheck::InitIntro()
{
    if( m_firstTimeEntered )
    {
        GetMemoryCardManager()->LoadMemcardInfo();
    }
    m_StatusPromptShown = false;
}


//===========================================================================
// CGuiScreenMemCardCheck::InitRunning
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
void CGuiScreenMemCardCheck::InitRunning()
{
    GetMemoryCardManager()->StartMemoryCardCheck( this );
}


//===========================================================================
// CGuiScreenMemCardCheck::InitOutro
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
void CGuiScreenMemCardCheck::InitOutro()
{
}


//---------------------------------------------------------------------
// Private Functions
//---------------------------------------------------------------------

void
CGuiScreenMemCardCheck::OnContinue()
{
    GetMemoryCardManager()->UnloadMemcardInfo();

    bool isCurrentDriveReady = GetMemoryCardManager()->IsCurrentDriveReady( true );

    if( m_isAutoLoadPending && isCurrentDriveReady )
    {
        m_pParent->HandleMessage( GUI_MSG_GOTO_SCREEN, GUI_SCREEN_ID_AUTO_LOAD, CLEAR_WINDOW_HISTORY );
    }
    else
    {
        m_pParent->HandleMessage( GUI_MSG_MEMCARD_CHECK_COMPLETED );
    }

    m_isAutoLoadPending = false;
}

