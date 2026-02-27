
#include <simcollision/collisionobject.hpp>
#include <simcollision/collisionmanager.hpp>
#include <p3d/anim/compositedrawable.hpp>

// for the loader
#include <p3d/chunkfile.hpp>
#include <constants/chunkids.hpp>
#include <string.h>

using namespace RadicalMathLibrary;

namespace sim
{

CollisionObject::CollisionObject(CollisionVolume* inCollisionVolume)
    : mSimState(NULL),
    mPhysicsProperties(NULL),
    mCollisionVolume(NULL),
    mNumJoint(0),
    mIsStatic(false),
    mCollideWithStatic(true),
    mDefaultArea(0),
    mHasMoved(true),
    mHasRelocated(true),
    mAutoPair(true),
    mManualUpdate(false),
    mCollisionEnabled(true),
    mSelfCollisionEnabled(false),
    mRayCastingEnabled(true),
    mPossibleCollisionEvents(cCollisionAnalyserPossibleEventAll)
{
    rAssert(inCollisionVolume != NULL);
    
    SetCollisionVolume(inCollisionVolume);
}

CollisionObject::~CollisionObject()
{
    if (mCollisionVolume)
        mCollisionVolume->Release();
    
    if (mPhysicsProperties)
        mPhysicsProperties->Release();
}

void CollisionObject::SetIsStatic(bool inIsStatic)
{
    rAssert(!(CollisionManager::GetCurrentInstance() && CollisionManager::GetCurrentInstance()->Contains(this)));

    if (mIsStatic && !inIsStatic)
    {
        mIsStatic = inIsStatic;
        mCollisionVolume->DeOptimize();
    }
    else if (!mIsStatic && inIsStatic)
    {
        mCollisionVolume->UpdateAll();
        mIsStatic = inIsStatic;
        mCollisionVolume->Optimize();
        mHasMoved = false;
        mHasRelocated = false;
    }
}

void CollisionObject::SetCollideWithStatic(bool inCollideWithStatic)
{
    mCollideWithStatic = inCollideWithStatic;
    if (CollisionManager::GetCurrentInstance())
    {
        for (int i=0; i< CollisionManager::GetCurrentInstance()->NbArea(); i++)
        {
            if (CollisionManager::GetCurrentInstance()->GetCollisionObjectList(i)->Contains(this))
            {
                CollisionManager::GetCurrentInstance()->RemoveCollisionObject(this, i);
                CollisionManager::GetCurrentInstance()->AddCollisionObject(this, i);
            }
        }
    }
}

void CollisionObject::SetPhysicsProperties(PhysicsProperties* inProperties)
{
    if (mPhysicsProperties)
        mPhysicsProperties->Release();
    
    mPhysicsProperties = inProperties;
    mPhysicsProperties->AddRef();
}

void CollisionObject::ConvertToRigidBody()
{
    Relocated();
    Update();
    mNumJoint = 0;
    mCollisionVolume->ConvertToRigidBody();
    mSelfCollisionList.Clear();
}

CollisionObject* CollisionObject::Clone(SimState* inSimState)
{
    rAssert(inSimState);
    
    CollisionObject* clone          = new CollisionObject;
    clone->SetNameObject(GetNameObject());
    
    clone->mCollisionVolume         = NULL;
    clone->mPhysicsProperties       = mPhysicsProperties;
    clone->mPhysicsProperties->AddRef();
    
    clone->SetCollisionVolume(mCollisionVolume->Clone());
    clone->mSimState                = inSimState;
    clone->mStringData              = mStringData;
    clone->mNumJoint                = mNumJoint;
    clone->mSelfCollisionList       = mSelfCollisionList; // test that
    clone->mCollisionVolumeOwner    = mCollisionVolumeOwner; // test that
    clone->mIsStatic                = mIsStatic;
    clone->mCollideWithStatic       = mCollideWithStatic;
    clone->mDefaultArea             = mDefaultArea;
    clone->mPossibleCollisionEvents = mPossibleCollisionEvents;
    clone->mSelfCollisionEnabled    = mSelfCollisionEnabled;
    clone->mRayCastingEnabled       = mRayCastingEnabled;
    clone->mAutoPair                = mAutoPair;
    clone->mManualUpdate            = mManualUpdate;
    
    for (int i=0; i<mSelfCollisionList.GetSize(); i++)
        clone->SetSelfCollision(clone->mSelfCollisionList[i]);

    return clone;
}

void CollisionObject::SetCollisionVolume(CollisionVolume* inCollisionVolume)
{
    rAssert(inCollisionVolume != NULL);
    
    if (mCollisionVolume)
        mCollisionVolume->Release();
    
    mCollisionVolume = inCollisionVolume;
    mCollisionVolume->AddRef();
    
    mCollisionVolume->SetCollisionObject(this);
    mHasMoved = true;
    
    for (int i=0; i<mSelfCollisionList.GetSize(); i++)
        SetSelfCollision(mSelfCollisionList[i]);
}

void CollisionObject::SetVisibility(tCompositeDrawable* inCompositeDrawable)
{
    rAssert(GetSimState()->GetSimulatedObject() == NULL);
    /*
    if there is a simulated object,
    user should use SimStateAriculated::SetVisibility mechanism instead of this.
    this method would work only if the SimStateAriculated::SetVisibility is not 
    used and there is no simulated object associated to this collision object.

    the SimStateAriculated::SetVisibility allows to set visibility on a joint basis,
    not owner basis, which can become a problem. but for now we didn't want to
    maintain more than one inertia matrix per joint in the art phys obj.
    */

    if (mCollisionVolumeOwner.mNumOwner > 0)
    {
        bool needUpdate = false;
        for (int i=0; i<mCollisionVolumeOwner.mNumOwner; i++)
        {
            bool ownerVis = false;
            for (int j=0; j<mCollisionVolumeOwner.mOwnerList[i].GetSize(); j++)
            {
                if (inCompositeDrawable->FindNode(mCollisionVolumeOwner.mOwnerList[i][j])->IsVisible())
                    ownerVis = true;
            }
            if (ownerVis != mCollisionVolumeOwner.mVisible[i])
            {
                mCollisionVolumeOwner.mVisible[i] = ownerVis;
                needUpdate = true;
            }
        }
        if (needUpdate)
        {
            UpdateVisibilityByOwner(mCollisionVolume, mCollisionVolumeOwner.mVisible);
            mCollisionVolume->UpdateBBoxVisibility();
        }
    }
}

void CollisionObject::SetVisibility(int inJoint, bool inVisible)
{
    UpdateVisibilityByJoint(mCollisionVolume, inJoint, inVisible);
}

void CollisionObject::UpdateVisibilityByJoint(CollisionVolume* inVol, int inJoint, bool inVisible)
{
    if (inVol->ObjRefIndex() == inJoint)
        inVol->mVisible = inVisible;

    if (inJoint >= inVol->ObjRefIndex() && inVol->HasSubVolume())
    {
        for (int i=0; i<inVol->SubVolumeList()->GetSize(); i++)
        {
            UpdateVisibilityByJoint(inVol->SubVolumeList()->GetAt(i), inJoint, inVisible);
        }
    }
}

void CollisionObject::UpdateVisibilityByOwner(CollisionVolume* inVol, TArray<bool>& inVisible)
{
    inVol->mVisible = inVisible[inVol->mOwnerIndex];
    if (inVol->HasSubVolume())
    {
        for (int i=0; i<inVol->SubVolumeList()->GetSize(); i++)
        {
            UpdateVisibilityByOwner(inVol->SubVolumeList()->GetAt(i), inVisible);
        }
    }
}

void CollisionObject::AddSelfCollision(int inIndex1, int inIndex2, bool inSelf1, bool inSelf2)
{
    mSelfCollisionEnabled = true;

    SelfCollision& sc = mSelfCollisionList.NewElem();
    sc.Set(inIndex1, inIndex2, inSelf1, inSelf2);
    if (mCollisionVolume)
    {
        SetSelfCollision(sc);
    }
}

void CollisionObject::SetSelfCollision(SelfCollision& inSelfColl)
{
    inSelfColl.mCollisionVolume1 = mCollisionVolume->GetSubCollisionVolume(inSelfColl.mIndex1, inSelfColl.mSelf1);
    inSelfColl.mCollisionVolume2 = mCollisionVolume->GetSubCollisionVolume(inSelfColl.mIndex2, inSelfColl.mSelf2);
    rAssert(inSelfColl.mCollisionVolume1 && inSelfColl.mCollisionVolume2);
}



//
//
//

CollisionObject::CollisionVolumeOwner &CollisionObject::CollisionVolumeOwner::operator= ( const CollisionObject::CollisionVolumeOwner &inCollisionVolumeOwner )
{
    mNumOwner = inCollisionVolumeOwner.mNumOwner;
    if (mNumOwner > 0)
    {
        mOwnerList = new TArray<tUID>[mNumOwner];
        
        for (int i=0; i<mNumOwner; i++)
        {
            mOwnerList[i] = inCollisionVolumeOwner.mOwnerList[i];
        }
        mVisible = inCollisionVolumeOwner.mVisible;
    }
    return *this;
}

//
// CollisionObjectLoader class
//

static const int COLLISIONOBJECT_VERSION = 1;

void LoadVectorFromCollisionVectorChunk(tChunkFile* file, Vector& v)
{
    file->BeginChunk();
    v.x = file->GetFloat();
    v.y = file->GetFloat();
    v.z = file->GetFloat();
    file->EndChunk();
}


CollisionObjectLoader::CollisionObjectLoader()
    : tSimpleChunkHandler(Simulation::Collision::OBJECT)
{
}

tEntity* CollisionObjectLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{ 
    char name[255], stringData[255];
    CollisionObject* newCollisionObject = new CollisionObject;
    
    f->GetPString(name);
    newCollisionObject->SetName(name);
    
    int version = f->GetLong();
    rAssert(version == COLLISIONOBJECT_VERSION);
    
    f->GetPString(stringData);
    
    newCollisionObject->mStringData.SetText(stringData);
    
    int nbSubObject = f->GetLong();
    newCollisionObject->SetNumJoint(nbSubObject);
    
    int currentOwner = 0;
    int numOwner = f->GetLong();
    if (numOwner > 0)
        newCollisionObject->mCollisionVolumeOwner.SetNumOwnerList(numOwner);
    
    while(f->ChunksRemaining())
    {      
        f->BeginChunk();
        switch(f->GetCurrentID())
        {
        case Simulation::Collision::OWNER:
            {
                int numNames = f->GetLong();
                if (numNames > 0)
                {
                    newCollisionObject->mCollisionVolumeOwner.mOwnerList[currentOwner].ResizeArray( numNames );
                    while(f->ChunksRemaining())
                    {
                        f->BeginChunk();
                        switch(f->GetCurrentID())
                        {
                        case Simulation::Collision::OWNERNAME:
                            {
                                char newName[256];
                                f->GetPString(newName);
                                newCollisionObject->mCollisionVolumeOwner.mOwnerList[currentOwner].NewElem() = tEntity::MakeUID(newName);
                                break;
                            }
                        default:
                            rAssert(false); // should not get here
                            break;
                        }
                        f->EndChunk();
                    }
                    rAssert(newCollisionObject->mCollisionVolumeOwner.mOwnerList[currentOwner].GetSize() == numNames);
                }
                currentOwner++;
                break;
            }
        case Simulation::Collision::SELFCOLLISION:
            {
                int index1 = f->GetLong();
                int index2 = f->GetLong();
                bool self1  = f->GetUWord() == 0 ? false : true;
                bool self2  = f->GetUWord() == 0 ? false : true;
                newCollisionObject->AddSelfCollision(index1, index2, self1, self2);
                break;
            }
        case Simulation::Collision::VOLUME:
            {
                CollisionVolume* collisionVolume = LoadCollisionVolume(f);
                rAssert(collisionVolume);
                newCollisionObject->SetCollisionVolume(collisionVolume);
                break;
            }
        case Simulation::Collision::ATTRIBUTE:
            {
                newCollisionObject->mIsStatic = f->GetUWord() == 0 ? false : true;
                newCollisionObject->mDefaultArea = f->GetLong();
                bool canRoll = f->GetUWord() == 0 ? false : true;
                bool canSlide = f->GetUWord() == 0 ? false : true;
                bool canSpin = f->GetUWord() == 0 ? false : true;
                bool canBounce = f->GetUWord() == 0 ? false : true;
                unsigned int extraAttribute1 = f->GetLong();
                unsigned int extraAttribute2 = f->GetLong();
                unsigned int extraAttribute3 = f->GetLong();
                
                CollisionAnalyserPossibleEvent possibleEvent = cCollisionAnalyserPossibleEventAll;
                if (!canRoll)
                    possibleEvent -= cCollisionAnalyserPossibleEventRolling;
                if (!canSlide)
                    possibleEvent -= cCollisionAnalyserPossibleEventSliding;
                if (!canSpin)
                    possibleEvent -= cCollisionAnalyserPossibleEventSpinning;
                if (!canBounce)
                    possibleEvent -= cCollisionAnalyserPossibleEventBouncing;
                
                newCollisionObject->SetPossibleCollisionEvents(possibleEvent);
                
                break;
            }
        }
        f->EndChunk();
    }
    rAssert(newCollisionObject->GetCollisionVolume() != NULL);
    rAssert(currentOwner == numOwner);
    
    // Don't search through all of the inventory sections for the physics
    // property.  Simpsons had really long load times when it did.
    //PhysicsProperties* tmpprop = PhysicsProperties::FindPhysicsProperty(stringData, store);
    PhysicsProperties* tmpprop = NULL;

    if (!tmpprop)
    {
        if (nbSubObject == 0)
        {
            tmpprop = PhysicsProperties::DefaultPhysicsProperties(store);
        }
        else
        {
            tmpprop = PhysicsProperties::DefaultArtPhysicsProperties(store);
        }
    }
    newCollisionObject->SetPhysicsProperties(tmpprop);

    if (newCollisionObject->mIsStatic)
    {
        newCollisionObject->mCollisionVolume->Optimize();
        newCollisionObject->mHasMoved = false;
        newCollisionObject->mHasRelocated = false;
    }
    
    return newCollisionObject;
}

CollisionVolume* CollisionObjectLoader::LoadCollisionVolume(tChunkFile* file)
{
    int objrefIndex   = file->GetLong();
    int ownerIndex    = file->GetLong();
    int numSubVolume  = file->GetLong();
    
    CollisionVolume* newCollisionVolume = NULL;
    
    file->BeginChunk();
    switch(file->GetCurrentID())
    {
    case Simulation::Collision::SPHERE:
        {
            float radius = file->GetFloat();
            Vector p;
            LoadVectorFromCollisionVectorChunk(file, p);
            newCollisionVolume = (CollisionVolume*) new SphereVolume(p, radius);
            break;
        }
    case Simulation::Collision::CYLINDER:
        {
            float radius = file->GetFloat();
            float length = file->GetFloat();
            bool flatEnd = file->GetUWord() == 0 ? false : true;
            Vector p, o;
            LoadVectorFromCollisionVectorChunk(file, p);
            LoadVectorFromCollisionVectorChunk(file, o);
            newCollisionVolume = (CollisionVolume*) new CylinderVolume(p, o, length, radius, flatEnd);
            break;
        }
    case Simulation::Collision::OBBOX:
        {
            float l0 = file->GetFloat();
            float l1 = file->GetFloat();
            float l2 = file->GetFloat();
            Vector p, o0, o1, o2;
            LoadVectorFromCollisionVectorChunk(file, p);
            LoadVectorFromCollisionVectorChunk(file, o0);
            LoadVectorFromCollisionVectorChunk(file, o1);
            LoadVectorFromCollisionVectorChunk(file, o2);
            newCollisionVolume = (CollisionVolume*) new OBBoxVolume(p, o0, o1, o2, l0, l1, l2);
            break;
        }
    case Simulation::Collision::WALL:
        {
            Vector p, n;
            LoadVectorFromCollisionVectorChunk(file, p);
            LoadVectorFromCollisionVectorChunk(file, n);
            newCollisionVolume = (CollisionVolume*) new WallVolume(p, n);
            break;
        }
    case Simulation::Collision::BBOX:
        {
            newCollisionVolume = (CollisionVolume*) new BBoxVolume();
            int dum = file->GetLong();
            break;
        }
        
    default:
        rAssert(false);
        break;
    }
    file->EndChunk();
    
    newCollisionVolume->SetObjRefIndex(objrefIndex);
    newCollisionVolume->SetOwnerIndex(ownerIndex);
    newCollisionVolume->Reserve( numSubVolume );
    for( int i=0; i<numSubVolume; i++)
    {
        file->BeginChunk();
        CollisionVolume* newSubCollisionVolume = LoadCollisionVolume(file);
        file->EndChunk();
        newCollisionVolume->AddSubVolume(newSubCollisionVolume);
    }
    rAssert(newCollisionVolume);
    
    if (numSubVolume)
    {
        rAssert(newCollisionVolume->SubVolumeList()->GetSize() == numSubVolume);
    }
    return newCollisionVolume;
}

void CollisionObject::CollisionVolumeOwner::SetNumOwnerList( int inNum )
{
    delete[] mOwnerList;
    mVisible.Clear();
    mNumOwner = inNum;
    mOwnerList = new TArray< tUID >[ mNumOwner ];
    mVisible.ResizeArray( mNumOwner );
    for( int i = 0; i < mNumOwner; ++i )
    {
        mVisible.NewElem() = true;
    }
}

} // sim

