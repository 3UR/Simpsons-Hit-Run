
#ifndef _CollisionVolume_HPP_
#define _CollisionVolume_HPP_

#include <p3d/refcounted.hpp>
#include <p3d/p3dtypes.hpp>
#include <radmath/radmath.hpp>
#include <simcommon/tlist.hpp>
#include <simcommon/symmatrix.hpp>

namespace sim
{

class CollisionObject;
class CollisionVolume;

enum CollisionVolumeTypeEnum { CollisionVolumeType=0, SphereVolumeType, CylinderVolumeType, OBBoxVolumeType, WallVolumeType, BBoxVolumeType, MaxCollisionVolumeEnum };
enum VolAxisOrientation { VolAxisNotOriented=0, VolAxisOriented, VolAxisXOriented, VolAxisYOriented, VolAxisZOriented };

// base class
class CollisionVolume
    : public tRefCounted
{
public:
    CollisionVolume();
    virtual ~CollisionVolume();
    
    rmt::Vector mPosition;              // reference position of the Volume
    rmt::Vector mBoxSize;               // mPosition and mBoxSize determine the BBox position and size
    float mSphereRadius;                // sphere radius
    
    void SetCollisionObject(CollisionObject* inCollObject);
    inline CollisionObject* GetCollisionObject()    { return mCollisionObject; }
    void SetObjRefIndex(int i);
    void SetOwnerIndex(int i)                       { mOwnerIndex = i; }
    inline int ObjRefIndex()                  const { return mObjRefIndex; }
    
    inline bool Updated() const                     { return mUpdated; }
    inline CollisionVolumeTypeEnum Type()     const { return mType; }
    inline bool IsVisible() const                     { return mVisible; }
    inline TList<CollisionVolume*>* SubVolumeList()   { return mSubVolumeList; }
    
    void AddSubVolume(CollisionVolume* point);
    bool RemoveSubVolume(CollisionVolume* el);
    void EmptySubVolume();
    bool HasSubVolume()                       const { return (mSubVolumeList == NULL || mSubVolumeList->GetSize() == 0) ? false : true; }
    
    // GetSubCollisionVolume returns a sub volume that has the input index. 
    // if self = false, it will return the bbox of that joint with possibly the other subjoints 
    // of the hierarchy below it.
    CollisionVolume*  GetSubCollisionVolume(int inObjRefIndex, bool inSelfOnly = true);
    
    CollisionVolume* Clone();
    
    virtual void UpdateScale(float a);              // update the value to the new scale factor
    virtual void UpdateBBox() {}                    // update the BBox
    virtual void UpdateBSphere();                   // update the bounding sphere
    virtual void UpdateAll();                       // complete update
    
    bool ContainsBB(CollisionVolume* el);           // mostly for debug to make sure a sub BB is smaller than the parent
    //virtual bool IsBBIn(CollisionVolume* el, float precision = 0);
    
    /*
    float Volume() const;
    rmt::Vector VR() const;
    */
    sim::SymMatrix Inertia(const rmt::Vector& cm) const;
    /*
    virtual float VolumeSelf() const                { return 0; }*/
    virtual void InertiaSelf(SymMatrix& i0) const   { i0.Zero(); }
    
    tColour  GetColour(void) {return (mColour);}
    void    SetColour(tColour colour){mColour = colour;}

    void Optimize();
    void DeOptimize();

    const rmt::Vector& GetDP() const { return mDP; }

    bool UpdateBBoxVisibility();
    void Reserve( const size_t size );

    void GenerateHierarchy(void);
    void PreallocateSubVolumes(unsigned num);

protected:
    friend class CollisionObject;

    void ConvertToRigidBody();
    
    void UpdateRot();
    void UpdatePos();
    virtual void SetRotation() {}
    virtual void SetClone(CollisionVolume* clone);
    
    CollisionVolumeTypeEnum mType;
    int mObjRefIndex;              // object reference index, if >= 0 refers to a joint
    int mOwnerIndex;               // owner reference index used, refer to one ownerlist item in the collisionobject, for composite drawable LOD purposes
    
    CollisionObject* mCollisionObject;
    TList<CollisionVolume*>* mSubVolumeList;
    
    bool mVisible;    
    bool mUpdated;
    rmt::Vector mDP; // relative position of the Volume and its parent object
    
    tColour mColour;

private:
    CollisionVolume& operator=(CollisionVolume& inVolume) { rAssert(false); return *this; }
    CollisionVolume(const CollisionVolume& inCollisionVolume) {}
};


// SphereVolume class
class SphereVolume
    : public CollisionVolume
{
    friend class CollisionVolume; // to have access to the protected constructor
    
public:
    SphereVolume(const rmt::Vector& p, float r = 1.0f);
    virtual ~SphereVolume();
    
    virtual void UpdateScale(float a);

	// to change the radius on the fly from bizzaro AI code - Simpsons
    void UpdateRadius(float r);    
	
    float GetRadius()                const { return mSphereRadius; }
    /*
    virtual float VolumeSelf() const;*/
    virtual void InertiaSelf(SymMatrix& i0) const;
    
    
protected:
    SphereVolume() {}
    virtual void SetClone(CollisionVolume* clone);
    
    float mSphereRadiusRef;         // sphere initial radius
};

// CylinderVolume class

class CylinderVolume
    : public CollisionVolume
{
    friend class CollisionVolume;    // to have access to the protected constructor
    
public:
    CylinderVolume(const rmt::Vector& center, const rmt::Vector& o, float len, float r, bool flatend = false);
    virtual ~CylinderVolume();
    
    virtual void UpdateScale(float a);
    virtual void UpdateBBox();
    virtual void UpdateBSphere();
    
    rmt::Vector mAxis;               // cylinder's orientation 
    float mLength;                   // cylinder's half-length
    float mCylinderRadius;           // cylinder's radius
    
    bool mFlatEnd;
    VolAxisOrientation mAxisOrientation;
    
    /*
    virtual float VolumeSelf() const;*/
    virtual void InertiaSelf(SymMatrix& i0) const;
    
    // move up to object
    //bool mCharacter;

protected:
    CylinderVolume() {}
    virtual void SetRotation();
    virtual void SetClone(CollisionVolume* clone);
    void OptimizeAxis();
    
    rmt::Vector mAxisRef;              // cylinder's init orientation
    float mLengthRef;                  // cylinder's half-length
    float mCylinderRadiusRef;          // cylinder's initial radius
};

// OBBoxVolume class
//#define OBBoxStorePoints
class OBBoxVolume
    : public CollisionVolume
{
    friend class CollisionVolume;    // to have access to the protected constructor
    
public:
    OBBoxVolume(const rmt::Vector& center, const rmt::Vector& o0, const rmt::Vector& o1, const rmt::Vector& o2, float l0, float l1, float l2) 
    { 
        mType = OBBoxVolumeType; 
        Set(center, o0, o1, o2, l0, l1, l2); 
    }

    virtual ~OBBoxVolume();

    void Set(const rmt::Vector& center, const rmt::Vector& o0, const rmt::Vector& o1, const rmt::Vector& o2, float l0, float l1, float l2);
    
    virtual void UpdateScale(float a);
    virtual void UpdateBBox();
    virtual void UpdateBSphere();                   // update the bounding sphere
    
    float Projection(const rmt::Vector& axe) const;      // projection if the 3 axis
    float Projection(const rmt::Vector& axe, int i) const;// projection of 2 axis
    void Corner(rmt::Vector& n, int i) const;
    void Side(rmt::Vector& n, int i, int j) const;
    void Vertex(rmt::Vector& n, int i) const;
    
    rmt::Vector mAxis[3];   // orientation of the box
    float mLength[3];      // half length of the 3 cube's dimension
    
                           /*
                           virtual float VolumeSelf() const;*/
                           virtual void InertiaSelf(SymMatrix& i0) const;
    

    VolAxisOrientation mAxisOrientation;

protected:
    OBBoxVolume() {}
    void SubCorner(rmt::Vector& n, int i) const;
    virtual void SetRotation();
    virtual void SetClone(CollisionVolume* clone);

    void OptimizeAxis();
    void SwapAxis(int a, int b);
    
    rmt::Vector mAxisRef[3];  // init orientation of the box
    float mLengthRef[3];     // init half length of the 3 cube's dimension
#ifdef OBBoxStorePoints
    rmt::Vector mC[4];
#endif
};

// WallVolume class
class WallVolume
    : public CollisionVolume
{
    friend class CollisionVolume; // to have access to the protected constructor
    
public:
    WallVolume(const rmt::Vector& p, const rmt::Vector& n);
    virtual ~WallVolume();
    
    virtual void UpdateBBox();
    virtual void UpdateScale(float a);
    
    rmt::Vector mNormal;
    VolAxisOrientation mAxisOrientation;
    
    //void SetDimension(const rmt::Vector& inPoints, int inNbPoints, bool inSymetrical = false);
    //rmt::Vector* mAxis;
    //float* mSize;
    //int mNbAxis;
    
protected:
    WallVolume();
    virtual void SetRotation();
    virtual void SetClone(CollisionVolume* clone);
    void OptimizeAxis();

    //bool mInfinite;
    //bool mSymetrical;
    
    rmt::Vector mNormalRef; // Wall normal, points toward the inside (where the colliding objects should be
    //rmt::Vector* mAxisRef;
};

// BBoxVolume class
class BBoxVolume 
    : public CollisionVolume
{
public:
    BBoxVolume();
    virtual ~BBoxVolume();
    
    virtual void UpdateBBox();
    
protected:
    void AddBox(rmt::Vector& vmax, rmt::Vector& vmin, CollisionVolume* el) const;
};



//void ComputeICM(const scoll::CollisionVolume* collVol, SymMatrix& i0, rmt::Vector& cm, float& vol, float density);


// the flatend option is not completely implemented. it works only with the infinite plane
// undef the following is not an option for now so the relevant code is simply commented out
//#define DETECT_COLL_ON_CYL_FLATEND

}  // sim

#endif // _CollisionVolume_H_
