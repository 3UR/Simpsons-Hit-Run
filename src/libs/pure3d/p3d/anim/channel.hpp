/*===========================================================================
    channel.hpp
    26-Sep-97 Created by Neall
    04-Jan-00 Ported to v12

    Copyright (c)1997-1999 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _CHANNEL_HPP
#define _CHANNEL_HPP

#include <radmath/radmath.hpp>
#include <p3d/p3dtypes.hpp>
#include <p3d/refcounted.hpp>

class tChunkFile;
class tEntityStore;
class tName;
class tEntity;
class tEventOrigin;
class tEventRecipient;
class tEvent;
class tChannelLoader;
class tAnimationMemoryBlock;

//**************************************************************
// Class tChannel
//**************************************************************
class tChannel : public tNonCopyable
{
public:
    enum 
    {
        UNASSIGNED,
        INT,
        FLOAT1,
        FLOAT2,
        VECTOR,
        ROTATION,
        STRING,
        ENTITY,
        BOOL,
        COLOUR,
        EVENT
    };   

    tChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tChannel() {}

    virtual void CleanUp();
 
    unsigned GetDataType() const            { return dataType; }
    unsigned GetChannelCode() const         { return channelCode; }
    bool IsInterpolated() const             { return interpolate != 0; }
    unsigned GetInterpolate( ) const        { return interpolate;}

protected:
    tChannel() : channelCode(0), dataType(UNASSIGNED), numKeys(0), frames(NULL) {}

    bool FindBracketKeys(float frame, int& beginKeys, int& endKey) const;

    tAnimationMemoryBlock*  memoryBlock;
    unsigned                channelCode;
	unsigned short          interpolate : 1;
	unsigned short          dataType : 7;
    short                   numKeys;
    short*                  frames;

    friend class tChannelLoader;
};

//**************************************************************
// Class tIntChannel
//**************************************************************
class tIntChannel : public tChannel
{
public:
    tIntChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tIntChannel() { tIntChannel::CleanUp(); }
    
    virtual void CleanUp();

    virtual void GetValue(float frame, int* v) const;

protected:
    tIntChannel() : tChannel(), values(NULL) {}

    int* values;

    friend class tChannelLoader;
};

//**************************************************************
// Class tFloat1Channel
//**************************************************************
class tFloat1Channel : public tChannel
{
public:
    tFloat1Channel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tFloat1Channel() { tFloat1Channel::CleanUp(); }
    
    virtual void CleanUp();

    virtual void GetValue(float frame, float* v) const;

protected:
    tFloat1Channel() : tChannel(), values(NULL) {}

    float* values;

    friend class tChannelLoader;
};

//**************************************************************
// Class tFloat2Channel
//**************************************************************
class tFloat2Channel : public tChannel
{
public:
    tFloat2Channel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tFloat2Channel() { tFloat2Channel::CleanUp(); }
    
    virtual void CleanUp();

    virtual void GetValue(float frame, rmt::Vector2* v) const;

protected:
    tFloat2Channel() : tChannel(), values(NULL) {}

    rmt::Vector2* values;

    friend class tChannelLoader;
};

//**************************************************************
// Class tVectorChannel
//**************************************************************
class tVectorChannel : public tChannel
{
public:
    tVectorChannel(int nKeys, tAnimationMemoryBlock* block = NULL) : tChannel(nKeys,block) { dataType = VECTOR; }
    virtual ~tVectorChannel() {}
    
    virtual void GetValue(float frame, rmt::Vector* v) const = 0;

protected:
    tVectorChannel() : tChannel() {}

    friend class tChannelLoader;
};

//**************************************************************
// Class tVector1DOFChannel
//**************************************************************
class tVector1DOFChannel : public tVectorChannel
{
public:
    tVector1DOFChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tVector1DOFChannel() { tVector1DOFChannel::CleanUp(); }

    virtual void CleanUp();

    virtual void GetValue(float frame, rmt::Vector* v) const;

protected:
    tVector1DOFChannel() : tVectorChannel(), dynamicIndex(0), constants(0.0f,0.0f,0.0f), values(NULL) {} 

    int         dynamicIndex;
    rmt::Vector constants;
    float*      values; 

    friend class tChannelLoader;
};

//**************************************************************
// Class tVector2DOFChannel
//**************************************************************
class tVector2DOFChannel : public tVectorChannel
{
public:
    tVector2DOFChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tVector2DOFChannel() { tVector2DOFChannel::CleanUp(); }
         
    virtual void CleanUp();

    virtual void GetValue(float frame, rmt::Vector* v) const;

protected:
    tVector2DOFChannel() : tVectorChannel(), staticIndex(0), constants(0.0f,0.0f,0.0f), values(NULL) {} 

    int            staticIndex;
    rmt::Vector    constants;
    rmt::Vector2*  values;
    
    friend class tChannelLoader;
};

//**************************************************************
// Class tVector3DOFChannel
//**************************************************************
class tVector3DOFChannel : public tVectorChannel
{
public:
    tVector3DOFChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tVector3DOFChannel() { tVector3DOFChannel::CleanUp(); }
         
    virtual void CleanUp();

    virtual void GetValue(float frame, rmt::Vector* v) const;

protected:
    tVector3DOFChannel() : tVectorChannel(), values(NULL) {} 

    rmt::Vector* values;
    
    friend class tChannelLoader;
};

//**************************************************************
// Class tRotationChannel
//**************************************************************
class tRotationChannel : public tChannel
{
public:
    tRotationChannel(int nKeys, tAnimationMemoryBlock* block = NULL) : tChannel(nKeys, block) { dataType = ROTATION; }
    virtual ~tRotationChannel() {}

    virtual void GetEuler(float frame, rmt::Vector* v) const = 0;
    virtual void GetQuaternion(float frame, rmt::Quaternion* v) const = 0;
    virtual void GetMatrix(float frame, rmt::Matrix* v) const = 0;

protected:
    tRotationChannel() : tChannel() {}

    friend class tChannelLoader;
};

//**************************************************************
// Class tQuaternionChannel
//**************************************************************
class tQuaternionChannel : public tRotationChannel
{
public:
    tQuaternionChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tQuaternionChannel() { tQuaternionChannel::CleanUp(); }

    virtual void CleanUp();

    virtual void GetEuler(float frame, rmt::Vector* v) const;
    virtual void GetQuaternion(float frame, rmt::Quaternion* v) const;
    virtual void GetMatrix(float frame, rmt::Matrix* v) const;

protected:
    tQuaternionChannel() : tRotationChannel(), values(NULL) {}

    rmt::Quaternion* values;

    friend class tChannelLoader;
};

//**************************************************************
// Class tCompressedQuaternionChannel
//**************************************************************
class tCompressedQuaternionChannel : public tRotationChannel
{
public:
    tCompressedQuaternionChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tCompressedQuaternionChannel() { tCompressedQuaternionChannel::CleanUp(); }

    virtual void CleanUp();

    virtual void GetEuler(float frame, rmt::Vector* v) const;
    virtual void GetQuaternion(float frame, rmt::Quaternion* v) const;
    virtual void GetMatrix(float frame, rmt::Matrix* v) const;

protected:
    tCompressedQuaternionChannel() : tRotationChannel(), values(NULL) {}
   
    struct tCompressedQuaternion 
    {
    public :
        void Compress(rmt::Quaternion v)
        {
            w = (short)rmt::FtoL(v.w*32767.0f);
            x = (short)rmt::FtoL(v.x*32767.0f);
            y = (short)rmt::FtoL(v.y*32767.0f);
            z = (short)rmt::FtoL(v.z*32767.0f);
        }
        rmt::Quaternion UnCompress() const
        {
            const float invCompressFactor = 1.0f/32767.0f;
            rmt::Quaternion v;

            v.w = ((float)w)*invCompressFactor;
            v.x = ((float)x)*invCompressFactor;
            v.y = ((float)y)*invCompressFactor;
            v.z = ((float)z)*invCompressFactor;
            return v;
        }
        short w,x,y,z;
    };

    tCompressedQuaternion* values;

    friend class tChannelLoader;
};

//**************************************************************
// Class tStringChannel
//**************************************************************
class tStringChannel : public tChannel 
{
public:
    tStringChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tStringChannel() { tStringChannel::CleanUp(); }

    virtual void CleanUp();

    void GetValue(float frame, tName** v) const;

protected:
    tStringChannel() : tChannel(), values(NULL) {}

    tName* values;

    friend class tChannelLoader;
};

//**************************************************************
// Class tEntityChannel
//**************************************************************
class tEntityChannel : public tChannel 
{
public:
    tEntityChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tEntityChannel() { tEntityChannel::CleanUp(); }

    virtual void CleanUp();

    void GetValue(float frame, tEntity** v) const;    
    void FindEndKeys( float frame, float* beginframe, tEntity** beginkey, float* endframe, tEntity** endkey ) const;

protected:
    tEntityChannel() : tChannel(), values(NULL) {}

    tEntity** values;

    friend class tChannelLoader;
};

//**************************************************************
// Class tBoolChannel
//**************************************************************
class tBoolChannel : public tChannel
{
public:
    tBoolChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tBoolChannel() { tBoolChannel::CleanUp(); }
        
    virtual void CleanUp();

    virtual void GetValue(float frame, bool* v) const;
  
protected:
    tBoolChannel() : tChannel(), startState(true) {}
    
    bool startState;   // initial truth state
    
    friend class tChannelLoader;
}; 

//**************************************************************
// Class tColourChannel
//**************************************************************
class tColourChannel : public tChannel
{
public:
    tColourChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tColourChannel() { tColourChannel::CleanUp(); }

    virtual void CleanUp();

    virtual void GetValue(float frame, tColour* v) const;

protected:
    tColourChannel() : tChannel(), values(NULL) {}

    tColour* values;

    friend class tChannelLoader;
};

//**************************************************************
// Class tEventChannel
//**************************************************************
class tEventChannel : public tChannel
{
public:
    tEventChannel(int nKeys, tAnimationMemoryBlock* block = NULL);
    virtual ~tEventChannel() { tEventChannel::CleanUp(); }

    virtual void CleanUp();

    void DispatchEvents( tEventRecipient* recip, 
                         tEventOrigin* origin,
                         const tName& address,
                         float startTime, 
                         float endTime) const;

protected:
    tEventChannel() : tChannel(), values(NULL) {}

    tEvent** values;

    friend class tChannelLoader;
    friend class tEventAnimTextLoader;
};

//**************************************************************
// Class tChannelLoader
//**************************************************************
class tChannelLoader
{
public:
    bool HandlesChunk(unsigned chunkID);
    tChannel* LoadChannel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);

    tIntChannel*      LoadIntChannel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);
    tFloat1Channel*   LoadFloat1Channel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);
    tFloat2Channel*   LoadFloat2Channel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);
    tVectorChannel*   LoadVectorChannel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);
    tRotationChannel* LoadRotationChannel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);
    tStringChannel*   LoadStringChannel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);
    tEntityChannel*   LoadEntityChannel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);
    tBoolChannel*     LoadBoolChannel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);
    tColourChannel*   LoadColourChannel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);
    tEventChannel*    LoadEventChannel(tChunkFile* file, tEntityStore* store, tAnimationMemoryBlock* block = NULL);
protected:
    #define BUFFER1SIZE 1024 * 2
    #define BUFFER2SIZE 1024 * 16
    char buffer1[ BUFFER1SIZE ];
    char buffer2[ BUFFER2SIZE ];
};

#endif // _CHANNEL_HPP

