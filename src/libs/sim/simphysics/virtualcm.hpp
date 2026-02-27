
#ifndef _VIRTUALCM_HPP_
#define _VIRTUALCM_HPP_


#include <p3d/p3dtypes.hpp>
#include <radmath/radmath.hpp>
#include "simcommon/tlist.hpp"

// the vcm links the equivalent of a mass to a moving object using a damped spring system
// obsevation of the relative position of the vcm woth the position of the object gives
// information on the acceleration experienced by the object.

namespace sim
{

typedef unsigned VirtualCMMode;

const VirtualCMMode VCM_LinearBit      = 1<<1;
const VirtualCMMode VCM_AngularBit     = 1<<2;
const VirtualCMMode VCM_VerticalBit    = 1<<3;
const VirtualCMMode VCM_ActiveBit      = 1<<4;

const VirtualCMMode VCM_LinearMask     = ~VCM_LinearBit;
const VirtualCMMode VCM_AngularMask    = ~VCM_AngularBit;
const VirtualCMMode VCM_VerticalMask   = ~VCM_VerticalBit;
const VirtualCMMode VCM_ActiveMask     = ~VCM_ActiveBit;

const VirtualCMMode VCM_DefaultMode    = VCM_LinearBit | VCM_ActiveBit;

class JointVirtualCM;
typedef TList<JointVirtualCM*> JointVCMpArray;

class VirtualCM
{
public:
    VirtualCM();
    VirtualCM(VirtualCMMode inBits);
    virtual ~VirtualCM() {}

    // Enable or desable domain modes.
    inline void SetMode(VirtualCMMode inMode ) { mModeFlag = inMode;} 
    inline void SetLinearMode  (bool inVal);
    inline void SetAngularMode (bool inVal);
    inline void SetVerticalMode(bool inVal);
    inline void SetActive      (bool inVal);
    inline VirtualCMMode GetMode( )const { return mModeFlag; }
    inline bool GetLinearMode  ()const;
    inline bool GetAngularMode ()const;
    inline bool GetVerticalMode()const;
    inline bool GetActive      ()const;
    
    // Init and Update methods.
    virtual void InitLinear (const rmt::Vector& pos, const rmt::Vector& speedV);
    virtual void InitAngular(const rmt::Vector& ang, const rmt::Vector& speedW);
    void SetInvTP(float f) { mInvTP = f; } // lower = stiffer spring
    void SetInvTV(float f) { mInvTV = f; } // lower = more damping in the spring

    virtual void Update(const rmt::Vector& pos, const rmt::Vector& speed, float dt);
    void AddObjectCache(const rmt::Vector& v, const rmt::Vector& w);
    
    // Access to the state.
    inline const rmt::Vector& GetPosition() const { return mPosition; }
    inline const rmt::Vector& GetVelocity() const { return mVelocity; }
    inline const rmt::Vector& GetAngularVelocity() const { return mAngularVelocity; }
    inline const rmt::Vector& GetAngularDeviation() const { return mAngularDeviation; }
    
    void DebugDisplay() const;


protected:
    virtual void  PrintOut(float inDt)const;//For debug
    
    
private:
    rmt::Vector mPosition;                 // virtual position
    rmt::Vector mVelocity;                 // virtual speed
    rmt::Vector mAngularVelocity;          // virtual angular velocity
    rmt::Vector mAngularDeviation;         // virtual angular deviation
    
    float mInvTP;
    float mInvTV;
    float mInvTA;
    
    float mRestP;
    float mRestV;
    
    bool mVerticalEnabled;
    bool mEnabled;

    VirtualCMMode mModeFlag;
    
    static float sDefault_invTP; // units: inverse of time
    static float sDefault_invTV; // units: inverse of time
    static float sDefault_invTA; // units: inverse of time
    
    static float sDefault_restP; // no unit
    static float sDefault_restV; // no unit
};

class JointVirtualCM :
    public VirtualCM {

public:
    JointVirtualCM():
        VirtualCM(),
        mIndex(0){}

    JointVirtualCM(VirtualCMMode inBits, int inIndex):
        VirtualCM(inBits),
        mIndex(inIndex){}

    void SetIndex(int index){mIndex = index;}
    int GetIndex()const {return mIndex;}

    virtual void  PrintOut(float inDt)const;//For debug

protected:
    int mIndex;

};


inline void VirtualCM::SetLinearMode (bool inVal) 
{ 
    if(inVal)
        mModeFlag |= VCM_LinearBit;
    else
        mModeFlag &= VCM_LinearMask;
}

inline void VirtualCM::SetAngularMode (bool inVal) 
{ 
    if(inVal)
        mModeFlag |= VCM_AngularBit;
    else
        mModeFlag &= VCM_AngularMask;
}

inline void VirtualCM::SetVerticalMode(bool inVal)
{
    if(inVal)
        mModeFlag |= VCM_VerticalBit;
    else
        mModeFlag &= VCM_VerticalMask;
}

inline bool VirtualCM::GetActive()const
{
    return (mModeFlag&VCM_ActiveBit) ? true:false; 
}

//Client should probably call Init methods after activating the vcm.
inline void VirtualCM::SetActive(bool inVal)
{
    if(inVal)
        mModeFlag |= VCM_ActiveBit;
    else
        mModeFlag &= VCM_ActiveMask;

    if (!GetActive())
    {
        mPosition.Clear();        
        mVelocity.Clear();
        mAngularVelocity.Clear();
        mAngularDeviation.Clear();
    }
}

inline bool VirtualCM::GetLinearMode() const  
{ 
    return (mModeFlag&VCM_LinearBit)  ? true:false; 
}

inline bool VirtualCM::GetAngularMode() const 
{ 
    return (mModeFlag&VCM_AngularBit) ? true:false; 
}

inline bool VirtualCM::GetVerticalMode()const
{ 
    return (mModeFlag&VCM_VerticalBit) ? true:false; 
}

} // sim

#endif // _VIRTUALCM_HPP_
