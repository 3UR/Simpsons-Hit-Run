#include "simcommon/simutility.hpp"
#include "simcommon/simListinitsize.hpp"
#include "simcommon/dline2.hpp"
//#include "simflexible/psystongeom2d.hpp"
//#include "simflexible/flexjointmodifier.hpp"
#include "simcollision/collisionmanager.hpp"
#include "simphysics/physicsobject.hpp"

// for the loader
#include <p3d/chunkfile.hpp>
#include <p3d/file.hpp>
#include <p3d/utility.hpp>
#include <constants/chunkids.hpp>
#include <constants/chunks.h>
#include <string.h>

#include <radtime.hpp> // for simstats

namespace sim
{

//
// SimUnitsManager class
//

class SimUnitsManager 
: protected SimUnits
{
public:
    void SetUnits(float l, float m, float t)
    {
        rAssertMsg(sUnitSet == false, "SimUnitsManager::SetUnits: SetUnits has already been called by someone or SimUnits::Reset has not been called\n");
    
        // the 1 is 'this'
        rAssertMsg(sList->GetSize() == 1, "SimUnitsManager::SetUnits: SetUnits must only be called at the beginning");
    
        rAssert(m==1.0f && t==1.0f); // only length units are supported for the moment
    
        sL = l;
        sM = m;
        sT = t;
        sUnitSet = true;
        //UpdateAll();
    }
    
protected:
    /* A few choices here:
    -Or we modify the code so that all the speed and time related units 
    are instances of the SimTime et al. and SetUnits will be still available anytime.
    But the resulting code won't be optimized for both size and speed.
    Since SetUnits would not be that usefull to use after initialisation
    all the UpdateUnits()/UpdateAll methods are removed and we assert if the units are changed
    after they are used.
    */
    void UpdateUnits() {}
    
private:
    void UpdateAll()
    {
        for (int i=0; i<sList->GetSize(); i++)
            sList->GetAt(i)->UpdateUnits();
    }
};

//
//
//

void InstallSimLoaders()
{
    tP3DFileHandler* p3d = p3d::loadManager->GetP3DHandler();
    if(p3d)
    {   
        p3d->AddHandler(new tExportInfoLoader);
        //p3d->AddHandler(new PSystGeomLoader);
        p3d->AddHandler(new CollisionObjectLoader);
        p3d->AddHandler(new PhysicsObjectLoader);
        //p3d->AddHandler(new FlexibleJointDefinitionLoader);
    }
}



void InitializeSimulation(GeometryUnitsEnum units)
{
    rAssert(SimUnits::UnitSet() == false);

    float LSCALE = 1.0f;
    float MSCALE = 1.0f;
    float TSCALE = 1.0f;

    // X [centimeter] * LSCALE = Y [geomUnits]
    // LSCALE = Y / X
    
    switch (units)
    {
    case MillimetersUnits:
        LSCALE = 10.0f;
        break;
    case CentimetersUnits:
        LSCALE = 1.0f;
        break;
    case MetersUnits:
        LSCALE = 0.01f;
        break;
    case KilometersUnits:
        LSCALE = 0.00001f;
        break;
    case InchesUnits:
        LSCALE = 0.3937f;
        break;
    case FeetUnits:
        LSCALE = 0.03281f;
        break;
    case YardsUnits:
        LSCALE = 0.01094f;
        break;
    case MilesUnits:
        LSCALE = 0.0006214f;
        break;
    case NoUnits:
    default:
        break;
    }

    //radMemoryAllocator old = ::radMemorySetCurrentAllocator( RADMEMORY_ALLOC_TEMP );
    SimUnitsManager um;
    um.SetUnits(LSCALE, MSCALE, TSCALE); 

#ifndef RAD_RELEASE
    SimEnvironment::sListOfActiveSimulatedObject = new TList<SimulatedObject*>(SimulatedObjectListSize);
#endif
    //::radMemorySetCurrentAllocator( old );
}

void ResetSimulation()
{
#ifndef RAD_RELEASE
    rAssert(SimEnvironment::sListOfActiveSimulatedObject == NULL || SimEnvironment::sListOfActiveSimulatedObject->GetSize() == 0);
    delete SimEnvironment::sListOfActiveSimulatedObject;
    SimEnvironment::sListOfActiveSimulatedObject = NULL;
#endif

    if (CollisionManager::GetCurrentInstance())
        CollisionManager::GetCurrentInstance()->Release();

    rAssert(CollisionManager::GetCurrentInstance() == NULL);
    
    if (SimEnvironment::GetCurrentDefaultSimEnvironment())
        SimEnvironment::GetCurrentDefaultSimEnvironment()->Release();

    // nobody should be holding on to the defaultSimEnvironment anymore
    rAssert(SimEnvironment::GetCurrentDefaultSimEnvironment() == NULL);

    // Temp fix for defaultphysics being put in the main inventory.
    // maybe not the best way
    tPlatform::GetPlatform()->GetActiveContext()->GetInventory()->Remove( PhysicsProperties::DefaultPhysicsProperties() );

    // at this point, all instances of physics objects have been deleted
    // so we should be able to reset the SimUnits. If not this will assert.
    // the reason why we want to reset the SimUnits is that if the new file we load
    // has different Maya units, we want to be able to reflect that in SimUnits. 
    // This is only possible if all the PhObjects have been deleted.
    // if this assert, Look in ((SimUnits*)SimUnits::sList->mArray[i]) for i=0 
    // to SimUnits::sList->mSize to see what hasn't been cleaned.
    SimUnits::Reset();
    
    // reset some static variable in the physics objects class
    PhysicsObject::ResetUnitsRelatedValues();

}


void SetupLineDrawing ()
{
    // Allocate some global stuff for line drawing.
    // Pure3D and PDDI should be properly initialized before this is called
    //
    g_pDefaultShader = p3d::device->NewShader("simple");
    g_pDefaultShader->AddRef();
    g_pDefaultShader->SetInt(PDDI_SP_ISLIT, 0);
    g_pLineElemArray = new TArray<LineElem>(128);  // icky magic number; also no checking for overflow on this array
}


void CleanupLineDrawing ()
{
    // Cleanup the global line drawing stuff
    //
    g_pDefaultShader->Release ();
    delete g_pLineElemArray;
}


//
//
//

#define LongToBool(l) (l==0?false:true)

tExportInfoLoader::tExportInfoLoader() : tSimpleChunkHandler(P3D_EXPORT_INFO) {};

tEntity* tExportInfoLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char name[128];
    f->GetPString(name);
    
    tExportInfo* info =  new tExportInfo;
    info->SetName(name);

    bool completed = false;

    // keep only what I need
    while(f->ChunksRemaining() && !completed)
    {      
        f->BeginChunk();

        f->GetPString(name);
        if (strcmp(name, "UI Units") == 0)
        {
            f->GetPString(info->geomUnits); // units
            completed = true; // don't need anything else
        }
        f->EndChunk();
    }
    
    // this chunks is potentially in every p3d file 
    tExportInfo* previousInfo = NULL;//p3d::find<tExportInfo>(store,"Exported From Maya");
    if (previousInfo)
    {
        rAssert(strcmp(previousInfo->geomUnits, info->geomUnits) == 0);
        info->Release();
        info = NULL;
    }
    
    if (!SimulationInitialized() && info)
    {
        GeometryUnitsEnum units;

        if (strcmp(info->geomUnits, "Millimeters") == 0)
        {
            units = MillimetersUnits;
        }
        else if (strcmp(info->geomUnits, "Centimeters") == 0)
        {
            units = CentimetersUnits;
        }
        else if (strcmp(info->geomUnits, "Meters") == 0)
        {
            units = MetersUnits;
        }
        else if (strcmp(info->geomUnits, "Kilometers") == 0)
        {
            units = KilometersUnits;
        }
        else if (strcmp(info->geomUnits, "Inches") == 0)
        {
            units = InchesUnits;
        }
        else if (strcmp(info->geomUnits, "Feet") == 0)
        {
            units = FeetUnits;
        }
        else if (strcmp(info->geomUnits, "Yards") == 0)
        {
            units = YardsUnits;
        }
        else if (strcmp(info->geomUnits, "Miles") == 0)
        {
            units = MilesUnits;
        }
        else
        {
            units = NoUnits;
        }
        InitializeSimulation(units);
    }
    
    return info;
}

//
//
//

#ifdef USE_SIMSTATS

template <class T> inline void TestPeak(T value, unsigned frame, T& peakvalue, unsigned& peakframe)
{
    if (peakvalue < value)
    {
        peakvalue = value;
        peakframe = frame;
    }
}


float       SimStats::sSimTime       = 0.0f;
float       SimStats::sSimPeakTime   = 0.0f;
float       SimStats::sSimAveTime    = 0.0f;
unsigned    SimStats::sStartSimTime  = 0;
float       SimStats::sDColTime      = 0.0f;
float       SimStats::sDColPeakTime  = 0.0f;
float       SimStats::sDColAveTime   = 0.0f;
unsigned    SimStats::sStartDColTime = 0;
float       SimStats::sSColTime      = 0.0f;
float       SimStats::sSColPeakTime  = 0.0f;
float       SimStats::sSColAveTime   = 0.0f;
unsigned    SimStats::sStartSColTime = 0;
float       SimStats::sFlexTime      = 0.0f;
float       SimStats::sFlexPeakTime  = 0.0f;
float       SimStats::sFlexAveTime   = 0.0f;
unsigned    SimStats::sStartFlexTime = 0;
float       SimStats::sRCasTime      = 0.0f;
float       SimStats::sRCasPeakTime  = 0.0f;
float       SimStats::sRCasAveTime   = 0.0f;
unsigned    SimStats::sStartRCasTime = 0;

float       SimStats::sTotPeakTime   = 0.0f;

unsigned    SimStats::sFrameCounter  = 0;
char        SimStats::buf[1024] = "";

unsigned    SimStats::sMaxDetectedCollisions = 0;
unsigned    SimStats::sMaxCollisionPairTested = 0;

unsigned    SimStats::sSimPeakTimeFrame  = 0;
unsigned    SimStats::sDColPeakTimeFrame = 0;
unsigned    SimStats::sSColPeakTimeFrame = 0;
unsigned    SimStats::sFlexPeakTimeFrame = 0;
unsigned    SimStats::sRCasPeakTimeFrame = 0;
unsigned    SimStats::sTotPeakTimeFrame  = 0;
unsigned    SimStats::sMaxDetectedCollisionsFrame = 0;
unsigned    SimStats::sMaxCollisionPairTestedFrame = 0;

LevelOfStatsDetail SimStats::sLevelOfStatsDetail = AdvancedStats;

void SimStats::StartDetectCollision()
{
    if (sLevelOfStatsDetail == AdvancedStats)
        StartTime(sStartDColTime);
}

void SimStats::EndDetectCollision()
{
    if (sLevelOfStatsDetail == AdvancedStats)
        EndTime(sStartDColTime, sDColTime);
}

void SimStats::StartSolveCollision()
{
    if (sLevelOfStatsDetail == AdvancedStats)
        StartTime(sStartSColTime);
}

void SimStats::EndSolveCollision()
{
    if (sLevelOfStatsDetail == AdvancedStats)
        EndTime(sStartSColTime, sSColTime);
}

void SimStats::StartSimulation()
{
    if (sLevelOfStatsDetail == AdvancedStats)
        StartTime(sStartSimTime);
}

void SimStats::EndSimulation()
{
    if (sLevelOfStatsDetail == AdvancedStats)
        EndTime(sStartSimTime, sSimTime);
}

void SimStats::StartFlexSimulation()
{
    if (sLevelOfStatsDetail == AdvancedStats)
        StartTime(sStartFlexTime);
}

void SimStats::EndFlexSimulation()
{
    if (sLevelOfStatsDetail == AdvancedStats)
        EndTime(sStartFlexTime, sFlexTime);
}

void SimStats::StartRCasSimulation()
{
    if (sLevelOfStatsDetail == AdvancedStats)
        StartTime(sStartRCasTime);
}

void SimStats::EndRCasSimulation()
{
    if (sLevelOfStatsDetail == AdvancedStats)
        EndTime(sStartRCasTime, sRCasTime);
}

void SimStats::StartTime(unsigned& starttime)
{
    starttime = radTimeGetMicroseconds();
}

void SimStats::EndTime(unsigned& starttime, float& tottime)
{
    float tmp = float(radTimeGetMicroseconds() - starttime)/1000.0f;
    if (tmp > 0)
        tottime += tmp;
}

//
//
//

SimStats::SimStats()
{
    Reset();
}

void SimStats::Reset()
{
    mCollisionArea = 0;
    mCollisionObject = 0;
    mStaticCollisionObject = 0;
    mCollisionPair = 0;
    mCollisionPairTested = 0;
    mDetectedCollisions = 0;

    mSimulatedObject = 0;
    mSimulatedObjectAtRest = 0;
}

void SimStats::Compute()
{
    Reset();

    if (CollisionManager::GetCurrentInstance())
    {
        CollisionManager* cm = CollisionManager::GetCurrentInstance();

        for (int i=0; i<cm->NbArea(); i++)
        {
            if (cm->AreaActivated(i))
            {
                mCollisionArea++;
                mCollisionObject += cm->GetCollisionObjectList(i)->GetSize();
                mCollisionPair += cm->GetCollisionObjectPairList(i)->GetSize();
                mDetectedCollisions += cm->GetCollisionList(i)->GetSize();

                if (sLevelOfStatsDetail > BasicsStats)
                {
                    int j;
                    for (j=0; j<cm->GetCollisionObjectList(i)->GetSize(); j++)
                    {
                        CollisionObject* o = cm->GetCollisionObjectList(i)->GetAt(j);
                        if (o->IsStatic())
                            mStaticCollisionObject++;
                        else if (o->GetSimState()->GetControl() == simSimulationCtrl)
                        {
                            mSimulatedObject++;
                            if (o->GetSimState()->GetSimulatedObject()->IsAtRest())
                            {
                                mSimulatedObjectAtRest++;
                            }
                        }
                    }
                    for (j=0; j<cm->GetCollisionObjectPairList(i)->GetSize(); j++)
                    {
                        if (cm->GetCollisionObjectPairList(i)->GetAt(j).mSafeTimeBeforeCollision == 0)
                        {
                            if (cm->GetCollisionObjectPairList(i)->GetAt(j).mNeedsUpdate == true)
                                mCollisionPairTested++;
                        }
                    }
                }
            }
        }
    }
}

void SimStats::Display(bool compute, unsigned xpos, unsigned ypos, tColour* inColor)
{
    if (compute)
    {
        Compute();

        switch (sLevelOfStatsDetail)
        {
        case BasicsStats:
            sprintf(buf,    "NumCollisionArea activated = %d,\n"
                            "NumCollisionObject         = %d,\n"
                            "NumCollisionPair           = %d,\n"
                            "NumCollision               = %d,\n"
                            "NumSimulatedObject        = %d\n"
                            "\n",
                            mCollisionArea,
                            mCollisionObject,
                            mCollisionPair,
                            mDetectedCollisions,
                            mSimulatedObject);
            break;
        case IntermediateStats:
            sprintf(buf,    "NumCollisionArea activated = %d,\n"
                            "NumCollisionObject         = %d (%d are static),\n"
                            "NumCollisionPair           = %d (%d have been tested),\n"
                            "NumCollision               = %d,\n"
                            "NumSimulatedObject        = %d (%d are at rest)\n"
                            "\n",
                            mCollisionArea,
                            mCollisionObject, mStaticCollisionObject,
                            mCollisionPair, mCollisionPairTested,
                            mDetectedCollisions,
                            mSimulatedObject, mSimulatedObjectAtRest);
            break;
        case AdvancedStats:

            sFrameCounter ++;

            static float aveFact = 0.1f;

            sDColAveTime += aveFact*(sDColTime - sDColAveTime);
            sSColAveTime += aveFact*(sSColTime - sSColAveTime);
            sSimAveTime  += aveFact*(sSimTime  - sSimAveTime);
            sFlexAveTime += aveFact*(sFlexTime - sFlexAveTime);
            sRCasAveTime += aveFact*(sRCasTime - sRCasAveTime);
            float totTime = sDColTime + sSColTime + sSimTime + sFlexTime + sRCasTime;

            if (sFrameCounter > 25)
            {
                TestPeak(sSimTime, sFrameCounter, sSimPeakTime, sSimPeakTimeFrame);
                TestPeak(sDColTime, sFrameCounter, sDColPeakTime, sDColPeakTimeFrame);
                TestPeak(sSColTime, sFrameCounter, sSColPeakTime, sSColPeakTimeFrame);
                TestPeak(sFlexTime, sFrameCounter, sFlexPeakTime, sFlexPeakTimeFrame);
                TestPeak(sRCasTime, sFrameCounter, sRCasPeakTime, sRCasPeakTimeFrame);
                
                TestPeak(totTime, sFrameCounter, sTotPeakTime, sTotPeakTimeFrame);
                
                TestPeak(mDetectedCollisions, sFrameCounter, sMaxDetectedCollisions, sMaxDetectedCollisionsFrame);
                TestPeak(mCollisionPairTested, sFrameCounter, sMaxCollisionPairTested, sMaxCollisionPairTestedFrame);

            }
            totTime = sDColAveTime + sSColAveTime + sSimAveTime + sFlexAveTime + sRCasAveTime;
            
            sprintf(buf,    "NumCollisionArea  activated = %d\n"
                            "NumCollisionObject   = %d (%d are static)\n"
                            "NumCollisionPair     = %d, %d have been tested (peak=%d at frame %d)\n"
                            "NumCollision         = %d (peak=%d at frame %d)\n"
                            "NumSimulatedObject  = %d (%d are at rest)\n"
                            "\nBenchmark (msec)     time       peak     frame #\n"
                            "detecting collision      %2.3f      %2.3f    %d\n"
                            "solving collision        %2.3f      %2.3f    %d\n"
                            "simulating rigidbody    %2.3f      %2.3f    %d\n"
                            "simulating flexible      %2.3f      %2.3f    %d\n"
                            "ray casting             %2.3f      %2.3f    %d\n"
                            "Total                   %2.3f      %2.3f    %d",
                            mCollisionArea,
                            mCollisionObject, mStaticCollisionObject,
                            mCollisionPair, mCollisionPairTested, sMaxCollisionPairTested, sMaxCollisionPairTestedFrame,
                            mDetectedCollisions, sMaxDetectedCollisions, sMaxDetectedCollisionsFrame,
                            mSimulatedObject, mSimulatedObjectAtRest,
                            sDColAveTime,  sDColPeakTime, sDColPeakTimeFrame,
                            sSColAveTime,  sSColPeakTime, sSColPeakTimeFrame,
                            sSimAveTime,   sSimPeakTime,  sSimPeakTimeFrame,
                            sFlexAveTime,  sFlexPeakTime, sFlexPeakTimeFrame,
                            sRCasAveTime,  sRCasPeakTime, sRCasPeakTimeFrame,
                            totTime,       sTotPeakTime,  sTotPeakTimeFrame);
            break;
        }
    }

    tColour color (127,200,100);

    if (inColor)
        color = *inColor;

    p3d::pddi->DrawString(buf, xpos, ypos, color);

    sSimTime = 0.0f;
    sFlexTime = 0.0f;
    sRCasTime = 0.0f;
    sDColTime = 0.0f;
    sSColTime = 0.0f;
}

#endif //  USE_SIMSTATS

} // sim
