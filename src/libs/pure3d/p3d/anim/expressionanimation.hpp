/*=============================================================================
  expressionanimation.hpp
  29-Mar-2000
  Created by: Katrina

  Description: Pure3D expression classes for animation of
                    parameterized facial expressions

  Copyright (c) 1997-2000 Radical Entertainment, Inc.
  All Rights Reserved

=============================================================================*/

#ifndef _EXPRESSIONANIMATION_HPP
#define _EXPRESSIONANIMATION_HPP

#include <p3d/anim/animate.hpp>

class tExpression;
class tExpressionMixer;

//**************************************************************
// Class tExpressionAnimationController
//**************************************************************
class tExpressionAnimationController : public tBlendFrameController
{
public:
    tExpressionAnimationController();
    tExpressionAnimationController(tExpressionAnimationController* c);

    virtual tFrameController* Clone(void);

    virtual bool ValidateAnimation(tAnimation* anim);

    virtual void              SetTargetMixer(tExpressionMixer* t);
    virtual tExpressionMixer* GetTargetMixer() const;

protected:
    virtual ~tExpressionAnimationController();

    virtual void UpdateNoBlending();
    virtual void UpdateWithBlending();

    void ResetAccumulators();

    tExpressionMixer* mixer;
    float*            expStateAccumulator;
};

#endif

