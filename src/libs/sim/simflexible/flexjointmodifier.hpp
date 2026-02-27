#ifndef _FLEXJOINTMODIFIER_HPP_
#define _FLEXJOINTMODIFIER_HPP_

#include "simcommon/tarray.hpp"
#include "simflexible/psystmanual.hpp"
#include "poser/posedriver.hpp"

#include "p3d/loadmanager.hpp"

namespace poser
{
    class Pose;
}

namespace sim
{
    
    enum RestPositionMethodEnum {RELATIVE1 = 1, RELATIVE2, RELATIVE3, RELATIVE4, RELATIVE5, RELATIVE6};
    enum UpdateJointsMethodEnum {SIMPLEDEF1 = 1, SIMPLEDEF2, SIMPLEDEF3 };
    enum SimMethodEnum { SIMMETHOD1 = 1 }; // unused for now
    
    
    class SimTarget;
    class SimStateArticulated;
    
    class FlexibleJointDefinition
        : public tEntity
    {
    public:
        
        class FlexibleJointConnection
        {
        public:
            FlexibleJointConnection() {}
            ~FlexibleJointConnection() {}
            
            bool Contains(short inIndex) const { return (inIndex == mIndex1 || inIndex == mIndex2) ? true : false; }
            
            short mIndex1;
            short mIndex2;
            float mLength;
            bool  mOnBranch;
        };
        
        class FlexibleJointHinge
        {
        public:
            FlexibleJointHinge() {}
            ~FlexibleJointHinge() {}
            
            short mIndex1;
            short mIndex2;
            short mIndex3;
        };
        
        class FlexibleJointDefinitionItem
        {
        public:
            FlexibleJointDefinitionItem()
                : mGravity(true),
                mSimMethod(0),
                mUseRestingPos(0),
                mRestMethod(0),
                mUpdateMethod(0),
                mUseVirtualJoint(false),
                mSolver(0),
                mExternalDensityFactor(1.0f)
            {}
            virtual ~FlexibleJointDefinitionItem() {}
            
            tUID mName;
            //tName mName;
            tUID mParametersName;
            tUID mRestingPosParametersName;
            tUID mConnectionParametersName;
            
            bool  mGravity;               // add gravity force to the system
            short mSimMethod;             // specifies the simulation method
            bool  mUseRestingPos;         // specifies to add constraint between the system and the resting position
            short mRestMethod;            // specifies the resting position extraction method
            short mUpdateMethod;          // specifies the update method
            bool  mUseVirtualJoint;       // 
            short mSolver;                    // specifies the particle system solver to be used 
            float mExternalDensityFactor; // specifies the density of the flexible object as seen by the object object during collision
            
            TArray<short> mFlexibleJointIndex;
            TArray<short> mFlexibleFixJointIndex;
            TArray<short> mEndOfBranchIndex;
            TArray<FlexibleJointConnection> mFlexibleJointConnection;
            TArray<FlexibleJointHinge> mFlexibleJointHinge;
        };
        
        FlexibleJointDefinition() 
        {
            mDefinitions.DontUseMemCpy();
        }
        
        TArray<FlexibleJointDefinitionItem> mDefinitions;
        TArray<ConditionParams> mParameters;
        
        protected:
            virtual ~FlexibleJointDefinition() {}
    };
    
    //
    //
    //
    
    class FlexJointModifier
        : public poser::PoseDriver
    {
    public:
        FlexJointModifier(poser::Pose* inPose, FlexibleJointDefinition* def, int item);
        
        virtual void Reset() { ResetParticleSystem(); }
        virtual void SetIsEnabled(bool isEnabled) 
        { 
            if (isEnabled) Reset(); 
            PoseDriver::SetIsEnabled(isEnabled); 
        }

        void SetMaxdef(float inMaxdef) { mMaxdef = inMaxdef; }
        void SetNumSubIteration(int inNumSubIteration) { mNumSubIteration = inNumSubIteration; }
        void SetSpeedDampingFactor(float inSpeedDampingFactor) { mSpeedDampingFactor = inSpeedDampingFactor; }        
        class Connection
        {
        public:
            short mFlexJointParticleIndex1;
            short mFlexJointParticleIndex2;
            float mLength;
            rmt::Vector mDPosition;
        };
        
        class FlexJointRestParticle
        {
        public:
            FlexJointRestParticle() {}
            ~FlexJointRestParticle() {}
            short mPoseIndex;
            short mParticleIndex;
            bool mVirtual;
        };
        
        class FlexJointParticle
        {
        public:
            FlexJointParticle() {}
            ~FlexJointParticle() {}
            
            short mPoseIndex;
            short mFlexParentIndex;
            short mParticleIndex;
            bool mFix;
            bool mVirtual;
            bool mNonVirtualEndOfBranch;

            rmt::Matrix mInitialMat;
            rmt::Matrix mFinalMat;
            TList<Connection*> mFlexibleJointConnection;
            FlexJointRestParticle* mRestParticle;
        };
        
        virtual int GetMinimumJointIndex() const { return int(mFirstPoseIndex); }
        virtual int GetPriority() const { return poser::FlexibleJointPriority; }
        virtual void Advance(float dt) { mAdvanceDt = dt; }
        virtual void Update(poser::Pose* pose);
        
        //void SetScale(float f);
        
        void ResetParticleSystem(rmt::Vector* newSpeed = NULL);
        void ComputeInternalSpeed(float dt, poser::Pose* inPose);
        
        void SetSimMethod(SimMethodEnum method) { mSimMethod = method; }
        void SetUpdateJointsMethod(UpdateJointsMethodEnum method) { mUpdateJointsMethod = method; }
        void SetRestMethod(RestPositionMethodEnum method) { mRestPositionMethod = method; }
        
        virtual void DebugDisplay();
        
        ParticleSystem*   GetParticleSystem()           { return mParticleSystem; }
        
        void SetSimStateArticulated(SimStateArticulated* inSimState); // need that to setup the plugs
        
        void SetBlendingFactor(float inBlendingFactor);
        float BlendingFactor()                    const { return mBlendingFactor; }
        
        bool ContainsJoint(int inIndex)           const { return mPoseToParticleIndex[inIndex] > 0 ? true : false; }
        short PoseToParticleIndex(int inIndex)    const { return mPoseToParticleIndex[inIndex]; }
        short LastPoseIndex()                     const { return mLastPoseIndex; }
        
        tUID GetUID()                             const { return mDefinitionItem->mName; }
        
        protected:
            virtual ~FlexJointModifier();
            
            void GetRelRotation(const int i, rmt::Matrix& m);
            void GetRelAxisAngle(const int i, rmt::Vector& axis, float& angle);
            void UpdateJoints();
            bool UpdateRestingPosition();
            void CacheCurrentPose();
            
            void TestExtension1D();
            
            void ComputeM(const rmt::Vector& p1, const rmt::Vector& p2, const rmt::Vector& p3, rmt::Matrix& m);
            
            rmt::Vector& GetPosePosition(int flexParticleIndex, rmt::Vector& pos);
            rmt::Vector& GetBonePose(int flexIndex, rmt::Vector& bone);
            rmt::Matrix& GetPoseTransform(int flexParticleIndex, rmt::Matrix& pos);
            void SetPoseTransform(int flexParticleIndex, const rmt::Matrix& inTransform);
            void SetPoseTranslation(int flexParticleIndex, const rmt::Vector& inTranslation);
            
        private:
            ParticleSystem*            mParticleSystem;
            poser::Pose*               mPose;
            float                      mAdvanceDt;
            
            short                      mNumFlexJointParticle;
            short                      mNumFlexJointRestParticle;
            short                      mNumFixParticle;
            FlexJointParticle*         mFlexJointParticle;
            FlexJointRestParticle*     mFlexJointRestParticle;
            short*                     mPoseToParticleIndex;
            TArray<Connection>         mFlexibleJointConnection;
            
            short                      mRootIndex;
            rmt::Vector*               mPreviousFilteredSpeed;
            rmt::Vector*               mRootFilteredSpeed;
            rmt::Vector*               mPreviousRootSpeed;
            rmt::Vector*               mNewRootSpeed;
            
            short                      mFirstPoseIndex;
            short                      mLastPoseIndex;
            
            
            ConditionParams            mParameters;
            ConditionParams            mRestingPosParameters;
            ConditionParams            mConnectionParameters;
            
            float                      mAveLen;
            float                      mSystemScale;
            rmt::Vector*               mTeleportVelocity;   // Used when filtering speed

            RestPositionMethodEnum     mRestPositionMethod;
            UpdateJointsMethodEnum     mUpdateJointsMethod;
            SimMethodEnum              mSimMethod;
            bool                       mUseRestingPos;
            bool                       mUseGravity;
            bool                       mUseVirtualJoint;
            short                      mSolver;
            float                      mExternalDensityFactor;
            
            SimStateArticulated*       mSimState;
            FlexibleJointDefinition*   mDefinition;
            FlexibleJointDefinition::FlexibleJointDefinitionItem* mDefinitionItem;
            
            float                      mBlendingFactor;
            float                      mMaxdef;
            float                      mSpeedDampingFactor;
            int                        mNumSubIteration;
            
            rmt::Vector                mRootSpeed;
            static float               sFilterRootSpeedValue;
            static bool                sUseRootFilteredSpeed;
};

//
//
//

class FlexibleJointDefinitionLoader
: public tSimpleChunkHandler
{
public:
    FlexibleJointDefinitionLoader();
    ~FlexibleJointDefinitionLoader() {}
    
    virtual tEntity* LoadObject(tChunkFile* chunk, tEntityStore* store);
    
protected:
};

} // sim

#endif // _SimFlexJoint_HPP_
