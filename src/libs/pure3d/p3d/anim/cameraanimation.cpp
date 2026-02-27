//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/anim/cameraanimation.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/error.hpp>
#include <p3d/vectorcamera.hpp>
#include <radmath/radmath.hpp>

using namespace RadicalMathLibrary;

const float cameraCutEpsilon = 0.5f;
const bool detectCameraCut = true;

static inline void DetectCameraCut(float* frame, const tVectorChannel* pos, const tVectorChannel* look)
{
        // detect camera cuts
    if(detectCameraCut)
    {
        // integer values to snap to if we are in a cut
        float frame0 = floorf(*frame);        
        float frame1 = floorf(*frame + 1.0f);

        rmt::Vector v0, v1;

        if(pos != NULL)
        {
            pos->GetValue(frame0, &v0);
            pos->GetValue(frame1, &v1);
            v0.Sub(v1);
            if(rmt::Fabs(v0.Magnitude()) > cameraCutEpsilon)
            {
                *frame = frame0;
            }
        }

        if(look != NULL)
        {
            look->GetValue(frame0, &v0);
            look->GetValue(frame1, &v1);
            v0.Sub(v1);
            if(rmt::Fabs(v0.Magnitude()) > cameraCutEpsilon)
            {
                *frame = frame0;
            }
        }
    }



}

//*********************************************************
// Class tCameraAnimationController
//*********************************************************
tCameraAnimationController::tCameraAnimationController() : 
    tBlendFrameController(),
    mCamera(NULL)
{
    mOffsetMatrix.Identity();
}

//---------------------------------------------------------
tCameraAnimationController::tCameraAnimationController(tCameraAnimationController* c) :
    tBlendFrameController(c),
    mCamera(NULL)
{
    SetOffsetMatrix( c->mOffsetMatrix );
    SetCamera(c->mCamera);
}

//---------------------------------------------------------
tCameraAnimationController::~tCameraAnimationController()
{
    tRefCounted::Release(mCamera);
}

//---------------------------------------------------------
tFrameController* tCameraAnimationController::Clone(void)
{
    return new tCameraAnimationController(this);
}

//---------------------------------------------------------
void tCameraAnimationController::SetCamera(tVectorCamera* camera)
{
    tRefCounted::Assign(mCamera, camera);
}

//---------------------------------------------------------
bool tCameraAnimationController::ValidateAnimation(tAnimation* anim)
{
    return (anim->GetAnimationType()==Pure3DAnimationTypes::CAMERA_CAM);
}

//---------------------------------------------------------
void tCameraAnimationController::UpdateNoBlending()
{
    // If there is no destination, why do the work?
    if (mCamera == NULL) 
        return;

    tAnimation* anim = playInfo[0].GetAnimation();
    float frame = playInfo[0].GetCurrentFrame();


    const tVectorChannel *pos  = NULL;
    const tVectorChannel *look = NULL;
    const tVectorChannel *up   = NULL;
    const tFloat1Channel *fov  = NULL;
    // Added by Bryan Ewert on 22 Feb 2002
    const tFloat1Channel *nclp = NULL;
    const tFloat1Channel *fclp = NULL;

    const tAnimationGroup* animationGroup = anim->GetGroupById(0);
    if(animationGroup)
    {
        pos  = animationGroup->GetVectorChannel(Pure3DAnimationChannels::Camera::TRANSLATION_TRAN);
        look = animationGroup->GetVectorChannel(Pure3DAnimationChannels::Camera::LOOKVECTOR_LOOK);
        up   = animationGroup->GetVectorChannel(Pure3DAnimationChannels::Camera::UPVECTOR_UP);
        fov  = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::Camera::FIELDOFVIEW_FOV);
        // Added by Bryan Ewert on 22 Feb 2002
        nclp = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::Camera::NEARCLIP_NCLP);
        fclp = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::Camera::FARCLIP_FCLP);

        Vector v(0.0f,0.0f,0.0f);

        DetectCameraCut(&frame, pos, look);

        if (pos != NULL)
        {
            pos->GetValue(frame, &v);
            mOffsetMatrix.Transform( v, &v );
            mCamera->SetPosition(v);
        }

        rmt::Matrix rotationMatrix = mOffsetMatrix;
        rotationMatrix.FillTranslate( rmt::Vector( 0.0f, 0.0f, 0.0f ) );
        if (look != NULL)
        {
            look->GetValue(frame, &v);
            rotationMatrix.Transform( v, &v );
            mCamera->SetDirection(v);
        }

        if (up != NULL)
        {
            up->GetValue(frame, &v);
            rotationMatrix.Transform( v, &v );
            mCamera->SetUpVector(v);
        }

        // Set the FOV preserving the aspec ratio
        if (fov != NULL)
        {
            float prevfov, aspect;
            fov->GetValue(frame, &(v.x));
            mCamera->GetFOV(&prevfov, &aspect);
            mCamera->SetFOV(v.x, aspect);
        }

        // Added by Bryan Ewert on 22 Feb 2002
        if (nclp != NULL)
        {
            nclp->GetValue(frame, &(v.x));
            mCamera->SetNearPlane(v.x);
        }

        if (fclp != NULL)
        {
            fclp->GetValue(frame, &(v.x));
            mCamera->SetFarPlane(v.x);
        }

    }
}

//---------------------------------------------------------
void tCameraAnimationController::UpdateWithBlending()
{
    // If there is no destination, why do the work?
    if (mCamera == NULL) return;

    Vector vpos(0.0f, 0.0f, 0.0f);
    Vector vlook(0.0f, 0.0f, 0.0f);
    Vector vup(0.0f, 0.0f, 0.0f);
    float vfov = 0.0f;
    float vnclp = 0.0f;
    float vfclp = 0.0f;

    for (int a=0; a < nAnim; a++)
    {
        tAnimation* anim = playInfo[a].GetAnimation();
        float frame = playInfo[a].GetCurrentFrame();
        float weight = playInfo[a].GetWeight();

        if (a == 0) 
        {
            weight = 1.0F;
        }

        const tVectorChannel *pos  = NULL;
        const tVectorChannel *look = NULL;
        const tVectorChannel *up   = NULL;
        const tFloat1Channel *fov  = NULL;
        // Added by Bryan Ewert on 22 Feb 2002
        const tFloat1Channel *nclp = NULL;
        const tFloat1Channel *fclp = NULL;

        const tAnimationGroup* animationGroup = anim->GetGroupById(0);
        if(animationGroup)
        {
            pos  = animationGroup->GetVectorChannel(Pure3DAnimationChannels::Camera::TRANSLATION_TRAN);
            look = animationGroup->GetVectorChannel(Pure3DAnimationChannels::Camera::LOOKVECTOR_LOOK);
            up   = animationGroup->GetVectorChannel(Pure3DAnimationChannels::Camera::UPVECTOR_UP);
            fov  = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::Camera::FIELDOFVIEW_FOV);
            // Added by Bryan Ewert on 22 Feb 2002
            nclp = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::Camera::NEARCLIP_NCLP);
            fclp = animationGroup->GetFloat1Channel(Pure3DAnimationChannels::Camera::FARCLIP_FCLP);

            Vector v(0.0f,0.0f,0.0f);

            DetectCameraCut(&frame, pos, look);

            if (pos != NULL)
            {
                pos->GetValue(frame, &v);
                vpos.ScaleAdd(weight, v);
            }

            if (look != NULL)
            {
                look->GetValue(frame, &v);
                vlook.ScaleAdd(weight, v);
            }

            if (up != NULL)
            {
                up->GetValue(frame, &v);
                vup.ScaleAdd(weight, v);
            }

            if (fov != NULL)
            {
                fov->GetValue(frame, &(v.x));
                vfov += weight * v.x;
            }

            // Added by Bryan Ewert on 22 Feb 2002
            if (nclp != NULL)
            {
                nclp->GetValue(frame, &(v.x));
                vnclp += weight * v.x;
            }

            if (fclp != NULL)
            {
                fclp->GetValue(frame, &(v.x));
                vfclp += weight * v.x;
            }

        }
    }

    mCamera->SetPosition(vpos);
    mCamera->SetDirection(vlook);
    mCamera->SetUpVector(vup);

    float prevfov, aspect;
    mCamera->GetFOV(&prevfov, &aspect);
    mCamera->SetFOV(vfov, aspect);

    // Added by Bryan Ewert on 22 Feb 2002
    mCamera->SetNearPlane(vnclp);
    mCamera->SetFarPlane(vfclp);

}

void tCameraAnimationController::SetOffsetMatrix( const rmt::Matrix& m )
{
    mOffsetMatrix = m;
}




