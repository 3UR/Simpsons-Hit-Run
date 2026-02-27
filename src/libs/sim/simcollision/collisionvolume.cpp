
#include "simcollision/collisionvolume.hpp"

#include "simcommon/simmath.hpp"
#include "simcollision/collisionobject.hpp"

#include <radmath/radmath.hpp>

using namespace RadicalMathLibrary;

static float AXIS_ALIGNED_SNAPPING_FACTOR = 0.9999f;

namespace sim
{

CollisionVolume::CollisionVolume()
    : mSphereRadius(1.0f), // default
    mType(CollisionVolumeType),
    mObjRefIndex(-1),
    mOwnerIndex(-1),
    mCollisionObject(NULL),
    mSubVolumeList(NULL),
    mVisible(true),
    mUpdated(false)
{
    mPosition.Set(0, 0, 0);
    mBoxSize.Set(0, 0, 0);
    mDP.Set(0, 0, 0);
    mColour.Set(0, 255, 0);
}

CollisionVolume::~CollisionVolume()
{
    if (mSubVolumeList != NULL)
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
        {
            mSubVolumeList->GetAt(i)->Release();
        }
        mSubVolumeList->RemoveAll();
        delete mSubVolumeList;
    }
}

CollisionVolume* CollisionVolume::GetSubCollisionVolume(int inObjRefIndex, bool inSelfOnly)
{
    CollisionVolume* ret = NULL;
    
    if (mObjRefIndex <= inObjRefIndex)
    {
        if ((mType != BBoxVolumeType || !inSelfOnly) && mObjRefIndex == inObjRefIndex)
        {
            ret = this;
        }
        if (!ret && mSubVolumeList)
        {
            for (int i=0; i<mSubVolumeList->GetSize(); i++)
            {
                CollisionVolume* cv = mSubVolumeList->GetAt(i);
                if (cv->mObjRefIndex == inObjRefIndex && cv->Type() != BBoxVolumeType)
                {
                    ret = cv;
                    break;
                }
                else
                {
                    cv = cv->GetSubCollisionVolume(inObjRefIndex, inSelfOnly);
                    if (cv)
                    {
                        ret = cv;
                        break;
                    }
                }
            }
        }
    }
    return ret;
}

void CollisionVolume::ConvertToRigidBody()
{
    mObjRefIndex = -1;

    if (mType != BBoxVolumeType)
    {
        mDP = mPosition;
    }
    if (mSubVolumeList != NULL)
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
        {
            mSubVolumeList->GetAt(i)->ConvertToRigidBody();
        }
    }
}

bool CollisionVolume::UpdateBBoxVisibility()
{
    if (Type() == BBoxVolumeType)
    {
        if (mSubVolumeList != NULL)
        {
            mVisible = false;
            for (int i=0; i<mSubVolumeList->GetSize(); i++)
            {
                if (mSubVolumeList->GetAt(i)->UpdateBBoxVisibility())
                    mVisible = true;
            }
        }
    }
    return mVisible;
}


CollisionVolume* CollisionVolume::Clone()
{
    CollisionVolume* clone = NULL;
    
    switch (mType)
    {
    case CollisionVolumeType:
        rAssert(false); // this type is not valid
        break;
    case SphereVolumeType:
        clone = new SphereVolume;
        break;
    case CylinderVolumeType:
        clone = new CylinderVolume;
        break;
    case OBBoxVolumeType:
        clone = new OBBoxVolume;
        break;
    case WallVolumeType:
        clone = new WallVolume;
        break;
    case BBoxVolumeType:
        clone = new BBoxVolume;
        break;
    case MaxCollisionVolumeEnum: // save a warning
    default:
        break;
    }
    
    if (clone)
    {
        SetClone(clone);
    }
    return clone;
}

void CollisionVolume::Optimize()
{
    if (!mCollisionObject || !mCollisionObject->IsStatic())
        return;

    mUpdated = true;

    switch (mType)
    {
        case OBBoxVolumeType:
            ((OBBoxVolume*)this)->OptimizeAxis();
            break;
        case CylinderVolumeType:
            ((CylinderVolume*)this)->OptimizeAxis();
            break;
        case WallVolumeType:
            ((WallVolume*)this)->OptimizeAxis();
            break;
        case CollisionVolumeType:
        case SphereVolumeType:
        case BBoxVolumeType:
        case MaxCollisionVolumeEnum: // save a warning
        default:
            break;
    }

    if (mSubVolumeList != NULL)
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
        {
            mSubVolumeList->GetAt(i)->Optimize();
        }
    }
}

void CollisionVolume::DeOptimize()
{
    switch (mType)
    {
        case OBBoxVolumeType:
            {
                OBBoxVolume* box = (OBBoxVolume*)this;

                box->mAxisOrientation = VolAxisNotOriented;
                box->mLength[0] = box->mLengthRef[0];
                box->mLength[1] = box->mLengthRef[1];
                box->mLength[2] = box->mLengthRef[2];
            }
            break;
        case CylinderVolumeType:
            ((CylinderVolume*)this)->mAxisOrientation = VolAxisNotOriented;
            break;
        case WallVolumeType:
            ((WallVolume*)this)->mAxisOrientation = VolAxisNotOriented;
            break;
        case CollisionVolumeType:
        case SphereVolumeType:
        case BBoxVolumeType:
        case MaxCollisionVolumeEnum: // save a warning
        default:
            break;
    }

    if (mSubVolumeList != NULL)
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
        {
            mSubVolumeList->GetAt(i)->DeOptimize();
        }
    }
}

void CollisionVolume::SetClone(CollisionVolume* clone)
{
    clone->mType = mType;
    clone->mPosition = mPosition;
    clone->mBoxSize = mBoxSize;
    clone->mSphereRadius = mSphereRadius;
    clone->mVisible = mVisible;
    clone->mObjRefIndex = mObjRefIndex;
    clone->mOwnerIndex = mOwnerIndex;
    clone->mDP = mDP;
    
    if (mSubVolumeList != NULL)
    {
        clone->mSubVolumeList = new TList<CollisionVolume*>;
        int size = mSubVolumeList->GetSize();
        if( size != 0 )
        {
            clone->mSubVolumeList->ResizeArray( size );
        }
        for( int i = 0; i < size; ++i )
        {
            CollisionVolume* subClone = mSubVolumeList->GetAt(i)->Clone();
            rAssert(subClone);
            subClone->AddRef();
            clone->mSubVolumeList->Add(subClone);
        }
    }
}

void CollisionVolume::UpdateAll()
{
    UpdatePos();
    UpdateRot();
    UpdateBBox();
}

void CollisionVolume::UpdateBSphere()
{
    if (mSubVolumeList != NULL)
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
        {
            mSubVolumeList->GetAt(i)->UpdateBSphere();
        }
    }
}

void CollisionVolume::UpdatePos()
{  
    if (mCollisionObject != NULL)
    {
        mUpdated = false; // orientation not updated
        
		// Greg doesn't want the position updated if the collision object is
		// being manually updated (he is moving it himself)
		if (mType != BBoxVolumeType && !(mCollisionObject->IsManualUpdate()))  // new, greg - I think this is correct
        {
            mCollisionObject->GetTransform(mObjRefIndex).Transform(mDP, &mPosition);
        }
    }
    else
    {
        mUpdated = true;
    }
    
    if (mSubVolumeList != NULL)
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
        {
            mSubVolumeList->GetAt(i)->UpdatePos();
        }
    }
}

void CollisionVolume::UpdateRot()
{  
    if (mCollisionObject != NULL)
    {
        SetRotation();
    }
    
    if (mSubVolumeList != NULL)
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
        {
            mSubVolumeList->GetAt(i)->UpdateRot();
        }
    }
}

void CollisionVolume::UpdateScale(float a)
{
    if (mSubVolumeList != NULL)
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
            mSubVolumeList->GetAt(i)->UpdateScale(a);
    }
}

//=============================================================================
// CollisionVolume::Reserve
//=============================================================================
// Description: reserves the size of the collision volume so that we don't have
//              to keep on resizing these things when we load them
//
// Parameters:  size - how many elements will the collision volume hold
//
// Return:      void 
//
//=============================================================================
void CollisionVolume::Reserve( const size_t size )
{
    if( size == 0 )
    {
        return;
    }
    if (mSubVolumeList == NULL)
    {
        mSubVolumeList = new TList< CollisionVolume* >( size );
    }
    mSubVolumeList->ResizeArray( size );
}

void CollisionVolume::AddSubVolume(CollisionVolume* inEl)
{
    rAssert(inEl != NULL);
    rAssert(inEl != this);
    rAssertMsg(mType == BBoxVolumeType, "Only BBoxVolumeType supports this feature");
    rAssert(inEl->mObjRefIndex >= mObjRefIndex);
    bool added = false;
    
    if (mSubVolumeList == NULL)
    {
        mSubVolumeList = new TList<CollisionVolume*>;
    }
    else if (mSubVolumeList->Contains(inEl))
    {
        added = true;
    }
    if (!added)
    {
        mSubVolumeList->Add(inEl);
        inEl->AddRef();
        inEl->SetCollisionObject(mCollisionObject);
    }
}

bool CollisionVolume::RemoveSubVolume(CollisionVolume* el)
{
    rAssert(el != NULL);
    
    if (mSubVolumeList == NULL)
        return false;
    
    bool ret = false;
    if (mSubVolumeList->Contains(el))
    {
        mSubVolumeList->Remove(el);
        el->Release();
        ret = true;
    }
    else
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
        {
            ret = mSubVolumeList->GetAt(i)->RemoveSubVolume(el);
        }
    }
    return ret;
}

void CollisionVolume::SetCollisionObject(CollisionObject* obj)
{
    if (mCollisionObject == obj)
        return;
    
    mCollisionObject = obj;
    
    if (mSubVolumeList != NULL)
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
        {
            mSubVolumeList->GetAt(i)->SetCollisionObject(mCollisionObject);
        }
    }
}

void CollisionVolume::SetObjRefIndex(int ref)
{
    mObjRefIndex = ref;
    if (mSubVolumeList != NULL)
    {
        for (int i=0; i<mSubVolumeList->GetSize(); i++)
        {
            mSubVolumeList->GetAt(i)->SetObjRefIndex(ref);
        }
    }
}

// mostly for debug to make sure a sub BB is smaller than the parent
bool CollisionVolume::ContainsBB(CollisionVolume* el)
{
    return mBoxSize.DotProduct(mBoxSize) > el->mBoxSize.DotProduct(el->mBoxSize);
}

void CollisionVolume::EmptySubVolume()                    
{ 
    if (mSubVolumeList != NULL) 
    {
        for(int i = 0; i < mSubVolumeList->GetSize(); i++)
        {
            mSubVolumeList->GetAt(i)->Release();
        }
        mSubVolumeList->RemoveAll(); 
    }
}

void CollisionVolume::PreallocateSubVolumes(unsigned n)
{
    if (mSubVolumeList == NULL)
    {
        mSubVolumeList = new TList<CollisionVolume*>;
    }

    mSubVolumeList->ResizeArray(n);
}

// take a flat set of volumes and generate a little hierarchy to speed up tests
void CollisionVolume::GenerateHierarchy(void)
{
    if(!mSubVolumeList)
    {
        return;
    }

    unsigned nVols = mSubVolumeList->GetSize();

    if(nVols < 16)
    {
        return;
    }
   
    rmt::Vector origBox = mBoxSize;
    CollisionVolume* sub[1024];

    rAssert( nVols < 1024);

    unsigned i;
    for(i = 0; i < nVols; i++)
    {
        sub[i] = mSubVolumeList->GetAt(i);
        sub[i]->AddRef();
    }

    EmptySubVolume();

    unsigned counts[4] = {0,0,0,0};

    for(i = 0; i < nVols; i++)
    {
        if(sub[i]->mPosition.x > mPosition.x)
        {
            if(sub[i]->mPosition.z > mPosition.z)
            {
                counts[0]++;
            }
            else
            {
                counts[1]++;
            }
        }
        else
        {
            if(sub[i]->mPosition.z > mPosition.z)
            {
                counts[2]++;
            }
            else
            {
                counts[3]++;
            }
        }
    }

    CollisionVolume* newSubs[4] = {NULL, NULL, NULL, NULL};

    for(i = 0; i < 4; i++)
    {
        if(counts[i])
        {
            newSubs[i] = Clone();
            newSubs[i]->PreallocateSubVolumes(counts[i]);
        }
    }

    rmt::Box3D boxes[4];

    for(i = 0; i < nVols; i++)
    {
        if(sub[i]->mPosition.x > mPosition.x)
        {
            if(sub[i]->mPosition.z > mPosition.z)
            {
                rAssert(newSubs[0]);
                newSubs[0]->AddSubVolume(sub[i]);
                sub[i]->Release();
                boxes[0].Expand(sub[i]->mPosition + sub[i]->mBoxSize);
                boxes[0].Expand(sub[i]->mPosition - sub[i]->mBoxSize);
            }
            else
            {
                rAssert(newSubs[1]);
                newSubs[1]->AddSubVolume(sub[i]);
                sub[i]->Release();
                boxes[1].Expand(sub[i]->mPosition + sub[i]->mBoxSize);
                boxes[1].Expand(sub[i]->mPosition - sub[i]->mBoxSize);
            }
        }
        else
        {
            if(sub[i]->mPosition.z > mPosition.z)
            {
                rAssert(newSubs[2]);
                newSubs[2]->AddSubVolume(sub[i]);
                sub[i]->Release();
                boxes[2].Expand(sub[i]->mPosition + sub[i]->mBoxSize);
                boxes[2].Expand(sub[i]->mPosition - sub[i]->mBoxSize);
            }
            else
            {
                rAssert(newSubs[3]);
                newSubs[3]->AddSubVolume(sub[i]);
                sub[i]->Release();
                boxes[3].Expand(sub[i]->mPosition + sub[i]->mBoxSize);
                boxes[3].Expand(sub[i]->mPosition - sub[i]->mBoxSize);
            }
        }
    }

    for(i = 0; i < 4; i++)
    {
        if(newSubs[i])
        {
            rmt::Vector mid = boxes[i].Mid();
            newSubs[i]->mPosition = mid;
            newSubs[i]->mBoxSize = boxes[i].high - mid;
            newSubs[i]->GenerateHierarchy();
            AddSubVolume(newSubs[i]);
        }
    }
}

//
// SphereVolume class
//

SphereVolume::SphereVolume(const Vector& pos, float rad)
: CollisionVolume()
{
    mType = SphereVolumeType;
    mPosition = pos;
    mDP = mPosition;
    mSphereRadiusRef = rad;
    mSphereRadius = rad;
    mBoxSize.x = mSphereRadius;
    mBoxSize.y = mSphereRadius;
    mBoxSize.z = mSphereRadius;
    mColour.Set(255, 0, 0);
}

SphereVolume::~SphereVolume()
{
}

void SphereVolume::SetClone(CollisionVolume* clone)
{
    CollisionVolume::SetClone(clone);
    ((SphereVolume*)clone)->mSphereRadiusRef = mSphereRadiusRef;
}

void SphereVolume::UpdateScale(float a)
{  
    CollisionVolume::UpdateScale(a);
    mSphereRadius = mSphereRadiusRef * a;
    mBoxSize.x = mSphereRadius;
    mBoxSize.y = mSphereRadius;
    mBoxSize.z = mSphereRadius;
}

// Like bounding boxes, Simpsons updates the size of the bounding spheres
void SphereVolume::UpdateRadius(float r)    
{
    rAssert(r > 0.0f);

    mSphereRadius = r;
    mSphereRadiusRef = r;

    mBoxSize.x = mSphereRadius;
    mBoxSize.y = mSphereRadius;
    mBoxSize.z = mSphereRadius;
}


//
// CylinderVolume class
//

CylinderVolume::CylinderVolume(const Vector& centre, const Vector& o, float len, float r, bool flatend)
: CollisionVolume(),
mAxis(o),
mLength(len),
mCylinderRadius(r),
mFlatEnd(flatend),
mAxisRef(o),
mLengthRef(len),
mCylinderRadiusRef(r)
{
    mType = CylinderVolumeType;
    mPosition = centre;
    mDP = mPosition;
    
    if (mFlatEnd)
        mSphereRadius = Sqrt(Sqr(mLength) + Sqr(mCylinderRadius));
    else
        mSphereRadius = mLength + mCylinderRadius;

    mAxisOrientation = VolAxisNotOriented;

    // moved up to collision object
    //mCharacter = false;
}

CylinderVolume::~CylinderVolume()
{
}

void CylinderVolume::SetClone(CollisionVolume* clone)
{
    CollisionVolume::SetClone(clone);
    CylinderVolume* volume = ((CylinderVolume*)clone);
    volume->mAxis = mAxis;
    volume->mLength = mLength;
    volume->mCylinderRadius = mCylinderRadius;
    volume->mFlatEnd = mFlatEnd;
    volume->mAxisRef = mAxisRef;
    volume->mLengthRef = mLengthRef;
    volume->mCylinderRadiusRef = mCylinderRadiusRef;
    volume->mAxisOrientation = mAxisOrientation;
}

void CylinderVolume::UpdateScale(float a)
{  
    mLength = mLengthRef * a;
    mCylinderRadius = mCylinderRadiusRef * a;
    mAxisRef.Normalize();
    mAxisRef.Scale(Inverse(a));
    
    UpdateBSphere();
    
    CollisionVolume::UpdateScale(a);
}

void CylinderVolume::SetRotation()
{  
    mUpdated = true;
    
    // the rotate method will also scale if the GetTransform contains scaling
    mCollisionObject->GetTransform(mObjRefIndex).RotateVector(mAxisRef, &mAxis);
}

// approx Sqrt(1-val) at the second degree
inline float ApproxSqrt(float val)
{
    return 1.0f - 0.5f*val + 0.125f*Sqr(val);
}

void CylinderVolume::UpdateBBox()
{
    if( mFlatEnd )
    {
        float val = Fabs(mAxis.x);
        mBoxSize.x = mLength * val + mCylinderRadius * ApproxSqrt(Sqr(val));
        val = Fabs(mAxis.y);
        mBoxSize.y = mLength * val + mCylinderRadius * ApproxSqrt(Sqr(val));
        val = Fabs(mAxis.z);
        mBoxSize.z = mLength * val + mCylinderRadius * ApproxSqrt(Sqr(val));
    }
    else
    {
        mBoxSize.x = mLength * Fabs(mAxis.x) + mCylinderRadius;
        mBoxSize.y = mLength * Fabs(mAxis.y) + mCylinderRadius;
        mBoxSize.z = mLength * Fabs(mAxis.z) + mCylinderRadius;
    }
}

void CylinderVolume::UpdateBSphere()
{
    if (mFlatEnd)
        mSphereRadius = Sqrt(Sqr(mLength) + Sqr(mCylinderRadius));
    else
        mSphereRadius = mLength + mCylinderRadius;

    CollisionVolume::UpdateBSphere();
}

void CylinderVolume::OptimizeAxis()
{
    mAxisOrientation = VolAxisNotOriented;
/*
    if (Fabs(mAxis.x) > AXIS_ALIGNED_SNAPPING_FACTOR)
    {
        mAxisOrientation = VolAxisXOriented;
        //mAxisRef.Set(1.0f, 0.0f, 0.0f);
        mAxis.Set(1.0f, 0.0f, 0.0f);
    }
    else if (Fabs(mAxis.y) > AXIS_ALIGNED_SNAPPING_FACTOR)
    {
        mAxisOrientation = VolAxisYOriented;
        //mAxisRef.Set(0.0f, 1.0f, 0.0f);
        mAxis.Set(0.0f, 1.0f, 0.0f);
    }
    else if (Fabs(mAxis.z) > AXIS_ALIGNED_SNAPPING_FACTOR)
    {
        mAxisOrientation = VolAxisZOriented;
        //mAxisRef.Set(0.0f, 0.0f, 1.0f);
        mAxis.Set(0.0f, 0.0f, 1.0f);
    }
*/
}

// OBBoxVolume class
void OBBoxVolume::Set(const Vector& center, const Vector& o0, const Vector& o1, const Vector& o2, float l0, float l1, float l2)
{
    mPosition = center;
    mDP = mPosition;
    
    mLengthRef[0] = mLength[0] = l0;
    mLengthRef[1] = mLength[1] = l1;
    mLengthRef[2] = mLength[2] = l2;
    
    mSphereRadius = Sqrt(Sqr(l0) + Sqr(l1) + Sqr(l2));
    
    mAxisRef[0] = o0;
    mAxisRef[1] = o1;
    mAxisRef[2] = o2;
    
    mAxis[0] = mAxisRef[0];
    mAxis[1] = mAxisRef[1];
    mAxis[2] = mAxisRef[2];

    mAxisOrientation = VolAxisNotOriented;
    
#ifdef OBBoxStorePoints
    for (int i=0; i<4; i++)
        SubCorner(mC[i], i); 
#endif
    mColour.Set(255, 0, 0);
}

OBBoxVolume::~OBBoxVolume()
{
}

void OBBoxVolume::SetClone(CollisionVolume* clone)
{
    CollisionVolume::SetClone(clone);
    OBBoxVolume* volume = ((OBBoxVolume*)clone);
    for (int i=0; i<3; i++)
    {
        volume->mAxis[i]      = mAxis[i];
        volume->mAxisRef[i]   = mAxisRef[i];
        volume->mLength[i]    = mLength[i];
        volume->mLengthRef[i] = mLengthRef[i];
    }
    volume->mAxisOrientation = mAxisOrientation;
    
#ifdef OBBoxStorePoints
    volume->mC[0] = mC[0];
    volume->mC[1] = mC[1];
    volume->mC[2] = mC[2];
    volume->mC[3] = mC[3];
#endif
}

void OBBoxVolume::UpdateScale(float a)
{  
    float inva = Inverse(a);
    
    for (int i=0; i<3; i++)
    {
        mLength[i] = mLengthRef[i] * a;
        mAxisRef[i].Normalize();
        mAxisRef[i].Scale(inva);
    }
    
    UpdateBSphere();
    
    CollisionVolume::UpdateScale(a);
}

void OBBoxVolume::SetRotation()
{  
    mUpdated = true;
    const Matrix m = mCollisionObject->GetTransform(mObjRefIndex);
    
    m.RotateVector(mAxisRef[0], &mAxis[0]);
    m.RotateVector(mAxisRef[1], &mAxis[1]);
    m.RotateVector(mAxisRef[2], &mAxis[2]);
    
    rAssert(mAxisOrientation == VolAxisNotOriented);

#ifdef OBBoxStorePoints 
    for (int i=0; i<4; i++)
        SubCorner(mC[i], i); 
#endif
}

void OBBoxVolume::UpdateBBox()
{
    if (mAxisOrientation == VolAxisNotOriented)
    {
        mBoxSize.x = mLength[0] * Fabs(mAxis[0].x) + mLength[1] * Fabs(mAxis[1].x) + mLength[2] * Fabs(mAxis[2].x);
        mBoxSize.y = mLength[0] * Fabs(mAxis[0].y) + mLength[1] * Fabs(mAxis[1].y) + mLength[2] * Fabs(mAxis[2].y);
        mBoxSize.z = mLength[0] * Fabs(mAxis[0].z) + mLength[1] * Fabs(mAxis[1].z) + mLength[2] * Fabs(mAxis[2].z);
    }
    else
    {
        mBoxSize.x = mLength[0];
        mBoxSize.y = mLength[1];
        mBoxSize.z = mLength[2];
    }
}

void OBBoxVolume::UpdateBSphere()
{
    mSphereRadius = Sqrt(Sqr(mLength[0]) + Sqr(mLength[1]) + Sqr(mLength[2]));

    CollisionVolume::UpdateBSphere();
}

float OBBoxVolume::Projection(const Vector& axe) const
{
    if (mAxisOrientation == VolAxisNotOriented)
    {
        return mLength[0] * Fabs(axe.DotProduct(mAxis[0]))
             + mLength[1] * Fabs(axe.DotProduct(mAxis[1]))
             + mLength[2] * Fabs(axe.DotProduct(mAxis[2]));
    }
    else
    {
        return mLength[0] * Fabs(axe.x)
             + mLength[1] * Fabs(axe.y)
             + mLength[2] * Fabs(axe.z);
    }
}

float OBBoxVolume::Projection(const Vector& axe, int i) const
{
    float sum = 0;
  
    if (mAxisOrientation == VolAxisNotOriented)
    {
        if (i != 0) sum += mLength[0] * Fabs(axe.DotProduct(mAxis[0]));
        if (i != 1) sum += mLength[1] * Fabs(axe.DotProduct(mAxis[1]));
        if (i != 2) sum += mLength[2] * Fabs(axe.DotProduct(mAxis[2]));
    }
    else
    {
        if (i != 0) sum += mLength[0] * Fabs(axe.x);
        if (i != 1) sum += mLength[1] * Fabs(axe.y);
        if (i != 2) sum += mLength[2] * Fabs(axe.z);
    }  

    return sum;
}

void OBBoxVolume::SubCorner(Vector& n, int i) const
{
    float a, b, c;
    float sgn = (i>3 ? -1.0f : 1.0f);
    
    a = sgn * mLength[0];
    b = sgn * mLength[1];
    c = sgn * mLength[2];
    
    if (i>1)          b *= -1.0f;
    if (i==1 || i==3) c *= -1.0f;
    
    if (mAxisOrientation == VolAxisNotOriented)
    {
        n.x = a * mAxis[0].x + b * mAxis[1].x + c * mAxis[2].x;
        n.y = a * mAxis[0].y + b * mAxis[1].y + c * mAxis[2].y;
        n.z = a * mAxis[0].z + b * mAxis[1].z + c * mAxis[2].z;
    }
    else
    {
        n.x = a * mAxis[0].x;
        n.y = b * mAxis[1].y;
        n.z = c * mAxis[2].z;
    }
}

void OBBoxVolume::Corner(Vector& n, int i) const
{
#ifdef OBBoxStorePoints
    if (i < 4)
        n = mC[i];
    else
        n.Scale(-1.0f, mC[i]);
#else
    SubCorner(n, i);
#endif
}

void OBBoxVolume::Vertex(Vector& n, int i) const
{
    Corner(n, i);
    n.Add(mPosition);
}

void OBBoxVolume::Side(Vector& n, int i, int j) const
{
    rAssert(i<3);
    int ni = NXTI(i);
    int nni = NXTI(ni);
    float sgn = (j==0 ? mLength[ni] : -mLength[ni]);
    
    n.Scale(sgn, mAxis[ni]);
    n.ScaleAdd(mLength[nni], mAxis[nni]);
}

void OBBoxVolume::OptimizeAxis()
{
    int axis[3]={0, 0, 0};

    if (Fabs(mAxis[0].x) > AXIS_ALIGNED_SNAPPING_FACTOR)
        axis[0] = 1;
    else if (Fabs(mAxis[1].x) > AXIS_ALIGNED_SNAPPING_FACTOR)
        axis[1] = 1;
    else if (Fabs(mAxis[2].x) > AXIS_ALIGNED_SNAPPING_FACTOR)
        axis[2] = 1;
    else
        return;

    if (Fabs(mAxis[0].y) > AXIS_ALIGNED_SNAPPING_FACTOR)
        axis[0] = 2;
    else if (Fabs(mAxis[1].y) > AXIS_ALIGNED_SNAPPING_FACTOR)
        axis[1] = 2;
    else if (Fabs(mAxis[2].y) > AXIS_ALIGNED_SNAPPING_FACTOR)
        axis[2] = 2;
    else
        return;

    if (Fabs(mAxis[0].z) > AXIS_ALIGNED_SNAPPING_FACTOR)
        axis[0] = 3;
    else if (Fabs(mAxis[1].z) > AXIS_ALIGNED_SNAPPING_FACTOR)
        axis[1] = 3;
    else if (Fabs(mAxis[2].z) > AXIS_ALIGNED_SNAPPING_FACTOR)
        axis[2] = 3;
    else
    {
        return;
    }

    // if we're still there, the box is axis aligned
    rAssert(axis[0] != axis[1] && axis[0] != axis[2] && axis[1] != axis[2]);
    mAxisOrientation = VolAxisOriented;

    // swap to have axis ordered along xyz
    if (axis[0] != 1)
    {
        if (axis[1] == 1)
        {
            Swap(axis[0], axis[1]);
            SwapAxis(0, 1);
        }
        else
        {
            rAssert (axis[2] == 1);
            Swap(axis[0], axis[2]);
            SwapAxis(0, 2);
        }
    }
    if (axis[1] != 2)
    {
        rAssert (axis[2] == 2);
        Swap(axis[2], axis[1]);
        SwapAxis(2, 1);
    }

    // modifying the AxifRef and LengthRef could be dangerous if 
    // we set a transformed collision as static.

    //mAxisRef[0].Set(1.0f, 0.0f, 0.0f);
    mAxis[0].Set(1.0f, 0.0f, 0.0f);
    //mAxisRef[1].Set(0.0f, 1.0f, 0.0f);
    mAxis[1].Set(0.0f, 1.0f, 0.0f);
    //mAxisRef[2].Set(0.0f, 0.0f, 1.0f);
    mAxis[2].Set(0.0f, 0.0f, 1.0f);
}

void OBBoxVolume::SwapAxis(int a, int b)
{
    // don't do the swap because we snap them to the main axis instead;
    //Swap(mAxisRef[a], mAxisRef[b]);
    //Swap(mAxis[a], mAxis[b]);
    //Swap(mLengthRef[a], mLengthRef[b]);
    Swap(mLength[a], mLength[b]);
}


// WallVolume class
WallVolume::WallVolume(const Vector& p, const Vector& n)
: CollisionVolume(),
mNormal(n)
{
    mType = WallVolumeType;
    mPosition = p;
    mDP = mPosition;
    mNormal.Normalize();
    mNormalRef = mNormal;
    mSphereRadius = VERY_LARGE;
}

WallVolume::WallVolume()
: CollisionVolume()
{
    mType = WallVolumeType;
    mPosition.Set(0, 0, 0);
    mNormal.Set(0, 1.0f, 0);
    mNormalRef = mNormal;
    mAxisOrientation = VolAxisNotOriented;
    mSphereRadius = VERY_LARGE;
}

WallVolume::~WallVolume()
{
}

void WallVolume::SetClone(CollisionVolume* clone)
{
    CollisionVolume::SetClone(clone);
    WallVolume* volume = ((WallVolume*)clone);
    volume->mNormal = mNormal;
    volume->mNormalRef = mNormalRef;
    volume->mAxisOrientation = mAxisOrientation;
}

void WallVolume::UpdateBBox()
{
    // if wall perp to an axe, its projection on this axe is 0
    mBoxSize.x = (1.0f - Fabs(mNormal.x) < RELA_SMALL ? 0.0f : RELA_LARGE);
    mBoxSize.y = (1.0f - Fabs(mNormal.y) < RELA_SMALL ? 0.0f : RELA_LARGE);
    mBoxSize.z = (1.0f - Fabs(mNormal.z) < RELA_SMALL ? 0.0f : RELA_LARGE);
}

void WallVolume::UpdateScale(float a)
{  
    
    mNormalRef.Normalize();
    mNormalRef.Scale(Inverse(a));
    
    CollisionVolume::UpdateScale(a);
}

void WallVolume::SetRotation()
{  
    mUpdated = true;
    mCollisionObject->GetTransform(mObjRefIndex).RotateVector(mNormalRef, &mNormal);
}

void WallVolume::OptimizeAxis()
{
    mAxisOrientation = VolAxisNotOriented;

    if (mNormal.x > AXIS_ALIGNED_SNAPPING_FACTOR)
    {
        mAxisOrientation = VolAxisXOriented;
        mNormal.Set(1.0f, 0.0f, 0.0f);
    }
    else if (mNormal.y > AXIS_ALIGNED_SNAPPING_FACTOR)
    {
        mAxisOrientation = VolAxisYOriented;
        mNormal.Set(0.0f, 1.0f, 0.0f);
    }
    else if (mNormal.z > AXIS_ALIGNED_SNAPPING_FACTOR)
    {
        mAxisOrientation = VolAxisZOriented;
        mNormal.Set(0.0f, 0.0f, 1.0f);
    }
}

//
//
//

// BBoxVolume class
BBoxVolume::BBoxVolume()
: CollisionVolume()
{
    mType = BBoxVolumeType;
    mBoxSize.Set(1.0f,  1.0f,  1.0f);
    mSphereRadius = 0;
}

BBoxVolume::~BBoxVolume()
{
}

void BBoxVolume::UpdateBBox()
{
    mUpdated = true;
    
    if (mVisible)
    {
        if (mSubVolumeList != NULL && mSubVolumeList->GetSize() > 0)
        {
            Vector vmax, vmin;
        
            bool first = true;
        
            for (int i=0; i<mSubVolumeList->GetSize(); i++)
            {
                CollisionVolume* el = mSubVolumeList->GetAt(i);
                if (el->IsVisible())
                {
                    el->UpdateBBox();

                    if (first)
                    {
                        first = false;
                        vmax.Add(el->mPosition, el->mBoxSize);
                        vmin.Sub(el->mPosition, el->mBoxSize);
                    }
                    else
                    {
                        AddBox(vmax, vmin, el);
                    }
                }
            }
            mPosition.Add(vmax, vmin);
            mPosition.Scale(0.5f);
            mBoxSize.Sub(vmax, vmin);
            mBoxSize.Scale(0.5f);
        
            // compute mSphereRadius only for the bbox at the top of the hierarchy
            if (mCollisionObject && mCollisionObject->GetCollisionVolume() == this)
            {
                mSphereRadius = 1.732f * Max(mBoxSize.x, Max(mBoxSize.y, mBoxSize.z)); // sqrt(3)*longest edge, worst case
            }
        }
    }
}

void BBoxVolume::AddBox(Vector& vmax, Vector& vmin, CollisionVolume* el) const
{
    vmax.x = Max(vmax.x, el->mPosition.x + el->mBoxSize.x);
    vmin.x = Min(vmin.x, el->mPosition.x - el->mBoxSize.x);
    
    vmax.y = Max(vmax.y, el->mPosition.y + el->mBoxSize.y);
    vmin.y = Min(vmin.y, el->mPosition.y - el->mBoxSize.y);
    
    vmax.z = Max(vmax.z, el->mPosition.z + el->mBoxSize.z);
    vmin.z = Min(vmin.z, el->mPosition.z - el->mBoxSize.z);
}

/*

//
// the computation of the cm, inertia and volume was moved in tollib but with 
// PhComposite we have to bring it back to the run time but its use will be
// minimal.
//

void ComputeICM(const CollisionVolume* collVol, SymMatrix& i0, Vector& cm, float& vol, float density)
{
   // first need to find the cm
   vol = collVol->Volume();
   if (vol > 0)
   {
      cm.Scale(1.0f/vol, collVol->VR());
      i0 = collVol->Inertia(cm);
      i0.Scale(vol*density);
   }
}

float CollisionVolume::Volume() const
{
   float v = 0;
   if (mSubVolumeList != NULL)
   {
      for (int i=0; i<mSubVolumeList->GetSize(); i++)
      {
         v += (*mSubVolumeList)[i]->Volume();
      }
   }
   else
   {
      v = VolumeSelf();
   }
   return v;
}

Vector CollisionVolume::VR() const
{
   Vector vr(0, 0, 0);
   if (mSubVolumeList != NULL)
   {
      for (int i=0; i<mSubVolumeList->GetSize(); i++)
      {
         if ((*mSubVolumeList)[i]->mObjRefIndex == mObjRefIndex)
         {
            vr.Add((*mSubVolumeList)[i]->VR());
         }
      }
   }
   else
   {
      vr.Scale(VolumeSelf(), mPosition); 
   }
   return vr;
}
*/
SymMatrix CollisionVolume::Inertia(const Vector& cm) const
{
   SymMatrix i0;
   i0.Zero();

   if (mSubVolumeList != NULL)
   {
      for (int i=0; i<mSubVolumeList->GetSize(); i++)
      {
         if ((*mSubVolumeList)[i]->mObjRefIndex == mObjRefIndex)
         {
            i0.Add((*mSubVolumeList)[i]->Inertia(cm));
         }
      }
   }
   else
   {
      InertiaSelf(i0);
      Vector dp;
      dp.Sub(mPosition, cm);
      i0.xx += Sqr(dp.y) + Sqr(dp.z);
      i0.xy -= dp.x * dp.y;
      i0.xz -= dp.x * dp.z;
      i0.yy += Sqr(dp.x) + Sqr(dp.z);
      i0.yz -= dp.y * dp.z;
      i0.zz += Sqr(dp.x) + Sqr(dp.y);
   }
   return i0;
}/*

float SphereVolume::VolumeSelf() const
{
   return (4.0f/3.0f) * PI * mSphereRadius * mSphereRadius * mSphereRadius;
}

float CylinderVolume::VolumeSelf() const
{
   if (mFlatEnd)
      return PI_2 * mCylinderRadius * mCylinderRadius * mLength;
   else
      return PI_2 * mCylinderRadius * (mLength + (2.0f/3.0f) * mCylinderRadius * mCylinderRadius);
}

float OBBoxVolume::VolumeSelf() const
{
   return 8.0f * mLength[0] * mLength[1] * mLength[2];
}
*/
void SphereVolume::InertiaSelf(SymMatrix& m) const
{
   m.Identity();
   float f = 2.0f / 5.0f * Sqr(mSphereRadius);
   m.xx = m.yy = m.zz = f;
}

void CylinderVolume::InertiaSelf(SymMatrix& m) const
{
   m.Identity();
   float h;

   if (mFlatEnd)
      h = 2.0f * mLength;
   else
      h = 2.0f * (mLength + mCylinderRadius*2.0f/3.0f); // approx. of an height to include the capsule's half spheres
   
   // first set m as if the cyl orientation was along x
   m.xx = Sqr(mCylinderRadius) / 2.0f;
   m.yy = m.zz = (Sqr(h) + 3.0f * Sqr(mCylinderRadius)) / 12.0f;
   //
   Vector x(1.0f, 0, 0);
   Vector O2, O3;
   O2.CrossProduct(x, mAxis);
   
   if (O2.NormalizeSafe() < VERY_SMALL) 
      return; // axis already oriented along x
   
   O3.CrossProduct(mAxis, O2); 
   O3.Normalize(); // mAxis, O2 and O3 form an ortho basis to transform m
   Matrix r;
   r.Identity();
   r.Row(0) = mAxis;
   r.Row(1) = O2;
   r.Row(2) = O3;

   m.TransformSelf(r);
}

void OBBoxVolume::InertiaSelf(SymMatrix& m) const
{
   m.Identity();
   // first set m as if the axis were oriented along xyz
   m.xx = (Sqr(mLength[1]) + Sqr(mLength[2])) / 3.0f;
   m.yy = (Sqr(mLength[0]) + Sqr(mLength[2])) / 3.0f;
   m.zz = (Sqr(mLength[0]) + Sqr(mLength[1])) / 3.0f;
   //
   Matrix r;
   r.Identity();
   r.Row(0) = mAxis[0];
   r.Row(1) = mAxis[1];
   r.Row(2) = mAxis[2];

   m.TransformSelf(r);
}


} // sim

