
#include "simflexible/psystongeom1d.hpp"
#include "p3d/geometry.hpp"
#include "p3d/primgroup.hpp"
#include "p3d/vertexlist.hpp"
#include "simflexible/condition1d.hpp"

#define MAX_VERTICES_IN_GEOM_PS 100

class PGLineIterator
{
public:
   inline virtual void SetPG(tPrimGroup* p) { pg = p; }
   virtual bool Next(unsigned n[]) = 0;
   virtual unsigned NLines() const = 0;
protected:
   PGLineIterator() : pg(NULL) { }
   tPrimGroup* pg;
   unsigned current;
};

class PGLineIteratorL
: public PGLineIterator
{
public:
   void SetPG(tPrimGroup* p);
   bool Next(unsigned n[]);
   unsigned NLines() const;
};

void PGLineIteratorL::SetPG(tPrimGroup* p) 
{ 
   pg = p; 
   rAssert(pg->GetPrimType() == PDDI_PRIM_LINES); 
//   current = pg->GetOffset(); 
}

bool PGLineIteratorL::Next(unsigned i[])
{
//   if (current < pg->GetCount() + pg->GetOffset() - 1)
   if (0)
   {
      i[0] = current++;
      i[1] = current++;
      return true;
   }
   else
      return false;
}

unsigned PGLineIteratorL::NLines() const
{
   return pg->GetCount() / 2;
}

//

class PGLineIteratorLS
: public PGLineIterator
{
public:
   void SetPG(tPrimGroup* p);
   bool Next(unsigned n[]);
   unsigned NLines() const;
};

void PGLineIteratorLS::SetPG(tPrimGroup* p) 
{ 
   pg = p; 
   rAssert(pg->GetPrimType() == PDDI_PRIM_LINESTRIP); 
//   current = pg->GetOffset(); 
}

bool PGLineIteratorLS::Next(unsigned i[])
{
//   if (current < pg->GetCount() + pg->GetOffset() - 1)
   {
      i[0] = current;
      i[1] = current+1;
      current += 1;
      return true;
   }
   else
      return false;
}

unsigned PGLineIteratorLS::NLines() const
{
   return pg->GetCount() - 1;
}

//

/* do I need that? don't think so
class PGLineIteratorIPG
: public PGLineIterator
{
public:
   void SetPG(tPrimGroup* p);
   bool Next(unsigned n[]);
   unsigned NLines() const;
   tIndexedPrimGroup* ipg;
};

void PGLineIteratorIPG::SetPG(tPrimGroup* p) 
{ 
   pg = p;
   ipg = dynamic_cast<tIndexedPrimGroup*> (pg);
   rAssert(ipg); 
   current = pg->GetOffset(); 
}

bool PGLineIteratorIPG::Next(unsigned i[])
{
   if (current < ipg->GetNumIndices() - 1)
   {
      i[0] = ipg->GetIndices()[current++];
      i[1] = ipg->GetIndices()[current++];
      return true;
   }
   else
      return false;
}

unsigned PGLineIteratorIPG::NLines() const
{
   return ipg->GetNumIndices() / 2;
}
*/

//
//
//


ParticleSystemOnGeom1D::ParticleSystemOnGeom1D(tGeometry* g)
: tgeom(g)
{
   rAssert(tgeom != NULL);
   tgeom->AddRef();
   Vector pos[MAX_VERTICES_IN_GEOM_PS];

   maxdef = 0.1f;

   CreateVertexMapList(pos);
   Init(); // creates the particles

   for(int i=0; i<nbp; i++)
      SetVector(lp[i]->x, pos[i].x, pos[i].y, pos[i].z);

   CreateLineList();
   BuildConditions();
}

ParticleSystemOnGeom1D::~ParticleSystemOnGeom1D()
{
   delete mapPtoVL;
   delete lineMap;
   delete lineLength;
   tgeom->Release();
}

void ParticleSystemOnGeom1D::CreateVertexMapList(Vector* pos)
{
   // first: get the map of all the vertices in all the PrimGroups
   tVertexList* vl = tgeom->GetVertexList();
   mapPtoVL = new unsigned[vl->GetNumVertices()];

   for (unsigned j=0; j<vl->GetNumVertices(); j++)
   {
      Vector v = vl->Position(j);
      bool alreadythere = false;
      int m;
      for (m=0; m<nbp; m++)
      {
         if (v.Equals(pos[m]))
         {
            alreadythere = true;
            break;
         }
      }
      if (alreadythere)
      {
         mapPtoVL[j] = m;
      }
      else
      {
         mapPtoVL[j] = nbp;
         pos[nbp] = v;
         nbp++;
         rAssertMsg(nbp < MAX_VERTICES_IN_GEOM_PS, "too many vertices in tGeometry", "ParticleSystemOnGeom1D::CreateVertexMapList");
      }
   }
}

void ParticleSystemOnGeom1D::CreateLineList()
{
   PGLineIteratorL il;
   PGLineIteratorLS ils;
   PGLineIterator* iterator;
   nLines = 0;
   unsigned i;

   for (i=0; i<unsigned(tgeom->GetNumPrimGroup()); i++)
   {
      tPrimGroup* pg = tgeom->GetPrimGroup(i);
      if (pg->GetType() == PDDI_PRIM_LINESTRIP)
         iterator = &ils;
      else if (pg->GetType() == PDDI_PRIM_LINES)
         iterator = &il;

      iterator->SetPG(pg);
      nLines += iterator->NLines();
   }

   lineMap = new unsigned[nLines * 2];
   unsigned* current = lineMap;

   for (i=0; i<unsigned(tgeom->GetNumPrimGroup()); i++)
   {
      bool testdegeneratedlines = false;
      tPrimGroup* pg = tgeom->GetPrimGroup(i);
      if (pg->GetType() == PDDI_PRIM_LINESTRIP)
      {
         iterator = &ils;
         testdegeneratedlines = true;
      }
      else if (pg->GetType() == PDDI_PRIM_LINES)
         iterator = &il;

      iterator->SetPG(pg);
      while(iterator->Next(current)) // this sets current to the index in the pg
      {
         bool found = false;
         if (testdegeneratedlines)
         {
            for (int ii = 0; ii<current - lineMap; ii+=2)
            {
               if (lineMap[ii] == mapPtoVL[current[0]] || lineMap[ii] == mapPtoVL[current[1]])
                  if (lineMap[ii+1] == mapPtoVL[current[0]] || lineMap[ii+1] == mapPtoVL[current[1]])
                     found = true;
            }
         }
         if (!found)
         {
            // map the indices to the particle's indices
            current[0] = mapPtoVL[current[0]];
            current[1] = mapPtoVL[current[1]];
            current += 2;
            rAssert(current - lineMap >= 0);
         }
         else
         {
            nLines--;
         }
      }
   }
   lineLength = new float[nLines];
   deli = 0;

   float d[3];
   for (i=0; i<nLines; i++)
   {
      SubVector(lp[lineMap[2*i]]->x, lp[lineMap[2*i+1]]->x, d);
      lineLength[i] = LenVector(d);
      deli += lineLength[i];
      deli /= nLines;
   }
}

void ParticleSystemOnGeom1D::BuildConditions()
{
   unsigned i;
   for (i=0; i<nLines; i++)
   {
      // assume that the edge is at the equilibrium already
      Conditions->Insert( new Stretch1DCondition(lp[lineMap[2*i]], lp[lineMap[2*i+1]], cparams, lineLength[i]));
      Conditions->Insert( new Wind1DCondition(lp[lineMap[2*i]], lp[lineMap[2*i+1]], cparams, wind));
   }
   if (nLines > 1)
   {
      for(i=0; i<nLines; i++)
      {
         Conditions->Insert( new Bend1DCondition(lp[lineMap[2*i]], lp[lineMap[2*i+1]], lp[lineMap[2*i+1]], cparams) );
      }
   }
}

void ParticleSystemOnGeom1D::PreStepModification()
{
return; // not yet, broken...

   // test extension of every segments
   static int maxiter = 20;
   int n = 0;
   bool moved = true;

   while (moved && n < maxiter)
   {
      moved = false;
      for (unsigned i=0; i<nLines; i++)
      {
         if (TestExtension1D(lp[lineMap[2*i]], lp[lineMap[2*i+1]], lineLength[i], maxdef))
         {
            moved = true;
         }
      }
      n++;
   }
}

void ParticleSystemOnGeom1D::PreDisplay()
{
   // assign the vertices position
   tVertexList* li = tgeom->GetVertexList();

   for (unsigned j=0; j<li->GetNumVertices(); j++)
   {
      // map the indices to the particle's indices
      float* ppos = lp[mapPtoVL[j]]->x;
      li->Position(j).Set(ppos[0], ppos[1], ppos[2]);
   }
}

void ParticleSystemOnGeom1D::Display()
{
   tgeom->Display();

   tColour c1(0, 50, 200), c2(200, 0, 50);
   Condition *C=Conditions->next(); // Skip the fencepost NullCondition
   while (C) 
   {
//      /*
      Bend1DCondition* bc = dynamic_cast<Bend1DCondition*>(C);
      if (bc)
      {
         float d[3];
         SetVector(d, bc->pj->x[0], bc->pj->x[1]+2, bc->pj->x[2]);
         DrawLine(bc->pj->x, d, c1);
      }
      Stretch1DCondition* tc = dynamic_cast<Stretch1DCondition*>(C);
      if (tc)
      {
         float d[3], e[3];
         AddVector(tc->pi->x, tc->pj->x, d);
         ScaleVector(d, 0.5f, d);
         SetVector(e, d[0], d[1]+2, d[2]);
         DrawLine(d, e, c2);
      }
//      */
      C = C->next();
   }
}
