#include <raddebug.hpp>
#include "simcommon/largesymmetricmatrix.hpp"
#include "simcommon/arraymath.hpp"

#if defined(RAD_WIN32) && defined(RAD_DEBUG)
#include <stdio.h>
#include "windows.h"
#endif

namespace sim
{

#undef TEST_INVERSION  //use to test if the matrix inversion works.

LargeSymmetricMatrix::LargeSymmetricMatrix( int iRows )
{
    rAssert(iRows>0);
    m_iRows = iRows;
    mA = new float[ m_iRows * (m_iRows+1) / 2 ];
    rAssert(mA!=NULL);
    Zero();
}

LargeSymmetricMatrix::~LargeSymmetricMatrix()
{
    delete [] mA;
}

void LargeSymmetricMatrix::Zero()
{
    memset( mA, 0, m_iRows * (m_iRows+1) / 2 * sizeof( float ) );
}

//
// Calculate Dst = this * V
//
bool LargeSymmetricMatrix::PostMultiply( const float *V, float *Dst ) const
{
    int i, j;
    
    ZeroVectorN(Dst, m_iRows);
    
    float *pV, *pDst=Dst;
    for ( i=0; i<m_iRows; i++, pDst+=1 )
    {
        pV=(float*)V;
        for ( j=0; j<m_iRows; j++, pV+=1)
        {
            if (j>i)
                *(pDst) += mA[i*m_iRows - i*(i-1)/2+j-i] * *(pV);
            else
                *(pDst) += mA[j*m_iRows - j*(j-1)/2+i-j] * *(pV);
        }
    }
    
    return true;
}

//
// Calculate Dst = V * this
//
bool LargeSymmetricMatrix::PreMultiply( const float *V, float *Dst ) const
{
    return PostMultiply(V, Dst);
}

//
// Calculate Dst = this * V
// Here 3x mean that V is a vector with 3 times more elements than the number of rows of
// this matrix. 
//
bool LargeSymmetricMatrix::PostMultiply3x( const float *V, float *Dst ) const
{
    rAssert(V!=Dst);
    
    int i, j;
    
    ZeroVectorN(Dst, 3*m_iRows);
    
    float val, *pV, *pDst=Dst;
    for ( i=0; i<m_iRows; i++ )
    {
        pV=(float*)V;
        for ( j=0; j<m_iRows; j++ )
        {
            if (j>i)
                val = mA[i*m_iRows - i*(i-1)/2+j-i];
            else
                val = mA[j*m_iRows - j*(j-1)/2+i-j];
            *(pDst+0) += val * *(pV+0);
            *(pDst+1) += val * *(pV+1);
            *(pDst+2) += val * *(pV+2);
            
            pV+=3;
        }
        pDst+=3;
    }
    
    return true;
}

//
// Calculate Dst = V * this
// Here 3x mean that V is a vector with 3 times more elements than the number of rows of
// this matrix. 
//
bool LargeSymmetricMatrix::PreMultiply3x( const float *V, float *Dst ) const
{
    return PostMultiply3x(V, Dst);
}

//
// Calculate Dst = this + M
//
bool LargeSymmetricMatrix::Add( const LargeSymmetricMatrix &M, LargeSymmetricMatrix &Dst ) const
{
    if( (M.m_iRows != m_iRows) || (Dst.m_iRows != m_iRows) )
        return false;
    
    int i, ct;
    
    ct = m_iRows * (m_iRows+1) / 2;
    for( i=0; i<ct; i++ )
        Dst.mA[i] = mA[i] + M.mA[i];
    
    return true;
}

//
// Calculate Dst = this - M
//
bool LargeSymmetricMatrix::Subtract( const LargeSymmetricMatrix &M, LargeSymmetricMatrix &Dst ) const
{
    if( (M.m_iRows != m_iRows) || (Dst.m_iRows != m_iRows) )
        return false;
    
    int i, ct;
    
    ct = m_iRows * (m_iRows+1) / 2;
    for( i=0; i<ct; i++ )
        Dst.mA[i] = mA[i] - M.mA[i];
    
    return true;
}

//dst = this * M
LargeSymmetricMatrix & LargeSymmetricMatrix::Mult(const LargeSymmetricMatrix &M, LargeSymmetricMatrix &Dst ) const
{
    rAssert( this != &Dst );
    int i,j,k;
    
    Dst.Zero();
    for( i=0; i<m_iRows; i++ )
    {
        for( j=0; j<m_iRows; j++ )
        {
            for( k=0; k<m_iRows; k++ )
                Dst(i,j) += (*this)(i,k)*M(k,j);
        }
    }
    return Dst;
}

bool LargeSymmetricMatrix::Scale( float scale, LargeSymmetricMatrix &Dst )
{
    if( Dst.m_iRows != m_iRows )
        return false;
    
    int i, ct;
    
    ct = m_iRows * (m_iRows+1) / 2;
    for( i=0; i<ct; i++ )
        Dst.mA[i] = scale * mA[i];
    
    return true;
}


float & LargeSymmetricMatrix::operator() (int row, int col) const
{
    int k;
    
    if( row > col  )
    {
        k = row;
        row = col;
        col = k;
    }
    
    return mA[ row*m_iRows - row*(row-1)/2 + col - row ];
}

void LargeSymmetricMatrix::Dump( char *szTitle ) const
{
#if defined(RAD_WIN32) && defined(RAD_DEBUG)
    char szTemp[100];
    int i,j;
    
    if( szTitle != NULL )
    {
        sprintf( szTemp, "%-95s\r\n", szTitle);
        OutputDebugString( szTemp );
    }
    
    sprintf( szTemp, "Rows: %d Columns: %d\r\n", m_iRows, m_iRows );
    OutputDebugString( szTemp );
    
    for( i=0; i<m_iRows; i++ )
    {
        sprintf( szTemp, "%3d: ", i );
        OutputDebugString( szTemp );
        for( j = 0; j<m_iRows; j++ )
        {
            sprintf( szTemp, "%8.4f ", this->operator ()(i,j) );
            OutputDebugString( szTemp );
        }
        OutputDebugString( "\r\n" );
    }
#endif
}

//
// Set this = M
//
bool LargeSymmetricMatrix::Copy( const LargeSymmetricMatrix &M )
{
    if(mA)
        delete [] mA;
    
    m_iRows  = M.m_iRows;
    mA       = new float[m_iRows * (m_iRows+1) / 2];
    if(mA==NULL)
        return false;
    memcpy( mA, M.mA, (m_iRows * (m_iRows+1)/2) * sizeof( float ) );
    return true;
}

#define SWAP(a,b) {temp=(a);(a)=(b);(b)=temp;}

bool LargeSymmetricMatrix::Invert()
{
    int i, j, l, ll;
    int irow, icol;
    float dum, pivinv;
    float *pCopy;
    
#if defined(RAD_DEBUG) && defined(TEST_INVERSION)
    LargeSymmetricMatrix testInversion;
    testInversion.Copy(*this);
#endif
    // Make a temp copy of the data in a fully populated fashion
    
    pCopy = new float[m_iRows * m_iRows];
    if(pCopy==NULL)
    {
        return false;
    }
    
    float maxVal=0.0f;
    ll = m_iRows * (m_iRows+1) / 2;
    for( i=0 ; i<ll; i++ )
    {
        if (rmt::Fabs(mA[i])>rmt::Fabs(maxVal))
            maxVal = mA[i];
    }
    
    // Populate the dense matrix
    for( i=0; i<m_iRows; i++ )
    {
        for( j=0; j<m_iRows; j++ )
        {
            if( j<i )
            {
                pCopy[ i*m_iRows+j ] = mA[ j*m_iRows - j * (j-1) / 2 + i-j ]/maxVal;
            }
            else
            {
                pCopy[ i*m_iRows+j ] = mA[ i*m_iRows - i * (i-1) / 2 + j-i ]/maxVal;
            }
        }
    }
    
    // Use Gauss-Jordan elimination without pivoting (because my matrices have their
    // appropiate pivot values already on the diagonal).  Code from "Numerical Recipes in C"
    for( i=0; i<m_iRows; i++ )
    {
        icol = irow = i;
        
        if( pCopy[icol*m_iRows+icol] == 0.0f) 
        {
#if defined(RAD_WIN32) && defined(RAD_DEBUG)
            OutputDebugString( "Singular Matrix\r\n" );
#endif
            return false;
        }
        pivinv=1.0f/pCopy[icol*m_iRows+icol];
        pCopy[icol*m_iRows+icol] = 1.0f;
        for( l=0;l<m_iRows;l++)
            pCopy[icol*m_iRows+l] *= pivinv;
        for( ll=0; ll<m_iRows; ll++ )
        {
            if( ll != icol )
            {
                dum=pCopy[ll*m_iRows+icol];
                pCopy[ll*m_iRows+icol]=0.0f;
                for( l=0; l<m_iRows; l++) 
                    pCopy[ll*m_iRows+l] -= pCopy[icol*m_iRows+l]*dum;
            }			
        }
    }
    
    //
    // Okay, now we can store the data as the upper triangle only because of the symmetric
    // assumption this class uses -- I'm sure their are many better ways to invert a 
    // symmetric matrix, but I just wanted something quick
    //
    
    Zero();
    
    for( i=0; i<m_iRows; i++ )
    {
        for( j=i; j<m_iRows; j++ )
        {
            mA[i*m_iRows - i * (i-1) / 2 +j-i] = pCopy[i*m_iRows+j]/maxVal;
        }
    }
    
    delete [] pCopy;
    
#if defined(RAD_DEBUG) && defined(TEST_INVERSION)
    
    LargeSymmetricMatrix res(m_iRows);
    
    testInversion.Mult(*this,res);
    bool test = res.IsIdentity(res);
    rAssert(test == true);
#endif
    
    return true;
}

void LargeSymmetricMatrix::SetIdentity()
{
    int i;
    
    Zero();
    for( i=0; i<m_iRows; i++ )
    {
        mA[i*m_iRows - i * (i-1) / 2] = 1.0f;
    }
}

bool LargeSymmetricMatrix::IsIdentity( const LargeSymmetricMatrix &inMat, float inEps) const
{
    
    for( int i=0 ; i<m_iRows ; i++ )
    {
        for( int j=i ; j<m_iRows ; j++ )
        {
            if (i==j)
            {
                if ( rmt::Fabs( inMat(i,j) - 1.0f) > inEps )
                    return false;
            }
            else
            {
                if ( rmt::Fabs( inMat(i,j) ) > inEps )
                    return false;
            }
        }
    }
    
    return true;
}

} // sim