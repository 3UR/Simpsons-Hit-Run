


#include "simcommon/arraymath.hpp"

namespace sim
{

float NormalizeVector(float f[])
{
    float n = LenVector(f);
    if (n > 0)
        ScaleVector(f, 1.0f/n, f);
    else
        ZeroVector(f);
    return n;
}

void CrossVector(const float v1[], const float v2[], float r[])
{
    float res[3];
    res[0] = v1[1]*v2[2] - v1[2]*v2[1];
    res[1] = v1[2]*v2[0] - v1[0]*v2[2];
    res[2] = v1[0]*v2[1] - v1[1]*v2[0];
    EqualVector(res, r);
}

void MultVectorVectorT(const float v[], const float vt[], float m[][3])
{
    m[0][0] = v[0]*vt[0];
    m[0][1] = v[0]*vt[1];
    m[0][2] = v[0]*vt[2];
    m[1][0] = v[1]*vt[0];
    m[1][1] = v[1]*vt[1];
    m[1][2] = v[1]*vt[2];
    m[2][0] = v[2]*vt[0];
    m[2][1] = v[2]*vt[1];
    m[2][2] = v[2]*vt[2];
}

void MultVectorVectorT(const float v[], float m[][3])
{
    // case VectorT == Vector, save a few mult
    m[0][0] = v[0]*v[0];
    m[1][1] = v[1]*v[1];
    m[2][2] = v[2]*v[2];
    m[0][1] = m[1][0] = v[0]*v[1];
    m[0][2] = m[2][0] = v[0]*v[2];
    m[1][2] = m[2][1] = v[1]*v[2];
}

void ComputeTriNormal(const float v1[], const float v2[], const float v3[], float n[])
{
    float d1[3], d2[3];
    SubVector(v2, v1, d1);
    SubVector(v3, v1, d2);
    CrossVector(d1, d2, n);
    NormalizeVector(n);
}



void AddMatrix(const float m1[][3], const float m2[][3], float mt[][3])
{
    for(int ind1=0; ind1<3; ind1++) 
    {
        for(int ind2=0; ind2<3; ind2++) 
        {
            mt[ind1][ind2] = m1[ind1][ind2] + m2[ind1][ind2];
        }
    }
}

void SubMatrix(const float m1[][3], const float m2[][3], float mt[][3])
{
    for(int ind1=0; ind1<3; ind1++) 
    {
        for(int ind2=0; ind2<3; ind2++) 
        {
            mt[ind1][ind2] = m1[ind1][ind2] - m2[ind1][ind2];
        }
    }
}

void AddScaleMatrix(const float m1[][3], const float m2[][3], float s, float mt[][3])
{
    for(int ind1=0; ind1<3; ind1++) 
    {
        for(int ind2=0; ind2<3; ind2++) 
        {
            mt[ind1][ind2] = m1[ind1][ind2] + m2[ind1][ind2] * s;
        }
    }
}

void AddLinearMatrix(const float m1[][3], float s1, const float m2[][3], float s2, float mt[][3])
{
    for(int ind1=0; ind1<3; ind1++) 
    {
        for(int ind2=0; ind2<3; ind2++) 
        {
            mt[ind1][ind2] = m1[ind1][ind2] * s1 + m2[ind1][ind2] * s2;
        }
    }
}

void ScaleMatrix(const float m1[][3], const float s, float mt[][3])
{
    for(int ind1=0; ind1<3; ind1++) 
    {
        for(int ind2=0; ind2<3; ind2++) 
        {
            mt[ind1][ind2] = m1[ind1][ind2] * s;
        }
    }
}

void EqualMatrix(const float m1[][3], float mt[][3])
{
    for(int ind1=0; ind1<3; ind1++) 
    {
        for(int ind2=0; ind2<3; ind2++) 
        {
            mt[ind1][ind2] = m1[ind1][ind2];
        }
    }
}

void MultMatrix( const float m1[][3], const float m2[][3], float mt[][3])
{
    float l_m[3][3];
    
    ZeroMatrix( l_m );
    
    for( int i=0; i<3 ; i++ )
    {
        for( int j=0 ; j<3 ; j++ )
        {
            l_m[i][j] = m1[i][0] * m2[0][j] + m1[i][1] * m2[1][j] +  m1[i][2] * m2[2][j];
        }
    }
    EqualMatrix( l_m, mt );
}

void TransposeMatrix( float m[][3] )
{
    float tmp;
    
    tmp = m[0][1];
    m[0][1] = m[1][0];
    m[1][0] = tmp;
    
    tmp = m[0][2];
    m[0][2] = m[2][0];
    m[2][0] = tmp;
    
    tmp = m[1][2];
    m[1][2] = m[2][1];
    m[2][1] = tmp;
    
}

void TransformVector(const float k[][3], float res[])
{
    float a[3] = { res[0], res[1], res[2] };
    res[0] = k[0][0]*a[0] + k[0][1]*a[1] + k[0][2]*a[2];
    res[1] = k[1][0]*a[0] + k[1][1]*a[1] + k[1][2]*a[2];
    res[2] = k[2][0]*a[0] + k[2][1]*a[1] + k[2][2]*a[2];
}

void TransformVector(const float k[][3], const float a[], float res[])
{
    res[0] = k[0][0]*a[0] + k[0][1]*a[1] + k[0][2]*a[2];
    res[1] = k[1][0]*a[0] + k[1][1]*a[1] + k[1][2]*a[2];
    res[2] = k[2][0]*a[0] + k[2][1]*a[1] + k[2][2]*a[2];
}

void TransformVectorAdd(const float k[][3], const float a[], float res[])
{
    res[0] += k[0][0]*a[0] + k[0][1]*a[1] + k[0][2]*a[2];
    res[1] += k[1][0]*a[0] + k[1][1]*a[1] + k[1][2]*a[2];
    res[2] += k[2][0]*a[0] + k[2][1]*a[1] + k[2][2]*a[2];
}

void ZeroMatrix(float k[][3])
{
    ZeroVector(k[0]);
    ZeroVector(k[1]);
    ZeroVector(k[2]);
}

bool AreMatricesEqual(const float j[][3], const float k[][3], float in_eps)
{
    if( rmt::Fabs( j[0][0] - k[0][0] ) > in_eps )
        return false;
    if( rmt::Fabs( j[0][1] - k[0][1] ) > in_eps )
        return false;
    if( rmt::Fabs( j[0][2] - k[0][2] ) > in_eps )
        return false;
    if( rmt::Fabs( j[1][0] - k[1][0] ) > in_eps )
        return false;
    if( rmt::Fabs( j[1][1] - k[1][1] ) > in_eps )
        return false;
    if( rmt::Fabs( j[1][2] - k[1][2] ) > in_eps )
        return false;
    if( rmt::Fabs( j[2][0] - k[2][0] ) > in_eps )
        return false;
    if( rmt::Fabs( j[2][1] - k[2][1] ) > in_eps )
        return false;
    if( rmt::Fabs( j[2][2] - k[2][2] ) > in_eps )
        return false;
    return true;
}

bool AreMatricesEqualTranspose(const float j[][3], const float k[][3], float in_eps)
{
    if( rmt::Fabs( j[0][0] - k[0][0] ) > in_eps )
        return false;
    if( rmt::Fabs( j[0][1] - k[1][0] ) > in_eps )
        return false;
    if( rmt::Fabs( j[0][2] - k[2][0] ) > in_eps )
        return false;
    if( rmt::Fabs( j[1][0] - k[0][1] ) > in_eps )
        return false;
    if( rmt::Fabs( j[1][1] - k[1][1] ) > in_eps )
        return false;
    if( rmt::Fabs( j[1][2] - k[2][1] ) > in_eps )
        return false;
    if( rmt::Fabs( j[2][0] - k[0][2] ) > in_eps )
        return false;
    if( rmt::Fabs( j[2][1] - k[1][2] ) > in_eps )
        return false;
    if( rmt::Fabs( j[2][2] - k[2][2] ) > in_eps )
        return false;
    return true;
}

void RemoveComponent(const float dir[], float v[]) 
{ 
    float d = DotVector(v, dir);
    if (d < 0)
        AddScaleVector(v, dir, -d, v); 
}

float TriangleArea(const float p1[], const float p2[], const float p3[])
{
    float dpbase[3], dp2[3], base, height;
    SubVector(p2, p1, dpbase);
    SubVector(p3, p1, dp2);
    base = NormalizeVector(dpbase);
    AddScaleVector(dp2, dpbase, -DotVector(dpbase, dp2), dp2);
    height = LenVector(dp2);
    return 0.5f * base * height;
}

//
//
//

#ifdef USE_FAST_MATH4

void MakeMultipleOf4(int& vecSize)
{
    // want to make sure that the vectors length is multiple of 4
    int i = vecSize % 4;
    if (i != 0 )
    {
        vecSize += (4 - i);
    }
}

float DotVectorN(const float a[], const float b[], int size) 
{
    rAssert(size % 4 == 0);
    float r = 0.0f;
    
    for(int i=0;i<size;i+=4)
        r += DotVector4(&a[i], &b[i]);
    
    return r;
}

#else

float DotVectorN(const float a[], const float b[], int size) 
{
    float r = 0.0f;
    
    for(int i=0;i<size;i++)
        r += a[i]*b[i];
    
    return r;
}


#endif // USE_FAST_MATH4

} // sim
