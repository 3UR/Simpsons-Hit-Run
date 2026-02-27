//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   CGuiScreenMessage
//
// Description: Implementation of the CGuiScreenMessage class.
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
#include <presentation/gui/guiscreenmessage.h>
#include <presentation/gui/guimenu.h>
#include <presentation/gui/utility/specialfx.h>
#include <presentation/gui/guiscreenmemorycard.h>
#include <presentation/gui/guitextbible.h>
#include <presentation/gui/ingame/guimanageringame.h>

#include <data/memcard/memorycardmanager.h> 
#include <gameflow/gameflow.h>
#include <memory/srrmemory.h>

#include <p3d/unicode.hpp>
#include <raddebug.hpp> // Foundation
#include <layer.h>
#include <page.h>
#include <screen.h>
#include <sprite.h>
#include <text.h>

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================

int CGuiScreenMessage::s_ControllerDisconnectedPort = 0;

int CGuiScreenMessage::s_messageIndex = -1;
CGuiEntity* CGuiScreenMessage::s_pMessageCallback = NULL;

//===========================================================================
// Public Member Functions
//===========================================================================

//===========================================================================
// CGuiScreenMessage::CGuiScreenMessage
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
CGuiScreenMessage::CGuiScreenMessage
(
	Scrooby::Screen* pScreen,
	CGuiEntity* pParent
)
:   CGuiScreen( pScreen, pParent, GUI_SCREEN_ID_GENERIC_MESSAGE ),
    m_pMenu( NULL ),
    m_messageText( NULL ),
    m_messageIcon( NULL ),
    m_elapsedTime( 0 )
{
    m_originalStringBuffer[ 0 ] = '\0';

MEMTRACK_PUSH_GROUP( "GUIScreenMessage" );
    // Retrieve the Scrooby drawing elements.
    //
    Scrooby::Page* pPage = m_pScroobyScreen->GetPage( "Message" );
    rAssert( pPage != NULL );

    Scrooby::Layer* pLayer = pPage->GetLayer( "Foreground" );
    rAssert( pLayer != NULL );

    m_messageText = pLayer->GetText( "Message" );
    rAssert( m_messageText );

    // wrap text message
    //
    m_messageText->SetTextMode( Scrooby::TEXT_WRAP );
    m_messageText->ResetTransformation();
    m_messageText->ScaleAboutCenter(0.9f);

    // Retrieve the Scrooby drawing elements (from MessageBox page).
    //
    pPage = m_pScroobyScreen->GetPage( "MessageBox" );
    rAssert( pPage != NULL );

    m_messageIcon = pPage->GetSprite( "ErrorIcon" );

    this->AutoScaleFrame( pPage );

	this->SetFadeTime( 0.0f );
MEMTRACK_POP_GROUP( "GUIScreenMessage" );
}


//===========================================================================
// CGuiScreenMessage::~CGuiScreenMessage
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
CGuiScreenMessage::~CGuiScreenMessage()
{
    if( m_pMenu != NULL )
    {
        delete m_pMenu;
        m_pMenu = NULL;
    }
}


//===========================================================================
// CGuiScreenMessage::HandleMessage
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
void CGuiScreenMessage::HandleMessage
(
	eGuiMessage message, 
	unsigned int param1,
	unsigned int param2 
)
{
    if( m_state == GUI_WINDOW_STATE_RUNNING )
    {
        switch( message )
        {
            case GUI_MSG_UPDATE:
            {
                m_elapsedTime += param1;

                // pulse message icon
                //
                float scale = GuiSFX::Pulse( (float)m_elapsedTime,
                                             500.0f,
                                             1.0f * MESSAGE_ICON_CORRECTION_SCALE,
                                             0.1f * MESSAGE_ICON_CORRECTION_SCALE );

                rAssert( m_messageIcon );
                m_messageIcon->ResetTransformation();
                m_messageIcon->ScaleAboutCenter( scale );

                if( s_pMessageCallback != NULL )
                {
				    s_pMessageCallback->HandleMessage( GUI_MSG_MESSAGE_UPDATE, param1 );
                }

                break;
            }

            case GUI_MSG_CONTROLLER_BACK:
            {
                // ignore BACK inputs, thereby, not allowing users to back
                // out of prompt
                return;

                break;
            }
/*
            case GUI_MSG_MENU_SELECTION_MADE:
            {
                rAssert( s_pMessageCallback );
                s_pMessageCallback->HandleMessage( GUI_MSG_MENU_MESSAGE_RESPONSE,
                                                   param1, param2 );

                break;
            }
*/
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
    else if( m_state == GUI_WINDOW_STATE_OUTRO )
    {
        if( m_numTransitionsPending < 0 )
        {
            // restore original string buffer
            //
            if( m_originalStringBuffer[ 0 ] != '\0' )
            {
                rAssert( m_messageText != NULL );
                UnicodeChar* stringBuffer = m_messageText->GetStringBuffer();
                rAssert( stringBuffer != NULL );

                p3d::UnicodeStrCpy( static_cast<P3D_UNICODE*>( m_originalStringBuffer ),
                                    static_cast<P3D_UNICODE*>( stringBuffer ),
                                    sizeof( m_originalStringBuffer ) );

                m_originalStringBuffer[ 0 ] = '\0';
            }
        }
    }

	// Propogate the message up the hierarchy.
	//
	CGuiScreen::HandleMessage( message, param1, param2 );
}


void
CGuiScreenMessage::Display( int messageIndex, CGuiEntity* pCallback )
{
    s_messageIndex = messageIndex;
    s_pMessageCallback = pCallback;
}
void
CGuiScreenMessage::GetControllerDisconnectedMessage(int controller_id, char *str_buffer, int max_char)
{
    P3D_UNICODE* multitapString = NULL;

    s_ControllerDisconnectedPort = controller_id;

    P3D_UNICODE* uni_string = GetTextBibleString( "MSG_CONTROLLER_DISCONNECTED_(XBOX)" );

    CGuiScreenMessage::ConvertUnicodeToChar(str_buffer, uni_string, max_char);

    if( multitapString != NULL )
    {
        delete multitapString;
        multitapString = NULL;
    }
}
void
CGuiScreenMessage::ConvertUnicodeToChar(char *str, P3D_UNICODE* uni_str, int max_char)
{
    const UnicodeChar CONTROLLER_PORT_WILDCARD_CHARACTER = 0xa5;// since > 0x7f, '�' wont work;
    const UnicodeChar NEWLINE_CHARACTER = 0x5c; //'\';
    int i = 0;
    while (*uni_str && i < max_char)
    {
        if (*uni_str==NEWLINE_CHARACTER)
            *str = '\n';
        else if (*uni_str==CONTROLLER_PORT_WILDCARD_CHARACTER)
        {
            *str = '1' + s_ControllerDisconnectedPort;
        }
        else
            *str = (char)*uni_str;
        
        
        i++;
        str++;
        uni_str++;
    }
    *str++ = 0;
}
void
CGuiScreenMessage::FormatMessage( Scrooby::Text* pText,
                                  UnicodeChar* originalStringBuffer,
                                  int stringBufferLength )
{
    const UnicodeChar SLOT_WILDCARD_CHARACTER = '$';
	const UnicodeChar BLOCKS_WILDCARD_CHARACTER = '#';
	const UnicodeChar CONTROLLER_PORT_WILDCARD_CHARACTER = 0xa5;// since > 0x7f, '�' wont work;

    rAssert( pText != NULL );
    UnicodeChar* stringBuffer = pText->GetStringBuffer();
    rAssert( stringBuffer != NULL );

    // save copy of original string buffer
    //
    if( originalStringBuffer != NULL )
    {
        p3d::UnicodeStrCpy( static_cast<P3D_UNICODE*>( stringBuffer ),
                            static_cast<P3D_UNICODE*>( originalStringBuffer ),
                            stringBufferLength );
    }
 
	for( int i = 0; stringBuffer[ i ] != '\0'; i++ )
	{
		// search for slot wildcard character
		//
		if( stringBuffer[ i ] == CONTROLLER_PORT_WILDCARD_CHARACTER )
		{
			stringBuffer[ i ] = '1' + s_ControllerDisconnectedPort;
			break;
		}
	}
}

//===========================================================================
// CGuiScreenMessage::InitIntro
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
void CGuiScreenMessage::InitIntro()
{
    rAssert( m_messageText );
    rAssert( s_messageIndex >= 0 &&
             s_messageIndex < m_messageText->GetNumOfStrings() );

    // set message text
    //
    m_messageText->SetIndex( s_messageIndex );

    CGuiScreenMessage::FormatMessage( m_messageText,
                                      m_originalStringBuffer,
                                      sizeof( m_originalStringBuffer ) );

    rAssert( m_messageIcon );
    m_messageIcon->ResetTransformation();

    m_elapsedTime = 0;

/*
    // show/hide accept button depending on whether menu is visible or not
    //
    rAssert( s_menuGroup );
    this->SetButtonVisible( BUTTON_ICON_ACCEPT, s_menuGroup->IsVisible() );
*/
}


//===========================================================================
// CGuiScreenMessage::InitRunning
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
void CGuiScreenMessage::InitRunning()
{
    if( s_pMessageCallback != NULL )
    {
        s_pMessageCallback->HandleMessage( GUI_MSG_ON_DISPLAY_MESSAGE );
    }
}


//===========================================================================
// CGuiScreenMessage::InitOutro
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
void CGuiScreenMessage::InitOutro()
{
}


//---------------------------------------------------------------------
// Private Functions
//---------------------------------------------------------------------

