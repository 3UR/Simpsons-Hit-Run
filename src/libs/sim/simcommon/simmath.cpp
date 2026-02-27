#include "simcommon/simmath.hpp"

using namespace RadicalMathLibrary;

namespace sim
{

//#define USE_P3DANGLE_IN_QUAT

/*
void PhSkewMat(const Vector& v, Matrix &m)
{
PhSetRotElement(0, 0, 0, &m);
PhSetRotElement(0, 1, -v.z, &m);
PhSetRotElement(0, 2,  v.y, &m);
PhSetRotElement(1, 0,  v.z, &m);
PhSetRotElement(1, 1, 0, &m);
PhSetRotElement(1, 2, -v.x, &m);
PhSetRotElement(2, 0, -v.y, &m);
PhSetRotElement(2, 1,  v.x, &m);
PhSetRotElement(2, 2, 0, &m);
PhSetTransElement(0, 0, &m);
PhSetTransElement(1, 0, &m);
PhSetTransElement(2, 0, &m);

  }
*/

// R(t+dt) = R(t) + dt * dR/dt where dR/dt = dt * skew(w) * R or (dt * w) cross product with R 
// this function do the cross product fo wdt ith the rotation matrix, the result r is then
// dR/dt * dt and will be added to R(t) in order to get R(t+dt)
/*
void PhVecCrossMatrix(const Vector& v, const Matrix& m, Matrix& r)
{
for (int i=0; i<3; i++)
{
r[i][0] = -rmMult(v.z, r.m[i][1]) + rmMult(v.y, r.m[i][2]);
r[i][1] =  rmMult(v.z, r.m[i][0]) - rmMult(v.x, r.m[i][2]);
r[i][2] = -rmMult(v.y, r.m[i][0]) + rmMult(v.x, r.m[i][1]);
r[i][3] = 0;
}
}
*/

void SubUpdateRotMatrix(const Vector& v, Matrix& r);
void SubUpdateRotMatrix(const Vector& v, Matrix& r)
{
    Matrix m = r;
    for (int i=0; i<3; i++)
    {
        r.m[i][0] = m.m[i][0] - v.z * m.m[i][1] + v.y * m.m[i][2];
        r.m[i][1] = m.m[i][1] + v.z * m.m[i][0] - v.x * m.m[i][2];
        r.m[i][2] = m.m[i][2] - v.y * m.m[i][0] + v.x * m.m[i][1];
    }
}

// this function is similar to the previous one exept that the result is the addition of 
// the rotation matrix with the cross product, so r = R(t+dt) directly. This function 
// is accurate for small angle only. If the angle is subjected to become large, use 
// PhUpdateRotMatrixSafe instead.
void UpdateRotMatrix(const Vector& v, Matrix& r)
{
    SubUpdateRotMatrix(v, r);
    r.OrthoNormal();
}

// same as PhUpdateRotMatrix with protection against too large angle
void UpdateRotMatrixSafe(const Vector& v, Matrix& r, float safemax)
{
    float p = Fabs(v.x) + Fabs(v.y) + Fabs(v.z);
    int nj = int(p / safemax) + 1;
    Vector subv = v;
    subv.Scale(1.0f/float(nj));
    
    for (int j=0; j<nj; j++)
    {
        SubUpdateRotMatrix(subv, r);
    }
    r.OrthoNormal();
}

void VecNT(Vector& v, Vector& n, Vector& vn, Vector& vt)
{
    vn.Scale(v.DotProduct(n), n);
    vt.Sub(v, vn);
}

void WFromRotMatrix(Vector& w, const Matrix& r0, const Matrix& r1, float dt)
{
    // in SubUpdateRotMatrix we update the new rotation matrix from the old one and the skew matrix of wdt:
    // r1 = r0 + skew * r0; then skew = (r1-r0) * inverse(r0); 
    // we simply do a manual multiplication of 3 term of the skew matrix and get our wdt vector
    // then we scale it with 1/dt to get the w.
    Matrix d, r0t;
    d.Identity();

    d.m[0][0] = r1.m[0][0] - r0.m[0][0];
    d.m[0][1] = r1.m[0][1] - r0.m[0][1];
    d.m[0][2] = r1.m[0][2] - r0.m[0][2];
    d.m[1][0] = r1.m[1][0] - r0.m[1][0];
    d.m[1][1] = r1.m[1][1] - r0.m[1][1];
    d.m[1][2] = r1.m[1][2] - r0.m[1][2];
    d.m[2][0] = r1.m[2][0] - r0.m[2][0];
    d.m[2][1] = r1.m[2][1] - r0.m[2][1];
    d.m[2][2] = r1.m[2][2] - r0.m[2][2];

    w.z = ( d.m[0][1] * r0.m[0][0] ) + ( d.m[1][1] * r0.m[1][0] ) + ( d.m[2][1] * r0.m[2][0] ) ;
    w.y = ( d.m[0][0] * r0.m[0][2] ) + ( d.m[1][0] * r0.m[1][2] ) + ( d.m[2][0] * r0.m[2][2] ) ;
    w.x = ( d.m[0][2] * r0.m[0][1] ) + ( d.m[1][2] * r0.m[1][1] ) + ( d.m[2][2] * r0.m[2][1] ) ;
    w.Scale(1.0f/dt);

/* michel's version. 
    w.z = ( r0.m[1][0] * r1.m[0][0] ) + ( r0.m[1][1] * r1.m[0][1] ) + ( r0.m[1][2] * r1.m[0][2] );
    w.y = ( r0.m[0][0] * r1.m[2][0] ) + ( r0.m[0][1] * r1.m[2][1] ) + ( r0.m[0][2] * r1.m[2][2] );
    w.x = ( r0.m[2][0] * r1.m[1][0] ) + ( r0.m[2][1] * r1.m[1][1] ) + ( r0.m[2][2] * r1.m[1][2] );
    w.Scale(1.0f/dt);
*/
}

void ExactThetaFromRotMatrix(Vector& axis, float &angle, Matrix r0, Matrix r1)
{
	float mag;
	r0.Invert();
	r0.Mult(r1);

	axis.x = r0.m[2][1]-r0.m[1][2];
	axis.y = r0.m[0][2]-r0.m[2][0];
	axis.z = r0.m[1][0]-r0.m[0][1];
	mag = axis.Magnitude();
	axis.Scale(1/mag);
	angle = ASin(mag*0.5f);
	mag = ACos((r1.m[0][0]+r1.m[1][1]+r1.m[2][2]-1.0f)*0.5f);
}

void ScaleRotMatrix(float f, const Matrix& m, Matrix& r)
{
    r.m[0][0] = f * m.m[0][0];
    r.m[0][1] = f * m.m[0][1];
    r.m[0][2] = f * m.m[0][2];
    //r.m[0][3] = f * m.m[0][3];
    r.m[1][0] = f * m.m[1][0];
    r.m[1][1] = f * m.m[1][1];
    r.m[1][2] = f * m.m[1][2];
    //r.m[1][3] = f * m.m[1][3];
    r.m[2][0] = f * m.m[2][0];
    r.m[2][1] = f * m.m[2][1];
    r.m[2][2] = f * m.m[2][2];
    //r.m[2][3] = f * m.m[2][3];
    //r.m[3][0] = f * m.m[3][0];
    //r.m[3][1] = f * m.m[3][1];
    //r.m[3][2] = f * m.m[3][2];
    //r.m[3][3] = f * m.m[3][3];
}

float ComputeScaleFromMatrix(const Matrix& m, float epsilon)
{
    float scale = 1.0f;
    Vector v = *(Vector*)m.m[0];
    float f_0 = v.DotProduct(v);
    v = *(Vector*)m.m[1];
    float f_1 = v.DotProduct(v);
    v = *(Vector*)m.m[2];
    float f_2 = v.DotProduct(v);
    
    rAssert(f_0 > 0.0f);
    
    if (Fabs(f_0 - f_1)/f_0 < epsilon && Fabs(f_0 - f_2)/f_0 < epsilon)
    {
        scale = Sqrt(f_0);
    }
    else
    {
        rAssertMsg(false, "SimStateMatrix::ExtractScaleFromTransform: Matrix anisotropic, can't do it");
    }
    return scale;
}

void InverseNComponent(Vector& v, Vector& n)
{
    // divide v in vn (the component || to n and vt the transverse component
    // set new v'=vt-vn, but vt=v-vn => v'=v-2vn
    Vector vn;
    vn.Scale(-2.0f * v.DotProduct(n), n);
    v.Add(vn);
}

bool WDtToQuat(const Vector& wdt, Quaternion& q)
{
    static float small_angle = 0.0001f;
    Vector n = wdt;
    bool ret = false;
    float nn = n.NormalizeSafe();
    if (nn > small_angle)
    {
        AngleToQuat(nn, n, q);
        ret = true;
    }
    return ret;
}

void AngleToQuat(float angle, const Vector& axis, Quaternion& q)
{
#ifdef USE_P3DANGLE_IN_QUAT
    q.BuildFromAngleAxis(angle, axis);
#else
    q.w = float(Cos(angle * 0.5f));
    angle = float(Sin(angle * 0.5f));
    q.x = axis.x * angle;
    q.y = axis.y * angle;
    q.z = axis.z * angle;
#endif
}

void SimCheckValue(float val)
{
    unsigned int mask = 0x7f800000;
    unsigned int test;
    
    test = *((unsigned int*)&(val));
    if((test & mask) == mask)
    {
        rAssert(false);
    }
    else if (Fabs(val) > 999999999999999.0f)
    {
        rAssert(false);
    }
}

float GetMaxVectorComponentValue( const rmt::Vector& in_v, bool in_bInAbs )
{
    if( in_bInAbs )
    {
        return Max(Max(Fabs(in_v.x), Fabs(in_v.y)), Fabs(in_v.z));
    }
    else
    {
        return Max(Max(in_v.x, in_v.y), in_v.z);
    }
}

int GetIndexOfMaxVectorComponent( const rmt::Vector& in_v, bool in_bInAbs )
{
    if( in_bInAbs )
    {
        if( Fabs(in_v.x) > Fabs(in_v.y) )
        {
            return ( Fabs(in_v.x) > Fabs(in_v.z) ) ? 0 : 2;
        }
        else
        {
            return ( Fabs(in_v.y) > Fabs(in_v.z) ) ? 1 : 2;
        }
    }
    else
    {
        if( in_v.x > in_v.y )
        {
            return ( in_v.x > in_v.z ) ? 0 : 2;
        }
        else
        {
            return ( in_v.y > in_v.z ) ? 1 : 2;
        }
    }
}

int GetIndexOfMinVectorComponent(  const rmt::Vector& in_v, bool in_bInAbs )
{
    if( in_bInAbs )
    {
        if( Fabs(in_v.x) < Fabs(in_v.y) )
        {
            return ( Fabs(in_v.x) < Fabs(in_v.z) ) ? 0 : 2;
        }
        else
        {
            return ( Fabs(in_v.y) < Fabs(in_v.z) ) ? 1 : 2;
        }
    }
    else
    {
        if( in_v.x < in_v.y )
        {
            return ( in_v.x < in_v.z ) ? 0 : 2;
        }
        else
        {
            return ( in_v.y < in_v.z ) ? 1 : 2;
        }
    }
}

void ComputePointMassInertia(SymMatrix& inertia, Vector& pos, float mass)
{
    inertia.xx = Sqr(pos.y) + Sqr(pos.z);
    inertia.xy = -pos.x * pos.y;
    inertia.xz = -pos.x * pos.z;
    inertia.yy = Sqr(pos.x) + Sqr(pos.z);
    inertia.yz = -pos.y * pos.z;
    inertia.zz = Sqr(pos.x) + Sqr(pos.y);
    inertia.Scale(mass);
}

bool SameMatrix(const rmt::Matrix& m1, const rmt::Matrix& m2, float epsilon)
{
    // look at the translation
    if (SameTranslation(m1, m2, epsilon) == false )
        return false;
    // look at the rotation
    if (SameRotation(m1, m2, epsilon) == false )
        return false;
    
    return true;
}

bool SameTranslation(const rmt::Matrix& m1, const rmt::Matrix& m2, float epsilon)
{
    if (Fabs(m1.m[3][0] - m2.m[3][0]) > epsilon)
        return false;
    else if (Fabs(m1.m[3][1] - m2.m[3][1]) > epsilon)
        return false;
    else if (Fabs(m1.m[3][2] - m2.m[3][2]) > epsilon)
        return false;
    else
        return true;
}

bool SameRotation(const rmt::Matrix& m1, const rmt::Matrix& m2, float epsilon)
{
    if (Fabs(m1.m[0][0] - m2.m[0][0]) > epsilon)
        return false;
    else if (Fabs(m1.m[1][1] - m2.m[1][1]) > epsilon)
        return false;
    else if (Fabs(m1.m[2][2] - m2.m[2][2]) > epsilon)
        return false;
    else
        return true;
}

//    Sets a rotation that will rotate around the normal of the plane
//    defined by the two given vectors (right-hand rule), with an angle
//    defined by the angle between those two vectors.
//    When vectors are aligned, a NULL rotation is set.
//    When vectors are diametrically opposed, they don't define a plane
//    anymore and an infinite number of rotation axis could be used. In
//    this case an arbitrary decision is taken (rotation of 180 degrees
//    around an arbitrary axis) and output parameter o_Flip is set to
//	   true to indicate that there is a discontinuity in the set of all
//	   rotations set by this function.
//    When one of the input vectors is NULL, we really don't know what
//    rotation must be set. Identity is set and false is returned.

bool BuildRotationFromTo( const rmt::Vector& in_vctFrom, const rmt::Vector& in_vctTo, rmt::Quaternion &o_quat, bool &o_Flip, float inMinAng, float inMaxAng )
{
    
    float dLen, dDot, dAngle;
    Vector vctAxis;
    
    // By default no discontinuity.
    o_Flip = false;
    
    dLen = in_vctFrom.Magnitude();
    dLen *= in_vctTo.Magnitude();
    
    if( dLen < MICRO_EPS )
    {
        // One of the input vector is NULL. Don't know what to do...
        o_quat.Identity();
        return( false );
    }
    
    // Compute axis of rotation and angle of rotation.
    dDot = in_vctFrom.DotProduct( in_vctTo );
    dAngle = dDot / dLen;
    ClampBetween( dAngle, -1.0f, 1.0f );
    
    dAngle = ACosSafe( dAngle );
    
    vctAxis.CrossProduct( in_vctFrom, in_vctTo );
    dLen = vctAxis.Magnitude();
    
    // If rotation axis is almost NULL, either From and To vectors are
    // aligned or diametrically opposed.
    if( dLen < MICRO_EPS )
    {
        if( dDot > 0.0f )
        {
            // Aligned: No rotation.
            o_quat.Identity();
            return( true );
        }
        else
        {
            Vector vctX( 1.0f, 0.0f, 0.0f );
            
            // Diametrically opposed: don't know around which axis
            // we should rotate. We make an arbitrary choice.
            o_Flip = true;
            
            // First try with an axis perpendicular to both X axis
            // and the From vector.
            vctAxis.CrossProduct( vctX, in_vctFrom );
            dAngle = PI;
            dLen = vctAxis.Magnitude();
            
            if( dLen < MICRO_EPS )
            {
                // X axis is aligned with From or To vector.
                // Simply apply a rotation around the Y axis.
                o_quat.BuildFromAngleY( PI );
                return( true );
            }
        }
    }
    
    // We now have a valid axis and a valid angle of rotation...
    if (vctAxis.NormalizeSafe())
    {
        ClampBetween( dAngle, inMinAng, inMaxAng );      
        AngleToQuat(dAngle, vctAxis, o_quat);
        o_quat.Normalize();
    }
    else
    {
        o_quat.Identity();
    }
    
    return( true );
}


bool BuildRotationFromTo( const rmt::Vector& in_vctFrom, const rmt::Vector& in_vctTo, rmt::Matrix &o_mat, bool &o_Flip, float inMinAng, float inMaxAng )
{
    Quaternion l_quat;
    BuildRotationFromTo(in_vctFrom, in_vctTo, l_quat, o_Flip, inMinAng, inMaxAng);
    
    l_quat.ConvertToMatrix(&o_mat);
    
    return true;
}

void CopyRotation(const rmt::Matrix& source, rmt::Matrix& dest)
{
    dest.Row(0) = source.Row(0);
    dest.Row(1) = source.Row(1);
    dest.Row(2) = source.Row(2);
}

rmt::Vector PerpComponent(const rmt::Vector& v_1, const rmt::Vector& v_2)
    // v_2 must be a normalized vector
    // calculates the perp component of v_1 wrt v_2,  v_1-v_2*dot(v_1,v_2) 
{
    rAssert( Fabs(v_2.Magnitude()-1.0f)<0.001f );
    Vector perp;
    float f = v_1.DotProduct(v_2);
    perp.ScaleAdd(v_1,-f,v_2);
    return perp;
}


void AxisAngle(const rmt::Vector& v_1, const rmt::Vector& v_2, rmt::Vector& axis, float& angle)
    // Computes the axis and angle (between 0 and 180) required to rotate v_1 into v_2
    // if v_1 = +/- v_2 then set axis = v_1 and angle = 0;
{
    // note the reversed order... due to left handed system
    axis.CrossProduct(v_2, v_1);   
    float dot;
    dot = v_1.DotProduct(v_2);
    // watch out for 0 or Pi
    if( axis.Equals(Vector(0,0,0), 0.0001f) )
    {
        if( dot < 0.0f )  // theta = Pi, pick a random perpendicular vector
        {
            angle = PI;              
        }
        else
        {
            angle = 0.0f;
        }
        axis = v_1;
        return;
    }
    angle = ACosSafe( dot/(v_1.Magnitude()*v_2.Magnitude()) );
}

Vector float2Vec(const float* f)
{
    
    return Vector(f[0], f[1], f[2]);
}

/*
Matrix& LeftMult(const Matrix& a)
{
    assert(a.m[0][3] == 0.0f);
    assert(a.m[1][3] == 0.0f);
    assert(a.m[2][3] == 0.0f);
    assert(a.m[3][3] == 1.0f);
    
    Matrix b;
    b = this;
    
    m[0][0] = (a.m[0][0] * b.m[0][0]) + (a.m[0][1] * b.m[1][0]) + (a.m[0][2] * b.m[2][0]);
    m[0][1] = (a.m[0][0] * b.m[0][1]) + (a.m[0][1] * b.m[1][1]) + (a.m[0][2] * b.m[2][1]);
    m[0][2] = (a.m[0][0] * b.m[0][2]) + (a.m[0][1] * b.m[1][2]) + (a.m[0][2] * b.m[2][2]);
    m[0][3] = 0.0f;
    
    m[1][0] = (a.m[1][0] * b.m[0][0]) + (a.m[1][1] * b.m[1][0]) + (a.m[1][2] * b.m[2][0]);
    m[1][1] = (a.m[1][0] * b.m[0][1]) + (a.m[1][1] * b.m[1][1]) + (a.m[1][2] * b.m[2][1]);
    m[1][2] = (a.m[1][0] * b.m[0][2]) + (a.m[1][1] * b.m[1][2]) + (a.m[1][2] * b.m[2][2]);
    m[1][3] = 0.0f;
    
    m[2][0] = (a.m[2][0] * b.m[0][0]) + (a.m[2][1] * b.m[1][0]) + (a.m[2][2] * b.m[2][0]);
    m[2][1] = (a.m[2][0] * b.m[0][1]) + (a.m[2][1] * b.m[1][1]) + (a.m[2][2] * b.m[2][1]);
    m[2][2] = (a.m[2][0] * b.m[0][2]) + (a.m[2][1] * b.m[1][2]) + (a.m[2][2] * b.m[2][2]);
    m[2][3] = 0.0f;
    
    m[3][0] = (a.m[3][0] * b.m[0][0]) + (a.m[3][1] * b.m[1][0]) + (a.m[3][2] * b.m[2][0]) + b.m[3][0];
    m[3][1] = (a.m[3][0] * b.m[0][1]) + (a.m[3][1] * b.m[1][1]) + (a.m[3][2] * b.m[2][1]) + b.m[3][1];
    m[3][2] = (a.m[3][0] * b.m[0][2]) + (a.m[3][1] * b.m[1][2]) + (a.m[3][2] * b.m[2][2]) + b.m[3][2];
    m[3][3] = 1.0f;
    
    return *this;
}
*/

} // sim
