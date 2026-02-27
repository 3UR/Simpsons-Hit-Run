/*
 * choreo/utility.cpp
 */

#include <choreo/utility.hpp>

#include <p3d/error.hpp>

#include <stdio.h>
#include <string.h>


namespace choreo
{


//----------------------------------------------------------------------
// Blending function implementations
//----------------------------------------------------------------------

void BlendMatrix(const rmt::Matrix& in0, const rmt::Matrix& in1,
                 float alpha, rmt::Matrix& out)
{
    BlendVector(const_cast<rmt::Matrix&>(in0).Row(3),
                const_cast<rmt::Matrix&>(in1).Row(3),
                alpha, out.Row(3));
    
    rmt::Quaternion q0, q1, qo;
    // NOTE:  hopefully someday rmt::Quaternion::BuildFromMatrix()
    //        will be const-corrected
    q0.BuildFromMatrix(const_cast<rmt::Matrix&>(in0));
    q1.BuildFromMatrix(const_cast<rmt::Matrix&>(in1));
    BlendQuaternion(q0, q1, alpha, qo);
    
    qo.ConvertToMatrix(&out);
}


//----------------------------------------------------------------------
// Orientation function implementations
//----------------------------------------------------------------------

float GetWorldAngle(float x, float z)
{
    // defaults to 0 angle
    if (rmt::Epsilon(x, 0.0f) && rmt::Epsilon(z, 0.0f))
        return 0.0f;
    
    // generate angle from x-z vector
    // - assumes DEFAULT_FACING_VECTOR is (0,0,-1)
    float angle = rmt::ATan2(-x, -z);
    
    // wrap to [0, 2*pi)
    if (angle < 0.0f)
    {
        return (angle + rmt::PI_2);
    }
    else
    {
        return angle;
    }
}

float GetSmallestArc(float start, float stop)
{
    return Wrap(-rmt::PI, rmt::PI, stop - start);
}

void RotateYVector(float angle, rmt::Vector& v)
{
    float sinY, cosY;
    rmt::SinCos(angle, &sinY, &cosY);
    
    float tmpX = v.x;
    float tmpZ = v.z;
    v.x = ((cosY * tmpX) + (sinY * tmpZ));
    v.z = ((-sinY * tmpX) + (cosY * tmpZ));
}

/*
 * Fills rotation portion of matrix (upper 3x3) to rotate
 * vector s to vector t.  This is useful for camera directions,
 * other stuff.
 */
void MatrixFillRotateVectorToVector(rmt::Matrix& m, const rmt::Vector& s, const rmt::Vector& t)
{
    rmt::Vector v;
	v.CrossProduct(s, t);
	float e = s.DotProduct(t);
	float h = (1.0f - e) / (v.DotProduct(v));

	m.m[0][0] = e + (h * v.x * v.x);
	m.m[0][1] = (h * v.x * v.y) + v.z;
	m.m[0][2] = (h * v.x * v.z) - v.y;

	m.m[1][0] = (h * v.x * v.y) - v.z;
	m.m[1][1] = e + (h * v.y * v.y);
	m.m[1][2] = (h * v.y * v.z) + v.x;

	m.m[2][0] = (h * v.x * v.z) + v.y;
	m.m[2][1] = (h * v.y * v.z) - v.x;
	m.m[2][2] = e + (h * v.z * v.z);
}


//----------------------------------------------------------------------
// String manip function implementations
//----------------------------------------------------------------------

int VStrPrintf(char* str, int len, const char* fmt, va_list ap)
{
    int rc = vsnprintf(str, len, fmt, ap);

    if (len > 0)
    {
        str[len - 1] = '\0';
    }

    return rc;
}

int StrPrintf(char* str, int len, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    
    int rc = VStrPrintf(str, len, fmt, ap);

    va_end(ap);
    
    return rc;
}

char* StrSet(char* str, int len, const char* orig)
{
    StrPrintf(str, len, "%s", orig);
    return str;
}

char* StrCat(char* str, int len, const char* orig)
{
    int i = strlen(str);
    char* strPtr = (str + i);
    
    while ((*orig != 0) && (i < len))
    {
        *strPtr = *orig;
        ++strPtr;
        ++orig;
        ++i;
    }
    
    if (len > 0)
    {
        *strPtr = '\0';
    }
    
    return str;
}

char* StrEscapify(char* str, int len, const char* orig)
{
    int strIndex = 0;
    const char* origPtr = orig;
    while ((*origPtr != '\0') && (strIndex < (len - 1)))
    {
        str[strIndex] = *origPtr;
        ++origPtr;

        if (str[strIndex] == '\\')
        {
            ++strIndex;
            str[strIndex] = '\\';
        }

        ++strIndex;
    }
    str[strIndex] = '\0';

    return str;
}


} // namespace choreo


// End of file.
