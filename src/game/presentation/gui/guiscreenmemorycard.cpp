//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   CGuiScreenMemoryCard
//
// Description: Implementation of the CGuiScreenMemoryCard class.
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
#include <presentation/gui/guiscreenmemorycard.h>
#include <presentation/gui/guimenu.h>
#include <presentation/gui/guitextbible.h>
#include <presentation/gui/guiscreenprompt.h>
#include <presentation/gui/guimanager.h>

#include <data/gamedatamanager.h>
#include <data/memcard/memorycardmanager.h>
#include <events/eventmanager.h>
#include <gameflow/gameflow.h>
#include <memory/srrmemory.h>

#include <raddebug.hpp>     // Foundation
#include <layer.h>
#include <group.h>
#include <page.h>
#include <screen.h>
#include <strings/unicodestring.h>
#include <text.h>

const tColour DEFAULT_DISABLED_ITEM_COLOUR_GREY( 128, 128, 128 ); // the same as in guimenu.cpp

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

enum eMemoryCardMenuItem
{
    MENU_ITEM_MEMORY_DEVICE,

    NUM_MEMORY_CARD_MENU_ITEMS
};

bool CGuiScreenLoadSave::s_forceGotoMemoryCardScreen = false;

int CGuiScreenMemoryCard::s_currentMemoryCardSlot = 0;

//===========================================================================
// Public Member Functions
//===========================================================================

CGuiScreenLoadSave::CGuiScreenLoadSave( Scrooby::Screen* pScreen )
:   m_currentSlot( -1 ),
    m_lastError( Success ),
    m_currentMemoryDevice( NULL ),
    m_currentDriveIndex( -1 ),
	m_minimumFormatTime(1000),
	m_formatState(false),
    m_operation( SCREEN_OP_IDLE )
{
    rAssert( pScreen != NULL );
#if defined(RAD_WIN32) || defined(RAD_UWP)
    Scrooby::Page* pPage = pScreen->GetPage( "GameSlots" );
    if( pPage != NULL )
    {
        Scrooby::Text* pText = pPage->GetText( "LoadSaveMessage" );
        if( pText != NULL )
        {
            pText->SetTextMode( Scrooby::TEXT_WRAP );
        }
    }
#else
    Scrooby::Page* pPage = pScreen->GetPage( "SelectMemoryDevice" );
	if( pPage != NULL )
    {
        Scrooby::Layer* foreground = pPage->GetLayer( "Foreground" );
        Scrooby::Text* pText = foreground->GetText( "LoadSave" );
        if( pText != NULL )
        {
            pText->SetTextMode( Scrooby::TEXT_WRAP );
        }

        m_currentMemoryDevice = foreground->GetText( "CurrentMemoryDevice" );
        rAssert( m_currentMemoryDevice != NULL );
        m_currentMemoryDevice->SetTextMode( Scrooby::TEXT_WRAP );

        // set platform-specific text
        //
        Scrooby::Group* selectMemoryDevice = foreground->GetGroup( "SelectMemoryDevice" );
        rAssert( selectMemoryDevice != NULL );

        pText = selectMemoryDevice->GetText( "SelectMemoryDevice" );
        if( pText != NULL )
        {
            pText->SetIndex( PLATFORM_TEXT_INDEX );

            // and apply wrapping
            //
            pText->SetTextMode( Scrooby::TEXT_WRAP );
        }
    }
#endif
}

CGuiScreenLoadSave::~CGuiScreenLoadSave()
{
}

void
CGuiScreenLoadSave::HandleMessage( eGuiMessage message,
                                   unsigned int param1,
                                   unsigned int param2 )
{
    switch( message )
    {
        case GUI_MSG_UPDATE:
        {
            GetMemoryCardManager()->Update( param1 );

            if( !GetMemoryCardManager()->IsCurrentDriveReady() )
            {
                this->GotoMemoryCardScreen();

                return;
            }

            break;
        }
        case GUI_MSG_CONTROLLER_AUX_X:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}
void 
CGuiScreenLoadSave::FormatCurrentDrive()
{
	m_formatState = true;
	m_elapsedFormatTime = 0;
	int currentDrive = GetMemoryCardManager()->GetCurrentDriveIndex();
	m_formatDone = false;
	GetMemoryCardManager()->FormatDrive(currentDrive, this);
}

void
CGuiScreenLoadSave::OnFormatOperationComplete( radFileError errorCode )
{
	if (m_formatDone != true) // check if we already received an error or not
		m_formatResult = errorCode;
	m_formatDone = true;
}

void
CGuiScreenLoadSave::UpdateCurrentMemoryDevice()
{
    // update current drive index
    //
    m_currentDriveIndex = GetMemoryCardManager()->GetCurrentDriveIndex();
}

void
CGuiScreenLoadSave::HandleErrorResponse( CGuiMenuPrompt::ePromptResponse response )
{
}


//===========================================================================
// CGuiScreenMemoryCard::CGuiScreenMemoryCard
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
CGuiScreenMemoryCard::CGuiScreenMemoryCard
(
	Scrooby::Screen* pScreen,
	CGuiEntity* pParent
)
:   CGuiScreen( pScreen, pParent, GUI_SCREEN_ID_MEMORY_CARD,
                SCREEN_FX_FADE | SCREEN_FX_SLIDE_Y ),
    m_layerSelectMemoryDevice( NULL ),
    m_layerNoMemoryDevice( NULL ),
    m_pMenu( NULL ),
    m_numAttachedDevices( -1 )
{
    // Retrieve the Scrooby drawing elements.
    //
    Scrooby::Page* pPage;
	pPage = m_pScroobyScreen->GetPage( "MemoryCard" );
	rAssert( pPage );

    m_layerSelectMemoryDevice = pPage->GetGroup( "SelectMemoryDevice" );
    rAssert( m_layerSelectMemoryDevice );
    m_layerSelectMemoryDevice->SetVisible( false );

    m_layerNoMemoryDevice = pPage->GetGroup( "NoMemoryDevice" );
    rAssert( m_layerNoMemoryDevice );
    m_layerNoMemoryDevice->SetVisible( false );
 
	m_memStatusText = pPage->GetText("Status");
	rAssert(m_memStatusText);

	m_memStatusText->SetTextMode( Scrooby::TEXT_WRAP );

    // hide free space display for non-Xbox platforms
    //
    Scrooby::Group* freeSpace = pPage->GetGroup( "FreeSpace" );
    rAssert( freeSpace != NULL );
    freeSpace->SetVisible( false );

    // set platform-specific text
    //
    Scrooby::Text* pText = m_layerSelectMemoryDevice->GetText( "SelectMemoryDevice" );
    if( pText != NULL )
    {
        pText->SetIndex( PLATFORM_TEXT_INDEX );

        // and apply wrapping
        //
        pText->SetTextMode( Scrooby::TEXT_WRAP );
    }

    pText = m_layerNoMemoryDevice->GetText( "NoMemoryDevice" );
    if( pText != NULL )
    {
        pText->SetIndex( PLATFORM_TEXT_INDEX );

        // and apply wrapping
        //
        pText->SetTextMode( Scrooby::TEXT_WRAP );
    }

    pText = m_layerSelectMemoryDevice->GetText( "MemoryDevice" );
    if( pText != NULL )
    {
        pText->SetTextMode( Scrooby::TEXT_WRAP );
    }

    // Create a menu.
    //
    m_pMenu = new CGuiMenu( this, NUM_MEMORY_CARD_MENU_ITEMS, GUI_TEXT_MENU, MENU_SFX_NONE );
    rAssert( m_pMenu != NULL );

    // Add menu items
    //
    Scrooby::Group* selectMemoryDevice = pPage->GetGroup( "SelectMemoryDevice" );
    m_pMenu->AddMenuItem( selectMemoryDevice->GetText( "SelectMemoryDevice" ),
                          selectMemoryDevice->GetText( "MemoryDevice" ),
                          NULL,
                          NULL,
                          selectMemoryDevice->GetSprite( "LArrow" ),
                          selectMemoryDevice->GetSprite( "RArrow" ) );
 	
	Scrooby::Text * selectDeviceText = selectMemoryDevice->GetText( "SelectMemoryDevice" );
	selectDeviceText->SetTextMode( Scrooby::TEXT_WRAP );

    this->AutoScaleFrame( m_pScroobyScreen->GetPage( "BigBoard" ) );
}


//===========================================================================
// CGuiScreenMemoryCard::~CGuiScreenMemoryCard
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
CGuiScreenMemoryCard::~CGuiScreenMemoryCard()
{
    if( m_pMenu != NULL )
    {
        delete m_pMenu;
        m_pMenu = NULL;
    }
}


//===========================================================================
// CGuiScreenMemoryCard::HandleMessage
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
void CGuiScreenMemoryCard::HandleMessage
(
	eGuiMessage message, 
	unsigned int param1,
	unsigned int param2 
)
{
    if ( message==GUI_MSG_PROMPT_START_RESPONSE )
    {
        m_pParent->HandleMessage( GUI_MSG_UNPAUSE_INGAME );
    }
    else if( m_state == GUI_WINDOW_STATE_RUNNING || message == GUI_MSG_MENU_PROMPT_RESPONSE)
    {
        switch( message )
        {
            case GUI_MSG_UPDATE:
            {
                GetMemoryCardManager()->Update( param1 );

                this->UpdateDeviceList();

                break;
            }

            case GUI_MSG_MENU_SELECTION_VALUE_CHANGED:
            {
                rAssert( param1 == MENU_ITEM_MEMORY_DEVICE );

                this->UpdateFreeSpace( param2 );

                break;
            }

            case GUI_MSG_MENU_SELECTION_MADE:
            {
                rAssert( param1 == MENU_ITEM_MEMORY_DEVICE );

                int selectedDevice = m_pMenu->GetSelectionValue( MENU_ITEM_MEMORY_DEVICE );

				GetMemoryCardManager()->SetCurrentDrive( m_availableDrives[ selectedDevice ] );

                s_currentMemoryCardSlot = GetMemoryCardManager()->GetCurrentDriveIndex();

                CGuiScreenLoadSave::s_forceGotoMemoryCardScreen = false;

                m_pParent->HandleMessage( GUI_MSG_BACK_SCREEN );

                break;
            }

            case GUI_MSG_CONTROLLER_BACK:
            {
                if( !GetMemoryCardManager()->IsCurrentDriveReady( true ) ||
                    CGuiScreenLoadSave::s_forceGotoMemoryCardScreen )
                {
                    if( GetGameFlow()->GetCurrentContext() == CONTEXT_FRONTEND )
                    {
                        this->StartTransitionAnimation( 230, 260 );
                    }

                    m_pParent->HandleMessage( GUI_MSG_BACK_SCREEN, 1 );

                    GetEventManager()->TriggerEvent( EVENT_FE_MENU_BACK );
                }

                break;
            }
            case GUI_MSG_MENU_PROMPT_RESPONSE:
            {
                ReloadScreen();
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
    }

	// Propogate the message up the hierarchy.
	//
	CGuiScreen::HandleMessage( message, param1, param2 );
}


//===========================================================================
// CGuiScreenMemoryCard::InitIntro
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
void CGuiScreenMemoryCard::InitIntro()
{


	this->UpdateDeviceList( true );

    // set selection to current device, if still attached
    //
    IRadDrive* currentDrive = GetMemoryCardManager()->GetCurrentDrive();
    if( currentDrive != NULL )
    {
        if( GetMemoryCardManager()->IsCurrentDriveReady( true ) )
        {
            for( int i = 0; i < m_numAttachedDevices; i++ )
            {
                if( m_availableDrives[ i ] == currentDrive )
                {
                    rAssert( m_pMenu );
                    m_pMenu->SetSelectionValue( MENU_ITEM_MEMORY_DEVICE, i );

                    break;
                }
            }
        }
    }
	this->UpdateFreeSpace( m_pMenu->GetSelectionValue( MENU_ITEM_MEMORY_DEVICE ) );
}


//===========================================================================
// CGuiScreenMemoryCard::InitRunning
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
void CGuiScreenMemoryCard::InitRunning()
{
}


//===========================================================================
// CGuiScreenMemoryCard::InitOutro
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
void CGuiScreenMemoryCard::InitOutro()
{
}


//---------------------------------------------------------------------
// Private Functions
//---------------------------------------------------------------------

void
CGuiScreenMemoryCard::UpdateDeviceList( bool forceUpdate )
{
    IRadDrive* currentSelectedDrive = NULL;

    if( m_numAttachedDevices > 0 )
    {
        currentSelectedDrive = m_availableDrives[ m_pMenu->GetSelectionValue( MENU_ITEM_MEMORY_DEVICE ) ];
    }

    int numAvailableDrives = GetMemoryCardManager()->GetAvailableDrives( m_availableDrives,
                                                                         m_mediaInfos);

    bool hasMoreDrive = false;
    bool memoryDevicesAvailable = (numAvailableDrives > 0);

//    this->SetButtonVisible( BUTTON_ICON_BACK, !memoryDevicesAvailable );

    if( numAvailableDrives == m_numAttachedDevices &&
        !forceUpdate )
    {
        // since number of attached memory devices remain the same,
        // assume no other changes occurred since last poll
        //
        return;
    }
    else
    {
        if (numAvailableDrives > m_numAttachedDevices)
            hasMoreDrive = true;
        // update number of attached memory devices
        //
        m_numAttachedDevices = numAvailableDrives;
    }

	this->SetButtonVisible( BUTTON_ICON_ACCEPT, memoryDevicesAvailable );

    for( int i = 0; i < numAvailableDrives; i++ )
    {
        rAssert( m_availableDrives[ i ] );
        rAssert( m_mediaInfos[ i ] );

        char textBibleEntry[ 32 ];
        sprintf( textBibleEntry, "XBOX_U:" );  // Temp.

        HeapMgr()->PushHeap( GetGameFlow()->GetCurrentContext() == CONTEXT_FRONTEND ?
                             GMA_LEVEL_FE : GMA_LEVEL_HUD );

        UnicodeString deviceName;
        deviceName.ReadUnicode( GetTextBibleString( textBibleEntry ) );

        Scrooby::Text* memoryDeviceText = dynamic_cast<Scrooby::Text*>( m_pMenu->GetMenuItem( MENU_ITEM_MEMORY_DEVICE )->GetItemValue() );
        rAssert( memoryDeviceText != NULL );
        memoryDeviceText->SetString( i, deviceName );

        HeapMgr()->PopHeap(GetGameFlow()->GetCurrentContext() == CONTEXT_FRONTEND ?
                             GMA_LEVEL_FE : GMA_LEVEL_HUD);

    }

    m_layerSelectMemoryDevice->SetVisible( memoryDevicesAvailable );
    m_layerNoMemoryDevice->SetVisible( !memoryDevicesAvailable );

    rAssert( m_pMenu );
    if( memoryDevicesAvailable )
    {
        m_pMenu->GetMenuItem( MENU_ITEM_MEMORY_DEVICE )->m_attributes |= SELECTABLE;
    }
    else
    {
        m_pMenu->GetMenuItem( MENU_ITEM_MEMORY_DEVICE )->m_attributes &= ~SELECTABLE;
        m_pMenu->SetSelectionValueCount( MENU_ITEM_MEMORY_DEVICE, 1 ); // to disable left/right scrolling
    }
//    m_pMenu->SetMenuItemEnabled( MENU_ITEM_MEMORY_DEVICE, memoryDevicesAvailable );

    if( memoryDevicesAvailable )
    {
        m_pMenu->SetSelectionValueCount( MENU_ITEM_MEMORY_DEVICE, numAvailableDrives );

        m_pMenu->SetSelectionValue( MENU_ITEM_MEMORY_DEVICE, 0 );

        for( int i = 0; i < m_numAttachedDevices; i++ )
        {
            if( m_availableDrives[ i ] == currentSelectedDrive )
            {
                m_pMenu->SetSelectionValue( MENU_ITEM_MEMORY_DEVICE, i );

                break;
            }
        }
    }
}

void
CGuiScreenMemoryCard::UpdateFreeSpace( unsigned int currentDriveIndex )
{
    Scrooby::BoundedDrawable* menu_drawable 
        = m_pMenu->GetMenuItem(MENU_ITEM_MEMORY_DEVICE)->GetItemValue();
    
    menu_drawable->SetColour(m_pMenu->GetHighlightColour());

    m_memStatusText->SetVisible(false);
 
    // enable selection
    //
    if( m_numAttachedDevices > 0 )
    {
        m_pMenu->GetMenuItem( MENU_ITEM_MEMORY_DEVICE )->m_attributes |= SELECTABLE;
        SetButtonVisible( BUTTON_ICON_ACCEPT, true );
    }

	HeapMgr()->PushHeap( GetGameFlow()->GetCurrentContext() == CONTEXT_FRONTEND ?
                         GMA_LEVEL_FE : GMA_LEVEL_HUD );

	if (((int)currentDriveIndex < m_numAttachedDevices) && m_mediaInfos[ currentDriveIndex ]->m_MediaState != IRadDrive::MediaInfo::MediaPresent)
	{
		m_memStatusText->SetVisible(true);
		m_memStatusText->SetIndex(
			(m_mediaInfos[ currentDriveIndex ]->m_MediaState - IRadDrive::MediaInfo::MediaNotFormatted) * 3 
			+ PLATFORM_TEXT_INDEX);
		bool disable = true;

		if (disable) 
		{
			// disable selection
			m_pMenu->GetMenuItem( MENU_ITEM_MEMORY_DEVICE )->m_attributes &= ~SELECTABLE;
			SetButtonVisible(BUTTON_ICON_ACCEPT , false);
            // change color
            menu_drawable->SetColour( DEFAULT_DISABLED_ITEM_COLOUR_GREY );
		}
	}
    else // no error
    {
        if ( ((int)currentDriveIndex < m_numAttachedDevices) && GetMemoryCardManager()->IsMemcardInfoLoaded() )
        {
            // this drive index is different than currentDriveIndex
            int memcard_driveindex = GetMemoryCardManager()->GetDriveIndex(m_availableDrives[ currentDriveIndex ]);
            rTuneAssert(memcard_driveindex >= 0);
            if ( GetMemoryCardManager()->EnoughFreeSpace( memcard_driveindex )==false
                && GetGameDataManager()->DoesSaveGameExist(m_availableDrives[ currentDriveIndex ], false)==false ) // check for full without save game
            { 
                int message_index = 5 * 3 + PLATFORM_TEXT_INDEX;

                m_memStatusText->SetVisible(true);
                m_memStatusText->SetIndex( message_index );

                // change color
                menu_drawable->SetColour( DEFAULT_DISABLED_ITEM_COLOUR_GREY ); 

                // disable selection
                m_pMenu->GetMenuItem( MENU_ITEM_MEMORY_DEVICE )->m_attributes &= ~SELECTABLE;
                SetButtonVisible(BUTTON_ICON_ACCEPT , false);
            }
        }
    }

	HeapMgr()->PopHeap( GetGameFlow()->GetCurrentContext() == CONTEXT_FRONTEND ?
						GMA_LEVEL_FE : GMA_LEVEL_HUD );

}

