
#ifndef _SIMLISTINITSIZE_HPP_
#define _SIMLISTINITSIZE_HPP_

/*
There is a few arrays and lists in the sim library that will dynamically grow
if need during the run-time. This might cause memory fragmentation.

In order to prevent that, the user can customize the initial size of these
lists so that the resize request will be minimal or even inexistant.
*/

namespace sim
{

const int CollisionAreaListSize           = 37;
const int CollisionObjectListSize         = 48;
const int CollisionObjectPairListSize     = 256;
const int CollisionListSize               = 128;

const int CollisionAnalyserDataListSize   = 40;
const int CollisionAnalyserEventListSize  = 10;

const int PhysicsObjectListSize           = 10;
const int SimulatedObjectListSize         = 512;

} // sim

#endif // _SIMLISTINITSIZE_HPP_