//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _SEQUENCER_HPP
#define _SEQUENCER_HPP



#include <p3d/entity.hpp>
#include <p3d/anim/animate.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/anim/event.hpp>
#include <p3d/array.hpp>

class tAnimation;
class tAnimationGroup;
class tEventAnimation;
class tEventChannel;
class tEventController;
class tGenericEvent;
class tSequencerState;
class TextDataParser;
class tSubPartState;
class tSequencerLoader;

//**************************************************
// Class tEventOriginSequencer
//**************************************************
class tEventOriginSequencer : public tRefCounted, public tEventOrigin
{
public:
    virtual void AddRef(void) { tRefCounted::AddRef(); }
    virtual void Release(void) { tRefCounted::Release(); }
    virtual unsigned Origin(void) { return EVENT_ORIGIN_SEQUENCER; }
};

//**************************************************
// Class tStateSequencerCommand
// Used by the sequencer to do an action
// Passed from the sequencer state to the sequencer in response to an update
//**************************************************
class tStateSequencerCommand
{
public:
    tStateSequencerCommand(void);
    virtual ~tStateSequencerCommand();

    // Stuffed by the transition engine
    virtual void SetStartFrame(float f) { mStartFrame = f; }

    void  SetSequencerState(tSequencerState *stateParam);
    tSequencerState* GetSequencerState(void){return(mStateParam);}

    enum Commands 
    { 
        Invalid, 
        Start, 
        Replace, 
        Kill, 
        BlendIn, 
        BlendOut, 
        CrossFade, 
        UseDefault 
    } mCommand;

    float mTimeParam;
    float mStartFrame;

protected:
    tSequencerState *mStateParam; // weak reference, don't addref
    friend class tSequencerLoader;
};

//***************************************************
// Class tStateTransition
// Used by the sequencer state to determine when to tell the 
// sequencer to do an action
//***************************************************
class tStateTransition : public tStateSequencerCommand
{
public:
    tStateTransition(void);
    virtual ~tStateTransition();

    // Should I wait for a specific frame before transitioning?
    float mOutFrameMin;
    float mOutFrameRange;
    float mInFrame;
    float mInFrameRangeScale;
    tName mName;
    int   mTransitionDirection;
};

//***************************************************
// Class tSequencerState
//***************************************************
class tSequencerState : public tEntity
{
public:
    tSequencerState(void);
    tSequencerState(tSequencerState *src);

    tStateSequencerCommand *Update(float deltatime, tSubPartState *sub_part);

    // Figures out what the transition should be
    // Called on the state that IS active.
    // Should be called before the Update() that frame.
    tStateSequencerCommand *TransitionOn(tUID transitionNameUID);
    tStateSequencerCommand *TransitionTo(tSequencerState *new_state, bool queryTransition = false);

    // This returns the default transition INTO this state.
    // Called on the state that WILL be activated.
    tStateSequencerCommand *DefaultTransition(void);

    void ResetLoopCount(void) { mLoopCount = 1; }

    float FrameRangeMin(void) { return mFrameRangeMin; }
    float FrameRangeMax(void) { return mFrameRangeMax; }

    tAnimation      *ObjectAnimation(void) { return mObjectAnimation; }
    tAnimation      *EventAnimation(void)  { return mEventAnimation; }

    // Unique partial hierarchy ID
    int SubPart(void) { return mSubPart; }

    // used by the GUI tool
    int TransitionCount(void) { return mTransitionCount; }
    tStateTransition *Transition(int t) { return &mTransitions[t]; }

    // State based speed
    void  SetRelativeSpeed(float rs){mStateAnimationSpeed = rs;}
    float GetRelativeSpeed() { return mStateAnimationSpeed; }

    // Reset user specified transition
    void ResetLatchedTransition(void){mOutDestUser = NULL;}

private: 
    ~tSequencerState();

    tAnimation* mObjectAnimation;
    tAnimation* mEventAnimation;
    float       mFrameRangeMin;
    float       mFrameRangeMax;
    float       mCurrentFrame;

    // When there is no specific way to get to this animation,
    // Use this.  Only a limited set of sequencer commands 
    // make sense for the default.  
    tStateTransition mDefaultTransition;

    // Which part of the hierarchy does this anim animate.  Zero is whole thing.
    // This number needs to be set in the tool/exporter.  The numbers don't 
    // mean anything in particular, just a way to differentiate between animations
    // that can run concurrently on the same hierarchy. (Split body)
    int mSubPart;

    int mTransitionCount; 
    tStateTransition* mTransitions;

    // How does this state loop?
    enum LoopType 
    { 
        Loop, 
        LoopNHold, 
        LoopNOut, 
        PlayKill 
    } mLoopType;

    int mMaxLoopCount;
    int mLoopCount; 

    tStateTransition* mOutDest;
    tStateTransition* mOutDestUser;
    
    // This gets scaled by the global speed
    float mStateAnimationSpeed;

    friend class tSequencerLoader;
};

//*******************************************************
// Class tSubPartState
// Sequencer internal data needed per sub part animation
//*******************************************************
class tSubPartState
{
public:
    tSubPartState(void);
    ~tSubPartState();

    void SetState(tSequencerState *new_state, float baseSpeed, float start_frame = 0.0F, float blendframes = 0.0F);

    void SetAnimationTarget(tBlendFrameController *target);
    void SetEventController(tEventController *eventcontroller);

    void SetEventRecipient(tEventRecipient *new_recip);
    void SetEventOrigin(tEventOrigin *new_origin);

    tSequencerState       *State(void)            { return mCurrentState; }
    tBlendFrameController *ObjectController(void) { return mObjectController; }
    tEventController      *EventController(void)  { return mEventController; }

    void SetRelativeSpeed(float s);
    void Kill(void);

    tStateSequencerCommand *Update(float deltatime);

private:
    tSequencerState*        mCurrentState;
    tBlendFrameController*  mObjectController;
    tEventController*       mEventController;

    friend class tSequencerLoader;
};

//*******************************************************
//  Class tSequencer
//*******************************************************
class tSequencer : public tEntity
{
public:
    tSequencer();

    // Clone the current sequencer but NOTE The animation target is NOT copied!
    tSequencer* Clone(void);

    // Returns true if the state was changed
    bool SetState(char *state, bool kill_sub_states = false);
    bool SetState(tUID state,  bool kill_sub_states = false);

    // Used to terminate a sub part anim
    // returns true if successful
    // NOTE: this will NEVER kill a whole body animation
    bool KillState(tUID state);
    bool KillAllSubStates(void);

    // Does what it says 
    bool Update(float deltatime);

    // Sets the relative speed of animation on this sequencer
    // returns the previous speed
    // 1.0 == normal speed
    // 0.5 == half speed
    void  SetRelativeSpeed(float rs);
    float GetRelativeSpeed() { return mSequencerSpeed; }

    // Used to set the thing that is animated
    void SetAnimationTarget(tEntity *anim_target, bool clone = true);

    void SetEventRecipient(tEventRecipient *new_recip);
    void SetEventOrigin(tEventOrigin *new_origin);

    // Used by the GUI tool
    int StateCount(void) { return mStateCount; }
    tSequencerState *State(int s) { return mStates[s]; }

    // used by the GUI tool... Perhaps this should be re-thought
    void PrintStates(char* statesString, unsigned int* size) const;
    void PrintTransitions( char* transitionsString, unsigned int* size ) const;

    //  Returns the name of the current state of the requested subpart
    //  Subpart = 0 means the whole character
    #ifdef TENTITY_USE_NAMES
    //
    // This is here for debuging only... 
    // Use tUID GetCurrentSubPartState() for game code
    //
    inline const char* GetCurrentSubPartState(int subPartIndex){return(mSubParts[subPartIndex].State()->GetName());}
    #endif

    inline tUID GetCurrentSubPartStateUID(int subPartIndex){return(mSubParts[subPartIndex].State()->GetUID());}

    void TransitionOn(char* transitionName){ mSubParts[0].State()->TransitionOn(tEntity::MakeUID(transitionName)); }

private:
  ~tSequencer();

    tSequencerState *FindState(tUID state);
    void HandleCommand(tStateSequencerCommand *cmd, tSubPartState *subpart);

    int                     mStateCount;
    tPtrArray<tSequencerState*>  mStates;

    int                     mSubPartCount;
    tArray<tSubPartState>    mSubParts;

    tEntity*                mAnimationTarget;

    float                   mSequencerSpeed;

    tEventRecipient*        mEventRecipient;
    tEventOriginSequencer*  mEventOrigin;
        
    friend class tSequencerLoader;
};

//*******************************************************
// Class tSequencerLoader
//*******************************************************
class tSequencerLoader : public tChunkHandler
{
public:
    tSequencerLoader();
    virtual ~tSequencerLoader();


    tLoadStatus Load(tFile *file, tEntityStore *store);

    virtual tLoadStatus Load(tChunkFile*, tEntityStore *store);
    bool CheckChunkID(unsigned);
	unsigned GetChunkID() { return 0; }

private:
    tLoadStatus  ParseScript(TextDataParser *parser);
    tSequencer  *ParseSequencer(TextDataParser *parser);

    bool ParseStates(TextDataParser *parser, tSequencer *seq);
    bool ParseState(TextDataParser *parser, tSequencerState *dest);

    bool ParseTransitions(TextDataParser *parser, tSequencerState *state);
    bool ParseTransition(TextDataParser *parser, tStateTransition *dest);
    bool FixupTransitions(tSequencer *seq);
    bool BuildSubParts(tSequencer *seq);

    tSequencerState *FindState(tSequencer *seq, char* name);

    tEntityStore *mStore;

    tBlendFrameController *mAnimationTarget;

};


//*******************************************************
// Class tSEQFileHandler
//*******************************************************
class tSEQFileHandler : public tFileHandler
{
public :
    tSEQFileHandler();
    virtual bool CheckExtension(char* file);
    virtual tLoadStatus Load(tFile* file, tEntityStore* store);

protected:
    ~tSEQFileHandler();
    tSequencerLoader* sequencerLoader;
    tEventAnimTextLoader* eventAnimLoader; 
};

#endif // _SEQUENCER_HPP

