
#ifndef _PARTICLESYSTXH_HPP_
#define _PARTICLESYSTXH_HPP_

#include "simflexible/particlesyst.hpp"
#include "simcommon/sbmatrix.hpp"

class pddiShader;

namespace sim
{
    
    class ParticleSystem3D 
        : public ParticleSystem 
    {
        
    public:
        ParticleSystem3D(int a, int b, int c, float l);
        ~ParticleSystem3D();
        
        virtual void Scale(float f);
        virtual void BuildConditions();
        
        virtual void Display();
        virtual void PreDisplay();
        virtual void GetExtension(float& u, float& v);
        //virtual void PostStepModification(int i) {}
        virtual void PreStepModification();
        
        virtual void StartIntermediateParticle() { mInteri = mInterj = mInterk = 0; ParticleSystem::StartIntermediateParticle(); }
        virtual bool GetIntermediateParticle(float pos[]);
        virtual bool NeighborParticle(int i, int j) const { return (rmt::Abs(i-j) == 1 || rmt::Abs(i-j) == mNbj); }
        
        virtual int GetNbI()const {return mNbi;} 
        virtual int GetNbJ()const {return mNbj;}  
        virtual int GetNbK()const {return mNbk;}  
        
        virtual float GetLen0I()const {return mDeli;} 
        virtual float GetLen0J()const {return mDelj;}  
        virtual float GetLen0K()const {return mDelk;}  
        
        inline FlexParticle* P(int i, int j, int k) { return mP[mMapIndex[i][j][k]]; }
        
    protected:
        ParticleSystem3D() {}
        
        int*** mMapIndex;
        float  mDiagonalLength;
        int    mInteri, mInterj, mInterk;
        int    mNbi, mNbj, mNbk;
        float  mDelj, mDelk;
        
    };
    
    class ParticleSystem2D 
        : public ParticleSystem 
    {
        
    public:
        ParticleSystem2D(int r, int c, float l);
        ~ParticleSystem2D();
        
        virtual void Scale(float f);
        virtual void BuildConditions();
        
        void SetShader(pddiShader* mShader);
        
        virtual void Display();
        virtual void PreDisplay();
        virtual void GetExtension(float& u, float& v);
        virtual void PreStepModification();
        
        virtual void StartIntermediateParticle() { mInteri = mInterj = 0; ParticleSystem::StartIntermediateParticle(); }
        virtual bool GetIntermediateParticle(float pos[]);
        virtual bool NeighborParticle(int i, int j) const { return (rmt::Abs(i-j) == 1 || rmt::Abs(i-j) == mNbj); }
        
        virtual int GetNbI()const {return mNbi;} 
        virtual int GetNbJ()const {return mNbj;}  
        virtual int GetNbK()const {return 0;}  
        
        virtual float GetLen0I()const {return mDeli;} 
        virtual float GetLen0J()const {return mDelj;}  
        virtual float GetLen0K()const {return 0;}  
        
        inline FlexParticle* P(int i, int j) { return mP[mMapIndex[i][j]]; }
        
    protected:
        
        ParticleSystem2D() {}
        
        pddiShader     *mShader;
        float          *mTriNormals;
        int             mNbTri;
        unsigned short *mTriIndex;
        
        int**           mMapIndex;
        int             mInteri, mInterj;
        int             mNbi, mNbj;
        float           mDelj;
    };
    
    class ParticleSystem1D 
        : public ParticleSystem 
    {
    public:
        ParticleSystem1D(int n, float l);
        ~ParticleSystem1D();
        
        virtual void BuildConditions();
        
        void Display();
        virtual void GetExtension(float& u, float& v);
        //virtual void PostStepModification(int i);
        virtual void PreStepModification();
        virtual void BreakAt(int i);
        
        virtual void StartIntermediateParticle() { mInteri = 0; }
        virtual bool GetIntermediateParticle(float pos[]);
        virtual bool NeighborParticle(int i, int j) const { return rmt::Abs(i-j) == 1; }
        
        virtual int GetNbI()const {return mNbp;} 
        virtual int GetNbJ()const {return 0;}  
        virtual int GetNbK()const {return 0;}  
        
        virtual float GetLen0I()const {return mDeli;} 
        virtual float GetLen0J()const {return 0;}  
        virtual float GetLen0K()const {return 0;}  
        
    protected:
        ParticleSystem1D() {}
        
        int mInteri;
    };
    
} // sim

#endif // _PARTICLESYSTXH_HPP_
