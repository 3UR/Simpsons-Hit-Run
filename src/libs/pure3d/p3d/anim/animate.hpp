/*===========================================================================
    animate.hpp
    27-Mar-96 Created by Neall
    30-Nov-99 version 12

    Copyright (c)1996-1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

#ifndef _ANIMATE_HPP
#define _ANIMATE_HPP

#include <constants/atenum.hpp>
#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/anim/channel.hpp>

class tVectorChannel;
class tAnimation;
class tEventAnimTextLoader;

const int P3D_MAX_BLEND_ANIMS = 5;

enum p3dCycleMode
{
    DEFAULT_CYCLE_MODE,
    FORCE_CYCLIC,
    FORCE_NON_CYCLIC
};

//**************************************************************
// Class tFrameController
//**************************************************************
class tFrameController : public tEntity
{
public:
    tFrameController();
    tFrameController(tFrameController* c);
    virtual ~tFrameController();

    // Advance animation by deltaTime milliseconds
    virtual void Advance(float deltaTime, bool update = true) = 0;

    // adjust animation speed multiplier
    virtual void  SetRelativeSpeed(float rs) = 0;
    virtual float GetRelativeSpeed() = 0;

    // gets the number of frames used by this frame controller
    virtual float GetNumFrames() = 0;
    
    // raw frame control - this overrides Advance(), and SetRelativeSpeed()
    virtual void SetFrame(float f) = 0;

    // returns the current frame of the controller
    virtual float GetFrame() = 0;

    // returns the current frame of the controller
    virtual void SetFrameRange(float minframe, float maxframe) = 0;

    // set the cycling mode
    virtual void SetCycleMode(p3dCycleMode cyclemode) = 0;

    // set the cycling mode
    virtual p3dCycleMode GetCycleMode() = 0;

    // clones frame controller
    virtual tFrameController *Clone(void) = 0;

    // returns the number of cycles that have occured
    virtual int LastFrameReached() = 0;

    // resets the internals
    virtual void Reset(void) = 0;

protected:
    // update object that is being animated
    virtual void Update() = 0;
};

//**************************************************************
// Class tAnimationFrameController
//**************************************************************
class tAnimationFrameController : public tFrameController
{
public:
    tAnimationFrameController();
    tAnimationFrameController(tAnimationFrameController* c);
    virtual ~tAnimationFrameController();
   
    // set the animation
    virtual void SetAnimation(tAnimation* anim) = 0;
    virtual void SetAnimation(tAnimation* anim, float startFrame, float blendFrame) = 0;

    // get the animation
    virtual tAnimation* GetAnimation() = 0;

    // set the valid frame range -1.0f means no frame range, play all frames
    virtual void SetFrameRange(float minframe, float maxframe)                          { minFrame = minframe; maxFrame = maxframe; }

    // set the cycling mode
    virtual void SetCycleMode(p3dCycleMode cyclemode)                                   { cycleMode = cyclemode; }

    // set the cycling mode
    virtual p3dCycleMode GetCycleMode()                                                 { return cycleMode; }

    // adjust animation speed multiplier
    virtual void SetRelativeSpeed(float rs)                                             { relativeSpeed = rs; }

    // return animation speed multiplier
    virtual float GetRelativeSpeed()                                                    { return relativeSpeed; }

    // gets the number of frames used by this frame controller
    virtual float GetNumFrames() = 0;

    // raw frame control - this overrides Advance(), and SetRelativeSpeed()
    virtual void SetFrame(float f) = 0;

    // returns the current frame of the controller
    virtual float GetFrame() = 0;

    // Advance animation by deltaTime milliseconds
    virtual void Advance(float deltaTime, bool update = true) = 0;

    // returns true if a non-cyclic animation has reached its final frame
    virtual int LastFrameReached() = 0;

    // resets the internals
    virtual void Reset(void);

protected:
    // update object that is being animated
    virtual void Update() = 0;

    p3dCycleMode    cycleMode;
    float           minFrame;
    float           maxFrame;
    float           relativeSpeed;
};

//**************************************************************
// Class tSimpleFrameController
//    This frame controller will do most of the frame calculation work
//    for you but it doesn't support blending.
//**************************************************************
class tSimpleFrameController : public tAnimationFrameController
{
public:
    tSimpleFrameController();
    tSimpleFrameController(tSimpleFrameController* c);

    // switch animations
    virtual void SetAnimation(tAnimation* anim);
    virtual void SetAnimation(tAnimation* anim, float startFrame, float blendFrame);
    virtual tAnimation* GetAnimation();

    // gets the number of frames used by this frame controller
    virtual float GetNumFrames();

    // raw frame control - this overrides Advance(), and SetRelativeSpeed()
    virtual void SetFrame(float f);

    // returns the current frame of the controller
    virtual float GetFrame();

    // returns true if a non-cyclic animation has reached its final frame
    virtual int LastFrameReached();                     

    // Advance animation by deltaTime milliseconds
    virtual void Advance(float deltaTime, bool update = true);

    // returns true if the animation is compatible with the controller
    virtual bool ValidateAnimation(tAnimation* anim) = 0;

    // resets the internals, removes any/all animations
    virtual void Reset(void);

protected:
    ~tSimpleFrameController();

    float frame;
    int   numCycles;
    tAnimation* animation;
};

//**************************************************************
// Class tBlendFrameController
//    This frame controller can do a lot of the blending bookkeeping
//**************************************************************
class tBlendFrameController : public tAnimationFrameController
{
public:
    tBlendFrameController();
    tBlendFrameController(tBlendFrameController* c);

    // switch animations
    virtual void SetAnimation(tAnimation* anim);
    virtual void SetAnimation(tAnimation* anim, float startframe);

    // switch animations with blending
    virtual void SetAnimation(tAnimation* anim, float startFrame, float blendFrame);
    virtual tAnimation* GetAnimation();

    // gets the number of frames used by this frame controller
    virtual float GetNumFrames();

    // raw frame control - this overrides Advance(), and SetRelativeSpeed()
    virtual void SetFrame(float f);

    // returns the current frame of the controller
    virtual float GetFrame();

    // Advance animation by deltaTime milliseconds
    virtual void Advance(float deltaTime, bool update = true);

    // returns true if a non-cyclic animation has reached its final frame
    virtual int LastFrameReached();

    // returns true if the animation is compatible with the controller
    virtual bool ValidateAnimation(tAnimation* anim) = 0;

    // resets the internals, removes any/all animations
    virtual void Reset(void);


protected:
    virtual ~tBlendFrameController();

    virtual void Update();
    virtual void UpdateNoBlending() = 0;
    virtual void UpdateWithBlending() = 0;
    virtual void RemoveOldestAnim();

    class PlayInfo
    {
        public:
            PlayInfo();
            ~PlayInfo();

            void SetAnimation(tAnimation* a);
            tAnimation* GetAnimation() { return anim; }
            float GetWeight()          { return t; }
            float GetCurrentFrame()    { return currentFrame; }
            void  Reset(void);
            int   LastFrameReached() { return numCycles; }

        private:
            friend class tBlendFrameController;
            float currentFrame;
            int   numCycles;
            float blendFrames;   // number of frames to blend over
            float ooBlendFrames; // 1/blendFrames - so we can avoid a divide during interpolation
            float accumDelta;    // how many frames have passed since the blend has started
            float t;             // blend weighting factor
            void NullAnimation();
            tAnimation* anim;
    };

    int      nAnim;
    PlayInfo playInfo[P3D_MAX_BLEND_ANIMS];
};

//**************************************************************
// Class tFrameControllerLoader
//    Loads all framecontroller
//**************************************************************
class tFrameControllerLoader : public tSimpleChunkHandler
{
public:
    class Handler : public tRefCounted
    {
        public:
            virtual bool CheckFC(unsigned) = 0;
            virtual tFrameController* LoadFC(char* name, unsigned type, tChunkFile* file, tEntityStore* store) = 0;
    };

    tFrameControllerLoader();

    void AddHandler(Handler*);

    tEntity* LoadObject(tChunkFile* file, tEntityStore* store);

protected:
    ~tFrameControllerLoader();

    virtual bool CheckFC(unsigned);

    virtual tFrameController* LoadFC(char* name, unsigned type, tChunkFile* file, tEntityStore* store);

    int nHandlers;
    Handler** handlers;
};

//**************************************************************
// Class tAnimationMemoryBlock
//**************************************************************
class tAnimationMemoryBlock : public tRefCounted
{
public:
    enum { FOUR_BYTE, EIGHT_BYTE };
    tAnimationMemoryBlock(unsigned int size);
    ~tAnimationMemoryBlock();

    char* Allocate(unsigned int size, int alignment = FOUR_BYTE);

protected:
    tAnimationMemoryBlock();

    unsigned int    blockSize;
    unsigned int    usedBlocks;
    char*           block;

    friend class tAnimationLoader;
};

//**************************************************************
// Class tAnimationGroup
//**************************************************************
class tAnimationGroup
{
public:
    tAnimationGroup ();
    
    void SetName(const char *n)             { name.SetText(n); }
    void SetUID(tUID uid)                   { name.SetUID(uid); }
    void SetNameObject(const tName& n)      { name = n; }
    tUID GetUID(void)  const                { return name.GetUID(); }
    const tName& GetNameObject(void) const  { return name; }

    int GetGroupId() const                  { return groupId; }
    int GetChannelCount() const             { return numChannels; }

    const tIntChannel*            GetIntChannel(unsigned channelCode) const;
    const tFloat1Channel*         GetFloat1Channel(unsigned channelCode) const;
    const tFloat2Channel*         GetFloat2Channel(unsigned channelCode) const;
    const tVectorChannel*         GetVectorChannel(unsigned channelCode) const;
    const tRotationChannel*       GetRotationChannel(unsigned channelCode) const;
    const tStringChannel*         GetStringChannel(unsigned channelCode) const;
    const tEntityChannel*         GetEntityChannel(unsigned channelCode) const;
    const tBoolChannel*           GetBoolChannel(unsigned channelCode) const;
    const tColourChannel*         GetColourChannel(unsigned channelCode) const;
    const tEventChannel*          GetEventChannel(unsigned channelCode) const;   

    const tChannel*               GetChannel(int index) const;

protected:
    virtual ~tAnimationGroup ();

    virtual void CleanUp();

    const tChannel* FindChannel(unsigned channelCode) const;

    tName       name;
    int         groupId;
    int         numChannels;
    tChannel**  channels;
    tAnimationMemoryBlock* memoryBlock;
    
    friend class tAnimation;
    friend class tAnimationLoader;
    friend class tEventAnimTextLoader;
};

//**************************************************************
// Class tAnimation
//**************************************************************
class tAnimation : public tEntity
{
public:
    tAnimation();

    float GetNumFrames(void) const                                             { return numFrames; }
    float GetSpeed(void) const                                                 { return speed; }
    bool GetCyclic(void) const                                                 { return cyclic; }

    void SetSpeed(float s)                                                     { speed = s; }
    void SetCyclic(bool c)                                                     { cyclic = c; }

    unsigned int GetAnimationType() const                                      { return animType; }

    int                     GetNumGroups() const                               { return numGroups; }
    const tAnimationGroup*  GetGroupByIndex(int index) const;
    const tAnimationGroup*  GetGroupById(int id) const;
    const tAnimationGroup*  GetGroupByName(const char* name) const;
    const tAnimationGroup*  GetGroupByUID(tUID uid) const;

    int FindGroupIndexById(int id) const;
    int FindGroupIndexByName(const char* name) const;
    int FindGroupIndexByUID(tUID uid) const;
  
    // Given an arbitrary frame number, clamp or wrap the frame to the range specified
    float MakeValidFrame(float frame, float rangeMin = -1.0f, float rangeMax = -1.0f, p3dCycleMode mode = DEFAULT_CYCLE_MODE) const;

protected:
    virtual ~tAnimation();

    void SortAnimationGroups();

    float numFrames;
    float speed;
    bool  cyclic;

    //
    // New Architecture
    //
    unsigned int animType;
    int numGroups;
    tAnimationGroup** groups;
    tAnimationMemoryBlock* memoryBlock;
    
    int sortOrder; // 0 = None, 1 = UID, 2 = GroupID

    friend class tAnimationLoader;
    friend class tEventAnimTextLoader;
};

//**************************************************************
// Class tAnimationLoader
//**************************************************************
class tAnimationLoader : public tSimpleChunkHandler
{
public:
    tAnimationLoader();
    tEntity* LoadObject(tChunkFile* file, tEntityStore* store);

    // 0 = None, 1 = UID, 2 = GroupID
    void SetSortOrder(int order)        { sortOrder = order; }
    int  GetSortOrder()                 { return sortOrder; }

protected:
    ~tAnimationLoader() {};

    tAnimationGroup* LoadAnimationGroup(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);

    int sortOrder; // 0 = None, 1 = UID, 2 = GroupID
};

#endif //ANIMATE_HPP

