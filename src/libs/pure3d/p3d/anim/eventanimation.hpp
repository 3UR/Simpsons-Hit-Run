//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _EVENTANIMATION_HPP_
#define _EVENTANIMATION_HPP_

#include <p3d/anim/animate.hpp>

class tEventOrigin;
class tEventRecipient;
class TextDataParser;
class tGenericEvent;

//**************************************************************
// Class tEventController
//**************************************************************
class tEventController : public tSimpleFrameController
{
public:
    tEventController();
    tEventController(tEventController* c);

    virtual tFrameController* Clone(void);

    bool ValidateAnimation(tAnimation* anim);

    // switch animations
    virtual void SetAnimation(tAnimation* anim);
    // switch animations with blending
    // TODO: add mode for fixed-frame blends
    virtual void SetAnimation(tAnimation* anim, float startFrame, float blendFrames);

    void SetRecipient(tEventRecipient* recipient);
    tEventRecipient* GetRecipient(void);

    void SetOrigin(tEventOrigin* origin);
    tEventOrigin* GetOrigin(void);

protected :
    virtual ~tEventController();

private:
    void Update(void);

    tEventRecipient* recipient;
    tEventOrigin* origin;
    float lastFrame;
};

class tEventAnimTextLoader  : public tChunkHandler
{
 public:
    tEventAnimTextLoader() : mStore(NULL) {}

    tLoadStatus Load(tFile *file, tEntityStore *store);

    virtual tLoadStatus Load(tChunkFile*, tEntityStore *store);
    bool CheckChunkID(unsigned);
	unsigned int GetChunkID() { return 0; }

 protected:
    virtual ~tEventAnimTextLoader(){}

 private:
    tEntityStore *mStore;

    tLoadStatus  ParseScript(TextDataParser *parser);
    tAnimation* ParseEventAnim(TextDataParser *parser);
    tAnimationGroup* ParseEventChannel(TextDataParser *parser, char* &groupName);
    tEvent *ParseEvent(TextDataParser *parser, short &eventtime);
    bool ParseEventData(TextDataParser *parser, tGenericEvent *event);
};

#endif

