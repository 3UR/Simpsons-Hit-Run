
#ifndef _COLLISIONDISPLAY_HPP_
#define _COLLISIONDISPLAY_HPP_

#include "p3d/p3dtypes.hpp"

namespace sim
{

class CollisionVolume;
class CollisionObject;
class CollisionManager;

void DrawCollisionObject(CollisionObject* inObject);
void DrawCollisionVolume(CollisionVolume* volume);
bool TestPointOnSurf(CollisionVolume* volume, rmt::Vector r, bool rel);

void DisplayCollisions(CollisionManager* collisionManager, int area = 0);
void DisplayCollisionObjects(CollisionManager* collisionManager, int area = 0);
void DisplayCollisionObjectPairs(CollisionManager* collisionManager, int area = 0);


enum DrawVolumeMethod {DrawVolumeOutline=0, DrawVolumeShape};

void SetDrawVolumeMethod(DrawVolumeMethod inDrawVolumeMethod);

} // sim


#endif // _COLLISIONDISPLAY_HPP_