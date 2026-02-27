//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _VERTEXANIMCONTROLLER_HPP
#define _VERTEXANIMCONTROLLER_HPP

#include <p3d/anim/animate.hpp>

class tVertexAnimObject;
class tAnimation;

//derive vertex animation controller from simplefc, since we 
//don't do any blend here
class tVertexAnimController: public tSimpleFrameController
{
public:
    tVertexAnimController( );
    tVertexAnimController( tVertexAnimController *c );
    virtual bool ValidateAnimation(tAnimation* anim);
    virtual tFrameController * Clone( );

    void SetTarget( tVertexAnimObject * o );

protected:
    ~tVertexAnimController( );
    virtual void Update( );

    tVertexAnimObject *mObject;
        
};

#endif


