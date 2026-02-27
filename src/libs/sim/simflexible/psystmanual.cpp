
#include "simflexible/psystmanual.hpp"
#include "simflexible/condition1d.hpp"
#include "simflexible/condition2d.hpp"
#include "simcommon/simenvironment.hpp"
#include "simcommon/dline2.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

ParticleSystemManual::ParticleSystemManual(int nbparticle, Vector* ppos):
    mMasterSubPartSet(false)
{
    rAssert(nbparticle > 1); // otherwise it's not a system!!!
    
    mMaxDef = 0.1f;
    mNbp = nbparticle;
    Init();
    
    for (int i=0; i<mNbp; i++)
        SetVector(mP[i]->m_x, ppos[i].x, ppos[i].y, ppos[i].z);
}

ParticleSystemManual::~ParticleSystemManual()
{
}

void ParticleSystemManual::AddLine(int pa, int pb, ConditionParams* c)
{
    rAssert(pa < mNbp && pb < mNbp);
    float dp[3], len;
    
    SubVector(mP[pa]->m_x, mP[pb]->m_x, dp);
    len = LenVector(dp);
    ConditionParams* cp = c==NULL ? mCparams : c;
    
    switch (mPSystSolver->GetSolverType())
    {
    case eLargeStepSolver:
        mConditions->Insert( new Stretch1DCondition(mP[pa], mP[pb], cp, len));
        mConditions->Insert( new Wind1DCondition(mP[pa], mP[pb], cp, &mWind));
        break;
    case eSemiImplicitSolver1D:
    case eSemiImplicitSolver2D:
        mConditions->Insert( new Stretch1DCondition(mP[pa], mP[pb], cp, len, false));
        mConditions->Insert( new Wind1DCondition(mP[pa], mP[pb], cp, &mWind));
        break;
    default:
        rAssertMsg(0,"Invalid solver or solver not implemented");
    }
    if (cp->GetParamSet() == false)
    {
        AddSubPart(len, cp);
    }
}

void ParticleSystemManual::AddTriangle(int pa, int pb, int pc, ConditionParams* c)
{
    rAssert(pa < mNbp && pb < mNbp && pc < mNbp);
    
    FlexParticle* p0 = mP[pa];
    FlexParticle* p1 = mP[pb];
    FlexParticle* p2 = mP[pc];
    ConditionParams* cp = c==NULL ? mCparams : c;
    
    float area = TriangleArea(p0->m_x, p1->m_x, p2->m_x);
    switch (mPSystSolver->GetSolverType())
    {
    case eLargeStepSolver:
        mConditions->Insert( new Shear2DCondition(p0, p1, p2, cp));
        mConditions->Insert( new Wind2DCondition(p0, p1, p2, cp, &mWind, area));
        break;
    default:
        rAssertMsg(0,"Invalid solver or solver not implemented");
    }
    if (cp->GetParamSet() == false)
    {
        AddSubPart( Sqrt(area), cp);
    }
}

void ParticleSystemManual::AddBending1D(int pa, int pb, int pc, bool calcRestAngle, ConditionParams* c)
{
    rAssert(pa < mNbp && pb < mNbp && pc < mNbp);
    ConditionParams* cp = c==NULL ? mCparams : c;
    switch (mPSystSolver->GetSolverType())
    {
    case eLargeStepSolver:
        {
            mConditions->Insert( new Bend1DCondition(mP[pa], mP[pb], mP[pc], cp, calcRestAngle) );
        }
        break;
    case eSemiImplicitSolver1D:
    case eSemiImplicitSolver2D:
        {
            mConditions->Insert( new Bend1DCondition(mP[pa], mP[pb], mP[pc], cp, calcRestAngle, false) );
        }
        break;
    default:
        rAssertMsg(0,"Invalid solver or solver not implemented");
    }

    if (cp->GetParamSet() == false)
    {
        float dp[3], len;
    
        SubVector(mP[pa]->m_x, mP[pb]->m_x, dp);
        len = LenVector(dp);
        SubVector(mP[pb]->m_x, mP[pc]->m_x, dp);
        len += LenVector(dp);

        AddSubPart( len/2, cp);
    }

}


void ParticleSystemManual::AddBending2D(int pa, int pb, int pc, int pd, ConditionParams* c)
{
    rAssert(pa < mNbp && pb < mNbp && pc < mNbp && pd < mNbp);
    ConditionParams* cp = c==NULL ? mCparams : c;
    switch (mPSystSolver->GetSolverType())
    {
    case eLargeStepSolver:
        {
            mConditions->Insert( new Bend2DCondition(mP[pa], mP[pb], mP[pc], mP[pd], cp) );
        }
        break;
    default:
        rAssertMsg(0,"Invalid solver or solver not implemented");
    }
    if (cp->GetParamSet() == false)
    {
        float len;
        len  = Sqrt( TriangleArea(mP[pa]->m_x, mP[pb]->m_x, mP[pc]->m_x) );
        len += Sqrt( TriangleArea(mP[pb]->m_x, mP[pc]->m_x, mP[pd]->m_x) );
        AddSubPart( len/2, cp);
    }

}

void ParticleSystemManual::AddSubPart( float inLength, ConditionParams *inCparam )
{
    int i = FindSubPart(inCparam);

    if (i==-1) 
    {   //add a new subpart
        PsystSubPart &part = mSubPartList.NewElem();
        part.mCparam  = inCparam;
        part.mLength  = inLength;
        part.mNbPart  = 1;
    }
    else
    {
        mSubPartList.GetAt(i).mLength = (mSubPartList.GetAt(i).mLength + inLength)/2;
        mSubPartList.GetAt(i).mNbPart += 1;
    }
}

void ParticleSystemManual::SetMasterSubPart( ConditionParams *inMaster)
{

    int i = FindSubPart(inMaster);
    if ( i<0 )//Can't set this subpart as the master if not already in the sub-part list.
    {
        mMasterSubPartSet = false;
        return;
    }

    mMasterSubPartSet = true;
    if (i==0)
        return;

    //Make sure that the master part is the firt element of mSubPartList.
    PsystSubPart tmp;
    tmp = mSubPartList[0];
    mSubPartList[0] = mSubPartList[i];
    mSubPartList[i] = tmp;
}

bool  ParticleSystemManual::SetLengthEstimate( float inLength, int inNbPart, ConditionParams * inCparam )
{
 
    int i = FindSubPart(inCparam);

    if (i==-1) 
    {
        return false;
    }
    else
    {
        mSubPartList.GetAt(i).mLength = inLength;
        mSubPartList.GetAt(i).mNbPart = inNbPart;
        return true;
    }
}
/*
bool  ParticleSystemManual::SetLambda( float inStretchFact, float inDampingFact, int index, bool inUseLongSide)
{
    rAssert(index>-1 && index<mSubPartList.GetSize());
    PsystSubPart &part = mSubPartList[index];
    part.mCparam->LambdaF = inStretchFact; 
    part.mCparam->LambdaD = inDampingFact;
    part.mCparam->SetUseLongSide(inUseLongSide);
    part.mCparam->SetParamSet(false);
    return UpdateCparamsForLambda();
}

bool  ParticleSystemManual::SetKappa ( float inCurvatureFact, float inDampingFact, int index, bool inUseLongSide)
{
    rAssert(index>-1 && index<mSubPartList.GetSize());
    PsystSubPart &part = mSubPartList[index];
    part.mCparam->KappaF = inCurvatureFact; 
    part.mCparam->KappaD = inDampingFact;
    part.mCparam->SetUseLongSide(inUseLongSide);
    part.mCparam->SetParamSet(false);
    return UpdateCparamsForKappa ();
}

bool  ParticleSystemManual::SetIota  ( float inFact, float inDampingFact, int index, bool inUseLongSide)
{
    rAssert(index>-1 && index<mSubPartList.GetSize());
    PsystSubPart &part = mSubPartList[index];
    part.mCparam->IotaF = inFact; 
    part.mCparam->IotaD = inDampingFact;
    part.mCparam->SetUseLongSide(inUseLongSide);
    part.mCparam->SetParamSet(false);
    return UpdateCparamsForIota  ();
}
*/
bool  ParticleSystemManual::UpdateCparamsForLambda()
{
    int i;
    float mass, gravity;
    mass = mMass[0];
    if ( GetSimEnvironment()==NULL )
        return false;

    gravity = GetSimEnvironment()->GravityNorm();

    float   f, d;
    bool    result = true;
    for ( i=0 ; i<mSubPartList.GetSize() ; i++ )
    {
        PsystSubPart &part = mSubPartList[i];
        if (mMasterSubPartSet == false || i == 0)
        {
            result = ConditionParams::ComputeStretch1DFromLambda(part.mCparam->LambdaF, part.mCparam->LambdaD,
                                          part.mNbPart, part.mLength, gravity, f, d, mass);
            if (result)
            {
                part.mCparam->Stretch1Dkf = f; 
                part.mCparam->Stretch1Dkd = d;
                part.mCparam->Stretch2Dkf = f;
                part.mCparam->Stretch2Dkd = d;
                mPSystSolver->SetNeedPrecomputation(true);            
            }
            else
            {
                return false;
            }
        }
        else
        {
            PsystSubPart &master = mSubPartList[0];
            part.mCparam->Stretch1Dkf = part.mCparam->LambdaF * master.mCparam->Stretch1Dkf; 
            part.mCparam->Stretch1Dkd = 2 * part.mCparam->LambdaD * Sqrt(mass*part.mCparam->Stretch1Dkf);
            part.mCparam->Stretch2Dkf = part.mCparam->LambdaF * master.mCparam->Stretch2Dkf;
            part.mCparam->Stretch2Dkd = 2 * part.mCparam->LambdaD * Sqrt(mass*part.mCparam->Stretch2Dkf);
            mPSystSolver->SetNeedPrecomputation(true);            
        }
    }
    return result;
}

bool  ParticleSystemManual::UpdateCparamsForKappa ()
{
    int i;
    float mass, gravity;
    mass = mMass[0];
    if ( GetSimEnvironment()==NULL )
        return false;

    gravity = GetSimEnvironment()->GravityNorm();

    float   f, d;
    bool    result = false;
    for (i=0 ; i<mSubPartList.GetSize() ; i++ )
    {
        PsystSubPart &part = mSubPartList[i];
        if (mMasterSubPartSet == false || i == 0)
        {
            result = ConditionParams::ComputeBend1DFromKappa(part.mCparam->KappaF, part.mCparam->KappaD, 
                                      part.mNbPart, part.mLength, gravity, f, d, mass);
    
            if (result)
            {
                part.mCparam->Bend1Dkf = f;
                part.mCparam->Bend1Dkd = d;
                part.mCparam->Bend2Dkf = f;
                part.mCparam->Bend2Dkd = d;
                mPSystSolver->SetNeedPrecomputation(true);
            }
            else
            {
                return false;
            }
        }
        else
        {
            float len = part.mLength;
            int   nb  = Max(2,part.mNbPart);
            if (len==0.0f)
            {
                //No bending forces with 0 rest lenght.
                continue;
            }
            PsystSubPart &master = mSubPartList[0];
            part.mCparam->Bend1Dkf = part.mCparam->KappaF * master.mCparam->Bend1Dkf; 
            part.mCparam->Bend1Dkd = 2 * part.mCparam->KappaD * Sqrt(mass*part.mCparam->Bend1Dkf/len) / (nb-1);
            part.mCparam->Bend2Dkf = part.mCparam->KappaF * master.mCparam->Bend2Dkf;
            part.mCparam->Bend2Dkd = 2 * part.mCparam->KappaD * Sqrt(mass*part.mCparam->Bend2Dkf/len) / (nb-1);
            mPSystSolver->SetNeedPrecomputation(true);            
        }
    }

    return result;

}

bool  ParticleSystemManual::UpdateCparamsForIota  ()
{
    int i;
    float mass, gravity;
    mass = mMass[0];
    if ( GetSimEnvironment()==NULL )
        return false;

    gravity = GetSimEnvironment()->GravityNorm();

    float   f, d;
    bool    result = false;
    for (i=0 ; i<mSubPartList.GetSize() ; i++ )
    {
        PsystSubPart &part = mSubPartList[i];
        if (mMasterSubPartSet == false || i == 0 )
        {
            result = ConditionParams::ComputeStretch1DFromLambda( part.mCparam->IotaF, part.mCparam->IotaD, 
                                      part.mNbPart, part.mLength, gravity, f, d, mass);
    
            if (result)
            {
                part.mCparam->Shear2Dkf = f;
                part.mCparam->Shear2Dkd = d;
                mPSystSolver->SetNeedPrecomputation(true);
            }
            else
            {
                return false;
            }
        }
        else
        {
            PsystSubPart &master = mSubPartList[0];
            part.mCparam->Shear2Dkf = part.mCparam->IotaF * master.mCparam->Shear2Dkf;
            part.mCparam->Shear2Dkd = 2 * part.mCparam->IotaD * Sqrt(mass*part.mCparam->Shear2Dkf);
            mPSystSolver->SetNeedPrecomputation(true);            
        }
    }
    return result;
}

void ParticleSystemManual::Display()
{
    /*
    tColour c1(0, 50, 200), c2(0, 200, 50), c3(100, 100, 100);
    
    static float dely = 0.2f;
    float d[3], e[3];
    Condition *C=mConditions->next(); // Skip the fencepost NullCondition
    while (C) 
    {
        Bend1DCondition* b1d = dynamic_cast<Bend1DCondition*>(C);
        if (b1d)
        {
            // draw a little line in the middle of the edge
            SetVector(e, b1d->mPj->m_x[0], b1d->mPj->m_x[1]+dely, b1d->mPj->m_x[2]);
            DrawLine(b1d->mPj->m_x, e, c2);
        }
        Bend2DCondition* b2d = dynamic_cast<Bend2DCondition*>(C);
        if (b2d)
        {
            // draw a line between the two non shared vertices of two suposedly adjascent triangles
            DrawLine(b2d->mPi->m_x, b2d->pl->m_x, c1, c2);
        }
        Shear2DCondition* s2d = dynamic_cast<Shear2DCondition*>(C);
        if (s2d)
        {
            // draw a little line in the middle of the triangle
            AddVector(s2d->mPi->m_x, s2d->mPj->m_x, d);
            AddVector(s2d->pk->m_x, d, d);
            ScaleVector(d, 0.33333f, d);
            SetVector(e, d[0], d[1]+dely, d[2]);
            DrawLine(d, e, c1);
        }
        Stretch1DCondition* st1d = dynamic_cast<Stretch1DCondition*>(C);
        if (st1d)
        {
            // draw a little line in the middle of the edge
            AddVector(st1d->mPi->m_x, st1d->mPj->m_x, d);
            ScaleVector(d, 0.5f, d);
            SetVector(e, d[0], d[1]+dely, d[2]);
            DrawLine(d, e, c2);
            
            DrawLine(st1d->mPi->m_x, st1d->mPj->m_x, c2);
        }
        C = C->next();
    }
    */
}

} // sim
