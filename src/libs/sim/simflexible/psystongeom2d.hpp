
#ifndef _PARTICLESYSTEMONGEOM2D_hpp_
#define _PARTICLESYSTEMONGEOM2D_hpp_

#include <p3d/loadmanager.hpp>
#include "simflexible/particlesyst.hpp"

class tGeometry;

namespace sim
{
    
    class ParticleSystemOnGeom2D
        : public ParticleSystem
    {
    public:
        ParticleSystemOnGeom2D(tGeometry* geom, unsigned nparticle);
        virtual ~ParticleSystemOnGeom2D();
        
        virtual void Scale(float f);
        void BuildConditions();
        virtual void PreStepModification();
        virtual void PreDisplay();
        void StreamParticle(bool front);
        virtual void Display();
        
        // obsolete
        virtual void StartIntermediateParticle() { }
        virtual bool GetIntermediateParticle(float pos[]) { return false; }
        virtual bool NeighborParticle(int i, int j) const { return false; }
        
        void SetVertexMapList(unsigned nv, unsigned* map, rmt::Vector* pos);
        void SetTriangleList(unsigned n, unsigned* map);
        void SetEdgeList(unsigned n, unsigned* map, float* len);
        void DisplayBackFace(bool b) { mDisplayBackFace = b; }
        
        void ResetPos();
        
        virtual int GetNbI()const {return mNbp/2;} 
        virtual int GetNbJ()const {return mNbp/2 + mNbp - (mNbp/2 + mNbp/2);}  
        virtual int GetNbK()const {return 0;}  
        
        virtual float GetLen0I()const 
        {
            float length=0;
            for(unsigned i=0 ; i<mNbEdge ;i++ )
                length += mLenEdge[i];
            return length/mNbEdge;
        } 
        virtual float GetLen0J()const {return GetLen0I();}  
        virtual float GetLen0K()const {return 0.0f;}  
        
        
    protected:
        
        friend class PSystGeomLoader;
        
        bool Find2AdjascentTris(unsigned* edge, unsigned* tri, unsigned* extrap) const;
        bool EdgeOfTriangle(unsigned* edge, unsigned* t, unsigned& extrap) const;
        void ScaleEdge( float val )
        {
            for(unsigned i=0 ; i<mNbEdge ;i++ )
                mLenEdge[i] *= val;
            return;
        }
        
        void ComputeParticleNormal();
        
        tGeometry  *tgeom;
        
        unsigned    mNbVertex;
        unsigned    mNbPGM;
        unsigned    mNbTri;
        unsigned    mNbEdge;
        
        unsigned    *mMapVL;
        unsigned    *mPgmNbVertex;
        unsigned    *mMapTri;
        unsigned    *mMapEdge;
        float       *mLenEdge;
        float       *mTriNormal;
        float       *mNormal;
        
        rmt::Vector *mParticlePos;
        
        bool         mDisplayBackFace;
        bool         mFrontNormal;
    };
    
    
    class PSystGeomLoader : public tSimpleChunkHandler
    {
    public:
        PSystGeomLoader();
        
        virtual tEntity* LoadObject(tChunkFile* chunk, tEntityStore* store);
    };
    
} // sim

#endif // ParticleSystemOnGeom2D_hpp_
