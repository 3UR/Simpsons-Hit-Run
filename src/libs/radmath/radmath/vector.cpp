//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <radmath/vector.hpp>
#include <radmath/matrix.hpp>
#include <math.h>
#include <assert.h>


namespace RadicalMathLibrary
{

Vector& Vector::operator=(const Vector4& vect)
{
    if(vect.w == 1.0f)
    {
        x = vect.x;
        y = vect.y;
        z = vect.z;
    }
    else
    {
        float oow = 1.0f / vect.w;
        x = vect.x * oow;
        y = vect.y * oow;
        z = vect.z * oow;
    }
    return *this;
}

bool Vector::Equals(const Vector& a, float e) const
{
    return Epsilon(x, a.x, e) && Epsilon(y, a.y, e) && Epsilon(z, a.z, e);
}

// cross products
void Vector::CrossProduct(const Vector& vect)
{
    float tmpx = (y * vect.z) - (z * vect.y);
    float tmpy = (z * vect.x) - (x * vect.z);
    float tmpz = (x * vect.y) - (y * vect.x);
    x = tmpx; y = tmpy; z = tmpz;
}


void Vector::CrossProduct(const Vector& vect1, const Vector& vect2)
{
    x = (vect1.y * vect2.z) - (vect1.z * vect2.y);
    y = (vect1.z * vect2.x) - (vect1.x * vect2.z);
    z = (vect1.x * vect2.y) - (vect1.y * vect2.x);
}

// create a unit vector
void Vector::Normalize(void)
{
    float mag = 1.0f / Magnitude();
    x = (x * mag);
    y = (y * mag);
    z = (z * mag);

}

void Vector::Normalize(const Vector& vect)
{
    float mag = 1.0f / vect.Magnitude();
    x = (vect.x * mag);
    y = (vect.y * mag);
    z = (vect.z * mag);

}

void Vector::Transform(const Matrix& m)
{
    Vector tmp;
    m.Transform(*this, &tmp);
    *this = tmp;

}

void Vector::Transform(const Vector& v, const Matrix& m)
{
    m.Transform(v, this);

}

void Vector::Rotate(const Matrix& m)
{
    Vector tmp;
    m.RotateVector(*this, &tmp);
    *this = tmp;

}

void Vector::Rotate(const Vector& v, const Matrix& m)
{
    m.RotateVector(v, this);

}


void Vector::ConvertToEulerYZX(const Matrix& matrix)
{
    //
    // Quick check for the degenerate case
    // need to deal with degenerate case where cos(z) is very close to 0
    //
    // we can't individually query cos(z), so have to check -sin(z) and 
    // see if it is very close to 1 or -1
    //

    const float tolerance = 1.0e-15f;

    if(Abs(Abs(matrix.m[1][0]) - 1.0f) < tolerance)
    {
        //
        // Handle the degenerate case
        // TODO: Make this work
        //
//      assert(0 && "You've hit the degenerate case and right now it's not working");
        this->x = 0;
        this->y = 0;
        this->z = 0;
    
         // special case
        
        // need to whether sin(z) is +/-
        float sinz;
        if(matrix.m[1][0] > 0.0f)  
        {
            // recall element [1][0] is negated
            sinz = -1.0f;
        }
        else
        {
            sinz = 1.0f;
        }
        
        // we know z is 90 or -90 degrees

        // now we need to figure out if we should set x or y to an arbitrary value.

        // of the terms cycz, sycz, cxcz, sxcz
        // we want the x-pair or y-pair that are closer to zero - they are more likely safe values to set

        float x_pair = (float)(fabs(matrix.m[1][1]) + fabs(matrix.m[1][2]));
        float y_pair = (float)(fabs(matrix.m[0][0]) + fabs(matrix.m[2][0]));

        // range catchers
        const float one = 1.5708f / 90.0f;
        const float ninety = 1.5708f;
        const float oneeighty = 3.14159f;

        if(x_pair < y_pair)
        {
            // set x 
            
            // hmmm... since there is no way for us to know what the sign of cz is, we can't know what the 
            // sign of cx or sx should be 

            // perhaps the thing to do here is just use atan2 on the 2 elements.

            // then use the result as our setting to simplify the matrix.

            float tempx = (float)ATan2(matrix.m[1][2], matrix.m[1][1]);               
            
            // I would now expect tempx to be either 0, 90, -90, 180, -180
            //
            // we need to know this so we can simplify the matrix to get y from some source other than [0][0] and [2][0]

            if(fabs(tempx) < one) 
            {
                // 0
                // cx = 1
                // sx = 0

                // seems like we're always gonna get 2 choices of where to do atan
                this->x = (float)ATan2(-matrix.m[0][2], matrix.m[2][2]);
                this->x = (float)ASin(sinz);
                this->x = 0.0f;

            }
            else if(tempx > 0)
            {
                if(fabs(tempx - ninety) < one)
                {
                    // 90
                    // cx = 0

                    // sx = 1

                    // now 2 choices of what to take the atan of!
                    this->x = (float)ATan2(matrix.m[0][1], -matrix.m[2][1]);
                    this->x = (float)ASin(sinz);
                    this->x = ninety;              

                }
                else if(fabs(tempx - oneeighty) < one)
                {
                    // 180
                    // cx = -1
                    // sx = 0
                    this->x = (float)ATan2(matrix.m[0][2], -matrix.m[2][2]);
                    this->x = (float)ASin(sinz);
                    this->x = oneeighty;               

                }
                else
                {
                    // I hope we never get here!
                    //
                    // just do the 0 case
                    this->x = (float)ATan2(-matrix.m[0][2], matrix.m[2][2]);
                    this->x = (float)ASin(sinz);
                    this->x = 0.0f;   
                }

            }
            else
            {
                if(fabs(tempx + ninety) < one)
                {
                    // -90
                    // cx = 0
                    // sx = -1
                    this->x = (float)ATan2(-matrix.m[0][1], matrix.m[2][1]);
                    this->x = (float)ASin(sinz);
                    this->x = -ninety;                             

                }
                else if(fabs(tempx + oneeighty) < one)
                {
                    // -180
                    this->x = (float)ATan2(matrix.m[0][2], -matrix.m[2][2]);
                    this->x = (float)ASin(sinz);
                    this->x = -oneeighty;               
                }
                else
                {
                    // I hope we never get here!
                    //
                    // just set to 0
                    this->x = (float)ATan2(-matrix.m[0][2], matrix.m[2][2]);
                    this->x = (float)ASin(sinz);
                    this->x = 0.0f;   
                }

            }

        }
        else
        {
            // set y
            // I suppose in this case, the y pair is the one to do the atan on
            float tempy = (float)ATan2(matrix.m[2][0], matrix.m[0][0]);

            if(fabs(tempy) < one) 
            {
                // 0
                // cy = 1
                // sy = 0

                this->x = (float)ATan2(-matrix.m[2][1], matrix.m[2][2]);
                this->x = (float)ASin(sinz);
                this->x = 0.0f;

            }
            else if(tempy > 0)
            {
                if(fabs(tempy - ninety) < one)
                {
                    // 90
                    // cy = 0
                    // sy = 1
                
                    this->x = (float)ATan2(matrix.m[0][1], -matrix.m[0][2]);
                    this->x = (float)ASin(sinz);
                    this->x = ninety;

                }
                else if(fabs(tempy - oneeighty) < one)
                {
                    // 180
                    // cy = -1
                    // sy = 0

                    this->x = (float)ATan2(matrix.m[2][1], -matrix.m[2][2]);
                    this->x = (float)ASin(sinz);
                    this->x = oneeighty;
                }
                else
                {
                    // I hope we never get here!
                    //
                    // just do the 0 case
                    this->x = (float)ATan2(-matrix.m[2][1], matrix.m[2][2]);
                    this->x = (float)ASin(sinz);
                    this->x = 0.0f;

                }

            }
            else
            {
                if(fabs(tempy + ninety) < one)
                {
                    // -90
                    // cy = 0
                    // sy = -1
                
                    this->x = (float)ATan2(-matrix.m[0][1], matrix.m[0][2]);
                    this->x = (float)ASin(sinz);
                    this->x = -ninety;


                }
                else if(fabs(tempy + oneeighty) < one)
                {
                    // -180
                    // cy = -1
                    // sy = 0

                    this->x = (float)ATan2(matrix.m[2][1], -matrix.m[2][2]);
                    this->x = (float)ASin(sinz);
                    this->x = -oneeighty;
                    
                }
                else
                {
                    // I hope we never get here!
                    //
                    // just set to 0

                    this->x = (float)ATan2(-matrix.m[2][1], matrix.m[2][2]);
                    this->x = (float)ASin(sinz);
                    this->x = 0.0f;              

                }

            }
        }

    }
    else
    {
        //
        // This is the general case
        //
        this->x = ATan2(matrix.m[1][2], matrix.m[1][1]);
        this->y = ATan2(matrix.m[2][0], matrix.m[0][0]);
        this->z = ASin(-matrix.m[1][0]);
    }


}


//-------------------------------------------------------------------
// Vector4
//-------------------------------------------------------------------
bool Vector4::Equals(const Vector4& a, float e) const
{
    return Epsilon(x, a.x, e) && Epsilon(y, a.y, e) && Epsilon(z, a.z, e) && Epsilon(w, a.w, e);
}

void Vector4::Transform(const Matrix& m)
{
    Vector4 tmp;
    m.Transform(*this, &tmp);
    *this = tmp;
}

void Vector4::Transform(const Vector4& v, const Matrix& m)
{
    m.Transform(v, this);
}


}   // end namespace RadicalMathLibrary

