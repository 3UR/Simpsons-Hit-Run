//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _MULTICONTROLLER_HPP_
#define _MULTICONTROLLER_HPP_

#include <p3d/anim/animate.hpp>

class tMultiController : public tFrameController
{
public:
    struct TrackInfo
    {
        float startTime, endTime, offset;
        float scale;
    };

    tMultiController(int nTracks, float time);

    virtual tFrameController *Clone(void);

    unsigned GetNumTracks(void)                                     { return nTracks; }

    p3dCycleMode GetCycleMode()                                     { return cycleMode; }
    void SetCycleMode(p3dCycleMode mode)                            { cycleMode = mode; }

    float GetNumFrames(void)                                        { return maxFrame-minFrame; }
    void  SetNumFrames(float t)                                     { maxFrame = minFrame+t; }

    float GetFramerate(void)                                        { return speed; }
    void  SetFramerate(float s)                                     { speed = s; }

    tFrameController* GetTrack(unsigned i);
    void              SetTrack(unsigned i, tFrameController* t);
    TrackInfo*        GetTrackInfo(unsigned i)                      { return (i < nTracks) ? &trackInfo[i] : NULL; }
    void              SetTrackInfo(unsigned i, TrackInfo*  t)       { if (i < nTracks) trackInfo[i] = *t; }
    void              AddTrack(tFrameController* t);

    // remove last track in multicontroller
    void              RemoveLastTrack();

    // tFrameController ovverides
    void  Advance(float deltaTime, bool update = true);
    void  SetRelativeSpeed(float rs)                                { relativeSpeed = rs; }
    float GetRelativeSpeed()                                        { return relativeSpeed; }
    void  SetFrame(float f);
    float GetFrame();
    void  SetFrameRange(float minframe, float maxframe);
	void  GetFrameRange(float& MinFrame, float& MaxFrame);

    int LastFrameReached()                                          { return numCycles; }
    void Reset()                                                    { SetFrame(0.0f); }

    void SetLastFrameUpdated( unsigned int frame ) { mLastFrameUpdated = frame; };
    unsigned int GetLastFrameUpdated() const { return mLastFrameUpdated; };

protected:
    tMultiController(tMultiController *c) {};
    ~tMultiController();

    void Update();

    float deltaTime;

    float minFrame;
    float maxFrame;
    int numCycles;

    float time;
    float speed;
    float relativeSpeed;

    p3dCycleMode cycleMode;

    unsigned nTracks;
    TrackInfo* trackInfo;
    tFrameController** tracks;
    
    unsigned int mLastFrameUpdated;
};

class tMultiControllerLoader : public tSimpleChunkHandler
{
 public:
    tMultiControllerLoader();

    tEntity* LoadObject(tChunkFile*, tEntityStore* store);
 protected:
    ~tMultiControllerLoader() {};
};

#endif

