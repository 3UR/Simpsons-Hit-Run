
#include "simflexible/psystongeom2d.hpp"
#include "simflexible/condition1d.hpp"
#include "simflexible/condition2d.hpp"
#include "simflexible/flexibleobject.hpp"
#include "simcollision/collisionobject.hpp"

#include "p3d/geometry.hpp"
#include "p3d/primgroup.hpp"
#include "p3d/utility.hpp"

// for the loader
#include <p3d/chunkfile.hpp>
#include <constants/chunkids.hpp>

using namespace RadicalMathLibrary;

namespace sim
{

ParticleSystemOnGeom2D::ParticleSystemOnGeom2D(tGeometry* geom, unsigned nparticle)
: ParticleSystem(),
  tgeom(geom),
  mNbVertex(0),
  mNbPGM(0),
  mNbTri(0),
  mNbEdge(0),
  mMapVL(NULL),
  mPgmNbVertex(NULL),
  mMapTri(NULL),
  mMapEdge(NULL),
  mLenEdge(NULL),
  mTriNormal(NULL),
  mNormal(NULL),
  mParticlePos(NULL),
  mDisplayBackFace(true),
  mFrontNormal(true)
{
    rAssert(tgeom != NULL);
    
    for(int j=0; j < tgeom->GetNumPrimGroup(); j++)
    {
        tPrimGroupOptimised* pg = dynamic_cast<tPrimGroupOptimised*>(tgeom->GetPrimGroup(j));
        rAssertMsg(pg != NULL, "primgroup must be optimised?!? ParticleSystemOnGeom2D");
    }
    
    mMaxDef = 0.1f;
    mNbp = nparticle;
    Init();
}

ParticleSystemOnGeom2D::~ParticleSystemOnGeom2D()
{
    delete[] mMapVL;
    delete[] mPgmNbVertex;
    delete[] mMapTri;
    delete[] mMapEdge;
    delete[] mLenEdge;
    delete[] mTriNormal;
    delete[] mNormal;
    delete[] mParticlePos;
}

void ParticleSystemOnGeom2D::ResetPos()
{
    for(int i=0; i<mNbp; i++)
        SetVector(mP[i]->m_x, mParticlePos[i].x, mParticlePos[i].y, mParticlePos[i].z);
    
    if (mNormal)
    {
        ComputeParticleNormal();
        StreamParticle(true);
        this->SetSystemSpeed(Vector(0, 0, 0));
    }
}

void ParticleSystemOnGeom2D::SetVertexMapList(unsigned n, unsigned* map, Vector* pos)
{
    mNbVertex = n;
    mMapVL = map;
    
    mParticlePos = pos;
    ResetPos();
    
    int size = mNbp*3;
    
#ifdef USE_FAST_MATH4
    MakeMultipleOf4(size);
#endif
    
    mNormal = new float[size];
}

void ParticleSystemOnGeom2D::SetTriangleList(unsigned n, unsigned* map)
{
    mNbTri = n;
    mMapTri = map;
    mTriNormal = new float[mNbTri*3];
}

void ParticleSystemOnGeom2D::SetEdgeList(unsigned n, unsigned* map, float* len)
{
    mNbEdge = n;
    mMapEdge = map;
    mLenEdge = len;
    
    mDeli=0;
    for (unsigned i=0; i<mNbEdge; i++)
    {
        mDeli += mLenEdge[i];
    }
    mDeli /= mNbEdge;
    
    rAssert (mDeli != 0.0f);
    mPSystSolver->SetConvergenceCriteral( Max(Min(MAX_EPS_CG,mDeli/mNbEdge),MIN_EPS_CG), MAX_ITER_CG );
}

void ParticleSystemOnGeom2D::Scale(float f)
{
    ParticleSystem::Scale(f);
    
    for (unsigned i=0; i<mNbEdge; i++)
    {
        mLenEdge[i]*=f;
    }
}

void ParticleSystemOnGeom2D::BuildConditions()
{
    unsigned i;
    
    switch (mPSystSolver->GetSolverType())
    {
    case eLargeStepSolver:
        {
            for (i=0; i<mNbEdge ; i++) 
            {
                // assume that the edge is at the equilibrium already
                mConditions->Insert( new Stretch1DCondition(mP[mMapEdge[2*i]], mP[mMapEdge[2*i+1]], mCparams, mLenEdge[i]));
            }
            for (i=0; i<mNbTri; i++)
            {
                FlexParticle* p0 = mP[mMapTri[3*i]];
                FlexParticle* p1 = mP[mMapTri[3*i+1]];
                FlexParticle* p2 = mP[mMapTri[3*i+2]];
                float area = TriangleArea(p0->m_x, p1->m_x, p2->m_x);
                mConditions->Insert( new Shear2DCondition(p0, p1, p2, mCparams));
                mConditions->Insert( new Wind2DCondition(p0, p1, p2, mCparams, &mWind, area));
            }
            for (i=0; i<mNbEdge; i++)
            {
                unsigned tri[2], ep[2];
                // Must first find if this edges is associate to two triangles and so needs bending condition
                if (Find2AdjascentTris(&mMapEdge[2*i], tri, ep))
                {
                    mConditions->Insert( new Bend2DCondition(mP[ep[0]], mP[mMapEdge[2*i]], mP[mMapEdge[2*i+1]], mP[ep[1]], mCparams));
                }
            }
        }
        break;
    default:
        rAssertMsg(0,"Invalid solver or solver not implemented");
    }
    
}

bool ParticleSystemOnGeom2D::Find2AdjascentTris(unsigned* edge, unsigned* tri, unsigned* extrap) const
{
    bool found = false;
    unsigned i;
    for (i=0; i<mNbTri; i++)
    {
        if (EdgeOfTriangle(edge, &mMapTri[3*i], extrap[0]))
        {
            tri[0] = 3*i;
            break;
        }
    }
    for (i=i+1; i<mNbTri; i++)
    {
        if (EdgeOfTriangle(edge, &mMapTri[3*i], extrap[1]))
        {
            tri[1] = 3*i;
            found = true;
            break;
        }
    }
    return found;
}

bool ParticleSystemOnGeom2D::EdgeOfTriangle(unsigned* edge, unsigned* t, unsigned& extraPoint) const
{
    bool found = false;
    for (unsigned i=0; i<3; i++)
    {
        if (t[i] != edge[0] && t[i] != edge[1])
        {
            unsigned j = (i == 2 ? 0 : i+1);
            if (t[j] == edge[0] || t[j] == edge[1])
            {
                j = (j == 2 ? 0 : j+1);
                if (t[j] == edge[0] || t[j] == edge[1])
                {
                    extraPoint = t[i];
                    found = true;
                    break;
                }
            }
        }
    }
    return found;
}
int PreStepModification_iter = 0;

void ParticleSystemOnGeom2D::PreStepModification()
{
    // test extension of every segments
    static int maxiter = 20;
    int n = 0;
    bool moved = true;
    
    while (moved && n < maxiter)
    {
        moved = false;
        for (unsigned i=0; i<mNbEdge; i++)
        {
            if (TestExtension1D(mP[mMapEdge[2*i]], mP[mMapEdge[2*i+1]], mLenEdge[i], mMaxDef))
            {
                moved = true;
            }
        }
        n++;
    }
    PreStepModification_iter=n;
    
}

void ParticleSystemOnGeom2D::ComputeParticleNormal()
{
    ZeroVectorN(mNormal, 3*mNbp);
    
    unsigned i;
    for (i=0; i<mNbTri*3; i+=3)
    {
        ComputeTriNormal(mP[mMapTri[i]]->m_x, mP[mMapTri[i+1]]->m_x, mP[mMapTri[i+2]]->m_x, &mTriNormal[i]);
    }
    for (i=0; i<mNbTri*3; i+=3)
    {
        int ii = 3*mMapTri[i];
        AddVector(&mNormal[ii], &mTriNormal[i], &mNormal[ii]);
        ii = 3*mMapTri[i+1];
        AddVector(&mNormal[ii], &mTriNormal[i], &mNormal[ii]);
        ii = 3*mMapTri[i+2];
        AddVector(&mNormal[ii], &mTriNormal[i], &mNormal[ii]);
    }
    for (int j=0; j<mNbp; j++)
    {
        (void)NormalizeVector(&mNormal[3*j]);
    }
}

void ParticleSystemOnGeom2D::PreDisplay()
{
    ComputeParticleNormal();
    StreamParticle(true);
}

void ParticleSystemOnGeom2D::StreamParticle(bool front)
{
    mFrontNormal = front;
    
    for(unsigned j=0; j<mNbPGM; j++)
    {
        tPrimGroupOptimised* pg = (tPrimGroupOptimised*)tgeom->GetPrimGroup(j);
        pddiPrimBufferStream* stream = pg->GetBuffer()->Lock();
        
        int index = 0;
        
        if (front)
        {
            for(unsigned i = 0; i < mPgmNbVertex[j]; i++, index++)
            {
                // output transformed vertex
                float* p = mP[mMapVL[index]]->m_x;
                float* n = &mNormal[mMapVL[index]*3];
                stream->Vertex((pddiVector*)p, (pddiVector*)n);
            }
        }
        else
        {
            for(unsigned i = 0; i < mPgmNbVertex[j]; i++, index++)
            {
                // output transformed vertex
                float* p = mP[mMapVL[index]]->m_x;
                float n[3];
                ScaleVector(&mNormal[mMapVL[index]*3], -1.0f, n);
                stream->Vertex((pddiVector*)p, (pddiVector*)n);
            }
        }
        
        pg->GetBuffer()->Unlock(stream);
    }
}

void ParticleSystemOnGeom2D::Display()
{
    // once ParticleSystemOnGeom2D::PreDisplay has been called, 
    // tGeometry::Display can be called instead of ParticleSystemOnGeom2D::Display()
    // but this won't display back faces
    
    if (!mFrontNormal)
    {
        StreamParticle(true);
    }
    
    tgeom->Display();
    
    if (mDisplayBackFace)
    {
        pddiCullMode mode = p3d::pddi->GetCullMode();
        p3d::pddi->SetCullMode(mode == PDDI_CULL_NORMAL ? PDDI_CULL_INVERTED : PDDI_CULL_NORMAL);
        
        StreamParticle(false);
        tgeom->Display();
        
        p3d::pddi->SetCullMode(mode);
    }
    
    
    /*
    tVertexList* li = tgeom->GetVertexList();
    for (unsigned i=0; i<li->GetNumVertices(); i++)
    {
    static float scale = 2.0f;
    float d[3], e[3];
    SetVector(d, li->Position(i).x, li->Position(i).y, li->Position(i).z);
    SetVector(e, li->Normal(i).x, li->Normal(i).y, li->Normal(i).z);
    AddScaleVector(d, e, scale, e);
    DrawLine(d, e, tColour(200, 50, 0)); 
    }
    
      tColour c1(0, 50, 200), c2(200, 0, 50);
      Condition *C=mConditions->next(); // Skip the fencepost NullCondition
      while (C) 
      {
      Bend2DCondition* bc = dynamic_cast<Bend2DCondition*>(C);
      if (bc)
      {
      DrawLine(bc->mPi->m_x, bc->pl->m_x, tColour(0, 50, 200));           // stretch force in blue
      }
      Shear2DCondition* sc = dynamic_cast<Shear2DCondition*>(C);
      if (sc)
      {
      float d[3], e[3];
      AddVector(sc->mPi->m_x, sc->mPj->m_x, d);
      AddVector(sc->pk->m_x, d, d);
      ScaleVector(d, 0.33333f, d);
      SetVector(e, d[0], d[1]+2, d[2]);
      DrawLine(d, e, c1);
      }
      Stretch1DCondition* tc = dynamic_cast<Stretch1DCondition*>(C);
      if (tc)
      {
      float d[3], e[3];
      AddVector(tc->mPi->m_x, tc->mPj->m_x, d);
      ScaleVector(d, 0.5f, d);
      SetVector(e, d[0], d[1]+2, d[2]);
      DrawLine(d, e, c2);
      
        DrawLine(tc->mPi->m_x, tc->mPj->m_x, c2);
        }
        C = C->next();
        }
    */
}

//
//
//


static unsigned FLEXIBLEGEOM_VERSION = 2;

PSystGeomLoader::PSystGeomLoader() : tSimpleChunkHandler(Simulation::Flexible::OBJECT) {};

tEntity* PSystGeomLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char name[128];
    f->GetPString(name);
    
    tGeometry* geom = p3d::find<tGeometry>(store, name);
    rAssert(geom != NULL);
    
    FlexibleObject* ret = NULL;
    
    unsigned version = f->GetUShort();
    rAssert(version == FLEXIBLEGEOM_VERSION);
    
    unsigned nDim = f->GetUShort();
    float density = f->GetFloat();
    
    unsigned i;
    unsigned nParticle = f->GetUShort();
    Vector* ParticlePos = new Vector[nParticle];
    for (i=0; i<nParticle; i++)
    {
        ParticlePos[i].x = f->GetFloat();
        ParticlePos[i].y = f->GetFloat();
        ParticlePos[i].z = f->GetFloat();
    }
    
    unsigned nbPGM = f->GetUShort();
    rAssert(int(nbPGM) == geom->GetNumPrimGroup());
    
    unsigned* PgmNbVertex = new unsigned[nbPGM];
    for (i=0; i<nbPGM; i++)
    {
        PgmNbVertex[i] = f->GetUShort();
    }
    
    unsigned *fixParticle = NULL;
    unsigned *MapVL   = NULL;
    unsigned *MapTri  = NULL;
    unsigned *MapEdge = NULL;
    float    *LenEdge = NULL;
    unsigned nbFix=0, vCount=0, tCount=0, eCount=0, elCount=0;
    ConditionParams condParam;
    bool cparam = false;
    
    while(f->ChunksRemaining())
    {
        f->BeginChunk();
        switch(f->GetCurrentID())
        {
        case Simulation::Flexible::FIX_PARTICLE:
            {
                nbFix = f->GetUShort();
                if (nbFix > 0)
                {
                    fixParticle = new unsigned[nbFix];
                    for (i=0; i<nbFix; i++)
                    {
                        fixParticle[i] = f->GetUShort();
                    }
                }
            }
            break;
            
        case Simulation::Flexible::MAP_VL:
            {
                vCount = f->GetUShort();
                if (vCount > 0)
                {
                    MapVL = new unsigned[vCount];
                    for (i=0; i<vCount; i++)
                    {
                        MapVL[i] = f->GetUShort();
                    }
                }
            }
            break;
            
        case Simulation::Flexible::TRI_MAP:
            {
                tCount = f->GetUShort();
                if (tCount > 0)
                {
                    MapTri = new unsigned[tCount*3];
                    for (i=0; i<tCount; i++)
                    {
                        MapTri[3*i]   = f->GetUShort();
                        MapTri[3*i+1] = f->GetUShort();
                        MapTri[3*i+2] = f->GetUShort();
                    }
                }
            }
            break;
            
        case Simulation::Flexible::EDGE_MAP:
            {
                eCount = f->GetUShort();
                if (eCount > 0)
                {
                    MapEdge = new unsigned[eCount*2];
                    for (i=0; i<eCount; i++)
                    {
                        MapEdge[2*i]   = f->GetUShort();
                        MapEdge[2*i+1] = f->GetUShort();
                    }
                }
            }
            break;
            
        case Simulation::Flexible::EDGE_LEN:
            {
                elCount = f->GetUShort();
                if (elCount > 0)
                {
                    LenEdge = new float[elCount];
                    for (i=0; i<elCount; i++)
                    {
                        LenEdge[i] = f->GetFloat();
                    }
                }
            }
            break;
            
        case Simulation::Flexible::OBJECT_PARAMETERS:
            {
                cparam = true;
                condParam.Stretch1Dkf = f->GetFloat();
                condParam.Stretch1Dkd = f->GetFloat();
                condParam.Bend1Dkf = f->GetFloat();
                condParam.Bend1Dkd = f->GetFloat();
                condParam.Wind1DKf = f->GetFloat();
                condParam.Stretch2Dkf = f->GetFloat();
                condParam.Stretch2Dkd = f->GetFloat();
                condParam.Shear2Dkf = f->GetFloat();
                condParam.Shear2Dkd = f->GetFloat();
                condParam.Bend2Dkf = f->GetFloat();
                condParam.Bend2Dkd = f->GetFloat();
                condParam.Wind2DKf = f->GetFloat();
            }
            break;
            
        default:
            break;
        }
        f->EndChunk();
    }
    
    rAssert(eCount == elCount);
    
    if (nDim == 2)
    {
        rAssert(tCount > 0);
        
        ParticleSystemOnGeom2D* psyst = new ParticleSystemOnGeom2D(geom, nParticle);
        psyst->SetParticleSystemSolver(eLargeStepSolver);
        psyst->mExtDensity = density;
        
        psyst->mNbPGM = nbPGM;
        psyst->mPgmNbVertex = PgmNbVertex;
        
        psyst->SetVertexMapList(vCount, MapVL, ParticlePos);
        psyst->SetTriangleList(tCount, MapTri);
        psyst->SetEdgeList(eCount, MapEdge, LenEdge);
        
        for (unsigned i=0; i<nbFix; i++)
        {
            psyst->FixParticle(fixParticle[i]);
        }
        
        if (cparam)
            *psyst->mCparams = condParam;
        
        psyst->BuildConditions();
        
        FlexibleObject* obj = new FlexibleObject(PhysicsProperties::DefaultPhysicsProperties(store), psyst);
        obj->SetName(name);
        obj->SetSimEnvironment(SimEnvironment::GetDefaultSimEnvironment());
        
        ret = obj;
    }
    else if (nDim == 1)
    {
        // not implemented yet, might never be?!?
        rAssert(tCount == 0);
        //ParticleSystemOnGeom1D* psyst = new ParticleSystemOnGeom1D(nParticle);
        //psyst->SetName(name);
        //ret = psyst;
    }
    
    if (ret)
    {
        float volumescale = 0.5f;
        rmt::Vector zeropos(0, 0, 0);
        
        // temporarilly, create a collision object for that psyst
        CollisionVolume * bbox = new BBoxVolume;
        for (int i=0; i<ret->mPsyst->mNbp; i++)
        {
            CollisionVolume * sphere = new SphereVolume(zeropos, ret->mPsyst->mDeli*volumescale);
            sphere->SetObjRefIndex(i);
            bbox->AddSubVolume(sphere);
        }
        CollisionObject* collObj = new CollisionObject(bbox);
        collObj->SetName(name);
        collObj->SetNumJoint(ret->mPsyst->mNbp);
        store->Store(collObj);
    }
    
    // clean
    delete[] fixParticle;
    
    return ret;
}

} // sim
