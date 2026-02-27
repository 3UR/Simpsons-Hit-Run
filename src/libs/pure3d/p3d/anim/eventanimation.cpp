//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================

#include <p3d/utility.hpp>

#include <p3d/anim/eventanimation.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/anim/event.hpp>
#include <p3d/error.hpp>
#include <p3d/file.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/inventory.hpp>
#include <p3d/textdataparser.hpp>
#include <constants/chunks.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//**************************************************************
// Class tEventController
//**************************************************************
tEventController::tEventController() : 
    tSimpleFrameController(),
    recipient(NULL), 
    origin(NULL), 
    lastFrame(0.0f)
{
}

//--------------------------------------------------------------
tEventController::tEventController(tEventController* c) : 
    tSimpleFrameController(c),
    lastFrame(0.0f)
{
    SetRecipient(c->GetRecipient());
    SetOrigin(c->GetOrigin());
}

//--------------------------------------------------------------
tEventController::~tEventController()
{
    tRefCounted::Release(recipient);
    tRefCounted::Release(origin);
}

//--------------------------------------------------------------
tFrameController* tEventController::Clone(void)
{
    return new tEventController(this);
}

//--------------------------------------------------------------
bool tEventController::ValidateAnimation(tAnimation* anim)
{
    return (anim->GetAnimationType()==Pure3DAnimationTypes::EVENT_EVT);
}

//--------------------------------------------------------------
void tEventController::SetAnimation(tAnimation* anim)
{
    tSimpleFrameController::SetAnimation(anim);
    lastFrame = frame;
}

//--------------------------------------------------------------
void tEventController::SetAnimation(tAnimation* anim, float startFrame, float blendFrames)
{
    tSimpleFrameController::SetAnimation(anim, startFrame, blendFrames);
    lastFrame = frame;
}

//--------------------------------------------------------------
void tEventController::SetRecipient(tEventRecipient* r)
{
    tRefCounted::Assign(recipient,r);
}

//--------------------------------------------------------------
tEventRecipient* tEventController::GetRecipient(void)
{
    return recipient;
}

//--------------------------------------------------------------
void tEventController::SetOrigin(tEventOrigin* o)
{
    tRefCounted::Assign(origin,o);
}

//--------------------------------------------------------------
tEventOrigin* tEventController::GetOrigin(void)
{
    return origin;
}

//--------------------------------------------------------------
void tEventController::Update(void)
{
    if((lastFrame == frame) || (!animation) || (!recipient))
        return;

    for(int i=0;i<animation->GetNumGroups();i++)
    {
        const tAnimationGroup* animGroup = animation->GetGroupByIndex(i);
        const tName& groupName = animGroup->GetNameObject();
        if(recipient->HandlesEvents(groupName))
        {
            const tEventChannel* eventChannel = animGroup->GetEventChannel(Pure3DAnimationChannels::Event::EVENT_EVT);
            if(eventChannel)
            {
                if(lastFrame > frame)
                {
                    eventChannel->DispatchEvents(recipient, origin, groupName, animation->MakeValidFrame(lastFrame), animation->MakeValidFrame((float)animation->GetNumFrames()));
                    eventChannel->DispatchEvents(recipient, origin, groupName, animation->MakeValidFrame(0.0f), animation->MakeValidFrame(frame));
                }
                else
                {
                    eventChannel->DispatchEvents(recipient, origin, groupName, animation->MakeValidFrame(lastFrame), animation->MakeValidFrame(frame));
                }
            }
        }
    }   
    lastFrame = frame;
}

//*******************************************************
// Class tEventAnimTextLoader
//*******************************************************
tLoadStatus tEventAnimTextLoader::Load(tFile *file, tEntityStore *store)
{
    if (file == NULL)
    {
        tRefCounted::Release(mStore);
        mStore = NULL;
        return LOAD_ERROR;
    }

    P3DASSERT(store);
    tRefCounted::Assign(mStore,store);
    TextDataParser *parser = new TextDataParser(file);

    tLoadStatus result = ParseScript(parser);

    delete parser;

    tRefCounted::Release(mStore);

    return result;
}

//--------------------------------------------------
tLoadStatus tEventAnimTextLoader::Load(tChunkFile*, tEntityStore *store)
{
    return LOAD_OK;
}

//--------------------------------------------------
bool tEventAnimTextLoader::CheckChunkID(unsigned)
{
    return true;
}

//--------------------------------------------------
tLoadStatus  tEventAnimTextLoader::ParseScript(TextDataParser *parser)
{
    while (!parser->Eof())
    {
        char token[128];

        bool ok = parser->Advance();
        if (ok) 
        {
            parser->CurrentToken(token, sizeof(token));

            if (!strcmp(token, "eventanim"))
            {
                tAnimation *eventAnim = ParseEventAnim(parser);
                if (eventAnim != NULL) mStore->Store(eventAnim);
            }
            else if (!strcmp(token, "sequencer"))
            {
                int line = parser->CurrentLine();
                bool found = parser->AdvanceTo("{");
                if (found) found = parser->SkipBracketedSection();
                if (!found)
                {
                    p3d::printf("Error skipping sequencer in eventanim loader on line %d.\n", line);
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
tAnimation* tEventAnimTextLoader::ParseEventAnim(TextDataParser *parser)
{
    char token[128];
    char value[128];
    bool found;

    // Check to make sure I am at the start of the sequencer   
    parser->CurrentToken(token, sizeof(token));
    if (strcmp(token, "eventanim")) return NULL;

    int startline = parser->CurrentLine();

    // Look for the start of the event anim block ignoring all inbetween
    found = parser->AdvanceTo("{");
    if (!found) 
    {
        p3d::printf("EventAnim definition on line %d missing open brace.\n", startline);
        return NULL;
    }

    // Build the event animationChannel Keys
    tAnimation* eventanim = new tAnimation;
    eventanim->animType  = Pure3DAnimationTypes::EVENT_EVT;
    eventanim->numGroups = parser->TokenCountAtCurrentBracketLevel("eventChannel");
    eventanim->groups = new tAnimationGroup*[eventanim->numGroups];             
    memset(eventanim->groups, 0, sizeof(tAnimationGroup*)*(eventanim->numGroups));

    int currentAnimationGroup = 0;

    while (!parser->Eof())
    {
        parser->Advance();
        parser->CurrentToken(token, sizeof(token));

        if (!strcmp("name", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            eventanim->SetName(value);
            continue;
        }

        if (!strcmp("framerate", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            float fr = (float)atof(value);
            eventanim->SetSpeed(fr);
            continue;
        }

        if (!strcmp("length", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            eventanim->numFrames = (float)atoi(value);
            continue;
        }

        if (!strcmp("cyclic", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            bool cyclic = (atoi(value) != 0);
            eventanim->SetCyclic(cyclic);
            continue;
        }

        // New Channel version
        if (!strcmp("eventChannel", token))
        {
            // Create the eventChannel and group add it to the group
            char* groupName = NULL; 
            tAnimationGroup* animGroup = ParseEventChannel(parser, groupName);
            if(animGroup)
            {
                animGroup->SetName(groupName);
                eventanim->groups[currentAnimationGroup] = animGroup;
            }
            currentAnimationGroup++;
            continue;
        }

        // End of sequencer definition
        if (!strcmp("}", token))
        {
            break;
        }

        p3d::printf("ParseEventAnim Unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
    }

    // make sure I have not run off of the end of the data
    if (parser->Eof())
    {
        p3d::printf("Error parsing event anim defined on line %d  No end brace!\n", startline);
        eventanim->Release();
        return NULL;
    }

    return eventanim;
}

//--------------------------------------------------
tAnimationGroup* tEventAnimTextLoader::ParseEventChannel(TextDataParser *parser, char* &groupName)
{
    char token[128];
    char value[128];
    bool found;

    // Check to make sure I am at the start of the sequencer
    parser->CurrentToken(token, sizeof(token));
    if (strcmp(token, "eventChannel")) return NULL;

    int startline = parser->CurrentLine();

    // Look for the start of the event anim block ignoring all inbetween
    found = parser->AdvanceTo("{");
    if (!found) 
    {
        p3d::printf("EventAnim definition on line %d missing open brace.\n", startline);
        return NULL;
    }

    int maxevent = parser->TokenCountAtCurrentBracketLevel("event");

    // Here we should have a constructed channel and Group
    tEventChannel *eventChannel = new tEventChannel(maxevent);
    eventChannel->channelCode = Pure3DAnimationChannels::Event::EVENT_EVT;
    eventChannel->dataType = tChannel::EVENT;

    tAnimationGroup* animGroup = new tAnimationGroup;
    animGroup->numChannels = 1;
    animGroup->channels = new tChannel*[1];
    animGroup->channels[0] = eventChannel;

    // Build the event animationChannel Keys
    eventChannel->numKeys = maxevent;
    int a;
    for (a = 0; a < maxevent; a++)
    {
        eventChannel->frames[a] = 0;
        eventChannel->values[a] = NULL;
    }

    int currentevent = 0;

    while (!parser->Eof())
    {
        parser->Advance();
        parser->CurrentToken(token, sizeof(token));

        // I need to get the address since it's stored in the mapping
        if(!strcmp("address", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            groupName = new char[strlen(value) + 1];
            strcpy(groupName, value);
            continue;
        }

        if (!strcmp("event", token)) 
        {
            if (currentevent >= maxevent)
            {
                p3d::printf("ParseEventAnim error: more events in an event animation then specified, line %d\n", parser->CurrentLine());
                parser->AdvanceTo("}");
                continue;
            }

            short time;
            tEvent *event = ParseEvent(parser, time);
            if (event != NULL)
            {
                event->AddRef();
                eventChannel->values[currentevent] = event;
                eventChannel->frames[currentevent] = time;
                ++currentevent;
            }

            continue;
        }
        
        // End of Event Channel definition
        if (!strcmp("}", token))
        {
            break;
        }

        p3d::printf("ParseEventChannel Unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
    }

    return animGroup;
}

//--------------------------------------------------
tEvent *tEventAnimTextLoader::ParseEvent(TextDataParser *parser, short &time)
{
    char token[128];
    char value[128];
    bool found;

    // Check to make sure I am at the start of the sequencer   
    parser->CurrentToken(token, sizeof(token));
    if (strcmp(token, "event")) return NULL;

    int startline = parser->CurrentLine();

    // Look for the start of the event anim block ignoring all inbetween
    found = parser->AdvanceTo("{");
    if (!found) 
    {
        p3d::printf("Event definition on line %d missing open brace.\n", startline);
        return NULL;
    }

    tGenericEvent *event = new tGenericEvent;
    event->SetDataLength(0);
    event->SetData(NULL, false);

    while (!parser->Eof())
    {
        parser->Advance();
        parser->CurrentToken(token, sizeof(token));

        if (!strcmp("name", token)) 
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            event->SetName(value);
            continue;
        }

        if (!strcmp("address", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
//         event->SetAddressName(value);
            continue;
        }

        if (!strcmp("param", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            event->SetParameter((unsigned)atoi(value));
            continue;
        }

        if (!strcmp("time", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));
            time = (short)atoi(value);
            continue;
        }

        if (!strcmp("data", token)) 
        {
            ParseEventData(parser, event);
            continue;
        }

        // End of sequencer definition
        if (!strcmp("}", token))
        {
            break;
        }

        p3d::printf("ParseEvent Unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
    }

    // make sure I have not run off of the end of the data
    if (parser->Eof())
    {
        p3d::printf("Error parsing event defined on line %d  No end brace!\n", startline);
        delete event;
        time = 0;
        return NULL;
    }
    
    return event;
}

//--------------------------------------------------
bool tEventAnimTextLoader::ParseEventData(TextDataParser *parser, tGenericEvent *event)
{
    char token[128];
    char value[128];
    bool found;

    // Check to make sure I am at the start of the sequencer   
    parser->CurrentToken(token, sizeof(token));
    if (strcmp(token, "data")) return false;

    int startline = parser->CurrentLine();

    // make sure the data is defined only once
    if (event->GetDataLength() != 0)
    {
        p3d::printf("ParesEventData error: Data already defined for event. error on line %d\n", parser->CurrentLine());
        return false;
    }

    // Look for the start of the event data block ignoring all inbetween
    found = parser->AdvanceTo("{");
    if (!found) 
    {
        p3d::printf("EventData definition on line %d missing open brace.\n", startline);
        return false;
    }

    // We need to determine the data size before we allocate the event data structure
    parser->PushPosition();

    int datasize = 0;
    int stringstart = 0;

    int currentdata = 0;
    int maxdata = 0;
    char *data = NULL;

    while (!parser->Eof())
    {
        parser->Advance();
        parser->CurrentToken(token, sizeof(token));

        if (!strcmp("int", token))
        {
            datasize    += sizeof(int);
            stringstart += sizeof(int);
            maxdata++;
            parser->Advance();   // skip over token value for first pass
            continue;
        }

        if (!strcmp("float", token))
        {
            datasize    += sizeof(float);
            stringstart += sizeof(float);
            maxdata++;
            parser->Advance();   // skip over token value for first pass
            continue;
        }

        if (!strcmp("string", token))
        {
            parser->Advance();

            datasize    += sizeof(char *);
            stringstart += sizeof(char *);
            maxdata++;

            parser->CurrentToken(value, sizeof(value));

            int sl = strlen(value) + 1;
            sl = (sl + 3) & (~3); // make DWORD aligned
            datasize += sl;
            continue;
        }

        // End of sequencer definition
        if (!strcmp("}", token))
        {
            break;
        }

        p3d::printf("ParseEventData Unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
    }

    // data size should now be known

    // allocate the data
    data = new char[datasize];

    // tell the event about it
    event->SetDataLength(datasize);
    event->SetData((void *)data, true);

    // restore the tesxt parser to the beginning of the event data
    parser->PopPosition();

    while (!parser->Eof())
    {
        parser->Advance();
        parser->CurrentToken(token, sizeof(token));

        if (!strcmp("int", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));

            int *i = (int *) &data[currentdata];
            *i = atoi(value);
            currentdata += sizeof(int);
            continue;
        }

        if (!strcmp("float", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));

            float *f = (float*)&data[currentdata];
            *f = (float)(atof(value));
            currentdata += sizeof(float);
            continue;
        }

        if (!strcmp("string", token))
        {
            parser->Advance();
            parser->CurrentToken(value, sizeof(value));

            int len = parser->CurrentToken(NULL, 0);

            char **s = (char **)&data[currentdata];
            currentdata += sizeof(char *);

            *s = (char *) &data[stringstart];
            parser->CurrentToken(*s, len + 1);

            // DWORD align length
            len = (len + 3) & (~3);
            stringstart += len;

            continue;
        }

        // End of sequencer definition
        if (!strcmp("}", token))
        {
            break;
        }

        p3d::printf("ParseEventData Unrecognised token on line %d: %s\n", parser->CurrentLine(), token);
    }

    // make sure I have not run off of the end of the data
    if (parser->Eof())
    {
        p3d::printf("Error parsing event data defined on line %d  No end brace!\n", startline);
        return false;
    }

    return true;
}

