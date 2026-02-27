/*===========================================================================
    channel.cpp
    26-Sep-97 Created by Neall
    30-Nov-99 version 12

    Copyright (c)1996-1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <constants/chunks.h>
#include <constants/chunkids.hpp>
#include <constants/atenum.hpp>
#include <p3d/anim/animate.hpp>
#include <p3d/anim/channel.hpp>
#include <p3d/anim/event.hpp>
#include <p3d/chunkfile.hpp>
#include <p3d/error.hpp>
#include <p3d/entity.hpp>
#include <p3d/utility.hpp>

#include <string.h>
#include <new>

static const unsigned INTERPOLATION_MODE_VERSION = 0;
static const unsigned INT_CHANNEL_VERSION = 0;
static const unsigned FLOAT1_CHANNEL_VERSION = 0;
static const unsigned FLOAT2_CHANNEL_VERSION = 0;
static const unsigned VECTOR1DOF_CHANNEL_VERSION = 0;
static const unsigned VECTOR2DOF_CHANNEL_VERSION = 0;
static const unsigned VECTOR3DOF_CHANNEL_VERSION = 0;
static const unsigned QUATERNION_CHANNEL_VERSION = 0;
static const unsigned QUATERNION_FORMAT_VERSION = 0;
static const unsigned COMPRESSED_QUATERNION_CHANNEL_VERSION = 0;
static const unsigned STRING_CHANNEL_VERSION = 0;
static const unsigned ENTITY_CHANNEL_VERSION = 0;
static const unsigned BOOL_CHANNEL_VERSION = 0;
static const unsigned COLOUR_CHANNEL_VERSION = 0;
static const unsigned EVENT_CHANNEL_VERSION  = 0;

using namespace RadicalMathLibrary;

//**************************************************************
// Class tChannel
//**************************************************************
tChannel::tChannel(int nKeys, tAnimationMemoryBlock* block) :
    memoryBlock(block),
    channelCode(0), 
    interpolate(true),
    dataType(UNASSIGNED), 
    numKeys(nKeys), 
    frames(NULL)
{
    P3DASSERT(nKeys>0);
    numKeys = nKeys;
    if (!memoryBlock)
    {
        frames = new short[numKeys];
    }
    else
    {
        frames = (short*)(memoryBlock->Allocate(numKeys*sizeof(short)));
    }
}

//--------------------------------------------------------------
void tChannel::CleanUp()
{
    if (!memoryBlock)
    {
        delete [] frames;
        frames = NULL;
    }
}
 
//--------------------------------------------------------------
bool tChannel::FindBracketKeys(float frame, int& beginKeys, int& endKey) const
{
    P3DASSERT((frames) && (numKeys>0));
        
    // if before first key frame return value of first key frame
    if (frame <= (float)frames[0])
    {
        beginKeys = endKey = 0;
        return false;
    }

    // if after last key frame return value of last key frame
    if (frame >= (float)frames[numKeys - 1])
    {
        beginKeys = endKey = numKeys-1;
        return false;
    }

    int left = 0;
    int right = numKeys - 1;
    int current = right / 2;

    // binary search to find the closest key to frame without going over
    while(1)
    {      
        if((frame >= (float)frames[current]) && (frame < (float)frames[current+1]))
        {
            break;
        }
        if((float)frames[current] < frame)
        {
            left = current + 1;
        }
        else if(frames[current] > frame)
        {
            right = current - 1;
        }
        current = (left + right) / 2;
    }
   
    beginKeys = current;
    endKey = current+1;
    return true;
}

//**************************************************************
// Class tIntChannel
//**************************************************************
tIntChannel::tIntChannel(int nKeys, tAnimationMemoryBlock* block) :
    tChannel(nKeys,block),
    values(NULL)
{
    dataType = INT;
    if (!memoryBlock)
    {
        values = new int[numKeys];
    }
    else
    {
        values = (int*)(memoryBlock->Allocate(numKeys*sizeof(int)));
    }
}

//--------------------------------------------------------------
void tIntChannel::CleanUp()
{
    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tChannel::CleanUp();
}

//--------------------------------------------------------------
void tIntChannel::GetValue(float frame, int* v) const
{
    P3DASSERT(values);

    int start, end;

    if ((FindBracketKeys(frame, start, end))&&(interpolate != 0))
    {
        float frameDiff = frame - (float)frames[start];
        if (frameDiff == 0.0f)
        {
            *v = values[start];
        }
        else
        {
            float delta = (float)(frames[end] - frames[start]);
            *v = values[start] + FtoL((((float)(values[end] - values[start]))/delta) * frameDiff);
        }
    }
    else
    {
        *v = values[start];
    }
}

//**************************************************************
// Class tFloat1Channel
//**************************************************************
tFloat1Channel::tFloat1Channel(int nKeys, tAnimationMemoryBlock* block) :
    tChannel(nKeys,block),
    values(NULL)
{
    dataType = FLOAT1;
    if (!memoryBlock)
    {
        values = new float[numKeys];
    }
    else
    {
        values = (float*)(memoryBlock->Allocate(numKeys*sizeof(float)));
    }
}

//--------------------------------------------------------------
void tFloat1Channel::CleanUp()
{
    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tChannel::CleanUp();
}

//--------------------------------------------------------------
void tFloat1Channel::GetValue(float frame, float* v) const
{
    P3DASSERT(values);

    int start, end;

    if ((FindBracketKeys(frame, start, end))&&(interpolate))
    {
        float frameDiff = frame - (float)frames[start];
        if (frameDiff == 0.0f)
        {
            *v = values[start];
        }
        else
        {
            float delta = (float)(frames[end] - frames[start]);
            *v = values[start] + (((values[end] - values[start])/delta) * frameDiff);
        }
    }
    else
    {
        *v = values[start];
    }
}

//**************************************************************
// Class tFloat2Channel
//**************************************************************
tFloat2Channel::tFloat2Channel(int nKeys, tAnimationMemoryBlock* block) :
    tChannel(nKeys,block),
    values(NULL)
{
    dataType = FLOAT2;
    if (!memoryBlock)
    {
        values = new Vector2[numKeys];
    }
    else
    {
        values = (Vector2*)(memoryBlock->Allocate(numKeys*sizeof(Vector2)));
        for (int i = 0; i < numKeys; i++)
        {
            new(&values[i])Vector2;
        }
    }
}

//--------------------------------------------------------------
void tFloat2Channel::CleanUp()
{
    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tChannel::CleanUp();
}

//--------------------------------------------------------------
void tFloat2Channel::GetValue(float frame, Vector2* v) const
{
    P3DASSERT(values);

    int start, end;

    if ((FindBracketKeys(frame, start, end))&&(interpolate))
    {
        float frameDiff = frame - (float)frames[start];
        if (frameDiff == 0.0f)
        {
            *v = values[start];
        }
        else
        {
            float delta = frameDiff / (float)(frames[end] - frames[start]);
            v->u = values[start].u + ((values[end].u - values[start].u) * delta);
            v->v = values[start].v + ((values[end].v - values[start].v) * delta);
        }
    }
    else
    {
        *v = values[start];
    }
}

//**************************************************************
// Class tVector1DOFChannel
//**************************************************************
tVector1DOFChannel::tVector1DOFChannel(int nKeys, tAnimationMemoryBlock* block) :
    tVectorChannel(nKeys,block),
    dynamicIndex(0), 
    constants(0.0f,0.0f,0.0f),
    values(NULL)
{
    dataType = VECTOR;
    if (!memoryBlock)
    {
        values = new float[numKeys];
    }
    else
    {
        values = (float*)(memoryBlock->Allocate(numKeys*sizeof(float)));
    }
}

//--------------------------------------------------------------
void tVector1DOFChannel::CleanUp()
{
    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tVectorChannel::CleanUp();
}

//--------------------------------------------------------------
void tVector1DOFChannel::GetValue(float frame, Vector* v) const
{
    P3DASSERT(values);

    int start, end;

    *v = constants;
    
    if ((FindBracketKeys(frame, start, end))&&(interpolate))
    {
        float frameDiff = frame - (float)frames[start];
        if (frameDiff == 0.0f)
        {
            ((float*)(v))[dynamicIndex] = values[start];
        }
        else
        {
            float delta = (float)(frames[end] - frames[start]);
            ((float*)(v))[dynamicIndex] = values[start] + (((values[end] - values[start])/delta) * frameDiff);
        }
    }
    else
    {
        ((float*)(v))[dynamicIndex] = values[start];
    }
}

//**************************************************************
// Class tVector2DOFChannel
//**************************************************************
tVector2DOFChannel::tVector2DOFChannel(int nKeys, tAnimationMemoryBlock* block) :
    tVectorChannel(nKeys,block),
    staticIndex(0), 
    constants(0.0f,0.0f,0.0f),
    values(NULL)
{
    dataType = VECTOR;
    if (!memoryBlock)
    {
        values = new Vector2[numKeys];
    }
    else
    {
        values = (Vector2*)(memoryBlock->Allocate(numKeys*sizeof(Vector2)));
        for (int i = 0; i < numKeys; i++)
        {
            new(&values[i])Vector2;
        }
    }
}

//--------------------------------------------------------------
void tVector2DOFChannel::CleanUp()
{
    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tVectorChannel::CleanUp();
}

//--------------------------------------------------------------
void tVector2DOFChannel::GetValue(float frame, Vector* v) const
{
    P3DASSERT(values);

    int dynamicIndicies[3][2] = { {1,2}, {0,2}, {0,1} };

    int start, end;

    *v = constants;
    
    if ((FindBracketKeys(frame, start, end))&&(interpolate))
    {
        float frameDiff = frame - (float)frames[start];
        if (frameDiff == 0.0f)
        {
            ((float*)(v))[dynamicIndicies[staticIndex][0]] = values[start].u;
            ((float*)(v))[dynamicIndicies[staticIndex][1]] = values[start].v;
        }
        else
        {
            float delta = frameDiff / (float)(frames[end] - frames[start]);
            ((float*)(v))[dynamicIndicies[staticIndex][0]] = values[start].u + ((values[end].u - values[start].u) * delta);
            ((float*)(v))[dynamicIndicies[staticIndex][1]] = values[start].v + ((values[end].v - values[start].v) * delta);
        }
    }
    else
    {
        ((float*)(v))[dynamicIndicies[staticIndex][0]] = values[start].u;
        ((float*)(v))[dynamicIndicies[staticIndex][1]] = values[start].v;
    }
}

//**************************************************************
// Class tVector3DOFChannel
//**************************************************************
tVector3DOFChannel::tVector3DOFChannel(int nKeys, tAnimationMemoryBlock* block) :
    tVectorChannel(nKeys,block),
    values(NULL)
{
    dataType = VECTOR;
    if (!memoryBlock)
    {
        values = new Vector[numKeys];
    }
    else
    {
        values = (Vector*)(memoryBlock->Allocate(numKeys*sizeof(Vector)));
        for (int i = 0; i < numKeys; i++)
        {
            new(&values[i])Vector;
        }
    }
}

//--------------------------------------------------------------
void tVector3DOFChannel::CleanUp()
{
    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tVectorChannel::CleanUp();
}

//--------------------------------------------------------------
void tVector3DOFChannel::GetValue(float frame, Vector* v) const
{
    P3DASSERT(values);

    int start, end;
  
    if ((FindBracketKeys(frame, start, end))&&(interpolate))
    {
        float frameDiff = frame - (float)frames[start];
        if (frameDiff == 0.0f)
        {
            *v = values[start];
        }
        else
        {
            float delta = frameDiff / (float)(frames[end] - frames[start]);
            v->x = values[start].x + ((values[end].x - values[start].x) * delta);
            v->y = values[start].y + ((values[end].y - values[start].y) * delta);
            v->z = values[start].z + ((values[end].z - values[start].z) * delta);
        }
    }
    else
    {
        *v = values[start];
    }
}

//**************************************************************
// Class tQuaternionChannel
//**************************************************************
tQuaternionChannel::tQuaternionChannel(int nKeys, tAnimationMemoryBlock* block) :
    tRotationChannel(nKeys,block),
    values(NULL)
{
    dataType = ROTATION;
    if (!memoryBlock)
    {
        values = new Quaternion[numKeys];
    }
    else
    {
        values = (Quaternion*)(memoryBlock->Allocate(numKeys*sizeof(Quaternion)));
        for (int i = 0; i < numKeys; i++)
        {
            new(&values[i])Quaternion;
        }
    }
}

//--------------------------------------------------------------
void tQuaternionChannel::CleanUp()
{
    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tRotationChannel::CleanUp();
}

//--------------------------------------------------------------
void tQuaternionChannel::GetEuler(float frame, Vector* v) const
{
    P3DASSERT(values);
    Matrix m;
    GetMatrix(frame,&m);
    v->ConvertToEulerYZX(m);
}

//--------------------------------------------------------------
void tQuaternionChannel::GetQuaternion(float frame, Quaternion* v) const
{
    P3DASSERT(values);

    int start, end;
  
    if ((FindBracketKeys(frame, start, end))&&(interpolate))
    {
        float frameDiff = frame - (float)frames[start];
        if (frameDiff == 0.0f)
        {
            *v = values[start];
        }
        else
        {
            float delta = frameDiff / (frames[end] - frames[start]);
            v->Slerp(values[start],values[end],delta);
        }
    }
    else
    {
        *v = values[start];
    }
}

//--------------------------------------------------------------
void tQuaternionChannel::GetMatrix(float frame, Matrix* v) const
{
    P3DASSERT(values);

    Quaternion q;

    GetQuaternion(frame,&q);
    q.ConvertToMatrix(v); 
}

//**************************************************************
// Class tCompressedQuaternionChannel
//**************************************************************
tCompressedQuaternionChannel::tCompressedQuaternionChannel(int nKeys, tAnimationMemoryBlock* block) :
    tRotationChannel(nKeys,block),
    values(NULL)
{
    dataType = ROTATION;
    if (!memoryBlock)
    {
        values = new tCompressedQuaternion[numKeys];
    }
    else
    {
        values = (tCompressedQuaternion*)(memoryBlock->Allocate(numKeys*sizeof(tCompressedQuaternionChannel::tCompressedQuaternion)));
        for (int i = 0; i < numKeys; i++)
        {
            new(&values[i])tCompressedQuaternion;
        }
    }
}

//--------------------------------------------------------------
void tCompressedQuaternionChannel::CleanUp()
{
    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tRotationChannel::CleanUp();
}

//--------------------------------------------------------------
void tCompressedQuaternionChannel::GetEuler(float frame, Vector* v) const
{
    P3DASSERT(values);
    Matrix m;
    GetMatrix(frame,&m);
    v->ConvertToEulerYZX(m);
}

//--------------------------------------------------------------
void tCompressedQuaternionChannel::GetQuaternion(float frame, Quaternion* v) const
{
    P3DASSERT(values);

    int start, end;
  
    if ((FindBracketKeys(frame, start, end))&&(interpolate))
    {
        float frameDiff = frame - (float)frames[start];
        if (frameDiff == 0.0f)
        {
            *v = values[start].UnCompress();
        }
        else
        {
            float delta = frameDiff / (frames[end] - frames[start]);
            Quaternion startValue, endValue;
            startValue = values[start].UnCompress();
            endValue = values[end].UnCompress();

            v->Slerp(startValue,endValue,delta);
        }
    }
    else
    {
        *v = values[start].UnCompress();
    }
}

//--------------------------------------------------------------
void tCompressedQuaternionChannel::GetMatrix(float frame, Matrix* v) const
{
    P3DASSERT(values);

    Quaternion q;

    GetQuaternion(frame,&q);
    q.ConvertToMatrix(v); 
}

//**************************************************************
// Class tStringChannel
//**************************************************************
tStringChannel::tStringChannel(int nKeys, tAnimationMemoryBlock* block) :
    tChannel(nKeys),
    values(NULL)
{
    dataType = STRING;
    values = new tName[numKeys];
}

//--------------------------------------------------------------
void tStringChannel::CleanUp()
{
    delete [] values;
    values = NULL;
    tChannel::CleanUp();
}

//--------------------------------------------------------------
void tStringChannel::GetValue(float frame, tName** v) const
{
    P3DASSERT(values);

    int start, end;
  
    FindBracketKeys(frame, start, end);
    *v = &values[start];
}

//**************************************************************
// Class tEntityChannel
//**************************************************************
tEntityChannel::tEntityChannel(int nKeys, tAnimationMemoryBlock* block) :
    tChannel(nKeys,block),
    values(NULL)
{
    dataType = ENTITY;
    if (!memoryBlock)
    {
        values = new tEntity*[numKeys];
    }
    else
    {
        values = (tEntity**)(memoryBlock->Allocate(numKeys*sizeof(tEntity*)));
    }
    memset(values,0,sizeof(tEntity*)*numKeys);

    // For Entity channels the interpolate flag is a bitmask.
    interpolate = ~0;
}

//--------------------------------------------------------------
void tEntityChannel::CleanUp()
{
    for (int i = 0; i < numKeys; i++)
    {
        tRefCounted::Release(values[i]);
    }
    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tChannel::CleanUp();
}

//--------------------------------------------------------------
void tEntityChannel::GetValue(float frame, tEntity** v) const
{
    P3DASSERT(values);

    int start, end;
  
    FindBracketKeys(frame, start, end);
    *v = values[start];
}


void tEntityChannel::FindEndKeys( float frame, float* beginframe, tEntity** beginkey, float* endframe, tEntity**endkey ) const
{
    P3DASSERT(values);

    int start, end;
  
    FindBracketKeys(frame, start, end);

    *endkey = values[ end ];
    *beginkey = values[ start ];

    *beginframe = frames[ start ];
    *endframe = frames[ end ];
}

//**************************************************************
// Class tBoolChannel
//**************************************************************
tBoolChannel::tBoolChannel(int nKeys, tAnimationMemoryBlock* block) :
    tChannel(nKeys,block),
    startState(true)
{
    dataType = BOOL;
}

//--------------------------------------------------------------
void tBoolChannel::CleanUp()
{
    tChannel::CleanUp();
}

//--------------------------------------------------------------
void tBoolChannel::GetValue(float frame, bool* v) const
{
    *v = startState;
    for (int i = 0; i < numKeys; i++)
    {
        if (frame < (float)frames[i])
        {
            break;
        }
        *v = !(*v);
    }
}

//**************************************************************
// Class tColourChannel
//**************************************************************
tColourChannel::tColourChannel(int nKeys, tAnimationMemoryBlock* block) :
    tChannel(nKeys,block),
    values(NULL)
{
    dataType = COLOUR;
    if (!memoryBlock)
    {
        values = new tColour[numKeys];
    }
    else
    {
        values = (tColour*)(memoryBlock->Allocate(numKeys*sizeof(tColour)));
        for (int i = 0; i < numKeys; i++)
        {
            new(&values[i])tColour;
        }
    }
}

//--------------------------------------------------------------
void tColourChannel::CleanUp()
{
    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tChannel::CleanUp();
}

//--------------------------------------------------------------
void tColourChannel::GetValue(float frame, tColour* v) const
{
    P3DASSERT(values);

    int start, end;

    if ((FindBracketKeys(frame, start, end))&&(interpolate))
    {
        int frameDiff = FtoL(frame) - frames[start];
        if (frameDiff == 0)
        {
            *v = values[start];
        }
        else
        {
            int deltaFrame = frames[end] - frames[start];
            
            int r1 = values[start].Red();
            int g1 = values[start].Green();
            int b1 = values[start].Blue();
            int a1 = values[start].Alpha();

            int r2 = values[end].Red();
            int g2 = values[end].Green();
            int b2 = values[end].Blue();
            int a2 = values[end].Alpha();

            int r = r1 + (((r2 - r1) * frameDiff) / deltaFrame);
            int g = g1 + (((g2 - g1) * frameDiff) / deltaFrame);
            int b = b1 + (((b2 - b1) * frameDiff) / deltaFrame);
            int a = a1 + (((a2 - a1) * frameDiff) / deltaFrame);
            
            v->Set(r,g,b,a);         
        }
    }
    else
    {
        *v = values[start];
    }
}

//**************************************************************
// Class tEventChannel
//**************************************************************
tEventChannel::tEventChannel(int nKeys, tAnimationMemoryBlock* block) : 
    tChannel(nKeys,block),
    values(NULL)
{
    dataType = EVENT;
    if (!memoryBlock)
    {
        values = new tEvent*[numKeys];
    }
    else
    {
        values = (tEvent**)(memoryBlock->Allocate(numKeys*sizeof(tEvent*)));
    }
    memset(values,0,sizeof(tEvent*)*numKeys);
}

//--------------------------------------------------------------
void tEventChannel::CleanUp()
{
    for(int i = 0; i < numKeys; i++)
    {
        tRefCounted::Release(values[i]);
    }

    if (!memoryBlock)
    {
        delete [] values;
        values = NULL;
    }
    tChannel::CleanUp();
}

//--------------------------------------------------------------
void tEventChannel::DispatchEvents (tEventRecipient* recip, 
                                    tEventOrigin* origin,
                                    const tName& eventAddress,
                                    float startTime, 
                                    float endTime) const
{
    if(startTime == 0.0f)
    {  
        //
        // Fudge startTime backwards slightly so that events at 0 will be executed
        //
        startTime = -0.00001f;
    }

    for(int i = 0; i < numKeys; i++)
    {
        float frame = frames[i];
        if(values[i] && (frame > startTime) && (frame <= endTime))
        {
            recip->Dispatch(values[i], origin, eventAddress);
        }
    }
}

//**************************************************************
// Class tChannelLoader
//**************************************************************
bool tChannelLoader::HandlesChunk(unsigned chunkID)
{
    bool isHandled = false;
    switch(chunkID)
    {
        case Pure3D::Animation::ChannelData::FLOAT_1:
        case Pure3D::Animation::ChannelData::FLOAT_2:
        case Pure3D::Animation::ChannelData::VECTOR_1DOF:
        case Pure3D::Animation::ChannelData::VECTOR_2DOF:
        case Pure3D::Animation::ChannelData::VECTOR_3DOF:
        case Pure3D::Animation::ChannelData::QUATERNION:
        case Pure3D::Animation::ChannelData::STRING:
        case Pure3D::Animation::ChannelData::ENTITY:
        case Pure3D::Animation::ChannelData::BOOL:
        case Pure3D::Animation::ChannelData::COLOUR:
        case Pure3D::Animation::ChannelData::EVENT:
        {
            isHandled = true;
            break;
        }
        default:
        {
            isHandled = false;
            break;
        }
    }
    return(isHandled);
}

//--------------------------------------------------------------
tChannel* tChannelLoader::LoadChannel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    tChannel* channel = NULL;

    switch (file->GetCurrentID())
    {
        case Pure3D::Animation::ChannelData::INT:
        {
            channel = LoadIntChannel(file, store, block);
            break;
        }
        case Pure3D::Animation::ChannelData::FLOAT_1:
        {
            channel = LoadFloat1Channel(file, store, block);
            break;
        }

        case Pure3D::Animation::ChannelData::FLOAT_2:
        {
            channel = LoadFloat2Channel(file, store, block);
            break;
        }

        case Pure3D::Animation::ChannelData::VECTOR_1DOF:
        case Pure3D::Animation::ChannelData::VECTOR_2DOF:
        case Pure3D::Animation::ChannelData::VECTOR_3DOF:
        {
            channel = LoadVectorChannel(file, store, block);
            break;
        }

        case Pure3D::Animation::ChannelData::QUATERNION:
        case Pure3D::Animation::ChannelData::COMPRESSED_QUATERNION:
        {
            channel = LoadRotationChannel(file, store, block);
            break;
        }

        case Pure3D::Animation::ChannelData::STRING:
        {
            channel = LoadStringChannel(file, store, block);
            break;
        }

        case Pure3D::Animation::ChannelData::ENTITY:
        {
            channel = LoadEntityChannel(file, store, block);
            break;
        }

        case Pure3D::Animation::ChannelData::BOOL:
        {
            channel = LoadBoolChannel(file, store, block);
            break;
        }

        case Pure3D::Animation::ChannelData::COLOUR:
        {
            channel = LoadColourChannel(file, store, block);
            break;
        }

        case Pure3D::Animation::ChannelData::EVENT:
        {
            channel = LoadEventChannel(file, store, block);
            break;
        }

        default:
            break;
    }

    return(channel);
}

//--------------------------------------------------------------
tIntChannel* tChannelLoader::LoadIntChannel(tChunkFile *file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    P3DASSERT(file->GetCurrentID() == Pure3D::Animation::ChannelData::INT);

    unsigned int version = file->GetLong();
    P3DASSERT(version == INT_CHANNEL_VERSION);

    unsigned param = file->GetLong();
    unsigned nKeys = file->GetLong();  
    
    tIntChannel* channel = NULL;
    if (!block)
    {
        channel = new tIntChannel(nKeys);
    }
    else
    {
        channel = new(block->Allocate(sizeof(tIntChannel)))tIntChannel(nKeys,block);
    }
    channel->channelCode = param;
    file->GetData(channel->frames,nKeys,tFile::WORD);
    file->GetData(channel->values,nKeys,tFile::DWORD);

    while(file->ChunksRemaining())
    {
        switch(file->BeginChunk())
        {
            case Pure3D::Animation::ChannelData::INTERPOLATION_MODE:
            {
                unsigned int modeVersion = file->GetLong();
                P3DASSERT(modeVersion == INTERPOLATION_MODE_VERSION);
                channel->interpolate = file->GetLong();
                break;
            }
        }
        file->EndChunk();
    }
    
    return channel;
}

//--------------------------------------------------------------
tFloat1Channel* tChannelLoader::LoadFloat1Channel(tChunkFile *file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    P3DASSERT(file->GetCurrentID() == Pure3D::Animation::ChannelData::FLOAT_1);

    unsigned int version = file->GetLong();
    P3DASSERT(version == FLOAT1_CHANNEL_VERSION);

    unsigned param = file->GetLong();
    unsigned nKeys = file->GetLong();  
    
    tFloat1Channel* channel = NULL;
    if (!block)
    {
        channel = new tFloat1Channel(nKeys);
    }
    else
    {
        channel = new(block->Allocate(sizeof(tFloat1Channel)))tFloat1Channel(nKeys,block);
    }
    channel->channelCode = param;
    file->GetData(channel->frames,nKeys,tFile::WORD);
    file->GetData(channel->values,nKeys,tFile::DWORD);

    while(file->ChunksRemaining())
    {
        switch(file->BeginChunk())
        {
            case Pure3D::Animation::ChannelData::INTERPOLATION_MODE:
            {
                unsigned int modeVersion = file->GetLong();
                P3DASSERT(modeVersion == INTERPOLATION_MODE_VERSION);
                channel->interpolate = file->GetLong() != 0;
                break;
            }
        }
        file->EndChunk();
    }

    return channel;
}

//--------------------------------------------------------------
tFloat2Channel* tChannelLoader::LoadFloat2Channel(tChunkFile *file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    P3DASSERT(file->GetCurrentID() == Pure3D::Animation::ChannelData::FLOAT_2);

    unsigned int version = file->GetLong();
    P3DASSERT(version == FLOAT2_CHANNEL_VERSION);

    unsigned param = file->GetLong();
    unsigned nKeys = file->GetLong();  
    
    tFloat2Channel* channel = NULL;
    if (!block)
    {
        channel = new tFloat2Channel(nKeys);
    }
    else
    {
        channel = new(block->Allocate(sizeof(tFloat2Channel)))tFloat2Channel(nKeys,block);
    }
    channel->channelCode = param;
    file->GetData(channel->frames,nKeys,tFile::WORD);
    file->GetData(channel->values,nKeys*2,tFile::DWORD);

    while(file->ChunksRemaining())
    {
        switch(file->BeginChunk())
        {
            case Pure3D::Animation::ChannelData::INTERPOLATION_MODE:
            {
                unsigned int modeVersion = file->GetLong();
                P3DASSERT(modeVersion == INTERPOLATION_MODE_VERSION);
                channel->interpolate = file->GetLong();
                break;
            }
        }
        file->EndChunk();
    }

    return channel;
}

//--------------------------------------------------------------
tVectorChannel* tChannelLoader::LoadVectorChannel(tChunkFile *file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    switch (file->GetCurrentID())
    {
        case Pure3D::Animation::ChannelData::VECTOR_1DOF:
        {
            unsigned int version = file->GetLong();
            P3DASSERT(version == VECTOR1DOF_CHANNEL_VERSION);

            unsigned param = file->GetLong();
            int mapping = file->GetWord();
            Vector constants;
            constants.x = file->GetFloat();
            constants.y = file->GetFloat();
            constants.z = file->GetFloat();
            unsigned nKeys = file->GetLong();  
                
            tVector1DOFChannel* channel = NULL;
            if (!block)
            {
                channel = new tVector1DOFChannel(nKeys);
            }
            else
            {
                channel = new(block->Allocate(sizeof(tVector1DOFChannel)))tVector1DOFChannel(nKeys,block);
            }
            channel->channelCode = param;
            channel->dynamicIndex = mapping;
            channel->constants = constants;
            file->GetData(channel->frames,nKeys,tFile::WORD);
            file->GetData(channel->values,nKeys,tFile::DWORD);

            while(file->ChunksRemaining())
            {
                switch(file->BeginChunk())
                {
                    case Pure3D::Animation::ChannelData::INTERPOLATION_MODE:
                    {
                        unsigned int modeVersion = file->GetLong();
                        P3DASSERT(modeVersion == INTERPOLATION_MODE_VERSION);
                        channel->interpolate = file->GetLong();
                        break;
                    }
                }
                file->EndChunk();
            }
            
            return channel;
            break;
        }

        case Pure3D::Animation::ChannelData::VECTOR_2DOF:
        {
            unsigned int version = file->GetLong();
            P3DASSERT(version == VECTOR2DOF_CHANNEL_VERSION);

            unsigned param = file->GetLong();
            int mapping = file->GetWord();
            Vector constants;
            constants.x = file->GetFloat();
            constants.y = file->GetFloat();
            constants.z = file->GetFloat();
            unsigned nKeys = file->GetLong();  
                
            tVector2DOFChannel* channel = NULL;
            if (!block)
            {
                channel = new tVector2DOFChannel(nKeys);
            }
            else
            {
                channel = new(block->Allocate(sizeof(tVector2DOFChannel)))tVector2DOFChannel(nKeys,block);
            }
            channel->channelCode = param;
            channel->staticIndex = mapping;
            channel->constants = constants;
            file->GetData(channel->frames,nKeys,tFile::WORD);
            file->GetData(channel->values,nKeys*2,tFile::DWORD);

            while(file->ChunksRemaining())
            {
                switch(file->BeginChunk())
                {
                    case Pure3D::Animation::ChannelData::INTERPOLATION_MODE:
                    {
                        unsigned int modeVersion = file->GetLong();
                        P3DASSERT(modeVersion == INTERPOLATION_MODE_VERSION);
                        channel->interpolate = file->GetLong();
                        break;
                    }
                }
                file->EndChunk();
            }
            
            return channel;
            break;
        }

        case Pure3D::Animation::ChannelData::VECTOR_3DOF:
        {
            unsigned int version = file->GetLong();
            P3DASSERT(version == VECTOR3DOF_CHANNEL_VERSION);

            unsigned param = file->GetLong();
            unsigned nKeys = file->GetLong();  
                
            tVector3DOFChannel* channel = NULL;
            if (!block)
            {
                channel = new tVector3DOFChannel(nKeys);
            }
            else
            {
                channel = new(block->Allocate(sizeof(tVector3DOFChannel)))tVector3DOFChannel(nKeys,block);
            }
            channel->channelCode = param;
            file->GetData(channel->frames,nKeys,tFile::WORD);
            file->GetData(channel->values,nKeys*3,tFile::DWORD);

            while(file->ChunksRemaining())
            {
                switch(file->BeginChunk())
                {
                    case Pure3D::Animation::ChannelData::INTERPOLATION_MODE:
                    {
                        unsigned int modeVersion = file->GetLong();
                        P3DASSERT(modeVersion == INTERPOLATION_MODE_VERSION);
                        channel->interpolate = file->GetLong();
                        break;
                    }
                }
                file->EndChunk();
            }

            return channel;
            break;
        }

        default:
        {
            P3DASSERT(0);
            return NULL;
            break;
        }
    }
}
//--------------------------------------------------------------
tRotationChannel* tChannelLoader::LoadRotationChannel(tChunkFile *file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    switch (file->GetCurrentID())
    {
        case Pure3D::Animation::ChannelData::QUATERNION:
        {
            unsigned int version = file->GetLong();
            P3DASSERT(version == QUATERNION_CHANNEL_VERSION);

            unsigned    param = file->GetLong();
            unsigned    nKeys = file->GetLong();

            unsigned int frameSize = sizeof( short ) * nKeys;
            unsigned int valueSize = sizeof( Quaternion ) * nKeys;
            rAssert( frameSize <= BUFFER1SIZE );
            rAssert( valueSize <= BUFFER2SIZE );
            short*      frames = (short*)buffer1;
            Quaternion* values = (Quaternion*)buffer2;

            file->GetData(frames,nKeys,tFile::WORD);
            file->GetData(values,nKeys*4,tFile::DWORD);
            unsigned int quatFormat = 0;
            bool interpolate = true;

            while(file->ChunksRemaining())
            {
                switch(file->BeginChunk())
                {
                    case Pure3D::Animation::ChannelData::QUATERNION_FORMAT:
                    {
                        unsigned int quatFormatVersion = file->GetLong();
                        P3DASSERT(quatFormatVersion == QUATERNION_FORMAT_VERSION);
                        quatFormat = file->GetLong();
                        break;
                    }
                    case Pure3D::Animation::ChannelData::INTERPOLATION_MODE:
                    {
                        unsigned int modeVersion = file->GetLong();
                        P3DASSERT(modeVersion == INTERPOLATION_MODE_VERSION);
                        interpolate = file->GetLong() != 0;
                        break;
                    }
                }
                file->EndChunk();
            }
            
            switch (quatFormat)
            {
                case Pure3DAnimationChannels::Quaternion::ONE_DOT_FIFTEEN:
                {
                    tCompressedQuaternionChannel* channel = NULL;
                    if (!block)
                    {
                        channel = new tCompressedQuaternionChannel(nKeys);
                    }
                    else
                    {
                        channel = new(block->Allocate(sizeof(tCompressedQuaternionChannel)))tCompressedQuaternionChannel(nKeys,block);
                    }
                    channel->channelCode = param;
                    channel->interpolate = interpolate;
                    for (unsigned i = 0; i < nKeys; i++)
                    {
                        channel->frames[i] = frames[i];
                        channel->values[i].Compress(values[i]);
                    }
                    return channel;
                    break;
                }
                default:
                {
                    tQuaternionChannel* channel = NULL;
                    if (!block)
                    {
                        channel = new tQuaternionChannel(nKeys);
                    }
                    else
                    {
                        channel = new(block->Allocate(sizeof(tQuaternionChannel)))tQuaternionChannel(nKeys,block);
                    }
                    channel->channelCode = param;
                    channel->interpolate = interpolate;
                    for (unsigned i = 0; i < nKeys; i++)
                    {
                        channel->frames[i] = frames[i];
                        channel->values[i] = values[i];
                    }
                    return channel;
                    break;
                }
            }
            break;
        }

        case Pure3D::Animation::ChannelData::COMPRESSED_QUATERNION:
        {
            unsigned int version = file->GetLong();
            P3DASSERT(version == COMPRESSED_QUATERNION_CHANNEL_VERSION);

            unsigned    param = file->GetLong();
            unsigned    nKeys = file->GetLong();

            tCompressedQuaternionChannel* channel = NULL;
            if (!block)
            {
                channel = new tCompressedQuaternionChannel(nKeys);
            }
            else
            {
                channel = new(block->Allocate(sizeof(tCompressedQuaternionChannel)))tCompressedQuaternionChannel(nKeys,block);
            }
            channel->channelCode = param;
            file->GetData(channel->frames,nKeys,tFile::WORD);
            file->GetData(channel->values,nKeys*4,tFile::WORD);

            while(file->ChunksRemaining())
            {
                switch(file->BeginChunk())
                {
                    case Pure3D::Animation::ChannelData::INTERPOLATION_MODE:
                    {
                        unsigned int modeVersion = file->GetLong();
                        P3DASSERT(modeVersion == INTERPOLATION_MODE_VERSION);
                        channel->interpolate = file->GetLong();
                        break;
                    }
                }
                file->EndChunk();
            }

            return channel;
            break;
        }

        default:
        {
            P3DASSERT(0);
            return NULL;
            break;
        }
    }
}

//--------------------------------------------------------------
tStringChannel* tChannelLoader::LoadStringChannel(tChunkFile *file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    P3DASSERT(file->GetCurrentID() == Pure3D::Animation::ChannelData::STRING);

    unsigned int version = file->GetLong();
    P3DASSERT(version == STRING_CHANNEL_VERSION);

    unsigned param = file->GetLong();
    int nKeys = file->GetLong();  
    
    tStringChannel* channel = new tStringChannel(nKeys);
    channel->channelCode = param;
    file->GetData(channel->frames,nKeys,tFile::WORD);

    for(int i = 0; i < nKeys; i++)
    {
        char str[256];
        file->GetPString(str);
        channel->values[i].SetText(str);
    }      
    return channel;
}

//--------------------------------------------------------------
tEntityChannel* tChannelLoader::LoadEntityChannel(tChunkFile *file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    P3DASSERT(file->GetCurrentID() == Pure3D::Animation::ChannelData::ENTITY);

    unsigned int version = file->GetLong();
    P3DASSERT(version == ENTITY_CHANNEL_VERSION);

    unsigned param = file->GetLong();
    int nKeys = file->GetLong();  

    tEntityChannel* channel = NULL;
    if (!block)
    {
        channel = new tEntityChannel(nKeys);
    }
    else
    {
        channel = new(block->Allocate(sizeof(tEntityChannel)))tEntityChannel(nKeys,block);
    }
    channel->channelCode = param;
    file->GetData(channel->frames,nKeys,tFile::WORD);

    for(int i = 0; i < nKeys; i++)
    {
        char name[256];
        file->GetPString(name);
        channel->values[i] = p3d::find<tEntity>(store, name);
        if (channel->values[i])
        {
            channel->values[i]->AddRef();
        }
    }
    
    //read in interpolation mode for entity channel, since
    //vertex animation object has to use this mode to see
    //which component should be interoploated on.
    while(file->ChunksRemaining())
    {
        switch(file->BeginChunk())
        {
            case Pure3D::Animation::ChannelData::INTERPOLATION_MODE:
            {
                unsigned int modeVersion = file->GetLong();
                P3DASSERT(modeVersion == INTERPOLATION_MODE_VERSION);
                // For Entity channels the interpolate flag is a bitmask.
                channel->interpolate = file->GetLong( );
                break;
            }
        }
        file->EndChunk();
    }
    return channel;
}

//--------------------------------------------------------------
tBoolChannel* tChannelLoader::LoadBoolChannel(tChunkFile *file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    P3DASSERT(file->GetCurrentID() == Pure3D::Animation::ChannelData::BOOL);

    unsigned int version = file->GetLong();
    P3DASSERT(version == BOOL_CHANNEL_VERSION);

    unsigned param = file->GetLong();
    bool startState = (file->GetWord()==1);
    unsigned nKeys = file->GetLong();  
    
    tBoolChannel* channel = NULL;
    if (!block)
    {
        channel = new tBoolChannel(nKeys);
    }
    else
    {
        channel = new(block->Allocate(sizeof(tBoolChannel)))tBoolChannel(nKeys,block);
    }
    channel->channelCode = param;
    channel->startState = startState;
    file->GetData(channel->frames,nKeys,tFile::WORD);
    return channel;
}

//--------------------------------------------------------------
tColourChannel* tChannelLoader::LoadColourChannel(tChunkFile *file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    P3DASSERT(file->GetCurrentID() == Pure3D::Animation::ChannelData::COLOUR);

    unsigned int version = file->GetLong();
    P3DASSERT(version == COLOUR_CHANNEL_VERSION);

    unsigned param = file->GetLong();
    unsigned nKeys = file->GetLong();  
    
    tColourChannel* channel = NULL;
    if (!block)
    {
        channel = new tColourChannel(nKeys);
    }
    else
    {
        channel = new(block->Allocate(sizeof(tColourChannel)))tColourChannel(nKeys,block);
    }
    channel->channelCode = param;
    file->GetData(channel->frames,nKeys,tFile::WORD);
    file->GetData(channel->values,nKeys,tFile::DWORD);

    while(file->ChunksRemaining())
    {
        switch(file->BeginChunk())
        {
            case Pure3D::Animation::ChannelData::INTERPOLATION_MODE:
            {
                unsigned int modeVersion = file->GetLong();
                P3DASSERT(modeVersion == INTERPOLATION_MODE_VERSION);
                channel->interpolate = file->GetLong();
                break;
            }
        }
        file->EndChunk();
    }
    
    return channel;
}

//--------------------------------------------------------------
tEventChannel* tChannelLoader::LoadEventChannel(tChunkFile *file, tEntityStore* store, tAnimationMemoryBlock* block)
{
    P3DASSERT(file->GetCurrentID() == Pure3D::Animation::ChannelData::EVENT);

    unsigned int version = file->GetLong();
    P3DASSERT(version == EVENT_CHANNEL_VERSION);

    unsigned param = file->GetLong();
    unsigned nKeys = file->GetLong();
    
    tEventChannel* channel = NULL;
    if (!block)
    {
        channel = new tEventChannel(nKeys);
    }
    else
    {
        channel = new(block->Allocate(sizeof(tEventChannel)))tEventChannel(nKeys,block);
    }
    channel->channelCode = param;

    int eventCount = 0;

    while(file->ChunksRemaining())
    {
        switch(file->BeginChunk())
        {
            case Pure3D::Animation::ChannelData::EVENT_OBJECT:
            {
                char name[256];  file->GetString(name);
                unsigned param = file->GetLong();
                short time     = file->GetWord();

                //
                // Are there any Data Sub Chunks
                //
                tBaseEvent* event = NULL;
                if(file->ChunksRemaining())
                {
                    unsigned dataID = file->BeginChunk();
                    
                    if(dataID == Pure3D::Animation::ChannelData::EVENT_OBJECT_DATA_IMAGE)
                    {

                        char dataFormat[32];  file->GetString(dataFormat);
                        unsigned dataLength = file->GetLong();
                        char* data = new char[dataLength];
                        file->GetData(data, dataLength);
                    
                        tGenericEvent* genEvent = new tGenericEvent;
                        event = genEvent;
                        //
                        // Now bind the stringData
                        //
                        long formatLength = strlen(dataFormat);
                        char dataPosition = 0;
                        for(int i=0;i<formatLength;i++)
                        {
                            switch(dataFormat[i])
                            {
                                case 'i':
                                {
                                    dataPosition += sizeof(int);
                                }
                                break;
                                case 'f':
                                {
                                    dataPosition += sizeof(float);
                                }
                                break;
                                case 's':
                                {
                                    const char** stringAddress = (const char **)&data[dataPosition];
                                    long  pointerOffset  = (long)data[dataPosition];
                                    char* stringPosition = data+pointerOffset;
                                    *stringAddress = stringPosition;
                                    dataPosition += sizeof(char*);
                                }
                                break;

                                default:
                                    break;
                            }
                        }

                        genEvent->SetDataLength(dataLength);
                        genEvent->SetData(data, true);
                    }

                    file->EndChunk();
                }
                else
                {
                    //
                    // Create a simple event
                    //
                    event = new tBaseEvent;
                }

                event->SetName(name);
                event->SetParameter(param);
                channel->frames[eventCount] = time;
                channel->values[eventCount] = event;
                channel->values[eventCount]->AddRef();
                eventCount++;
            }
            break;
            
            default:
                break;

        } // switch chunkID
        file->EndChunk();
    }

    return channel;
}
    

