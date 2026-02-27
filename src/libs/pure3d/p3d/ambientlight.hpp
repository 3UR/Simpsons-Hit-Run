//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _AMBIENTLIGHT_HPP
#define _AMBIENTLIGHT_HPP

#include <p3d/entity.hpp>
#include <p3d/light.hpp>

class tAmbientLight : public tLight
{
public:
   tAmbientLight();
   tAmbientLight(tAmbientLight *ambLight);

protected:
   virtual ~tAmbientLight();
    virtual void Update(void);
};

#endif // #ifndef _AMBIENTLIGHT_HPP
