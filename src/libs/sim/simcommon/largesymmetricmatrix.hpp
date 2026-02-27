#ifndef _LARGESYMMETRICMATRIX_HPP_
#define _LARGESYMMETRICMATRIX_HPP_

namespace sim
{

class LargeSymmetricMatrix  
{
public:
    int m_iRows;
    
    float *mA;
    
public:
    LargeSymmetricMatrix( int iRows );
    virtual ~LargeSymmetricMatrix();
    
    void Zero();
    
    float &	operator() (int i, int j) const;
    
    bool PostMultiply( const float *V, float *Dst ) const;
    bool PreMultiply( const float *V, float *Dst ) const;
    bool PostMultiply3x( const float *V, float *Dst ) const;
    bool PreMultiply3x( const float *V, float *Dst ) const;
    bool Add( const LargeSymmetricMatrix &M, LargeSymmetricMatrix &Dst ) const;
    bool Subtract( const LargeSymmetricMatrix &M, LargeSymmetricMatrix &Dst ) const;
    LargeSymmetricMatrix & Mult( const LargeSymmetricMatrix &M, LargeSymmetricMatrix &Dst ) const;
    bool Scale( float inScale, LargeSymmetricMatrix &Dst );
    void Dump( char *szTitle = NULL ) const;
    bool Copy( const LargeSymmetricMatrix &inM );
    void SetIdentity();
    bool IsIdentity( const LargeSymmetricMatrix &inMat, float inEps=1.0e-6f) const;
    bool Invert();
    
protected:
    
    LargeSymmetricMatrix(): m_iRows(0),mA(NULL){}
    
    
};

} // sim

#endif // _largesymmetricmatrix_HPP_
