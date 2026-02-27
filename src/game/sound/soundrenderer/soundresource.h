//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All rights reserved.
//
// File:        soundresource.hpp
//
// Subsystem:   Dark Angel - Sound Resources
//
// Description: Defines sound resource
//
// Modification History:
//  + Created October 11, 2001 -- aking
//
//=============================================================================

#ifndef _SOUNDRESOURCE_HPP
#define _SOUNDRESOURCE_HPP

//=============================================================================
// Included Files
//=============================================================================

#include <radobject.hpp>
#include <radlinkedclass.hpp>

#include <sound/soundrenderer/soundsystem.h>
#include <sound/soundrenderer/idasoundresource.h>

//=============================================================================
// Prototypes
//=============================================================================

class daSoundResourceData;

//=============================================================================
// Class Declarations
//=============================================================================

//
// This contains the type of resource.
//
class daSoundResourceData
    :
    public IDaSoundResource
{
public:
    
    virtual void AddRef( void );
    virtual void Release( void );

    //
    // Constructor and destructor
    //
    daSoundResourceData( );
    virtual ~daSoundResourceData( );

    //
    // IDaSoundResourceData and IDaSoundResource
    //    
    virtual IDaSoundResourceData& AddFilename
    (
        const char* newFileName,
        float trim
    );

    inline virtual unsigned int GetNumFiles( void );
    
    virtual void GetFileNameAt( unsigned int index, char* buffer, unsigned int max );
    virtual void GetFileKeyAt( unsigned int index, char * buffer, unsigned int max );
    
    virtual IDaSoundResourceData& SetPitchRange
    (
        float minPitch,
        float maxPitch
    );
    virtual void GetPitchRange
    (
        float* pMinPitch,
        float* pMaxPitch
    );
    
    virtual IDaSoundResourceData& SetTrimRange
    (
        float minTrim,
        float maxTrim
    );
    virtual void GetTrimRange
    (
        float* pMinTrim,
        float* pMaxTrim
    );
    
    virtual IDaSoundResourceData& SetTrim( float trim );
    
    virtual IDaSoundResourceData& SetStreaming( bool streaming );
    virtual bool GetStreaming( void );

    virtual IDaSoundResourceData& SetLooping( bool looping );
    virtual bool GetLooping( void );

    virtual Type GetType( void );
    virtual void SetSoundGroup( Sound::daSoundGroup soundGroup );
    virtual Sound::daSoundGroup GetSoundGroup( void );
    /// <summary>
    /// 
    /// </summary>
    /// <param name=""></param>
    virtual void CaptureResource( void );
    virtual bool IsCaptured( void );
    virtual void ReleaseResource( void );

    // COMPOSERS ONLY!!
    virtual void Play( void );

    //
    // Create a daSoundResourceData object
    //
    static daSoundResourceData* ObjCreate( radMemoryAllocator allocator );

    //
    // The pitch variation
    //
    short m_MinPitch;
    short m_MaxPitch;
    
    //
    // The sound files for this resource
    //
    
    union FileId {
        char * m_pName;
        radKey32 m_Key;
    };
   
    FileId * m_pFileIds;
    
    
    //
    // The trim variation
    //
    unsigned char m_MinTrim;
    unsigned char m_MaxTrim;
    
    unsigned char m_Flags;

    unsigned char m_NumFiles;
    
    //
    // Hold a capture counter
    //
    unsigned char                       m_CaptureCount;
    unsigned char                       m_SoundGroup;
};

inline unsigned int daSoundResourceData::GetNumFiles( void )
{
    return m_NumFiles;
}
    
#endif //_SOUNDRESOURCE_HPP

