/*===========================================================================
    transitionmanager.cpp
    04-03-2002 Created by Mike Perzel

    Copyright (c)2002 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#include <p3d/texture.hpp>
#include <p3d/shader.hpp>
#include <p3d/utility.hpp>
#include <p3d/matrixstack.hpp>
#include <p3d/effects/transitionmanager.hpp>


tTransitionManager::tTransitionManager()
:
mTexture( NULL ),
mShader( NULL ),
mRand( 234 ),
mGrabbing( false ),
mDrawing( false ),
mType( TRANSITION_NONE ),
mExponent( 1 ),
mTimeElapsed( 0.0f ),
mTime( 2000.0f ),
mScale( 1.0f ),
mRotation( 0.0f ),
mXSize( 16 ),
mYSize( 16 ),
mPositions( NULL )
{
    mColour.Set( 0xFF, 0xFF, 0xFF, 0xFF );
    mVector.Set( 0.0f, 0.0f, 0.0f );
}

tTransitionManager::~tTransitionManager()
{
    if( mTexture )
    {
        mTexture->Release();
    }
    if( mShader )
    {
        mShader->Release();
    }
    if( mPositions )
    {
        delete[] mPositions;
        mPositions = NULL;
    }
}

void tTransitionManager::Display()
{
    if( !mDrawing || !mTexture || !mPositions )
    {
        return;
    }
    if( !mShader )
    {
        if( !mTexture->GetTexture() )
        {
            return;
        }
        mShader = new tShader();
        mShader->AddRef();
        mShader->SetTexture( PDDI_SP_BASETEX, mTexture );
        mShader->SetInt(PDDI_SP_ISLIT, 0);
        mShader->SetInt(PDDI_SP_BLENDMODE, PDDI_BLEND_ALPHA);
    }


// The display code is directly taken from the Scrooby FeScreen::Display()
    p3d::pddi->PushState( PDDI_STATE_RENDER );
    p3d::pddi->PushState( PDDI_STATE_VIEW );
    p3d::pddi->EnableZBuffer( false );
    float aspect;

    aspect = static_cast< float >( p3d::display->GetWidth() ) / static_cast< float >( p3d::display->GetHeight() );

    p3d::pddi->SetCamera( 0.1f, 10.0f, 1.5707f, aspect ); // 1.5707 = PI /2
    p3d::pddi->SetProjectionMode( PDDI_PROJECTION_PERSPECTIVE );
    p3d::pddi->SetCullMode( PDDI_CULL_NONE );
    p3d::stack->PushIdentity();

// Draw our screen overlay
    p3d::stack->RotateZ( mRotation );
    p3d::stack->Scale( mScale, mScale, 1.0f );
    p3d::stack->Translate( -0.5f, -0.5f / aspect, 0.5f );
    p3d::stack->Translate( mVector );

    pddiPrimStream* stream = p3d::pddi->BeginPrims( mShader->GetShader(), PDDI_PRIM_TRIANGLES, PDDI_V_CT, mXSize*mYSize*6 );
    int i = 0;
    for( i; i < mXSize*mYSize; i++ )
    {
        stream->Vertex( &mPositions[i].tri[0].point[0].pos, mColour, &mPositions[i].tri[0].point[0].uv );
        stream->Vertex( &mPositions[i].tri[0].point[1].pos, mColour, &mPositions[i].tri[0].point[1].uv );
        stream->Vertex( &mPositions[i].tri[0].point[2].pos, mColour, &mPositions[i].tri[0].point[2].uv );
        stream->Vertex( &mPositions[i].tri[1].point[0].pos, mColour, &mPositions[i].tri[1].point[0].uv );
        stream->Vertex( &mPositions[i].tri[1].point[1].pos, mColour, &mPositions[i].tri[1].point[1].uv );
        stream->Vertex( &mPositions[i].tri[1].point[2].pos, mColour, &mPositions[i].tri[1].point[2].uv );
    }    
    p3d::pddi->EndPrims( stream );

// Clean up.
    p3d::stack->Pop();
    p3d::pddi->PopState( PDDI_STATE_RENDER );
    p3d::pddi->PopState( PDDI_STATE_VIEW );

}

void tTransitionManager::StartTransition()
{
    mGrabbing = true;
    mScale = 1.0f;
    mColour.Set( 0xFF, 0xFF, 0xFF, 0xFF );
    mTimeElapsed = 0.0f;
    mVector.Set( 0.0f, 0.0f, 0.0f );
    mRotation = 0.0f;
    if( mPositions )
    {
        delete[] mPositions;
        mPositions = NULL;
    }
}

void tTransitionManager::Advance( float delta )
{
    if( mGrabbing )
    {
        if( !mTexture )
        {            
            mTexture = new tTexture();
            mTexture->Create( 512, 256, 32, 8, 0, PDDI_TEXTYPE_RENDERTARGET );
            p3d::context->GetContext()->SetRenderTarget( mTexture->GetTexture() );
            mTexture->AddRef();
            mColour.Set( 0xFF, 0xFF, 0xFF, 0xFF );
        }
        else
        {
            mGrabbing = false;
            p3d::context->GetContext()->SetRenderTarget( NULL );
            mDrawing = true;
        }
    }

    if( !mDrawing )
    {
        return;
    }

    mTimeElapsed += delta;
    if( mTimeElapsed > mTime )
    {
        mDrawing = false;
        if( mTexture )
        {
            mTexture->Release();
            mTexture = NULL;
        }
        if( mShader )
        {
            mShader->Release();
            mShader = NULL;
        }
        if( mPositions )
        {
            delete[] mPositions;
            mPositions = NULL;
        }
        return;
    }
    float percent = mTimeElapsed / mTime;
    for( unsigned int i = 1; i < mExponent; i++ )
    {
        percent *= percent;
    }

    if( !mPositions )
    {
        mPositions = new tTransitionManager::overlayQuad[mXSize * mYSize];
        int x = 0;
        int y = 0;
        float xUnit = 1.0f / static_cast<float>(mXSize);
        float vUnit = 1.0f / static_cast<float>(mYSize);
        float yUnit = vUnit / (p3d::display->GetWidth() / p3d::display->GetHeight());

        for( y; y < mYSize; y++ )
        {
            x = 0;
            for( x; x < mXSize; x++ )
            {
                mPositions[y*mYSize+x].tri[0].point[0].pos.Set( x*xUnit, y*yUnit, 0.0f );
                mPositions[y*mYSize+x].tri[0].point[0].uv.Set( x*xUnit, (mYSize-y)*vUnit );
                mPositions[y*mYSize+x].tri[0].point[1].pos.Set( (x+1)*xUnit, y*yUnit, 0.0f );
                mPositions[y*mYSize+x].tri[0].point[1].uv.Set( (x+1)*xUnit, (mYSize-y)*vUnit );
                mPositions[y*mYSize+x].tri[0].point[2].pos.Set( x*xUnit, (y+1)*yUnit, 0.0f );
                mPositions[y*mYSize+x].tri[0].point[2].uv.Set( x*xUnit, (mYSize-y-1)*vUnit );

                mPositions[y*mYSize+x].tri[1].point[0].pos.Set( (x+1)*xUnit, y*yUnit, 0.0f );
                mPositions[y*mYSize+x].tri[1].point[0].uv.Set( (x+1)*xUnit, (mYSize-y)*vUnit );
                mPositions[y*mYSize+x].tri[1].point[1].pos.Set( x*xUnit, (y+1)*yUnit, 0.0f );
                mPositions[y*mYSize+x].tri[1].point[1].uv.Set( x*xUnit, (mYSize-y-1)*vUnit );
                mPositions[y*mYSize+x].tri[1].point[2].pos.Set( (x+1)*xUnit, (y+1)*yUnit, 0.0f );
                mPositions[y*mYSize+x].tri[1].point[2].uv.Set( (x+1)*xUnit, (mYSize-y-1)*vUnit );
            }
        }
    }
    switch( mType )
    {
    case TRANSITION_FADE_TO_ALPHA:
        mColour.SetAlpha( 255 - static_cast<int>( 255.0f * percent ) );
        break;
    case TRANSITION_FADE_TO_BLACK:
        {
            int colour = 255 - static_cast<int>( 255.0f * percent );
            mColour.Set( colour, colour, colour );
            break;
        }
    case TRANSITION_SLIDE_LEFT:
        mVector.Set( percent, 0.0f, 0.0f );
        break;
    case TRANSITION_SLIDE_RIGHT:
        mVector.Set( -percent, 0.0f, 0.0f );
        break;
    case TRANSITION_SLIDE_TOP:
        mVector.Set( 0.0f, percent, 0.0f );
        break;
    case TRANSITION_SLIDE_BOTTOM:
        mVector.Set( 0.0f, -percent, 0.0f );
        break;
    case TRANSITION_SHRINK:
        mScale = 1.0f - percent;
        break;
    case TRANSITION_SPIN_AND_SHRINK:
        mRotation = percent*12.56637f; // 2PI * 2 for two rotations
        mScale = 1.0f - percent;
        break;
    case TRANSITION_EXPLODE:
        {
            int i = 0;
            for( i; i < mYSize*mXSize; i++ )
            {
                float rand = mRand.Float();
                mPositions[i].tri[0].point[0].pos.x += (mPositions[i].tri[0].point[0].pos.x - 0.49f) * percent + rand;
                mPositions[i].tri[0].point[1].pos.x += (mPositions[i].tri[0].point[0].pos.x - 0.49f) * percent + rand;
                mPositions[i].tri[0].point[2].pos.x += (mPositions[i].tri[0].point[0].pos.x - 0.49f) * percent + rand;
                rand = mRand.Float();
                mPositions[i].tri[1].point[0].pos.x += (mPositions[i].tri[0].point[0].pos.x - 0.49f) * percent + rand;
                mPositions[i].tri[1].point[1].pos.x += (mPositions[i].tri[0].point[0].pos.x - 0.49f) * percent + rand;
                mPositions[i].tri[1].point[2].pos.x += (mPositions[i].tri[0].point[0].pos.x - 0.49f) * percent + rand;

                rand = mRand.Float();
                mPositions[i].tri[0].point[0].pos.y += (mPositions[i].tri[0].point[0].pos.y - 0.49f) * percent + rand;
                mPositions[i].tri[0].point[1].pos.y += (mPositions[i].tri[0].point[0].pos.y - 0.49f) * percent + rand;
                mPositions[i].tri[0].point[2].pos.y += (mPositions[i].tri[0].point[0].pos.y - 0.49f) * percent + rand;
                rand = mRand.Float();
                mPositions[i].tri[1].point[0].pos.y += (mPositions[i].tri[0].point[0].pos.y - 0.49f) * percent + rand;
                mPositions[i].tri[1].point[1].pos.y += (mPositions[i].tri[0].point[0].pos.y - 0.49f) * percent + rand;
                mPositions[i].tri[1].point[2].pos.y += (mPositions[i].tri[0].point[0].pos.y - 0.49f) * percent + rand;
            }
        }
        break;
    case TRANSITION_SHATTER:
        {
            int i = 0;
            for( i; i < mYSize*mXSize; i++ )
            {
                float rand = (mRand.Float() - 0.005f)*100.0f;
                if( percent < rand )
                {
                    continue;
                }
                rand = (mRand.Float() - 0.005f);
                mPositions[i].tri[0].point[0].pos.x += rand;
                mPositions[i].tri[0].point[1].pos.x += rand;
                mPositions[i].tri[0].point[2].pos.x += rand;
                rand = (mRand.Float() - 0.005f);
                mPositions[i].tri[1].point[0].pos.x += rand;
                mPositions[i].tri[1].point[1].pos.x += rand;
                mPositions[i].tri[1].point[2].pos.x += rand;

                rand = (mRand.Float() - 0.005f);
                mPositions[i].tri[0].point[0].pos.y += ((mPositions[i].tri[0].point[0].pos.y - 1.0f) * percent) + rand;
                mPositions[i].tri[0].point[1].pos.y += ((mPositions[i].tri[0].point[0].pos.y - 1.0f) * percent) + rand;
                mPositions[i].tri[0].point[2].pos.y += ((mPositions[i].tri[0].point[0].pos.y - 1.0f) * percent) + rand;
                rand = (mRand.Float() - 0.005f);
                mPositions[i].tri[1].point[0].pos.y += ((mPositions[i].tri[0].point[0].pos.y - 1.0f) * percent) + rand;
                mPositions[i].tri[1].point[1].pos.y += ((mPositions[i].tri[0].point[0].pos.y - 1.0f) * percent) + rand;
                mPositions[i].tri[1].point[2].pos.y += ((mPositions[i].tri[0].point[0].pos.y - 1.0f) * percent) + rand;
            }
        }
        break;
    case TRANSITION_SPLIT_SLIDE_VERTICAL:
        {
            int i = 0;
            for( i; i < mYSize*mXSize; i++ )
            {
                float sign = (i % 2 == 0) ? 1.0f : -1.0f;
                mPositions[i].tri[0].point[0].pos.y += percent*sign;
                mPositions[i].tri[0].point[1].pos.y += percent*sign;
                mPositions[i].tri[0].point[2].pos.y += percent*sign;
                mPositions[i].tri[1].point[0].pos.y += percent*sign;
                mPositions[i].tri[1].point[1].pos.y += percent*sign;
                mPositions[i].tri[1].point[2].pos.y += percent*sign;
            }
        }
        break;
    case TRANSITION_SPLIT_SLIDE_HORIZONTAL:
        {
            int i = 0;
            float sign = 1.0f;
            for( i; i < mYSize*mXSize; i++ )
            {
                if( i % mYSize == 0 )
                {
                    sign *= -1.0f;
                }
                mPositions[i].tri[0].point[0].pos.x += percent*sign;
                mPositions[i].tri[0].point[1].pos.x += percent*sign;
                mPositions[i].tri[0].point[2].pos.x += percent*sign;
                mPositions[i].tri[1].point[0].pos.x += percent*sign;
                mPositions[i].tri[1].point[1].pos.x += percent*sign;
                mPositions[i].tri[1].point[2].pos.x += percent*sign;
            }
        }
        break;
    case TRANSITION_NONE:
    default:
        break;
    }

}
