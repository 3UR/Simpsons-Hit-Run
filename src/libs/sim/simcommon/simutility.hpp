
#ifndef _SIMUTILITY_HPP_
#define _SIMUTILITY_HPP_

#include <p3d/loadmanager.hpp>
#include "simcommon/simenvironment.hpp"


#ifndef RAD_RELEASE
#define USE_SIMSTATS
#endif

namespace sim
{

enum GeometryUnitsEnum { MillimetersUnits, CentimetersUnits, MetersUnits, KilometersUnits, InchesUnits, FeetUnits, YardsUnits, MilesUnits, NoUnits };

void InstallSimLoaders();

void InitializeSimulation(GeometryUnitsEnum units);
void ResetSimulation();
void SetupLineDrawing ();
void CleanupLineDrawing ();
inline bool SimulationInitialized() { return SimUnits::UnitSet(); }

//
//
//

class tExportInfo
    : public tEntity
{
public:
    tExportInfo() {}
    virtual ~tExportInfo() { }
    
    char geomUnits[32];
    bool NIS;
    bool autoBoundingVolume;
};

class tExportInfoLoader
    : public tSimpleChunkHandler
{
public:
    tExportInfoLoader();
    virtual tEntity* LoadObject(tChunkFile*, tEntityStore*);
};


//
// 
//
/*
    SimStats class allows to collect and print the the 
    content of the collisionmanager and the simulation manager
    as well as some benchmark results.

    the LevelOfStatsDetail allows to customize the amount of information will be 
    collected and print.

    BasicsStats: provide with size of different lists:
        number of collision objects, collision object pairs and simulation object only

    IntermediateStats: BasicsStats + loops through the lists and collects some info about their content  

    AdvancedStats: IntermediateStats + collects tiiming for collision detecter, collision solver and rigidbody simulator.


    usage:

    initialisation: 
        SimStats::SetLevelOfStatsDetail(desired details)

    gameloop:
        call SimStats::DisplaySimStats() in your display method

        
*/

#ifdef USE_SIMSTATS

enum LevelOfStatsDetail { BasicsStats = 0, IntermediateStats, AdvancedStats };

class SimStats
{
public:

    SimStats();
    ~SimStats() {}

    static void SetLevelOfStatsDetail(LevelOfStatsDetail in) { sLevelOfStatsDetail = in; }
    static LevelOfStatsDetail GetLevelOfStatsDetail() { return sLevelOfStatsDetail; }

    static void StartDetectCollision();
    static void EndDetectCollision();
    static void StartSolveCollision();
    static void EndSolveCollision();
    static void StartSimulation();
    static void EndSimulation();
    static void StartFlexSimulation();
    static void EndFlexSimulation();
    static void StartRCasSimulation();
    static void EndRCasSimulation();

    static void StartTime(unsigned& starttime);
    static void EndTime(unsigned& starttime, float& tottime);

    static void DisplayStats(bool compute = true, unsigned xpos = 50, unsigned ypos = 50, tColour* inColor = NULL)
    {
        SimStats stat;
        stat.Display(compute, xpos, ypos, inColor);
    }

    static void ResetPeakTimes()
    {
        sSimPeakTime = sDColPeakTime = sSColPeakTime = sFlexPeakTime = sRCasPeakTime = sTotPeakTime = 0.0f;
        sMaxDetectedCollisions = sMaxCollisionPairTested = 0;
        sMaxDetectedCollisionsFrame = sMaxCollisionPairTestedFrame = 0;
        sSimPeakTimeFrame = sSColPeakTimeFrame = sDColPeakTimeFrame = sFlexPeakTimeFrame = sRCasPeakTimeFrame = sTotPeakTimeFrame = 0;
        sFrameCounter = 0; // wait a few frames before starting accumulating the peaks.
    }

protected:
    void Display(bool compute = true, unsigned xpos = 50, unsigned ypos = 50, tColour* inColor = NULL);

    void Reset();   // called from compute
    void Compute(); // call from display

    unsigned mCollisionArea;
    unsigned mCollisionObject;
    unsigned mStaticCollisionObject;
    unsigned mCollisionPair;
    unsigned mCollisionPairTested;
    unsigned mDetectedCollisions;

    unsigned mSimulatedObject;
    unsigned mSimulatedObjectAtRest;

    static LevelOfStatsDetail sLevelOfStatsDetail;

    static unsigned sMaxDetectedCollisions;
    static unsigned sMaxDetectedCollisionsFrame;
    static unsigned sMaxCollisionPairTested;
    static unsigned sMaxCollisionPairTestedFrame;

    static float sSimTime;
    static float sSimPeakTime;
    static float sSimAveTime;
    static unsigned sSimPeakTimeFrame;
    static unsigned sStartSimTime;

    static float sDColTime;
    static float sDColPeakTime;
    static float sDColAveTime;
    static unsigned sDColPeakTimeFrame;
    static unsigned sStartDColTime;

    static float sSColTime;
    static float sSColPeakTime;
    static float sSColAveTime;
    static unsigned sSColPeakTimeFrame;
    static unsigned sStartSColTime;

    static float sFlexTime;
    static float sFlexPeakTime;
    static float sFlexAveTime;
    static unsigned sFlexPeakTimeFrame;
    static unsigned sStartFlexTime;

    static float sRCasTime;
    static float sRCasPeakTime;
    static float sRCasAveTime;
    static unsigned sRCasPeakTimeFrame;
    static unsigned sStartRCasTime;

    static float sTotPeakTime;
    static unsigned sTotPeakTimeFrame;

    static unsigned sFrameCounter;

    static char buf[1024];

};

#else // USE_SIMSTATS

class SimStats
{
public:

    SimStats() {}
    ~SimStats() {}
    static void StartDetectCollision() {}
    static void EndDetectCollision() {}
    static void StartSolveCollision() {}
    static void EndSolveCollision() {}
    static void StartSimulation() {}
    static void EndSimulation() {}
    static void StartFlexSimulation() {}
    static void EndFlexSimulation() {}
    static void StartRCasSimulation() {}
    static void EndRCasSimulation() {}
    static void DisplayStats(bool compute = true, unsigned xpos = 50, unsigned ypos = 50, tColour* inColor = NULL) {}
    static void ResetPeakTimes() {}
};

#endif // USE_SIMSTATS


} // sim

#endif // _SIMUTILITY_HPP_
