//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


//=============================================================================
//
// File:        binkrenderstrategy.cpp
// Subsystem:   Foundation Technologies - Movie
//
// Description:	Bink render stratetegy for all platforms
//
// Date:        October 31, 2002 RWS
//
//=============================================================================

//=============================================================================
// Include Files
//=============================================================================

#include "pch.hpp"
#include <radoptions.hpp>
#include <p3d/shader.hpp>
#include <p3d/utility.hpp>

#include <radprofiler.hpp>
#include "binkrenderstrategy.hpp"

extern bool inLoop;

//=============================================================================
// radMovieRenderStrategyBink::radMovieRenderStrategyBink
//=============================================================================

radMovieRenderStrategyBink::radMovieRenderStrategyBink( void )
    :
    radRefCount( 0 ),
    m_pShader( NULL ),
    m_MoviePosX( 0 ),
    m_MoviePosY( 0 ),
    m_MovieWidth( 0 ),
    m_MovieHeight( 0 ),
    m_DisplayMultiplier( 1.0f ),
    m_NumTiles( 0 ),
    m_CurrentDestIndex( 0 ),
    m_DestLocked( false )
{
    ::memset( m_pTile, 0, sizeof( m_pTile ) );

    // The shader is associated with the render strategy for both life times

    m_pShader = new tShader( );
    rAssert( m_pShader != NULL );
    m_pShader->AddRef();
}

//=============================================================================
// radMovieRenderStrategyBink::~radMovieRenderStrategyBink
//=============================================================================

radMovieRenderStrategyBink::~radMovieRenderStrategyBink( void )
{
    if( m_pShader != NULL )
    {
        m_pShader->Release( );
        m_pShader = NULL;
    }

    for( unsigned int i = 0; i < RMV_MAX_NUM_TILES; i++ )
    {
        if( m_pTile[ i ].m_pTexture != NULL )
        {
            m_pTile[ i ].m_pTexture->Release( );
            m_pTile[ i ].m_pTexture = NULL;
        }
    }
}

//=============================================================================
// radMovieRenderStrategyBink::ResetDestinations
//=============================================================================

void radMovieRenderStrategyBink::ResetDestinations( void )
{
    rAssert( m_DestLocked != true );
    m_CurrentDestIndex = 0;
}

//=============================================================================
// radMovieRenderStrategyBink::LockNextDestination
//=============================================================================

unsigned int radMovieRenderStrategyBink::LockNextDestination( IRadMovieRenderStrategy::LockedDestination * pLockedDestination )
{
    rAssert( m_DestLocked == false );

    if( m_CurrentDestIndex < m_NumTiles )
    {
        rAssert( m_pTile[ m_CurrentDestIndex ].m_pTexture != NULL );

        m_DestLocked = true;        

        pddiLockInfo * pPddiLockInfo = m_pTile[ m_CurrentDestIndex ].m_pTexture->Lock( 0, 0 );

        pLockedDestination->m_pDest = pPddiLockInfo->bits;
        pLockedDestination->m_DestPitch = pPddiLockInfo->pitch;
        pLockedDestination->m_DestPosX = 0;
        pLockedDestination->m_DestPosY = 0;
        pLockedDestination->m_SrcPosX = m_pTile[ m_CurrentDestIndex ].m_PosX;
        pLockedDestination->m_SrcPosY = m_pTile[ m_CurrentDestIndex ].m_PosY;
        pLockedDestination->m_Width = m_pTile[ m_CurrentDestIndex ].m_Width;
        pLockedDestination->m_Height = m_pTile[ m_CurrentDestIndex ].m_Height;

#ifdef RAD_MOVIEPLAYER_USE_BINK
        if( pLockedDestination->m_SrcPosY > 0 )
        {
            pLockedDestination->m_SrcPosY -= 1;
        }
#endif

        return m_NumTiles - m_CurrentDestIndex;
    }
    else
    {
        return 0;
    }
}

//=============================================================================
// radMovieRenderStrategyBink::SetParameters
//=============================================================================

void radMovieRenderStrategyBink::UnlockDestination( void )
{
    rAssert( m_DestLocked == true );

    m_pTile[ m_CurrentDestIndex ].m_pTexture->Unlock( 0 );
    m_CurrentDestIndex++;
    m_DestLocked = false;
}

//=============================================================================
// radMovieRenderStrategyBink::SetParameters
//=============================================================================

void radMovieRenderStrategyBink::ChangeParameters( unsigned int width, unsigned int height )
{
    rAssert( width > 0 && height > 0 );
    rAssert( width <= ( unsigned int ) p3d::display->GetWidth( ) );
    rAssert( height <= ( unsigned int ) p3d::display->GetHeight( ) );

    //
    // Only allocate things if the width or height has changed
    //

    if( width != m_MovieWidth || height != m_MovieHeight )
    {
        // Remember the new dimensions

        m_MovieWidth = width;
        m_MovieHeight = height;

        // Assume that we'll fit the screen
        if ( p3d::display->GetWidth( ) >= p3d::display->GetHeight( ) )
        {
            m_DisplayMultiplier = (float) p3d::display->GetHeight( ) / ( float ) m_MovieHeight;
        }
        else
        {
            m_DisplayMultiplier = (float) p3d::display->GetWidth( ) / ( float ) m_MovieWidth;
        }

        m_MoviePosX = (int) ( ( p3d::display->GetWidth( ) - m_MovieWidth * m_DisplayMultiplier ) / 2.0f );
        m_MoviePosY = (int) ( ( p3d::display->GetHeight( ) - m_MovieHeight * m_DisplayMultiplier ) / 2.0f );

        // How many tiles will this movie require?

        unsigned int horizontalTiles = ( width / RMV_TEXTURE_MAX_TEX_DIM ) + 1;
        unsigned int verticalTiles = ( height / RMV_TEXTURE_MAX_TEX_DIM ) + 1;
        m_NumTiles = horizontalTiles * verticalTiles;
        rAssert( m_NumTiles <= RMV_MAX_NUM_TILES );

        // Create the required number of textures and fill in tile info

        unsigned int tileIndex = 0;

        for( unsigned int y = 0; y < verticalTiles; y++ )
        {
            for( unsigned int x = 0; x < horizontalTiles; x++ )
            {
                if( m_pTile[ tileIndex ].m_pTexture != NULL )
                {
                    m_pTile[ tileIndex ].m_pTexture->Release( );
                    m_pTile[ tileIndex ].m_pTexture = NULL;
                }

                m_pTile[ tileIndex ].m_pTexture = new tTexture( );
                rAssert( m_pTile[ tileIndex ].m_pTexture != NULL );
                m_pTile[ tileIndex ].m_pTexture->AddRef( );

                //
                // Create texture
                //
                
                bool wasTextureCreated = m_pTile[ tileIndex ].m_pTexture->Create( RMV_TEXTURE_MAX_TEX_DIM, RMV_TEXTURE_MAX_TEX_DIM, RMV_TEXTURE_BITDEPTH, 8, 0, PDDI_TEXTYPE_RGB );
                
                rAssert( wasTextureCreated == true );

                m_pTile[ tileIndex ].m_PosX = x * RMV_TEXTURE_MAX_TEX_DIM;
                m_pTile[ tileIndex ].m_PosY = y * RMV_TEXTURE_MAX_TEX_DIM;
                
                if( ( x + 1 ) * RMV_TEXTURE_MAX_TEX_DIM > m_MovieWidth )
                {
                    m_pTile[ tileIndex ].m_Width = m_MovieWidth % RMV_TEXTURE_MAX_TEX_DIM;
                }
                else
                {
                    m_pTile[ tileIndex ].m_Width = RMV_TEXTURE_MAX_TEX_DIM;
                }

                if( ( y + 1 ) * RMV_TEXTURE_MAX_TEX_DIM > m_MovieHeight )
                {
                    m_pTile[ tileIndex ].m_Height = m_MovieHeight % RMV_TEXTURE_MAX_TEX_DIM;
                }
                else
                {
                    m_pTile[ tileIndex ].m_Height = RMV_TEXTURE_MAX_TEX_DIM;
                }

                tileIndex++;
            }
        }
    }
}

//=============================================================================
// radMovieRenderStrategyBink::Render
//=============================================================================

bool radMovieRenderStrategyBink::Render( void )
{
    // some camera settings
    p3d::pddi->PushState(PDDI_STATE_ALL);
    p3d::pddi->PushIdentityMatrix(PDDI_MATRIX_MODELVIEW);
    #if defined( RAD_WIN32 ) || defined( RAD_UWP )
    p3d::pddi->SetProjectionMode(PDDI_PROJECTION_DEVICE);
    #else
    p3d::pddi->SetProjectionMode(PDDI_PROJECTION_ORTHOGRAPHIC); //PDDI_PROJECTION_DEVICE
    #endif
    p3d::pddi->SetCullMode(PDDI_CULL_NONE);
    

    // Note: A faster render strategy is possible...
    //
    // Rendering could be made faster by ditching the use of textures and 
    // copying the pixels onto the back buffer directly.  I didn't do it this
    // way because it requires Pure3d to create the d3d device in a different mode
    // that allows the back buffer to be locked.  
    //
    // At this time, nobody requires movies faster than 30fps so I didn't want to
    // run into any weird problems.  I'm leaving this note for future generations.

    rAssert( m_pShader != NULL );

    // Render each tile 

    for( unsigned int tile = 0; tile < m_NumTiles; tile++ )
    {
        rAssert( m_pTile[ tile ].m_pTexture != NULL );

        // Set up position info of the tile
        
        #if defined( RAD_WIN32 ) || defined( RAD_UWP )

        float u = 0.0f;
        float du = m_pTile[ tile ].m_Width / ( float ) m_pTile[ tile ].m_pTexture->GetWidth( );
        
        float v = 1.0f;
        float dv = - ( float ) ( m_pTile[ tile ].m_Height / ( float ) m_pTile[ tile ].m_pTexture->GetHeight( ) );

        #elif RAD_XBOX // TODO(3UR): If videos still render weirdly on UWP its worth enabling this to see

        float u = 0.0f;
        float v = ( float ) m_pTile[ tile ].m_Height;
        float du = ( float ) m_pTile[ tile ].m_Width;
        float dv = - ( float ) m_pTile[ tile ].m_Height;

        #endif

        #if defined( RAD_WIN32 ) || defined( RAD_UWP )

        float x = ( float ) m_DisplayMultiplier * m_pTile[ tile ].m_PosX + m_MoviePosX;
        float y = ( float ) m_DisplayMultiplier * m_pTile[ tile ].m_PosY + m_MoviePosY;
        float z = 5.0f;     // arbitrary value greater than 0
        float dx = ( float ) m_DisplayMultiplier * m_pTile[ tile ].m_Width;
        float dy = ( float ) m_DisplayMultiplier * m_pTile[ tile ].m_Height;

        #else

        float x = -0.5f * m_DisplayMultiplier;
        float y = 0.5f / (4.0f / 3.0f);
        float z = 5.0f;
        float dx = x * -2.0f;
        float dy = y * -2.0f;

        #endif

        // Associate the tile's texture with our shader

        m_pShader->SetTexture( PDDI_SP_BASETEX, m_pTile[ tile ].m_pTexture );
        m_pShader->SetInt( PDDI_SP_BLENDMODE, PDDI_BLEND_NONE );
        if ( m_DisplayMultiplier != 1.0f )
        {
            m_pShader->SetInt( PDDI_SP_FILTER, PDDI_FILTER_BILINEAR );
        }

        // Stream the texture to the screen onto a couple of triangles

        pddiPrimStream* pStream = p3d::pddi->BeginPrims( m_pShader->GetShader( ), PDDI_PRIM_TRIANGLES, PDDI_V_CT, 6 );

        // bottom left

        pStream->Colour( tColour(255,255,255));
        pStream->UV( u, v + dv );
        pStream->Coord( x, y + dy, z );

        // bottom right

        pStream->Colour(tColour(255,255,255));  // Not sure if these need to happen every vertex
        pStream->UV( u + du, v + dv );
        pStream->Coord( x + dx, y + dy, z );

        // top right

        pStream->Colour(tColour(255,255,255));
        pStream->UV( u + du, v );
        pStream->Coord( x + dx, y, z );

        // bottom left

        pStream->Colour(tColour(255,255,255));
        pStream->UV( u, v + dv );
        pStream->Coord( x, y + dy, z );

        // top right

        pStream->Colour( tColour(255,255,255));
        pStream->UV( u + du, v );
        pStream->Coord( x + dx, y, z );

        // top left

        pStream->Colour( tColour(255,255,255));
        pStream->UV( u, v);
        pStream->Coord( x, y, z );

        // Done with this primgroup

        p3d::pddi->EndPrims( pStream );
    }

    p3d::pddi->PopMatrix(PDDI_MATRIX_MODELVIEW);
    p3d::pddi->PopState(PDDI_STATE_ALL);

    return true;
}

/*
bool radMovieRenderStrategyBink::Render( void )
{
    radProfilerBeginFrame( );
    radProfilerBeginProfile( "radMovieRenderStrategyBink::Render" );

    // Get the back buffer from pure3d

    IDirect3DSurface8 * pIDirect3DSurface8 = NULL;
    HRESULT hr = static_cast< d3dDisplay * >( p3d::display )->GetD3DDevice( )->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, & pIDirect3DSurface8 );
    rAssert( SUCCEEDED( hr ) );

    // Find out about the back buffer

    D3DSURFACE_DESC desc;
    hr = pIDirect3DSurface8->GetDesc( & desc );
    rAssert( SUCCEEDED( hr ) );

    // Lock the back buffer

    D3DLOCKED_RECT rect;
    hr = pIDirect3DSurface8->LockRect( & rect, NULL, NULL );
    rAssert( SUCCEEDED( hr ) );

    // Fill the back buffer

    unsigned char * dest = ( unsigned char * ) rect.pBits;
    unsigned char * src = ( unsigned char * ) m_pDecodedVideoFrame;

    int height = desc.Height;
    int width  = desc.Width;

    if( height > m_Height )
    {
        height = m_Height;
    }

    if( width > m_Width )
    {
        width = m_Width;
    }

    for (int j = 0; j < height; j++)
    {
        memcpy( dest, src, width * 4 );
        dest += rect.Pitch;
        src += m_Width * 4;

    }

    // Unlock the back buffer

    hr = pIDirect3DSurface8->UnlockRect( );
    rAssert( SUCCEEDED( hr ) );
    pIDirect3DSurface8->Release( );
    pIDirect3DSurface8 = NULL;

    radProfilerEndProfile( "radMovieRenderStrategyBink::Render" );
    radProfilerEndFrame( );

    return true;
}
*/

//=============================================================================
// ::radMovieSimpleFullScreenRenderStrategyCreate
//=============================================================================

IRadMovieRenderStrategy * radMovieSimpleFullScreenRenderStrategyCreate( radMemoryAllocator allocator )
{
    return new( allocator )radMovieRenderStrategyBink( );
}
