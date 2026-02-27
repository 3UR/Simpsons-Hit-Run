//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radmath/radmath.hpp>
#include <stdio.h>


#ifndef RAD_RELEASE
#include <math.h>
#include <assert.h>

#define RMT_MAX_ERROR 0.001

int RadmathSanityCheck()
{
    for(int i=0;i<1024;i++)
    {
        float theta = (i / 128.0f) - 4.0f;
        float s = rmt::Sin(theta);
        float c = rmt::Cos(theta);

        assert( fabs(s-sinf(theta)) < RMT_MAX_ERROR );
        assert( fabs(c-cosf(theta)) < RMT_MAX_ERROR );

        rmt::SinCos(theta,&s,&c);

        assert( fabs(s-sinf(theta)) < RMT_MAX_ERROR );
        assert( fabs(c-cosf(theta)) < RMT_MAX_ERROR );

        float t = rmt::Tan(theta);
        assert( fabs(t-tanf(theta)) < RMT_MAX_ERROR );
    }

    return 0;
}

#endif // RAD_RELEASE

// todo - add radMathInitiliaze() for other platforms if needed
int radMathInitialize()
{
#ifndef RAD_RELEASE
    RadmathSanityCheck();
#endif

    return 0;
}
