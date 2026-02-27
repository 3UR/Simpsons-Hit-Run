//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/anim/vertexanimkey.hpp>
#include <p3d/anim/vertexanimobject.hpp>
#include <p3d/anim/vertexanimcontroller.hpp>

tVertexAnimController::tVertexAnimController( )
{
    mObject = NULL;
}


tVertexAnimController::~tVertexAnimController( )
{
    if( mObject ){
        mObject->Release( );
    }
}


void tVertexAnimController::SetTarget( tVertexAnimObject *o )
{
    tRefCounted::Assign( mObject, o );
}

bool tVertexAnimController::ValidateAnimation( tAnimation *anim )
{
    return ( anim->GetAnimationType( ) == Pure3DAnimationTypes::VERTEX_VRTX ) ;
}

void tVertexAnimController::Update( )
{   
    if ( animation == NULL || mObject == NULL ) 
        return;

    float pframe = animation->MakeValidFrame(frame,minFrame,maxFrame,cycleMode);

    //A vertex animation could contain multiple animation-groups, with each
    //group holding the vertex-animation-key for a primitive group of
    //targeted geometry
    for( int i = 0; i < animation->GetNumGroups( ); ++i ){
        const tAnimationGroup *animGroup = animation->GetGroupByIndex( i );

        if (animGroup)
        {
            //the group id of a animation group of vertex anim specify
            //the primgroup index of the targeted geometry
            int idxPrim = animGroup->GetGroupId( );

            //prims holds all vertex-anim keys
            const tEntityChannel *keys = animGroup->GetEntityChannel(Pure3DAnimationChannels::Vertex::VERTEX_VRTX);

            if ( keys )
            {
                tEntity *k1, *k2;
                float frame1, frame2;
                
                k1 = k2 = NULL;

                //Get the vertex-animation key by interpolation
                keys->FindEndKeys( pframe, &frame1, &k1, &frame2, &k2 );

                tVertexAnimKey *endKey = (tVertexAnimKey *)k2;
                tVertexAnimKey *beginKey = (tVertexAnimKey *)k1;                

                
                //update the vertex animation object with the previous key first
                mObject->Update( beginKey, idxPrim );

                if( k1 != k2 ){     //if the end keys are differet, update the offset list with the right end key
                    float delta = ( pframe - frame1 )/( frame2 - frame1 );

                    int iMask =  keys->GetInterpolate( );
                    mObject->SetInterpolateMask( iMask );
                    mObject->AddDifference( endKey, idxPrim, delta  );
                }                
            }
        }
    }
}

tFrameController *tVertexAnimController::Clone( )
{
    return new tVertexAnimController( this );
}

tVertexAnimController::tVertexAnimController( tVertexAnimController * c )
:tSimpleFrameController( c ),
 mObject( NULL )
{
    SetTarget( c->mObject );    
}

