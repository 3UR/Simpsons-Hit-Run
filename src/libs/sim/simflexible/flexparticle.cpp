
#include "simflexible/flexparticle.hpp"
#include "simcommon/simmath.hpp"
#include "simcommon/arraymath.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

FlexParticle::FlexParticle(int i) 
    : mNbCollConstraint(0),
    mIndex(i),
    mFree(true),
    mBroken(false)
{ 
    mCollConstraint[0].Clear();
    mCollConstraint[1].Clear();
    mCollConstraint[2].Clear();
    ZeroVector(m_v);
}

FlexParticle::~FlexParticle()
{
}

void FlexParticle::Render()
{
    rAssert(0);  // DRAW PARTICLE POSITION WITH TRIS ETC.
}

/*
void FlexParticle::AddDisplacment(const float d[])
{
if (!bdxc)
{
bdxc = true;
EqualVector(d, dxc);
}
else
{
AddVector(d, dxc, dxc);
}
}
*/

void FlexParticle::MoveSelf(const float d[], float invTimeStep)
{
    //AddVector(m_x, d, m_x);
    
    float tmp[3] = {d[0], d[1], d[2]};
    CleanForConstraint((Vector*)tmp);
    AddVector(m_x, tmp, m_x);
    AddScaleVector(m_v, tmp, invTimeStep, m_v);
}

void FlexParticle::UnMoveSelf(const float d[], float invTimeStep)
{
    //SubVector(m_x, d, m_x);
    
    float tmp[3] = {-d[0], -d[1], -d[2]};
    CleanForConstraint((Vector*)tmp);
    AddVector(m_x, tmp, m_x);  
    AddScaleVector(m_v, tmp, invTimeStep, m_v);  
}

void FlexParticle::CleanForConstraint(Vector *io_d) const
{
    switch(mNbCollConstraint)
    {
    case 3: //0 dof
        {
            //Here we let as an exercice all the math done in the 1 dof case. But
            //it seems resonable to say that next code do the same thing.
            Vector filtered = *io_d;
            RemoveComponent((float*)&mCollConstraint[0], (float*)&filtered);
            RemoveComponent((float*)&mCollConstraint[1], (float*)&filtered);
            RemoveComponent((float*)&mCollConstraint[2], (float*)&filtered);
            float length = filtered.NormalizeSafe();
            
            if (filtered.DotProduct(mCollConstraint[0]) < -MICRO_EPS || filtered.DotProduct(mCollConstraint[1]) < -MICRO_EPS)
            {
                io_d->Clear();
            }
            else
            {
                filtered.Scale(length);
                *io_d = filtered;
            }
        }
        break;
    case 2: //1 dof
        {
            //When there is 2 vectors for filtering. We will filter along mCollConstraint[0]
            //then mCollConstraint[1]. But if we do so blendely, the second filtering may
            //introduce back a negative component along mCollConstraint[0].
            const Vector &dir1=mCollConstraint[0], &dir2=mCollConstraint[1];
            float dotAN1, dotAN2;
            dotAN1 = io_d->DotProduct(dir1);
            dotAN2 = io_d->DotProduct(dir2);
            if (dotAN1 >= 0.0f && dotAN2 >= 0.0f)
            {
                return;//Nothing to do
            }
            bool filter=false;
            float dotN1N2 = dir1.DotProduct(dir2);
            if (dotAN1 >= 0.0f && dotAN2 < 0.0f)
            {
                if (dotN1N2 >= 0.0f || dotAN1 >= dotAN2*dotN1N2) 
                {
                    filter = true;
                }
            }
            else if (dotAN1 < 0.0f && dotAN2 >= 0.0f)
            {
                if (dotN1N2 >= 0.0f) 
                {
                    filter = true;
                }
            }
            else if (dotAN1 < 0.0f && dotAN2 < 0.0f)
            {
                if (dotN1N2 >= 0.0f && dotAN2<dotAN1*dotN1N2) 
                {
                    filter = true;
                }
            }
            
            if (filter)
            {
                if (dotAN1<0.0f)
                {
                    io_d->ScaleAdd(-dotAN1, mCollConstraint[0]);
                }
                if (dotAN2<0.0f)
                {
                    io_d->ScaleAdd(-dotAN2, mCollConstraint[1]);
                }
                //Cross check if the second filter have introduce back some
                //negative component along  mCollConstraint[0].
                //rAssert(io_d->DotProduct( mCollConstraint[0] )/io_d->Magnitude()>=-MILLI_EPS);
            }
            else
            {
                //Can't filter along any given dir, can move in dir3.
                Vector dir3;
                dir3.CrossProduct(dir1,dir2);
                dir3.NormalizeSafe();
                io_d->Scale(io_d->DotProduct(dir3), dir3);
            }
        }
        break;
    case 1: // 2 dof
        RemoveComponent((float*)&mCollConstraint[0], (float*)io_d);
        break;
    }
}

void FlexParticle::ResetDirectionOfConstraint( )
{
    if (mNbCollConstraint)
    {
        mCollConstraint[0].Clear();
        if (mNbCollConstraint==2)
        {
            mCollConstraint[1].Clear();
        }
        if (mNbCollConstraint>2)
        {
            mCollConstraint[2].Clear();
        }
        mNbCollConstraint = 0;
    }
}

//Add a new direction of constraint, return true if effectively added.
//inDir is a candidate for a direction of constraint. This method guaranty that the set
//of constraining directions is disjoint and that all direction are unit vectors.
bool FlexParticle::AddDirectionOfConstraint( const rmt::Vector &inDir)
{
    //rAssert( Fabs( 1.0f - inDir.DotProduct(inDir) ) < MILLI_EPS );//The filtering mechanism assume that direction of constraint are unit vector.
    
    if (mNbCollConstraint == 0)
    {
        //First candidate for a direction of constraint. Store it.
        mCollConstraint[0] = inDir;
        mNbCollConstraint++;
        return true;
    }
    else if(mNbCollConstraint == 1)
    {
        float dot = inDir.DotProduct(mCollConstraint[0]);
        if ( 1.0f - Min( Fabs(dot), 1.0f ) > MILLI_EPS )
        {
            //Second candidate for a direction of constraint is not co-linear to the first one. 
            //Add it as the second direction of constraint.
            mCollConstraint[1] = inDir;
            mNbCollConstraint++;
            return true;
        }
    }
    else if(mNbCollConstraint == 2)
    {
        float dot = inDir.DotProduct(mCollConstraint[0]);
        if ( 1.0f - Min( Fabs(dot), 1.0f ) > MILLI_EPS )
        { 
            dot = inDir.DotProduct(mCollConstraint[1]);
            if ( 1.0f - Min( Fabs(dot), 1.0f ) > MILLI_EPS )
            {
                //Here comes an other candidate for a direction of constraint 
                //not co-linear to the two first.
                //Does the new candidate in the same plane as the two first? If yes, reject it,
                Vector axe;
                axe.CrossProduct(mCollConstraint[0],mCollConstraint[1]);
                dot = axe.DotProduct(inDir)/axe.Magnitude(); //axe is not zero as cns[0] and cns[1] are not parallel
                if (Fabs(dot) > MILLI_EPS )
                {
                    mCollConstraint[2] = inDir;
                    mNbCollConstraint++;
                    return true;
                }
            }
        }
    }
    return false;
}

} // sim
