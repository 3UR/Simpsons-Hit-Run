//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================



#include <p3d/anim/sequencer.hpp>
#include <p3d/utility.hpp>
#include <constants/chunks.h>
#include <p3d/error.hpp>
#include <p3d/file.hpp>
#include <p3d/anim/event.hpp>
#include <p3d/anim/eventanimation.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/textdataparser.hpp>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//**************************************************
// Class tStateSequencerCommand
//**************************************************
tStateSequencerCommand::tStateSequencerCommand(void) :
    mCommand(Invalid),
    mTimeParam(0.0F), 
    mStartFrame(0),
    mStateParam(NULL)
{ 
}

//--------------------------------------------------
tStateSequencerCommand::~tStateSequencerCommand()
{
}

void tStateSequencerCommand::SetSequencerState(tSequencerState *stateParam)
{
    mStateParam = stateParam; // weak reference
}


//***************************************************
// Class tStateTransition
//***************************************************
tStateTransition::tStateTransition(void) : 
    tStateSequencerCommand(),
    mOutFrameMin(-1.0F), 
    mOutFrameRange(0.0F), 
    mInFrame(0.0F), 
    mInFrameRangeScale(0.0F)
{ 
}

//--------------------------------------------------
tStateTransition::~tStateTransition() 
{
}


//***************************************************
// Class tSequencerState
//***************************************************
tSequencerState::tSequencerState(void) :
    mObjectAnimation(NULL),
    mEventAnimation(NULL),
    mFrameRangeMin(-1.0F),
    mFrameRangeMax(-1.0F),
    mCurrentFrame(0.0F),
    mSubPart(0),
    mTransitionCount(0),
    mTransitions(NULL),
    mLoopType(Loop),
    mMaxLoopCount(0),
    mOutDest(NULL),
    mOutDestUser(NULL),
    mStateAnimationSpeed(1.0f)
{
}

//--------------------------------------------------
tSequencerState::~tSequencerState()
{
    tRefCounted::Release(mObjectAnimation);
    tRefCounted::Release(mEventAnimation);
    delete [] mTransitions;
}

//***************************************************
//
// Class tSequencerState
// Update
//
tStateSequencerCommand *tSequencerState::Update(float deltatime, tSubPartState *sub_part)
{

    tBlendFrameController *ocontroller = sub_part->ObjectController();
    tEventController      *econtroller = sub_part->EventController();

    bool need_cyclic = false;
    bool check_for_out = true;//false;

    switch (mLoopType)
    {
        case Loop:
            need_cyclic = true;
            break;

        case LoopNHold:
        case LoopNOut:
            if (mLoopCount < mMaxLoopCount) need_cyclic = true;
            else 
            {
                need_cyclic = false;
                if (mLoopType == LoopNOut) check_for_out = true;
            }
            break;

        case PlayKill:
            need_cyclic = false;
            break;
    }

    //
    // We might advance
    //
    float prev_frame = mCurrentFrame;

    if ((ocontroller != NULL) && (ocontroller->GetAnimation() != NULL))
    {
        tAnimation *anim = ocontroller->GetAnimation();

        bool cyclic = anim->GetCyclic();
        anim->SetCyclic(need_cyclic);

        
        ocontroller->Advance(deltatime);
        mCurrentFrame = ocontroller->GetFrame();
        if (mCurrentFrame < prev_frame) ++mLoopCount;
        anim->SetCyclic(cyclic);
    }

    if ((econtroller != NULL) && (mEventAnimation != NULL)) 
    {
        tAnimation *anim = econtroller->GetAnimation();

        bool cyclic = anim->GetCyclic();

        anim->SetCyclic(need_cyclic);
        econtroller->Advance(deltatime);
        anim->SetCyclic(cyclic);
    }

    //  See if I should transition anywhere after 
    if (check_for_out && ocontroller && (ocontroller->GetAnimation() != NULL))
    {
        
        if(mOutDestUser && mOutDestUser->GetSequencerState() != NULL)
        {
            
            //
            // Transition right away
            //
            if(mOutDestUser->mCommand == tStateSequencerCommand::Start)
            {
                tStateSequencerCommand *newTransition = mOutDestUser;
                return newTransition;
            }

            //
            // Now test for a Valid Transition Range for a User latched transition
            //
            if((mCurrentFrame >= mOutDestUser->mOutFrameMin)
                && 
                (mCurrentFrame <= (mOutDestUser->mOutFrameMin + mOutDestUser->mOutFrameRange))
            )
            {
                tStateSequencerCommand *newTransition = mOutDestUser;
                return newTransition;
            }

// Better Boundary conditions!!


            //
            // Normal case... simple boundary crossing
            //
            if(prev_frame < mCurrentFrame)
            {
                if((prev_frame < mOutDestUser->mOutFrameMin)
                    &&
                    (mCurrentFrame >= mOutDestUser->mOutFrameMin)
                    )
                {
                        tStateSequencerCommand *newTransition = mOutDestUser;
                        return newTransition;
                }
            }
            else
            {
                //
                // Looping case
                //
                if((mCurrentFrame >= mOutDestUser->mOutFrameMin)
                    ||
                    (prev_frame <= mOutDestUser->mOutFrameMin)
                    )
                {
                    tStateSequencerCommand *newTransition = mOutDestUser;
                    return newTransition;
                }
            }
        }

        //
        // Now test for a Valid Transition Range
        //
        if(mOutDest != NULL)
        {
            if((mCurrentFrame >= mOutDest->mOutFrameMin)
                && 
                (mCurrentFrame <= (mOutDest->mOutFrameMin + mOutDest->mOutFrameRange))
            )
            {
                return mOutDest;
            }
        }
    }

    return NULL;
}


//--------------------------------------------------
tStateSequencerCommand *tSequencerState::TransitionTo(tSequencerState *new_state, bool queryTransition)
{
    if (new_state == NULL) return NULL;

    int a;
    for (a = 0; a < this->mTransitionCount; a++)
    {
        if((this->mTransitions[a].GetSequencerState() == new_state) && (mTransitions[a].mTransitionDirection == 1))
        {
            float minframe = this->mTransitions[a].mOutFrameMin;

            // No frame range limit
            if (this->mTransitions[a].mOutFrameMin < 0.0F) 
            {
                // Compute where the new state's animation should start
                float framediff  = mCurrentFrame - minframe;
                float startframe = this->mTransitions[a].mInFrame + (this->mTransitions[a].mInFrameRangeScale * framediff);
                this->mTransitions[a].SetStartFrame(startframe);

                return &(this->mTransitions[a]);
            }
            
            float maxframe = minframe + this->mTransitions[a].mOutFrameRange;

            if( (mCurrentFrame < minframe) || ( mLoopType == Loop ) )
            {
                // Not there yet but there is a Valid Transition
                if(queryTransition)
                {
                    return(&(this->mTransitions[a]));
                }
                else
                {
                    mOutDestUser = &mTransitions[a];
                    return(new_state->DefaultTransition()); // this is a differed evaluation
                }
            }
            else 
            {
                continue;
            }

            if (mCurrentFrame > maxframe) 
            {
                continue;
            }

            // Compute where the new state's animation should start
            float framediff  = mCurrentFrame - minframe;
            float startframe = this->mTransitions[a].mInFrame + (this->mTransitions[a].mInFrameRangeScale * framediff);
            this->mTransitions[a].SetStartFrame(startframe);

            return &(this->mTransitions[a]);
        }
    }

    // old way
    for (a = 0; a < new_state->mTransitionCount; a++)
    {
        if((new_state->mTransitions[a].GetSequencerState() == this) && (new_state->mTransitions[a].mTransitionDirection == 0))
        {

            float minframe = new_state->mTransitions[a].mOutFrameMin;

            // No frame range limit
            if (new_state->mTransitions[a].mOutFrameMin < 0.0F) 
            {
                // Compute where the new state's animation should start
                float framediff  = mCurrentFrame - minframe;
                float startframe = new_state->mTransitions[a].mInFrame + (new_state->mTransitions[a].mInFrameRangeScale * framediff);
                new_state->mTransitions[a].SetStartFrame(startframe);

                return &(new_state->mTransitions[a]);
            }
            
            float maxframe = minframe + new_state->mTransitions[a].mOutFrameRange;

            if( (mCurrentFrame < minframe) || ( mLoopType == Loop ) )
            {
                //
                // Not there yet but there is a Valid Transition
                //
                this->mOutDestUser = &(new_state->mTransitions[a]);
                this->mOutDestUser->SetSequencerState(new_state);
                return(new_state->DefaultTransition()); // this is a differed evaluation
                //continue;
            }
            else {continue;}
            if (mCurrentFrame > maxframe) continue;

            // Compute where the new state's animation should start
            float framediff  = mCurrentFrame - minframe;
            float startframe = new_state->mTransitions[a].mInFrame + (new_state->mTransitions[a].mInFrameRangeScale * framediff);
            new_state->mTransitions[a].SetStartFrame(startframe);

            return &(new_state->mTransitions[a]);
        }
    }

    // Here we have to make sure that this state is valid to Transition to... 

    // Always return the default even if it is invalid, It will be sorted out later
    return this->DefaultTransition();
}

//--------------------------------------------------
tStateSequencerCommand *tSequencerState::TransitionOn(tUID transitionNameUID)
{

    if (transitionNameUID == static_cast< tUID >( 0 ) ) return NULL;

    int a;
    for (a = 0; a < this->mTransitionCount; a++)
    {
        if((mTransitions[a].mName == transitionNameUID) && (mTransitions[a].mTransitionDirection == 1))
        {
            float minframe = mTransitions[a].mOutFrameMin;

            // No frame range limit
            if (this->mTransitions[a].mOutFrameMin < 0.0F) 
            {
                // Compute where the new state's animation should start
                float framediff  = mCurrentFrame - minframe;
                float startframe = mTransitions[a].mInFrame + (mTransitions[a].mInFrameRangeScale * framediff);
                mTransitions[a].SetStartFrame(startframe);

                // TransitionOn should work
                mOutDestUser = &mTransitions[a];

                return &(mTransitions[a]);
            }
            
            float maxframe = minframe + mTransitions[a].mOutFrameRange;

            if( (mCurrentFrame < minframe) || ( mLoopType == Loop ) )
            {
                // Not there yet but there is a Valid Transition
                mOutDestUser = &mTransitions[a];
                return(NULL);//&(this->mOutDestUser)); // this is a differed evaluation
                //continue;
            }
            else {continue;}
            if (mCurrentFrame > maxframe) continue;

            // Compute where the new state's animation should start
            float framediff  = mCurrentFrame - minframe;
            float startframe = mTransitions[a].mInFrame + (mTransitions[a].mInFrameRangeScale * framediff);
            mTransitions[a].SetStartFrame(startframe);

            return &(mTransitions[a]);
        }
    }

    // Here we have to make sure that this state is valid to Transition to... 

    // Always return the default even if it is invalid, It will be sorted out later
    return this->DefaultTransition();
}

//--------------------------------------------------
tStateSequencerCommand *tSequencerState::DefaultTransition(void)
{
    return &mDefaultTransition;
}

//
//  Class tSubPartState
//  Default contructor
//
tSubPartState::tSubPartState(void) :
    mCurrentState(NULL),
    mObjectController(NULL),
    mEventController(NULL) 
{
}


//*******************************************************
//  Class tSubPartState
//*******************************************************
tSubPartState::~tSubPartState()
{
    tRefCounted::Release(mCurrentState);
    tRefCounted::Release(mObjectController);
    tRefCounted::Release(mEventController);
}

//--------------------------------------------------
void tSubPartState::SetState(tSequencerState *new_state, float baseSpeed, float start_frame, float blendframes)
{
    tRefCounted::Assign(mCurrentState, new_state);

    mCurrentState->ResetLoopCount();

    tAnimation* oanim = new_state->ObjectAnimation();
    tAnimation* eanim = new_state->EventAnimation();

    float min = new_state->FrameRangeMin();
    float max = new_state->FrameRangeMax()+1; // since this is supposed to be inclusive!

    if ((oanim != NULL) && (mObjectController != NULL))
    {
        mObjectController->SetRelativeSpeed( baseSpeed * new_state->GetRelativeSpeed() );
        mObjectController->SetFrameRange(min, max);
        mObjectController->SetAnimation(oanim, start_frame, blendframes);
    }
    else if (mObjectController != NULL)
    {
        mObjectController->Reset();
    }

    if ((eanim != NULL) && (mEventController != NULL))
    {
        mEventController->SetRelativeSpeed( baseSpeed * new_state->GetRelativeSpeed() );
        mEventController->SetFrameRange(min, max);
        mEventController->SetAnimation(eanim, start_frame, blendframes);
    }
    else if (mEventController != NULL)
    {
        mEventController->Reset();
    }

    // Reset any user specified transition
    this->mCurrentState->ResetLatchedTransition();
}

//--------------------------------------------------
void tSubPartState::SetEventController(tEventController *eventcontroller)
{
    tRefCounted::Assign(mEventController, eventcontroller);
}

//--------------------------------------------------
void tSubPartState::SetRelativeSpeed(float s)
{
    if (mObjectController != NULL) 
    {
        mObjectController->SetRelativeSpeed(s);
    }
    if (mEventController != NULL)  
    {
        mEventController->SetRelativeSpeed(s);
    }
}

//--------------------------------------------------
void tSubPartState::SetEventRecipient(tEventRecipient *new_recip)
{
    if (mEventController != NULL)  
    {
        mEventController->SetRecipient(new_recip);
    }
}

//--------------------------------------------------
void tSubPartState::SetEventOrigin(tEventOrigin *new_origin)
{
    if (mEventController != NULL)
    {
        mEventController->SetOrigin(new_origin);
    }
}

//--------------------------------------------------
void tSubPartState::SetAnimationTarget(tBlendFrameController *fc)
{
    tRefCounted::Assign(mObjectController, fc);
}

//--------------------------------------------------
void tSubPartState::Kill(void)
{
    if(mCurrentState)
    {  
        mCurrentState->Release();
        mCurrentState = NULL;
    }
    if (mObjectController != NULL)
    {
        mObjectController->Reset();
    }
    if (mEventController != NULL)
    {
        mEventController->Reset();
    }
}

//--------------------------------------------------
tStateSequencerCommand *tSubPartState::Update(float deltatime)
{
    if (mCurrentState == NULL) 
    {
        return NULL;
    }
    return mCurrentState->Update(deltatime, this);
}


//*******************************************************
//  Class tSequencer
//*******************************************************
tSequencer::tSequencer(void) :
    mStateCount(0),
    mStates(0),
    mSubPartCount(0),
    mSubParts(0),
    mAnimationTarget(NULL),
    mSequencerSpeed(1.0F),
    mEventRecipient(NULL),
    mEventOrigin(NULL)
{
    mEventOrigin = new tEventOriginSequencer;
    mEventOrigin->AddRef();
}

//--------------------------------------------------
tSequencer::~tSequencer()
{
    int i=0;

    for(i=0;i<mStateCount;i++)
    {
        tRefCounted::Release(mStates[i]);
    }

    tRefCounted::Release(mAnimationTarget);
    tRefCounted::Release(mEventRecipient);
    tRefCounted::Release(mEventOrigin);
}

//--------------------------------------------------
//  NOTE:  The animation target is NOT copied
tSequencer *tSequencer::Clone(void)
{
    tSequencer *seq = new tSequencer;

    seq->mAnimationTarget = NULL;
    seq->mSequencerSpeed  = mSequencerSpeed;
    seq->mEventRecipient  = mEventRecipient;  
    
    if(seq->mEventRecipient)
    {
        seq->mEventRecipient->AddRef();
    }

    seq->mStateCount = this->mStateCount;
    seq->mStates.SetSize( seq->mStateCount );

    int a;
    for (a = 0; a < mStateCount; a++)
    {
        seq->mStates[a] = this->mStates[a];      
        seq->mStates[a]->AddRef();
    }

    seq->mSubPartCount = this->mSubPartCount;
    seq->mSubParts.SetSize( seq->mSubPartCount );

    // This create new tEventControllers so Cloned sequencers throw events. 
    for (a = 0; a < seq->mSubPartCount; a++) 
    { 
        seq->mSubParts[a].SetEventController(new tEventController); 
        seq->mSubParts[a].EventController()->SetOrigin(seq->mEventOrigin); 
    } 

    return seq;
}

//--------------------------------------------------
//  If kill_sub_states is true, all sub part animations will stop when this state is set
//  Returns true if the state was changed
bool tSequencer::SetState(char *state, bool kill_sub_states)
{
    return SetState(tEntity::MakeUID(state), kill_sub_states);
}

//--------------------------------------------------
//  If kill_sub_states is true, all sub part animations will stop when this state is set
//  Returns true if the state was changed
bool tSequencer::SetState(tUID state, bool kill_sub_states)
{
    // clean up any sub parts if requested
    if (kill_sub_states) 
    {
        KillAllSubStates();
    }

    // find the state 
    tSequencerState *new_state = FindState(state);
    if (new_state == NULL) 
    {
        return false;
    }

    // get the subpart this applys to
    tSubPartState *subpart = &mSubParts[new_state->SubPart()];

    // get the trnasition command    
    if (subpart->State() != NULL)
    {
        tStateSequencerCommand *cmd = subpart->State()->TransitionTo(new_state);

        // make a copy of the command and patch the real dest
        if (cmd != NULL)
        {
            if (cmd->mCommand == tStateSequencerCommand::Invalid) 
            {
                return false;
            }

            tStateSequencerCommand tmp;
            tmp.mCommand    = cmd->mCommand;
            tmp.SetSequencerState(new_state);
            tmp.mTimeParam  = cmd->mTimeParam;
            tmp.mStartFrame = cmd->mStartFrame;
            // do it
            HandleCommand(&tmp, subpart);
        }
        return true;
    } 
    else
    {
        subpart->SetState(new_state, mSequencerSpeed);
    }

    return false;
}

//--------------------------------------------------
// Used to terminate a sub part anim
// returns true if successful
// NOTE: this will NEVER kill a whole body animation
bool tSequencer::KillState(tUID state)
{
    int a;
    for (a = 1; a < mSubPartCount; a++)
    {
        if (mSubParts[a].State() == NULL) continue;
        
        if (state == mSubParts[a].State()->GetUID())
        {
            mSubParts[a].Kill();
            return true;
        }
    }

    return false;
}

//--------------------------------------------------
bool tSequencer::KillAllSubStates(void)
{  
    int a;
    for (a = 1; a < mSubPartCount; a++) 
    {
        mSubParts[a].Kill();
    }

    return true;
}

//--------------------------------------------------
bool tSequencer::Update(float deltatime)
{
    bool stateChanged = false;
    int a;
    for (a = 0; a < mSubPartCount; a++)
    {
        tStateSequencerCommand *cmd;
        cmd = mSubParts[a].Update(deltatime);
        
        if (cmd == NULL) continue;
        
        stateChanged = true;
        HandleCommand(cmd, &mSubParts[a]);
    }

    return stateChanged;
}

//--------------------------------------------------
// Sets the relative speed of animation on this sequencer
// returns the previous speed
// 1.0 == normal speed
// 0.5 == half speed
void tSequencer::SetRelativeSpeed(float rs)
{
    // Don't let a base speed ruin your day
    if (rs <= 0.0F) 
    {
        return;
    }

    mSequencerSpeed = rs;

// now set per state change basis
/*
    int a;
    for (a = 0; a < mSubPartCount; a++)
    {
        mSubParts[a].SetRelativeSpeed(mSequencerSpeed);
    }
*/
}

//--------------------------------------------------
//  Used to set the thing that is animated
void tSequencer::SetAnimationTarget(tEntity *anim_target, bool clone)
{
    tRefCounted::Assign(mAnimationTarget, anim_target);

    tBlendFrameController *fc = dynamic_cast<tBlendFrameController*>(mAnimationTarget);
    
    int a;
    for (a = 0; a < mSubPartCount; a++)
    {
        if (clone && (fc != NULL)) 
        {
            mSubParts[a].SetAnimationTarget((tBlendFrameController *)fc->Clone());
        }
        else
        {
            mSubParts[a].SetAnimationTarget(fc);
        }
    }
}

//--------------------------------------------------
//  Set Event Recipient
//  Used to set the thing that is animated
void tSequencer::SetEventRecipient(tEventRecipient *new_recip)
{
    tRefCounted::Assign(mEventRecipient, new_recip);
    
    int a;
    for (a = 0; a < mSubPartCount; a++)
    {
        mSubParts[a].SetEventRecipient(mEventRecipient);
    }
}


//--------------------------------------------------
//  Sets where the event comes from
//  Pass in NULL to set the origin as the sequencer default
void tSequencer::SetEventOrigin(tEventOrigin *new_origin)
{
    // set default origin if requested
    if (new_origin == NULL) 
    {
        new_origin = mEventOrigin;
    }

    new_origin->AddRef();

    int a;
    for (a = 0; a < mSubPartCount; a++)
    {
        mSubParts[a].SetEventOrigin(new_origin);
    }

    new_origin->Release();
}

//--------------------------------------------------
tSequencerState *tSequencer::FindState(tUID state)
{

    int a;
    for (a = 0; a < mStateCount; a++)
    {
        if (state == mStates[a]->GetUID()) 
        {
            return mStates[a];
        }
    }

    return NULL;
}

//--------------------------------------------------
// Handle Command
// updates the subpart state with the contents of the command
void tSequencer::HandleCommand(tStateSequencerCommand *cmd, tSubPartState *subpart)
{
    if (cmd == NULL) return;
    if (subpart == NULL) return;

    // If the command is to use the default, fetch it before we act
    if (cmd->mCommand == tStateSequencerCommand::UseDefault)
    {
        cmd = cmd->GetSequencerState()->DefaultTransition();
    }

    // Do the work
    switch (cmd->mCommand)
    {

        case tStateSequencerCommand::Start:
            subpart->SetState(cmd->GetSequencerState(), mSequencerSpeed, cmd->mStartFrame);
            break;

        case tStateSequencerCommand::Replace:
            subpart->Kill();
            subpart->SetState(cmd->GetSequencerState(), mSequencerSpeed, cmd->mStartFrame);
            break;

        case tStateSequencerCommand::Kill:
            subpart->Kill();
            break;

        case tStateSequencerCommand::BlendIn:
            subpart->SetState(cmd->GetSequencerState(), mSequencerSpeed, cmd->mStartFrame, cmd->mTimeParam);
            break;

        case tStateSequencerCommand::BlendOut:
            break;

        case tStateSequencerCommand::CrossFade:
            subpart->SetState(cmd->GetSequencerState(), mSequencerSpeed, cmd->mStartFrame, cmd->mTimeParam);
            break;
        
        default:
            break;
    }
}

//--------------------------------------------------
// fills a string with the name of each state in the sequence, sets size
// to the number of states written.
void tSequencer::PrintStates(char* statesString, unsigned int* size) const
{
#ifdef TENTITY_USE_NAMES
    if ( ( *size == 0 ) || ( mStateCount == 0 ) )
    {
        statesString[0] = 0;
        return;
    }

    unsigned int total = 0;
    int index = 0;
    char* name;

    name = mStates[index]->GetName();

    if(name)
    {
        while (( index < mStateCount ) && ( (total + strlen(name) + 1) <= *size ))
        {
            memcpy(&statesString[total], name, strlen(name));
            total += strlen(name);

            statesString[total] = '\t';
            total++;

            index++;

            if ( mStates[index] )
            {
                name = mStates[index]->GetName();
            }
        }
    }

    *size = index;
#else
    *size = 0;
    statesString[0] = 0;
#endif

    return;
}

//--------------------------------------------------
// fills a string with the name of each transition in the sequence, sets size
// to the number of states written.
void tSequencer::PrintTransitions( char* transitionsString, unsigned int* size ) const
{
#ifdef P3D_ALLOW_ENTITY_GETNAME

    if ( ( *size == 0 ) || ( mStateCount == 0 ) )
    {
        transitionsString[0] = 0;
        return;
    }

    unsigned int total = 0;
    int index = 0;
    int transitions = 0;

    //Note:  This is only for non-parts-based animations
    tSequencerState* currState = mSubParts[0].State();

    tStateSequencerCommand* transitionCmd;
    
    while( index < mStateCount )
    {
        if( (transitionCmd = currState->TransitionTo( mStates[index], true )) )
        {      
            tStateTransition* tran = dynamic_cast<tStateTransition*>(transitionCmd);
        
            const char* transitionName = NULL;
            int   transitionNameLen = 0;

            if(tran)
            {
                transitionName = tran->mName.GetText();
                if(transitionName)
                {
                    transitionNameLen = strlen(transitionName);
                }
            }

            //
            // Is there space to write it?
            //
            if((total + transitionNameLen + 1) <= *size )
            {
                if(transitionNameLen)
                {
                    memcpy(&transitionsString[total], transitionName,transitionNameLen);
                    total += transitionNameLen;
                }

                transitionsString[total] = '\t';
                total++;

                transitions++;         
            }
        }// is there a transition?
        index++;
    }

    *size = transitions;

#else

   *size = 0;
   transitionsString[0] = 0;

#endif
}

//*******************************************************
// Class tSequencerLoader
//*******************************************************
tSequencerLoader::tSequencerLoader() :
    mStore(NULL),
    mAnimationTarget(NULL)
{
}

//--------------------------------------------------
tSequencerLoader::~tSequencerLoader()
{
    tRefCounted::Release(mStore);
    tRefCounted::Release(mAnimationTarget);
}

//--------------------------------------------------
tLoadStatus tSequencerLoader::Load(tChunkFile*, tEntityStore *store)
{
    return LOAD_OK;
}

//--------------------------------------------------
tLoadStatus tSequencerLoader::Load(tFile *file, tEntityStore *store)
{
    if (file == NULL)
    {
        tRefCounted::Release(mStore);
        mStore = NULL;
        return LOAD_ERROR;
    }

    P3DASSERT(store);
    tRefCounted::Assign(mStore,store);
    tRefCounted::Release(mAnimationTarget);
    mAnimationTarget = NULL;

    TextDataParser *parser = new TextDataParser(file);

    tLoadStatus result = ParseScript(parser);

    delete parser;

    tRefCounted::Release(mStore);

    return result;
}

//--------------------------------------------------
bool tSequencerLoader::CheckChunkID(unsigned)
{
    return true;
}

//--------------------------------------------------
tLoadStatus tSequencerLoader::ParseScript(TextDataParser *parser)
{
    while (!parser->Eof())
    {
        char token[128];

        bool ok = parser->Advance();
        if(ok)
        {
            parser->CurrentToken(token, sizeof(token));

            if (!strcmp(token, "sequencer"))
            {
                tSequencer *seq = ParseSequencer(parser);
                if (seq != NULL) mStore->Store(seq);
            }
            else if (!strcmp(token, "eventanim"))
            {
                int line = parser->CurrentLine();
                bool found = parser->AdvanceTo("{");
                if (found) found = parser->SkipBracketedSection();
                if (!found)
                {
                    p3d::printf("Error skipping eventanim in sequencer loader on line %d.\n", line);
                }
            }
            else 
            {
                p3d::printf("Script Unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
            }
        }
    }

    return LOAD_OK;
}

//--------------------------------------------------
tSequencer *tSequencerLoader::ParseSequencer(TextDataParser *parser)
{
    char token[128];
    char value[128];
    bool found;

    // Check to make sure I am at the start of the sequencer   
    parser->CurrentToken(token, sizeof(token));
    if (strcmp(token, "sequencer")) return NULL;

    int startline = parser->CurrentLine();

    // Look for the start of the sequecner block ignoring all inbetween
    found = parser->AdvanceTo("{");
    if (!found) 
    {
        p3d::printf("Sequencer definition on line %d missing open brace.\n", startline);
        return NULL;
    }

    tSequencer* seq = new tSequencer;

    while (!parser->Eof())
    {
        parser->Advance();
        parser->CurrentToken(token, sizeof(token));
            
        if (!strcmp("name", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            seq->SetName(value);
            continue;
        }

        if (!strcmp("animtarget", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            tBlendFrameController* animationTarget = p3d::find<tBlendFrameController>(mStore, value);
            if(animationTarget)
            {
                tRefCounted::Assign(mAnimationTarget,animationTarget);
            }      
            continue;
        }

        if (!strcmp("speed", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            seq->SetRelativeSpeed((float)atof(value));
            continue;
        }

        if (!strcmp("states", token)) 
        {
            ParseStates(parser, seq);
            continue;
        }

        // End of sequencer definition
        if (!strcmp("}", token))
        {
            break;
        }

        p3d::printf("Sequencer Unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
    }

    // make sure I have not run off of the end of the data
    if (parser->Eof())
    {
        p3d::printf("Error parsing sequener defined on line %d  No end brace!\n", startline);
        seq->Release();
        return NULL;
    }
    
    // change transition from names to from pointers
    bool ok = FixupTransitions(seq);
    if (!ok) 
    {
        p3d::printf("Error fixing up the state transitions.\n");
        seq->Release();
        return NULL;
    }
    
    ok = BuildSubParts(seq);
    if (!ok) 
    {
        p3d::printf("Error building the subparts.\n");
        seq->Release();
        return NULL;
    }

    return seq;
}


//--------------------------------------------------
bool tSequencerLoader::ParseStates(TextDataParser *parser, tSequencer *seq)
{
    char token[128];
    bool found;

    int current_state = -1;

    // Check to make sure I am at the start of the sequencer   
    parser->CurrentToken(token, sizeof(token));
    if (strcmp(token, "states")) 
    {
        return false;
    }

    int startline = parser->CurrentLine();

    // Look for the start of the sequecner block ignoring all inbetween
    found = parser->AdvanceTo("{");
    if (!found) 
    {
        p3d::printf("States definition on line %d missing open brace.\n", startline);
        return false;
    }

    seq->mStateCount = parser->TokenCountAtCurrentBracketLevel("state");
    seq->mStates.SetSize( seq->mStateCount );
    int a;
    for (a = 0; a < seq->mStateCount; a++) 
    {
        seq->mStates[a] = new tSequencerState;
        seq->mStates[a]->AddRef();
    }

    current_state = 0;

    while (!parser->Eof())
    {
        parser->Advance();
        parser->CurrentToken(token, sizeof(token));
            
        if (!strcmp("state", token)) 
        {
            // make sur that the number of states has been defined 
            // before we try to define a state
            if (current_state < 0) 
            {
                p3d::printf("Error parsing states line %d, must set count before defining a state.\n", parser->CurrentLine());
                continue;
            }

            // Make sure that the number of states defined is within the count specified
            if (current_state >= seq->mStateCount)
            {
                p3d::printf("Error parsing states line %d, more states have been defined thet the count specifies.\n", parser->CurrentLine());
                continue;
            }

            // If we get here, things are OK so far         
            bool ok = ParseState(parser, seq->mStates[current_state]);
            if (ok) ++current_state;

            continue;
        }

        // End of states definition
        if (!strcmp("}", token))
        {
            break;
        }

        p3d::printf("States Unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
    }

    // make sure I have not run off of the end of the data
    if (parser->Eof())
    {
        p3d::printf("Error parsing states defined on line %d  No end brace!\n", startline);
        seq->Release();
        return false;
    }
    
    return true;
}

//--------------------------------------------------
bool tSequencerLoader::ParseState(TextDataParser *parser, tSequencerState *state)
{
    char token[128];
    char value[128];
    bool found;

    // Check to make sure I am at the start of the state definitions
    parser->CurrentToken(token, sizeof(token));
    if (strcmp(token, "state")) return false;

    int startline = parser->CurrentLine();

    // Look for the start of the state block ignoring all inbetween
    found = parser->AdvanceTo("{");
    if (!found) 
    {
        p3d::printf("State definition on line %d missing open brace.\n", startline);
        return false;
    }

    while (!parser->Eof())
    {
        parser->Advance();
        parser->CurrentToken(token, sizeof(token));
            
        if (!strcmp("name", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            state->SetName(value);
            continue;
        }

        if (!strcmp("anim", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            tAnimation* anim = p3d::find<tAnimation>(value);
            if(anim)
            {
                state->mObjectAnimation = anim;
                state->mObjectAnimation->AddRef();
            }
            P3DASSERTMSG(state->mObjectAnimation, "Sequencer loader failed to find animation", value);
            continue;
        }

        if (!strcmp("event", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            state->mEventAnimation = p3d::find<tAnimation>(value);
            if(state->mEventAnimation)
            {
                (state->mEventAnimation)->AddRef();
            }
            P3DASSERTMSG(state->mEventAnimation, "Sequencer loader failed to find event animation", value);
            continue;
        }

        if (!strcmp("minframe", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            state->mFrameRangeMin = (float)atof(value);
            continue;
        }

        if (!strcmp("maxframe", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            state->mFrameRangeMax = (float)atof(value);
            continue;
        }

        if (!strcmp("subpart", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            state->mSubPart = atoi(value);
            continue;
        }

        if (!strcmp("loop", token)) 
        {
            //enum LoopType { Loop, LoopNHold, LoopNOut, PlayKill } mLoopType;
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            if      (!strcmp("loopforever", value)) state->mLoopType = tSequencerState::Loop;
            else if (!strcmp("loopnhold", value))   state->mLoopType = tSequencerState::LoopNHold;
            else if (!strcmp("loopnout", value))    state->mLoopType = tSequencerState::LoopNOut;
            else if (!strcmp("playkill", value))    state->mLoopType = tSequencerState::PlayKill;
            else 
            {
                state->mLoopType = tSequencerState::Loop;
                p3d::printf("Error parsing State, Bad loop type on line %d\n", parser->CurrentLine());
            }

            continue;
        }

        if (!strcmp("loopcount", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            state->mMaxLoopCount = atoi(value);
            continue;
        }

        if (!strcmp("speed", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            state->mStateAnimationSpeed = (float)atof(value);
            continue;
        }

        // used only for looptype loopNout
        if (!strcmp("outtransition", token)) 
        {
            state->mOutDest = new tStateTransition;
            ParseTransition(parser, state->mOutDest);
            continue;
        }

        if (!strcmp("transitions", token)) 
        {
            ParseTransitions(parser, state);
            continue;
        }

        // End of states definition
        if (!strcmp("}", token))
        {
            break;
        }

        p3d::printf("State Unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
    }

    // make sure I have not run off of the end of the data
    if (parser->Eof())
    {
        p3d::printf("Error parsing state defined on line %d.  No end brace!\n", startline);
        return false;
    }
    
    return true;
}

//--------------------------------------------------
bool tSequencerLoader::ParseTransitions(TextDataParser *parser, tSequencerState *state)
{
    char token[128];
    bool found;
    int current_transition = -1;

    // Check to make sure I am at the start of the transition definitions
    parser->CurrentToken(token, sizeof(token));
    if (strcmp(token, "transitions")) 
    {
        return false;
    }

    int startline = parser->CurrentLine();

    // Look for the start of the transitions block ignoring all inbetween
    found = parser->AdvanceTo("{");
    if (!found) 
    {
        p3d::printf("Transitions definition on line %d missing open brace.\n", startline);
        return false;
    }

    state->mTransitions = NULL;
    state->mTransitionCount = parser->TokenCountAtCurrentBracketLevel("transition");
    if (state->mTransitionCount > 0)
    {
        state->mTransitions = new tStateTransition[state->mTransitionCount];
        int a;
        for (a = 0; a < state->mTransitionCount; a++)
        {
            state->mTransitions[a].mStateParam = NULL;
            state->mTransitions[a].mTimeParam  = 0;
            state->mTransitions[a].mOutFrameMin   = -1.0F;
            state->mTransitions[a].mOutFrameRange =  0.0F;
            state->mTransitions[a].mInFrame           =  0.0F;
            state->mTransitions[a].mInFrameRangeScale =  0.0F;
            state->mTransitions[a].mName.SetUID((tUID)0);
            state->mTransitions[a].mTransitionDirection = 0;
        }
    }

    current_transition = 0;

    while (!parser->Eof())
    {
        parser->Advance();
        parser->CurrentToken(token, sizeof(token));
            
        if (!strcmp("transition", token) ||
             !strcmp("default", token))
        {
            // If we get here, things are OK so far         
            bool isdefault = (token[0] == 'd');

            bool ok;
    
            if (isdefault)
            {
                ParseTransition(parser, &state->mDefaultTransition);
            }
            else
            {
                ok = ParseTransition(parser, &state->mTransitions[current_transition]);
                if (ok) ++current_transition;
            }
            continue;
        }

        // End of Transitions definition
        if (!strcmp("}", token))
        {
            break;
        }

        p3d::printf("Transitions unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
    }


    // make sure I have not run off of the end of the data
    if (parser->Eof())
    {
        p3d::printf("Error parsing transitions defined on line %d.  No end brace!\n", startline);
        return false;
    }
    
    return true;
}

//--------------------------------------------------
bool tSequencerLoader::ParseTransition(TextDataParser *parser, tStateTransition *dest)
{

    char token[128];
    char value[128];
    bool found;

    // Check to make sure I am at the start of the transition definitions
    parser->CurrentToken(token, sizeof(token));
    if (strcmp(token, "transition") &&
         strcmp(token, "default")    &&
         strcmp(token, "outtransition")) return false;

    bool is_transition = (strcmp(token, "outtransition") != 0);

    int startline = parser->CurrentLine();

    // Look for the start of the transition block ignoring all inbetween
    found = parser->AdvanceTo("{");
    if (!found) 
    {
        p3d::printf("%s definition on line %d missing open brace.\n", token, startline);
        return false;
    }

    while (!parser->Eof())
    {
        parser->Advance();
        parser->CurrentToken(token, sizeof(token));
        
        if (!strcmp("name", token) && is_transition)
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            // duplicate the token string so that I can fix up the state pointers
            // AFTER all of the other states are loaded   
            // NOTE: Don't forget to free the char*s after the pointer fixup
            dest->mName = parser->DupCurrentToken();         
            continue;
        }

        if (!strcmp("from", token) && is_transition)
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            // duplicate the token string so that I can fix up the state pointers
            // AFTER all of the other states are loaded   
            // NOTE: Don't forget to free the char*s after the pointer fixup
            dest->mStateParam = (tSequencerState *) parser->DupCurrentToken();
            dest->mTransitionDirection = 0;
            continue;
        }

        //if (!strcmp("dest", token) && !is_transition)
        if (!strcmp("dest", token) )// && is_transition)
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            // duplicate the token string so that I can fix up the state pointers
            // AFTER all of the other states are loaded   
            // NOTE: Don't forget to free the char*s after the pointer fixup
            dest->mStateParam = (tSequencerState *) parser->DupCurrentToken();
            dest->mTransitionDirection = 1;
            continue;
        }

        if (!strcmp("time", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            dest->mTimeParam = (float)atof(value);
            continue;
        }

        if (!strcmp("outframe", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            dest->mOutFrameMin = (float) atof(value);
            dest->mOutFrameRange = 1.0F;
            continue;
        }

        if (!strcmp("outframerange", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            dest->mOutFrameMin = (float)atof(value);
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            dest->mOutFrameRange = ((float)atof(value) - dest->mOutFrameMin) + 1.0F;
            continue;
        }

        //
        // This is confusing
        //
        if (!strcmp("startframe", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            //dest->mInFrame = (float) atof(value);
            dest->mStartFrame = (float) atof(value);
            dest->mInFrameRangeScale = 0.0F;
            continue;
        }

        if (!strcmp("startframerange", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            dest->mInFrame = (float) atof(value);
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            dest->mInFrameRangeScale = (float) atof(value);
            continue;
        }


        if (!strcmp("command", token)) 
        {
            //enum Commands { Invalid, Start, Replace, Kill, BlendIn, BlendOut, CrossFade, UseDefault } mCommand;
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            if      (!strcmp("invalid", value))    dest->mCommand = tStateSequencerCommand::Invalid;
            else if (!strcmp("start", value))      dest->mCommand = tStateSequencerCommand::Start;
            else if (!strcmp("replace", value))    dest->mCommand = tStateSequencerCommand::Replace;
            else if (!strcmp("kill", value))       dest->mCommand = tStateSequencerCommand::Kill;
            else if (!strcmp("blendin", value))    dest->mCommand = tStateSequencerCommand::BlendIn;
            else if (!strcmp("blendout", value))   dest->mCommand = tStateSequencerCommand::BlendOut;
            else if (!strcmp("crossfade", value))  dest->mCommand = tStateSequencerCommand::CrossFade;
            else if (!strcmp("usedefault", value)) dest->mCommand = tStateSequencerCommand::UseDefault;
            else 
            {
                dest->mCommand = tStateSequencerCommand::Replace;
                p3d::printf("Error parsing Transition, Bad command on line %d\n", parser->CurrentLine());
            }

            continue;
        }

        // End of states definition
        if (!strcmp("}", token))
        {
            break;
        }

        p3d::printf("Transition Unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
    }


    // make sure I have not run off of the end of the data
    if (parser->Eof())
    {
        p3d::printf("Error parsing transition defined on line %d.  No end brace!\n", startline);
        return false;
    }
    
    return true;  
}

//--------------------------------------------------
bool tSequencerLoader::FixupTransitions(tSequencer *seq)
{
    int a;
    for (a = 0; a < seq->mStateCount; a++)
    {
        tSequencerState *state = seq->mStates[a];

        // Fix up the out dest if there is one
        if (state->mOutDest != NULL)
        {
            char *name = (char*) state->mOutDest->mStateParam;
            state->mOutDest->mStateParam = FindState(seq, name);
            P3DASSERTMSG(state->mOutDest->mStateParam, "Sequencer couldn'd find sequencerState", name);
            delete[] name;
        }

        // Fixup the transitions
        int transition;
        for (transition = 0; transition < state->mTransitionCount; transition++)
        {
            char *name = (char *)state->mTransitions[transition].mStateParam;
            tSequencerState *dest = FindState(seq, name);
            P3DASSERTMSG(dest, "Sequencer couldn'd find sequencerState", name);
            state->mTransitions[transition].mStateParam = dest;
            delete[] name;
        }
    }

    return true;
}

//--------------------------------------------------
bool tSequencerLoader::BuildSubParts(tSequencer *seq)
{
    int maxsub = 0;
    int a;
    for (a = 0; a < seq->mStateCount; a++)
    {
        if (seq->mStates[a]->SubPart() > maxsub) maxsub = seq->mStates[a]->SubPart();
    }

    seq->mSubPartCount = maxsub + 1;
    seq->mSubParts.SetSize( seq->mSubPartCount );

    // use the cached animation target
    seq->SetAnimationTarget(mAnimationTarget);
    if(mAnimationTarget)
    {
        mAnimationTarget->Release();
        mAnimationTarget = NULL;
    }

    for (a = 0; a < seq->mSubPartCount; a++)
    {
        seq->mSubParts[a].SetEventController(new tEventController);
        seq->mSubParts[a].EventController()->SetOrigin(seq->mEventOrigin);
        
    }

    return true;
}

//--------------------------------------------------
tSequencerState *tSequencerLoader::FindState(tSequencer *seq, char* name)
{

    tUID uid = tEntity::MakeUID(name);

    int state;
    for (state = 0; state < seq->mStateCount; state++)
    {
        if (seq->mStates[state]->GetUID() == uid) return seq->mStates[state];
    }

    return NULL;
}


//*******************************************************
// Class tSEQFileHandler
//*******************************************************
tSEQFileHandler::tSEQFileHandler()
{
    sequencerLoader = new tSequencerLoader;
    sequencerLoader->AddRef();
    eventAnimLoader = new tEventAnimTextLoader;
    eventAnimLoader->AddRef();
}

//--------------------------------------------------
tSEQFileHandler::~tSEQFileHandler()
{
    tRefCounted::Release(sequencerLoader);
    tRefCounted::Release(eventAnimLoader);
}

//--------------------------------------------------
bool tSEQFileHandler::CheckExtension(char* file)
{
    int len = strlen(file);
    int extLen = 4;
    
    if(len >= extLen)
    {
        
        if ((file[len - extLen] == '.') &&
             (tolower(file[(len - extLen) + 1]) == 's') &&
             (tolower(file[(len - extLen) + 2]) == 'e') &&
             (tolower(file[(len - extLen) + 3]) == 'q')) 
        {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------
tLoadStatus tSEQFileHandler::Load(tFile* file, tEntityStore* store)
{
    tLoadStatus eventLoadStatus = eventAnimLoader->Load(file, store);
    file->SetPosition(0); // reset the file
    tLoadStatus sequencerLoadStatus = sequencerLoader->Load(file, store);

    if(eventLoadStatus == LOAD_ERROR)
    {
        return(eventLoadStatus);
    }
    if(sequencerLoadStatus == LOAD_ERROR)
    {
        return(sequencerLoadStatus);
    }

    return(LOAD_OK);
}

