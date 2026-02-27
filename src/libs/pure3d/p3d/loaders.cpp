//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/error.hpp>
#include <p3d/utility.hpp>
#include <p3d/png.hpp>
#include <p3d/bmp.hpp>
#include <p3d/targa.hpp>
#include <p3d/geometry.hpp>
#include <p3d/gameattr.hpp>
#include <p3d/locator.hpp>
#include <p3d/shader.hpp>
#include <p3d/camera.hpp>
#include <p3d/light.hpp>
#include <p3d/image.hpp>
#include <p3d/texturefont.hpp>
#include <p3d/imagefont.hpp>
#include <p3d/sprite.hpp>
#include <p3d/billboardobject.hpp>

#include <p3d/shadow.hpp>

#include <p3d/anim/skeleton.hpp>
#include <p3d/anim/polyskin.hpp>
#include <p3d/anim/compositedrawable.hpp>

#include <p3d/anim/animate.hpp>
#include <p3d/anim/animatedobject.hpp>
#include <p3d/anim/cameraanimation.hpp>
#include <p3d/anim/lightanimation.hpp>
#include <p3d/anim/multicontroller.hpp>
#include <p3d/anim/textureanimation.hpp>
#include <p3d/effects/particleloader.hpp>
#include <p3d/effects/opticloader.hpp>
#include <p3d/scenegraph/scenegraph.hpp>
#include <p3d/scenegraph/nodeanimation.hpp>

#include <p3d/anim/vertexoffsetexpression.hpp>
#include <p3d/anim/expression.hpp>
#include <p3d/anim/expressionanimation.hpp>
#include <p3d/anim/sequencer.hpp>
#include <p3d/anim/vertexanimkey.hpp>

#include <p3d/chunkfile.hpp>

#include <p3d/ignoreloader.hpp>

#ifdef P3D_TRACK_LOAD_STATS
#include <p3d/memorysection.hpp>
#endif

namespace p3d
{

//--------------------------------------------------------------------
void InstallDefaultLoaders()
{
    P3DASSERT(p3d::context);
    tP3DFileHandler* p3d = new tP3DFileHandler;
    loadManager->AddHandler(p3d, "p3d");
    context->GetLoadManager()->AddHandler(new tTargaHandler, "tga");
    context->GetLoadManager()->AddHandler(new tPNGHandler, "png");
    context->GetLoadManager()->AddHandler(new tBMPHandler, "bmp");

    p3d->AddHandler(new tGeometryLoader);
    p3d->AddHandler(new tTextureLoader);
    p3d->AddHandler(new tSetLoader );
    p3d->AddHandler(new tShaderLoader);
    p3d->AddHandler(new tCameraLoader);
    p3d->AddHandler(new tGameAttrLoader);
    p3d->AddHandler(new tLightLoader);
  
    p3d->AddHandler(new tLocatorLoader);
    p3d->AddHandler(new tLightGroupLoader);
    p3d->AddHandler(new tImageLoader);
    p3d->AddHandler(new tTextureFontLoader);
    p3d->AddHandler(new tImageFontLoader);
    p3d->AddHandler(new tSpriteLoader);
    p3d->AddHandler(new tBillboardQuadGroupLoader);
    p3d->AddHandler(new tSkeletonLoader);
    p3d->AddHandler(new tPolySkinLoader);
    p3d->AddHandler(new tCompositeDrawableLoader);

    //p3d->AddHandler(new tShadowMeshLoader);
    //p3d->AddHandler(new tShadowSkinLoader);

    p3d->AddHandler(new tAnimationLoader);
    p3d->AddHandler(new tFrameControllerLoader);
    p3d->AddHandler(new tMultiControllerLoader);
    p3d->AddHandler(new tAnimatedObjectFactoryLoader);
    p3d->AddHandler(new tAnimatedObjectLoader);
    p3d->AddHandler(new tParticleSystemFactoryLoader);
    p3d->AddHandler(new tParticleSystemLoader);
    p3d->AddHandler(new tLensFlareGroupLoader);
    p3d->AddHandler(new sg::Loader);

    p3d->AddHandler(new tExpressionGroupLoader);
    p3d->AddHandler(new tExpressionMixerLoader);
    p3d->AddHandler(new tExpressionLoader);
    
    p3d->AddHandler(new tVertexAnimKeyLoader);

    tIgnoreLoader* l = new tIgnoreLoader();
	p3d->AddHandler(l, P3D_HISTORY);
	p3d->AddHandler(l, P3D_ALIGN);

    tSEQFileHandler* sequencerFileHandler = new tSEQFileHandler;
    loadManager->AddHandler(sequencerFileHandler, "seq");

#ifdef P3D_TRACK_LOAD_STATS
    p3d->AddHandler(new tMemorySectionLoader);
#endif
}

} // namespace p3d
