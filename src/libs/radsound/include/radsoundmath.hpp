//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef RADSOUNDMATH_HPP
#define RADSOUNDMATH_HPP

//=============================================================================
// Build Configuration Check
//=============================================================================

#if !defined(RAD_UWP) && !defined(RAD_WIN32)
    #error 'FTech requires definition of RAD_UWP or RAD_WIN32'
#endif

//============================================================================
// Include Files
//============================================================================

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <raddebug.hpp>
#include <float.h>

//============================================================================
// Casting functions (hardware accelerated on some platforms)
//============================================================================

inline float radSoundIntToFloat( int x )
{
    return (float)(x);
}

inline int radSoundFloatToInt( float x )
{
    return (int)(x);
}

inline unsigned int radSoundFloatToUInt( float x )
{
    return (unsigned int) radSoundFloatToInt( x );
}

inline float radSoundUIntToFloat( unsigned int x )
{
    return radSoundIntToFloat( (int) x );
}

inline short radSoundFloatToShort( float x )
{
	return static_cast< short >( x );
}

inline float radSoundShortToFloat( short x )
{
	return static_cast< float >( x );
}

    inline double radSoundFloatToDouble( float x )
    {
        return (double) x;
    }

    inline float radSoundDoubleToFloat( double x )
    {
        return (float) x;
    }

//============================================================================
// Standard Math Functions ( wrapped for easy profiling )
//============================================================================

inline float radSoundFloor( float number )
{
    return floorf( number );
}

inline float radSoundSqr( float number )
{
    return number * number;
}

inline float radSoundSqrt( float number )
{
    return ::sqrtf( number );
}

inline float radSoundCos( float number )
{
    return ::cosf( number );
}

inline float radSoundACos( float number )
{
    return ::acosf( number );
}

inline float radSoundSin( float number )
{
    return ::sinf( number );
}

inline float radSoundASin( float number )
{
    return ::asinf( number );
}

inline float radSoundTan( float number )
{
    return ::tanf( number );
}

inline float radSoundATan( float number )
{
    return ::atanf( number );
}

inline float radSoundPow( float base, float exponent )
{
    return ::powf( base, exponent );
}

//============================================================================
// Memory/string functions (hardware accelerated on some platforms)
//============================================================================

inline void radSoundZeroMemory( void * pMemory, unsigned int bytes )
{
    ::memset( pMemory, 0, bytes );
}

inline void radSoundMemCopy( void * pMemoryDest, const void * pMemorySrc, unsigned int bytes )
{
	::memcpy( pMemoryDest, pMemorySrc, bytes );
}

inline bool radSoundMemCmp( const void * pMem1, const void * pMem2, unsigned int bytes )
{
	return ::memcmp( pMem1, pMem2, bytes ) == 0;
}

//============================================================================
// "In-range" Verification
//============================================================================

bool radSoundIsSampleRateOptimal( float sourceSampleRate, float playbackSampleRate );

inline void radSoundVerifyAnalogVolume( float analog )
{
	// 0 <= analog <= 1
	rAssert( analog >= 0.0f );
    rAssert( analog <= 1.0f );
}

inline void radSoundVerifyDbVolume( float db )
{
	// db <= 20
	rAssert( db <= 20.0f );
}

inline void radSoundVerifyAmplitudeVolume( float amplitude )
{
	// 0 <= volume <= 1
	rAssert( amplitude >= 0.0f );
    rAssert( amplitude <= 1.0f );
}

inline void radSoundVerifyPowerVolume( float power )
{
	// 0 <= power <= 1
	rAssert( power >= 0.0f );
    rAssert( power <= 1.0f );
}

inline void radSoundVerifyAnalogPan( float pan )
{
	// -1 <= pan <= 1
    rAssert( pan <=  1.0f );
    rAssert( pan >= -1.0f );
}

inline void radSoundVerifyAnalogPitch( float pitch )
{
	// pitch > 0;
    rAssert( pitch > 0.0f );
}

inline void radSoundVerifyAngle( float angle )
{
	// 0 <= angle <= 360
    rAssert( angle >= 0.0f );
	rAssert( angle <= 360.0f );
}

// Simple helper function to verify that you have not exceeded the maximum
// delta change for a given value.  (Used by voice to check client's trim/
// volume settings), but is available to all.

#define radSoundVolumeChangeThreshold 0.1f
const unsigned int radSoundMinFadeTime = 334; // .1 at 30fps

inline void radSoundVerifyChangeThreshold( bool doCheck, const char * pValueName, float oldVal, float newVal, float threshold )
{
    #ifndef RAD_RELEASE

        if ( doCheck )
        {
            if ( fabsf( oldVal - newVal ) > ( radSoundVolumeChangeThreshold + 0.0001f ) )
            {
                rTunePrintf( "radsound: TRC Violation: delta [%s] change exceeded [%d%%] was:[%d%%] now:[%d%%]\n",
                    pValueName, radSoundFloatToInt( threshold * 100.0f ),
                    radSoundFloatToInt( oldVal * 100.0f ), radSoundFloatToInt( newVal * 100.0f ) );
            }
        }

    #endif // ! RAD_RELEASE
}

//============================================================================
// Sound Conversion Functions
//============================================================================

//
// Db <--> Analog
//

inline float radSoundVolumeAnalogToDb( float analog )
{
	::radSoundVerifyAnalogVolume( analog );

    float db;

    if ( analog == 0.0f )
    {
        db =  -FLT_MAX;
    }
    else
    {
		db = 40.0f * log10f( analog );
    }

    return db;
}

inline float radSoundVolumeDbToAnalog( float db )
{
	::radSoundVerifyDbVolume( db );
	
	float analog = powf( 10.0f, db / 40 );

	return analog;
}

//
// Analog <-> Amplitude
//

inline float radSoundVolumeAnalogToAmplitude( float analog )
{
	::radSoundVerifyAnalogVolume( analog );

	float amplitude = analog * analog;

    return amplitude;
}

inline float radSoundVolumeAmplitudeToAnalog( float amplitude )
{
    ::radSoundVerifyAmplitudeVolume( amplitude );

    float analog = radSoundSqrt( amplitude );

    return analog;
}

//
// Analog <-> Power
//

inline float radSoundVolumeAnalogToPower( float analog )
{
    ::radSoundVerifyAnalogVolume( analog );

    float power = analog * analog * analog * analog;

    return power;
}

inline float radSoundVolumePowerToAnalog( float power )
{
    ::radSoundVerifyPowerVolume( power );

    float analog = radSoundPow( power, 0.25f );
    
    return analog;
}


//
// Power <-> Amplitude 
//

inline float radSoundPowerToAmplitude( float power )
{
    ::radSoundVerifyPowerVolume( power );

    return radSoundSqrt( power );
}

inline float radSoundAmplitudeToPower( float amplitude )
{
    ::radSoundVerifyAmplitudeVolume( amplitude );

    return amplitude * amplitude;
}

//
// Pan helpers.
//

inline void  radSoundPanToVolumePower( float pan, float * pLPanFactor, float * pRPanFactor )
{
    rAssert( pan >= -1.0f );
    rAssert( pan <= 1.0f );
    
    if ( pan >= 0.0f )
    {
        *pLPanFactor = 1.0f - pan;
        *pRPanFactor = 1.0f;
    }
    else
    {
        *pLPanFactor = 1.0f;
        *pRPanFactor = 1.0f + pan;
    }
}

inline void radSoundLeftRightVolumeToPan( float * pPan, float * pVolume, float leftVol, float rightVol )
{
    ::radSoundVerifyPowerVolume( leftVol );
    ::radSoundVerifyPowerVolume( rightVol );

    if ( leftVol > rightVol )
    {
        *pPan = - ( 1.0f - ( rightVol / leftVol ) );
        *pVolume = leftVol;
    }
    else if ( leftVol < rightVol )
    {
        *pPan = ( 1.0f - ( leftVol / rightVol ) );
        *pVolume = rightVol;
    }
    else
    {
        *pPan = 0.0f;
        *pVolume = leftVol;
    }

    ::radSoundVerifyAnalogPan( *pPan );
    ::radSoundVerifyPowerVolume( *pVolume );
}

inline void  radSoundPanToVolumePowerConstant( float pan, float * pLPanFactor, float* pRPanFactor )
{
    ::radSoundVerifyAnalogPan( pan );

	// Constant power version

    rAssert( pan >= -1.0f );
    rAssert( pan <= 1.0f );   
    
    *pLPanFactor = ( -0.5f * pan ) + 0.5f;
    *pRPanFactor = ( 0.5f * pan ) + 0.5f;
}

inline float radSoundPanAnalogToDb( float pan )
{
    ::radSoundVerifyAnalogPan( pan );

    if ( pan >= 0.0f )
    {
        return - ( radSoundVolumeAnalogToDb( 1.0f - pan ) );
    }

    return radSoundVolumeAnalogToDb( 1.0f + pan );
}

//============================================================================
// Randomization functions
//============================================================================

inline float radSoundRandMinMax( float min, float max )
{
    float v = radSoundIntToFloat( rand() % RAND_MAX ) / radSoundIntToFloat( RAND_MAX );

    return ((v * (max - min)) + min);
}

inline unsigned int radSoundRandMinMax( unsigned int min, unsigned int max )
{
	return (rand( ) % (max - min + 1) ) + min;
}

//============================================================================
// Endian Conversion functions
//============================================================================

inline unsigned int radSoundReverseEndian( unsigned int in )
{
	unsigned int out;
	
	((char*)&out)[ 0 ] = ((char*)&in)[ 3 ];
	((char*)&out)[ 1 ] = ((char*)&in)[ 2 ];
	((char*)&out)[ 2 ] = ((char*)&in)[ 1 ];
	((char*)&out)[ 3 ] = ((char*)&in)[ 0 ];
	
	return out;
}

inline unsigned long radSoundReverseEndian( unsigned long in )
{
	unsigned long out;
	
	((char*)&out)[ 0 ] = ((char*)&in)[ 3 ];
	((char*)&out)[ 1 ] = ((char*)&in)[ 2 ];
	((char*)&out)[ 2 ] = ((char*)&in)[ 1 ];
	((char*)&out)[ 3 ] = ((char*)&in)[ 0 ];
	
	return out;
}

inline signed int radSoundReverseEndian( signed int in )
{
	signed int out;
	
	((char*)&out)[ 0 ] = ((char*)&in)[ 3 ];
	((char*)&out)[ 1 ] = ((char*)&in)[ 2 ];
	((char*)&out)[ 2 ] = ((char*)&in)[ 1 ];
	((char*)&out)[ 3 ] = ((char*)&in)[ 0 ];
	
	return out;
}

inline signed long radSoundReverseEndian( signed long in )
{
	signed long out;
	
	((char*)&out)[ 0 ] = ((char*)&in)[ 3 ];
	((char*)&out)[ 1 ] = ((char*)&in)[ 2 ];
	((char*)&out)[ 2 ] = ((char*)&in)[ 1 ];
	((char*)&out)[ 3 ] = ((char*)&in)[ 0 ];
	
	return out;
}

inline unsigned short radSoundReverseEndian( unsigned short in )
{
	unsigned short out;
	
	((char*)&out)[ 0 ] = ((char*)&in)[ 1 ];
	((char*)&out)[ 1 ] = ((char*)&in)[ 0 ];
	
	return out;	
}

inline signed short radSoundReverseEndian( signed short in )
{
	signed short out;
	
	((char*)&out)[ 0 ] = ((char*)&in)[ 1 ];
	((char*)&out)[ 1 ] = ((char*)&in)[ 0 ];
	
	return out;	
}

//============================================================================
// Struct: radSoundVector
//============================================================================

struct radSoundVector
{
    radSoundVector( void ) : m_x( 0.0f ), m_y( 0.0f ), m_z( 0.0f ) { ; }
    radSoundVector( float x, float y, float z ) : m_x( x ), m_y( y ), m_z( z ) { ; }
     
	float GetLength( void ) const
	{
		return radSoundSqrt( ( m_x * m_x ) + ( m_y * m_y ) + ( m_z * m_z ) );
	}

    void Normalize( void )
	{
		float length = GetLength( );

		m_x /= length;
		m_y /= length;
		m_z /= length; 
	}

    const radSoundVector GetNormal( void ) const
    {
        float length = GetLength( );

        return radSoundVector( m_x / length, m_y / length, m_z / length );
    }

    float Dot( const radSoundVector & v ) const
    {
        return ( m_x * v.m_x ) + ( m_y * v.m_y ) + ( m_z * v.m_z );
    }
    
    const radSoundVector Cross( const radSoundVector & v ) const
    {
        return radSoundVector
        (
            m_y * v.m_z - m_z * v.m_y,
            m_z * v.m_x - m_x * v.m_z,
            m_x * v.m_y - m_y * v.m_x
        );
    }

    const radSoundVector operator+( const radSoundVector & v ) const
    {
        return radSoundVector( m_x + v.m_x, m_y + v.m_y, m_z + v.m_z );
    }

    const radSoundVector operator-( const radSoundVector & v ) const
    {
        return radSoundVector( m_x - v.m_x, m_y - v.m_y, m_z - v.m_z );
    }

    float GetAngleBetween( const radSoundVector & v ) const
    {
        const float radToDeg = 180.0f / 3.141592654f;
        return ( float ) radSoundACos( Dot( v ) / ( GetLength( ) * v.GetLength( ) ) ) * radToDeg;
    }

    const radSoundVector GetVectorTo( const radSoundVector & v ) const
    {
        return radSoundVector( v.m_x - m_x, v.m_y - m_y, v.m_z - m_z );
    }

    const radSoundVector GetVectorFrom( const radSoundVector & v ) const
    {
        return radSoundVector( m_x - v.m_x, m_y - v.m_y, m_z - v.m_z );
    }

    float GetDistanceBetween( const radSoundVector & v ) const
    {
        return GetVectorTo( v ).GetLength( );
    }

    void GetElements( float * pX, float * pY, float * pZ )
    {
        *pX = m_x; *pY = m_y; *pZ = m_z;
    }

    void SetElements( float x, float y, float z )
    {
        m_x = x; m_y = y; m_z = z;
    }

    bool operator==( const radSoundVector & v )
    {
        return ( m_x == v.m_x && m_y == v.m_y && m_z == v.m_z );
    }

    bool operator !=( const radSoundVector & v )
    {
        return ( m_x != v.m_x || m_y != v.m_y || m_z != v.m_z );
    }

    const radSoundVector & operator *= ( float s )
    {
        m_x *= s;
        m_y *= s;
        m_z *= s;

        return *this;
    }

    const radSoundVector operator *( float s )
    {
        return radSoundVector( m_x * s, m_y * s, m_z * s );
    }

    const radSoundVector & operator /= ( float s )
    {
        m_x /= s;
        m_y /= s;
        m_z /= s;

        return *this;
    }

    const radSoundVector operator /( float s )
    {
        return radSoundVector( m_x / s, m_y / s, m_z / s );
    }

    float m_x;
    float m_y;
    float m_z;
};

#endif //RADSOUNDMATH_HPP