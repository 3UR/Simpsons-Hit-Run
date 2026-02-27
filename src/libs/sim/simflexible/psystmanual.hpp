
#ifndef _PARTICLESYSTEMMANUAL_hpp_
#define _PARTICLESYSTEMMANUAL_hpp_

#include "simflexible/particlesyst.hpp"

namespace sim
{
    
/*
This class allow t o create mixte ParticleSystem. The particle's constraints 
are dictated by the users. No checking done on the constraints structures. 
Use at your own risk!!!
Unconstrained particles are simply going to fall as a free particle
*/

class ParticleSystemManual
    : public ParticleSystem
{
protected:
    class PsystSubPart
    {
        public:
            PsystSubPart() : mNbPart(0),mCparam(NULL),mLength(0) {}
        
            int             mNbPart;
            ConditionParams *mCparam;
            float           mLength;

            PsystSubPart & operator = ( const PsystSubPart &inSubPart )
            {
                mNbPart     = inSubPart.mNbPart;
                mCparam     = inSubPart.mCparam;
                mLength     = inSubPart.mLength;
                return *this;
            }

            bool operator == ( const PsystSubPart &inSubPart )
            {
                if (mCparam == inSubPart.mCparam)
                    return true;
                else
                    return false;
            }

    } ;

public:
    ParticleSystemManual(int nbparticle, rmt::Vector* ppos);
    virtual ~ParticleSystemManual();
    
    void AddLine(int pa, int pb, ConditionParams* c = NULL);                      // a line, not a triangle's edge
    void AddTriangle(int pa, int pb, int pc, ConditionParams* c = NULL);          // a triangle
    void AddBending1D(int pa, int pb, int pc, bool calcRestAngle = false, ConditionParams* c = NULL);         // on two consecutive lines
    void AddBending2D(int pa, int pb, int pc, int pd, ConditionParams* c = NULL); // on two adjascent triangles, pb and pc being the shared edge
    
    virtual bool  SetLengthEstimate( float inLength, int inNbPart, ConditionParams * inCparam=NULL );
    /*
    virtual bool  SetLambda( float inStretchFact, float inDampingFact, int index=-1, bool inUseLongSide=true );
    virtual bool  SetKappa ( float inCurvatureFact, float inDampingFact, int index=-1, bool inUseLongSide=true );
    virtual bool  SetIota  ( float inFact, float inDampingFact, int index=-1, bool inUseLongSide=true );
    */
    virtual bool  UpdateCparamsForLambda();
    virtual bool  UpdateCparamsForKappa ();
    virtual bool  UpdateCparamsForIota  ();

    virtual void  SetMasterSubPart( ConditionParams *inMaster);
    inline int FindSubPart( ConditionParams *inSubPart)
    {
        PsystSubPart part;
        part.mCparam   = inSubPart;
        return mSubPartList.Find(part);
    }

    void Display();  // allows to view the structure of the constrains

protected:    
    void AddSubPart( float inLength, ConditionParams *inCparam );

    bool mMasterSubPartSet;
    TArray<PsystSubPart> mSubPartList;
};
    
} // sim

#endif // _ParticleSystemManual_HPP_
