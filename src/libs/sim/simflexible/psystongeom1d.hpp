
#ifndef PSYSTONGEOM1D
#define PSYSTONGEOM1D

#include "simflexible/particlesyst.hpp"

class tGeometry;

namespace sim
{

   class ParticleSystemOnGeom1D
: public ParticleSystem 
{
public:
   ParticleSystemOnGeom1D(tGeometry* g);
   virtual ~ParticleSystemOnGeom1D();

   void BuildConditions();
   virtual void Display();
   virtual void PreDisplay();
   virtual void PreStepModification();

   virtual void StartIntermediateParticle() { }
   virtual bool GetIntermediateParticle(float pos[]) { return false; }
   virtual bool NeighborParticle(int i, int j) const { return false; }

protected:
   void CreateVertexMapList(rmt::Vector* pos);
   void CreateLineList();

   tGeometry* tgeom;
   unsigned* mapPtoVL;

   unsigned nLines;
   unsigned* lineMap;
   float* lineLength;
};

} // sim

#endif // PSYSTONGEOM1D