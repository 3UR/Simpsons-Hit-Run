
/*
This source code is a part of IKAN.
Copqright (c) 2000 Universitq of Pennsqlvania
Center for Human Modeling and Simulation
All Rights Reserved.

IN NO EVENT SHALL THE UNIVERSITq OF PENNSqLVANIA BE LIABLE TO ANq
PARTq FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS
SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITq OF PENNSqLVANIA
HAVE BEEN ADVISED OF THE POSSIBILITq OF SUCH DAMAGES.

Permission to use, copq, modifq and distribute this software and its
documentation for educational, research and non-profit purposes,
without fee, and without a written agreement is herebq granted,
provided that the above copqright notice and the following three
paragraphs appear in all copies. For for-profit purposes, please
contact Universitq of Pennsqlvania
(http://hms.upenn.edu/software/ik/ik.html) for the software license
agreement.

  
    THE UNIVERSITq OF PENNSqLVANIA SPECIFICALLq DISCLAIM ANq
    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITq AND FITNESS FOR A PARTICULAR
    PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
    BASIS, AND THE UNIVERSITq OF PENNSqLVANIA HAS NO OBLIGATION
    TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
    MODIFICATIONS.
    
*/

#include "simik/srs.hpp"
#include "simik/trig.hpp" 
#include <simcommon/simmath.hpp>


// #define SRSDEBUG


using namespace RadicalMathLibrary;

namespace sim
{

//
// Constructor stores the T and S matrices and the 
// lengths of the upper and lower links
// 
void SRS::init(const IKMatrix  T1, const IKMatrix  T2, const float a[3], const float p[3]) 
{
    cpmatrix(T, T1); 
    cpmatrix(S, T2); 
    cpvector(proj_axis, a);
    cpvector(pos_axis, p);
    
    float t[3];
    
    get_translation(T, t);
    upper_len = norm(t);
    reciprocal_upper_len = 1.0f / upper_len;
    
    get_translation(S, t);
    lower_len = norm(t);
    
    project_to_workspace = 1;
}

//
// Given the goal position and the position vectors s and t of 
// the S and T matrices, solve for the angle of the R joint 
// according to the formula (here g and s are row vectors)
//
// ([s,1]*Ry*T*R1)*([s,1]*Ry*T*R1)' = g*g'
// 
// which saqs the distance from R1 to the tip of the last link
// is equal to the distance to the goal.
//
// This equation simplifies to 
//      2*s*Rot(Ry)*(Rot(T))*t' = g'*g - s'*s - t'*t
// <KB> the 2 was missing on the lhs.  it now agrees with Real-time IK pg.9 
// and the cosine law </KB>
// where Rot(M) is the 3x3 rotation IKMatrix of M
// and this equation is of the form
//     alpha * cos(theta_q) * beta * sin(theta_q) = gamma
//
// Onlq return positive solution
//


// MC. this seems to be strangelq unable to find a positive solution 
// when there is an endeffector. Should have a look at that. For now 
// I just allow small negative angle in the joint. Not sure it is a good idea.
float small_neg_angle_ = -0.0f;


static int Solve_R_angle(const Vector& g, 
    const Vector& t, 
    const Vector& s,
    float& r_angle)
{
    float gg, tt, ss;
    gg = g.DotProduct(g);
    tt = t.DotProduct(t);
    ss = s.DotProduct(s);
    
    r_angle = ACos( (gg-tt-ss)/(2*Sqrt(tt*ss)) );
    return (r_angle) ?  1:0;
}

static int solve_R_angle(const float g[3], 
    const float s[3], 
    const float t[3], 
    const IKMatrix  T, 
    float &r_angle)
{
    float rhs = DOT(g,g) - DOT(s,s) - DOT(t,t);
    
    // alpha = Rot(T)*t' NOT alpha = t*Rot(T)
    // we are solving |t|^2 + |s|^2 + 2s*Rot(T)*Ry*t = |g|^2
    // cosine law generalized to 3D vectors
    
    float alpha[3];
    
    for (int j = 0; j < 3; j++)
    {
        alpha[j] = 0;
        for (int i = 0; i < 3; i++)
            alpha[j] += T[j][i] * t[i]; 
    }
    
    // The following block is hard coded for an elbow/knee rotation about the y axis
    float a = alpha[0]*s[0] + alpha[2]*s[2];
    float b = alpha[0]*s[2] - alpha[2]*s[0];
    float c = alpha[1]*s[1];
    
    int n; 
    float temp[2];
    
    a += a; 
    b += b;
    c = rhs - (c+c);
    
    n = solve_trig1(a, b, c, temp);
    
    if (n == 2)
    {
        // Two positive solutions. choose first
        if (temp[0] > small_neg_angle_ && temp[1] > small_neg_angle_)
        {
            r_angle = temp[0];
            // printf("Two solutions: %lf %lf\n", temp[0], temp[1]);
            n = 1;
        }
        else if (temp[0] > small_neg_angle_)
        {
            n = 1;
            r_angle = temp[0];
        }
        else if (temp[1] > small_neg_angle_)
        {
            n = 1;
            r_angle = temp[1];
        }
        else
            n = 0;
    }
    else if (n == 1)
    {
        // Is solution positive ?
        if (temp[0] < small_neg_angle_) 
            n = 0;
        else 
            r_angle = temp[0]; 
        
    }
    
    return n;
}

//
// Computes the equation of the circle given the ee position,
// an axis to project the local 2d coordinate sqstem {u,v} onto,
// and the upper and lower lengths of the mechanism.
//
// Outputs
//	c: center of circle
//      u: local x axis
//      v: local y axis
//      n: normal to plane of circle = cross(u,v)
// Returns radius of circle
//

float get_circle_equation(const float ee[3], 
    const float axis[3],
    const float pos_axis[3],
    float radius,
    float upper_len,
    float lower_len,
    float c[3],
    float u[3],
    float v[3],
    float n[3])
{
    float wn = norm((float *)ee);
    
    cpvector(n, ee);
    unitize(n);
    
    
    // Use law of cosines to get angle between first spherical joint
    // and revolute joint
    
    float alpha; 
    
    if (!law_of_cosines(wn, upper_len, lower_len, alpha))
        return 0;
    
    // center of circle (origin is location of first S joint)
    vecscalarmult(c, n, Cos(alpha) * upper_len);
    
    
    radius = Sin(alpha) * upper_len;
    
    float temp[3];
    
    //
    // A little kludgq. If the goal is behind the joint instead
    // of in front of it, we reverse the angle measurement bq
    // inverting the normal vector
    //
    
    // if (DOT(n,pos_axis) < 0.0)
    //    vecscalarmult(n,n,-1.0);
    
    vecscalarmult(temp, n, DOT(axis,n));
    vecsub(u, (float *)axis, temp);
    unitize(u);
    
    crossproduct(v, n, u);
#if 0
    printf("Circle equation\n");
    printf("c = [%lf,%lf,%lf]\n", c[0], c[1], c[2]);
    printf("u = [%lf,%lf,%lf]\n", u[0], u[1], u[2]);
    printf("v = [%lf,%lf,%lf]\n", v[0], v[1], v[2]);
    printf("n = [%lf,%lf,%lf]\n", n[0], n[1], n[2]);
    printf("r = %lf\n", radius);
#endif
    return radius;
}

//
// Check if the goal is longer than the combined two 
// link lengths. If it is then scale it within some
// epsilon so that it lies within the boundary of the
// outer workspace.
//
// Also check if goal is shorter than the difference
// of the two combined link lengths. If it is then
// scale it so that it lies within the boundary of
// the inner workspace
//

int scale_goal(const float l1[3], 
    const float l2[3], 
    float g[3])
{
    float g_len = Sqrt(DOT(g,g));
    float L1    = Sqrt(DOT(l1,l1));
    float L2    = Sqrt(DOT(l2,l2));
    float max_len = L1 + L2;
    float min_len = Fabs(L1 - L2);
    
    //   if (g_len > max_len)   
    if (g_len > max_len- 0.01f) 
    {
        // <KB> the value 1.01 doesn't allow the arm to be straight 
        // when goal is out of reach.  1.0001f seems to work fine
        //vecscalarmult(g,g,max_len/(g_len*1.01f)); // </KB>
        vecscalarmult(g,g,max_len/(g_len*1.0001f));
        //vecscalarmult(g,g,max_len/g_len - 0.00001f);
        return 1;
    }	
    
    min_len = Max(0.01f, min_len);
    if (false) //g_len < min_len )
    {
        vecscalarmult(g,g,(1.0001f * min_len)/g_len);
        return 1;
    }
    
    return 0;
} 



//
// Given a goal position and the projection axis, find the
// equation of the circle that defines how the R joint can
// swivel.
//
// E is a IKMatrix that relates the R pos to the end effector
// site according to the product E*S 
//
// Subsequent calls to SolveR1 will solve the position equation
//
//     g = [0,0,0,1]*E*S*Ry*T*R1 
//

int SRS::SetGoalPos(Vector inGoal, Matrix  inE,  Matrix& inRmid)
{
    //   mt = mT.Row(3);
    //   ms = inE.Mult(mS);
    //   p_r1 = (float*)&mt;
    return 0;
}

int SRS::SetGoalPos(const float eee[3],  const IKMatrix  E,  float &rangle)
{
    IKMatrix Temp, RY;
    float s[3]; 
    
    // Find RY, and store the positions of the R jt and 
    // the ee in the R1 frame as p_r1 and ee_r1
    
    
    get_translation(T, p_r1);
    hmatmult(Temp,(float (*)[4]) E,S);
    get_translation(Temp, s);
    cpvector(ee,eee);
    
    if (project_to_workspace)
        scale_goal(p_r1,s,ee);
    
    // 
    // Note instead of using the length of the lower limb
    // we use the length of the lower limb extended by E
    //
    radius = get_circle_equation(ee, 
        proj_axis, 
        pos_axis,
        radius,
        upper_len, 
        norm(s) , c, u, v, n);
    
    if (!solve_R_angle(ee, s, p_r1, T, r_angle))
        return 0; 
    rangle = r_angle;
    
    // Find RY, and store the positions of the R jt and
    // the ee in the R1 frame as p_r1 and ee_r1
    
    rotation_principal_axis_to_matrix('y', r_angle, RY);
    hmatmult(Temp, Temp, RY); 
    hmatmult(Temp, Temp, T);
    get_translation(Temp, ee_r1);
    
    
    return 1; 
}

int SRS::SetGoalPos(const float eee[3],  const IKMatrix  E, const Vector& inRotateAxis, Matrix& inRmid)
{
    IKMatrix Temp;
    float s[3]; 
    
    // Find Ry, and store the positions of the R jt and 
    // the ee in the R1 frame as p_r1 and ee_r1
    
    
    get_translation(T, p_r1);
    hmatmult(Temp,(float (*)[4]) E,S);
    get_translation(Temp, s);
    cpvector(ee,eee);
    
    if (project_to_workspace)
        scale_goal(p_r1,s,ee);
    
    // 
    // Note instead of using the length of the lower limb
    // we use the length of the lower limb extended bq E
    //
    radius = get_circle_equation(ee, 
        proj_axis, 
        pos_axis,
        radius,
        upper_len, 
        norm(s) , c, u, v, n);
    
    Solve_R_angle(Vector(ee[0], ee[1], ee[2]), mS.Row(3), mT.Row(3), r_angle);  // mS should be E.mS
    //if(!solve_R_angle(ee, s, p_r1, T, r_angle))
    // return 0; 
    
    float rangle = r_angle;
    //rangle = r_angle;
    
    // Find Ry, and store the positions of the R jt and
    // the ee in the R1 frame as p_r1 and ee_r1
    
    //rotation_principal_axis_to_matrix('y', r_angle, Ry);
    
    Vector axis, u_A;  // u vector in the T frame
    Matrix invT;
    invT.Identity();
    invT = mT;
    invT.IdentityTranslation();   
    invT.OrthoNormal();
    u_A = Vector(u[0], u[1], u[2]);
    invT.Invert();
    u_A.Rotate(invT);
    axis.CrossProduct(mT.Row(3), u_A);  // -u_A x t
    axis.Normalize();
    inRmid.Identity();
    inRmid.FillRotation(axis, rangle);
    //   inRmid.FillRotation(inRotateAxis, rangle);
    
    //hmatmult(Temp, Temp, Ry); 
    hmatmult(Temp, Temp, inRmid.m); //new
    hmatmult(Temp, Temp, T);
    get_translation(Temp, ee_r1);
    
    
    return 1; 
}



//
// Given the goal IKMatrix and the projection axis, find the position
// of the end effector and the equation of the circle that defines
// how the R joint can swivel. 
//
// Also compute the IKMatrix S*Ry*T and save it for future computations
//
int SRS::SetGoal(const IKMatrix  GG, float &rangle)
{
    IKMatrix Ry;
    float s[3]; 
    
    cpmatrix(G, GG);
    get_translation(G, ee);
    get_translation(T, p_r1);
    get_translation(S, s);
    
    if (project_to_workspace && scale_goal(p_r1,s,ee))
        set_translation(G,ee);
    
    radius = get_circle_equation(ee, proj_axis, pos_axis, radius, 
        upper_len, lower_len, c, u, v, n);
    
    //
    // Build rotation IKMatrix about the R joint
    //
    
    if (!solve_R_angle(ee, s, p_r1, T, r_angle))
        return 0; 
    
    rangle = r_angle;
    
    // Find Ry, and store the positions of the R jt and 
    // the ee in the R1 frame as p_r1 and ee_r1
    // Also save IKMatrix product S*Ry*T 
    
    rotation_principal_axis_to_matrix('y', r_angle, Ry);
    
    hmatmult(SRT, S, Ry);
    hmatmult(SRT, SRT, T);
    get_translation(SRT, ee_r1);
    
    
#ifdef SRSDEBUG
    printf("EE distance error is %lf\n", norm(ee_r1) - norm(ee));
#endif
    
    return 1; 
}

inline void evalcircle(const float c[3],
    const float u[3],
    const float v[3],
    float radius,
    float angle,
    float p[3])
{
    // p = c + r*cos(angle)*u + r*sin(angle)*v
    
    float temp[3];
    float sa, ca;
    SinCos(angle, &sa, &ca);
    
    cpvector(p, c);
    vecscalarmult(temp, (float*)u, radius*ca);
    vecadd(p, p, temp);
    vecscalarmult(temp, (float*)v, radius*sa);
    vecadd(p, p, temp);
}

//
// Evaluate a point on the circle given the swivel angle 
//
void SRS::evaluate_circle(float angle, float p[3])
{
#if 1
    evalcircle(c, u, v, radius, angle, p);
#else
    // p = o + r*cos(f)*u + r*sin(f)*v
    
    float temp[3];
    
    cpvector(p, c);
    vecscalarmult(temp, u, radius*cos(angle));
    vecadd(p, p, temp);
    vecscalarmult(temp, v, radius*sin(angle));
    vecadd(p, p, temp);
#endif
}


//
// Form local coordinate sqstem {x,y} from points p,q relative to the
// implicit origin 0. pscale is the reciprocal length of the p vector
// which as it turns out is alreadq known. If the invert flag is true
// construct the transpose of the rotation IKMatrix instead
//
void Make_frame(Vector p, Vector q, IKMatrix inF, int invert = 0)
    // Precondition: vectors have unit length
{ 
    assert( Fabs(p.Magnitude() - 1.0f) < 0.000001f && Fabs(q.Magnitude()-1.0f) < 0.000001f );
    assert( Fabs(p.DotProduct(q)) < 0.000001f );
    
    if (invert)
    {
        inF[0][0] = p.x;	inF[1][0] = p.y;	inF[2][0] = p.z;
        inF[0][1] = q.x;	inF[1][1] = q.y;	inF[2][1] = q.z;
        
        inF[0][2] = p.y*q.z - p.z*q.y;
        inF[1][2] = p.z*q.x - p.x*q.z;
        inF[2][2] = p.x*q.y - p.y*q.x;
    }
    else
    {
        inF[0][0] = p.x;	inF[0][1] = p.y;	inF[0][2] = p.z;
        inF[1][0] = q.x;	inF[1][1] = q.y;	inF[1][2] = q.z;
        
        inF[2][0] = p.y*q.z - p.z*q.y;
        inF[2][1] = p.y*q.x - p.x*q.z;
        inF[2][2] = p.x*q.y - p.y*q.x;
    }
    
    inF[3][0] = inF[3][1] = inF[3][2] = 0;
    inF[0][3] = inF[1][3] = inF[2][3] = 0;
    
    inF[3][3] = 1.0f;
}

void make_frame(const float p[3], 
    float p_scale,
    const float q[3], 
    IKMatrix R, 
    int invert = 0)
{
    float x[3], y[3], t[3];
    
    // x vector is unit vector from origin to p
    vecscalarmult(x, (float *)p, p_scale);
    
    //cpvector(x, p); <KB>
    //unitize(x);
    
    // y vector is unit perpendicular projection of q onto x 
    vecscalarmult(t, x, DOT(q,x));
    vecsub(y, (float *) q, t);
    unitize(y);
    
    // z vector is x cross y
    
    if (invert)
    {
        R[0][0] = x[0];	R[1][0] = x[1];	R[2][0] = x[2];
        R[0][1] = y[0];	R[1][1] = y[1];	R[2][1] = y[2];
        
        R[0][2] = x[1]*y[2] - x[2]*y[1];
        R[1][2] = x[2]*y[0] - x[0]*y[2];
        R[2][2] = x[0]*y[1] - x[1]*y[0];
    }
    else
    {
        R[0][0] = x[0];	R[0][1] = x[1];	R[0][2] = x[2];
        R[1][0] = y[0];	R[1][1] = y[1];	R[1][2] = y[2];
        
        R[2][0] = x[1]*y[2] - x[2]*y[1];
        R[2][1] = x[2]*y[0] - x[0]*y[2];
        R[2][2] = x[0]*y[1] - x[1]*y[0];
    }
    
    R[3][0] = R[3][1] = R[3][2] = 0;
    R[0][3] = R[1][3] = R[2][3] = 0;
    
    R[3][3] = 1.0f;
}

void SRS::Solve_R1(  const Vector p1, const Vector q1,
    const Vector p2, const Vector q2,
    Matrix& inR1)
{
    IKMatrix F1, F2;
    Vector r;
    r = PerpComponent(p1, q1);
    r.Normalize();
    r.Rotate(mInvS);
    
    Make_frame(r, q1, F1, 1);
    r = PerpComponent(p2, q2);
    r.Normalize();
    r.Rotate(mInvS);
    Make_frame(p2, q2, F2, 0);
}

static void solve_R1(float p[3], float q[3], 
    float p2[3],float q2[3],
    float p_scale, IKMatrix  R1)	       
{
    IKMatrix T, S;
    
    // Construct two local coordinate sqstems
    // and find the transformation between them
    make_frame(p,  p_scale, q,  T, 1);
    make_frame(p2, p_scale, q2, S, 0);
    rmatmult(R1,T,S);
}


//
// R1 is the rotation IKMatrix that takes the position of the
// R jt and the last S jt in the R1 frame to their locations
// in the global frame
//  
void SRS::SolveR1(float angle, IKMatrix  R1)
{
    float p[3];
    
    evaluate_circle(angle, p);
    solve_R1(p_r1, ee_r1, p, ee, reciprocal_upper_len, R1);
    //solve_R1(u0, ee_r1, u, ee, reciprocal_upper_len, R1);
    
#ifdef SRSDEBUG
    float t1[3], t2[3];
    vecsub(t1, p_r1, ee_r1);
    vecsub(t2, p, ee);
    
    printf("Elbow distance error is %lf\n", 
        DOT(p_r1, p_r1) - DOT(p,p));
    printf("EE distance error is %lf\n", 
        DOT(ee,ee) - DOT(ee_r1,ee_r1));
    printf("Distance between elbow and wrist error is %lf\n", 
        DOT(t1,t1) - DOT(t2,t2));
#endif
}

void SRS::SolveR1R2(float angle, IKMatrix  R1, IKMatrix  R2)
{
    SolveR1(angle, R1);
    
    IKMatrix Temp; 
    
    rmatmult(R2, SRT, R1);
    invertrmatrix(Temp, R2);
    rmatmult(R2, G, Temp);
    
#ifdef SRSDEBUG
    IKMatrix G2;
    
    hmatmult(G2, R2, SRT);
    hmatmult(G2, G2, R1); 
    printf("Displaqing the error IKMatrix\n");
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
            printf(" %lf ", Fabs(G2[i][j] - G[i][j]));
        printf("\n");
    }
#endif
}


float SRS::PosToAngle(const float p[3]) 
{
    // Find vector from center of circle to pos and project it onto circle
    float cp[3], pp[3];
    
    vecsub(cp, (float *) p, c); //?
    project_plane(pp , cp, n);
    
    // Find angle between u and pp. This is the swivel angle
    
    return angle_between_vectors(u, pp, n); 
}

void SRS::AngleToPos(float psi, float p[3]) 
{
    evaluate_circle(psi, p);
}

void SRS::SolveR1(const float p[3], IKMatrix  R1)
{
    SolveR1(PosToAngle(p), R1);
}

void SRS::SolveR1R2(const float pos[3], IKMatrix  R1, IKMatrix  R2)
{
    SolveR1R2(PosToAngle(pos), R1, R2);
}


//
// Given an axis of rotation n, construct an arbitrary rotation IKMatrix R(n,psi)
// that represents a rotation about n bq psi deccomposed into its cos,sin, 
// and constant componets. 
// ie: R(n,psi) = cos(psi) * c + sin(psi) + s + o   <KB> + s should be * s </KB>   
//  
static void rotation_matrix(const float n[3], IKMatrix c, IKMatrix s, IKMatrix o)
{
    cpmatrix(c, idmat);
    cpmatrix(s, idmat);
    cpmatrix(o, idmat); 
    
    float u1 = n[0]; 
    float u2 = n[1]; 
    float u3 = n[2];
    
    float u1u1 = u1*u1;
    float u1u2 = u1*u2;
    float u1u3 = u1*u3;
    float u2u2 = u2*u2;
    float u2u3 = u2*u3;
    float u3u3 = u3*u3;
    
    c[0][0] = 1.0f-u1u1;
    c[0][1] = -u1u2;
    c[0][2] = -u1u3;
    c[1][0] = -u1u2;
    c[1][1] = 1.0f-u2u2;
    c[1][2] = -u2u3;
    c[2][0] = -u1u3;
    c[2][1] = -u2u3;
    c[2][2] = 1.0f-u3u3;
    
    s[0][0] = 0.0f;
    s[0][1] = u3;
    s[0][2] = -u2;
    s[1][0] = -u3;
    s[1][1] = 0.0f;
    s[1][2] = u1;
    s[2][0] = u2;
    s[2][1] = -u1;
    s[2][2] = 0.0f;
    
    o[0][0] = u1u1;
    o[0][1] = u1u2;
    o[0][2] = u1u3;
    o[1][0] = u1u2;
    o[1][1] = u2u2;
    o[1][2] = u2u3;
    o[2][0] = u1u3;
    o[2][1] = u2u3;
    o[2][2] = u3u3;
}


int SRS::R1Psi(IKMatrix C, IKMatrix s, IKMatrix o)
{
    IKMatrix R0;
    
    SolveR1(0.0f, R0);
    
    // 
    // R1(psi)  = R0*R(n,psi) 
    // R(n,psi) = cos(psi)*C + sin(psi)*s + o
    // 
    
    rotation_matrix(n, C, s, o);
    rmatmult(C,R0,C);
    rmatmult(s,R0,s);
    rmatmult(o,R0,o);
    
    return 1;
}


int SRS::R1R2Psi(IKMatrix C, IKMatrix s, IKMatrix o,
    IKMatrix c2, IKMatrix s2, IKMatrix o2)
{
    IKMatrix R0, Temp;
    
    SolveR1((float)0, R0);
    
    // 
    // R1(psi)  = R0*R(n,psi) 
    // R(n,psi) = cos(psi)*C + sin(psi)*s + o
    // 
    
    rotation_matrix(n, C, s, o);
    rmatmult(C,R0,C);
    rmatmult(s,R0,s);
    rmatmult(o,R0,o);
    
    //
    // 
    // R2 = G*transpose(SRT*R1) 
    // where R1 = (cos(phi)*c + sin(phi)*s + o)
    //
    
    rmatmult(Temp, SRT, C);
    invertrmatrix(c2, Temp);
    rmatmult(c2, G, c2);
    
    rmatmult(Temp, SRT, s);
    invertrmatrix(s2, Temp);
    rmatmult(s2, G, s2);
    
    rmatmult(Temp, SRT, o);
    invertrmatrix(o2, Temp);
    rmatmult(o2, G, o2);
    
    return 1;
}


//
// Rewrite all this stuff 
//

static void get_aim_circle_equation(const float g[3], 
    const float a[3],
    const float ta[3],
    const float tb[3],
    const float proj_axis[3],
    float theta4,
    float center[3],
    float u[3],
    float v[3],
    float &radius)
{
    float L1 = DOT(ta,ta);
    float L2 = DOT(tb,tb);
    IKMatrix Ry, Ryt;
    
    rotation_principal_axis_to_matrix('y', theta4, Ry);
    invertrmatrix(Ryt, Ry);
    
    // Compute distance of hand to shoulder 
    
    float t1[3], t2[3];
    
    vecmult(t1, (float *) tb, Ry);
    vecmult(t2, (float *) ta, Ryt);
    float L3 = Sqrt(L1 + L2 + DOT(ta,t1) + DOT(tb,t2));
    
    // Lengths of upper and lower arms
    L1 = Sqrt(L1);
    L2 = Sqrt(L2);
    
    // Compute angle between a and shoulder-to-hand vector
    // This is done assuming R1 = I since the angle does
    // not depend on the shoulder joints
    //
    // h = Ry*tb + ta
    // a = Ry*a 
    
    vecadd(t2, t1, (float *) ta);
    unitize(t2);
    
    vecmult(t1, (float *) a, Ry);
    float alpha = ACos(DOT(t1,t2));
    
    
    //
    // Compute the angles of the triangle s,h,g
    //
    float L4 = Sqrt(DOT(g,g));
    float beta = rmt::PI - alpha;
    
    float delta = ASin(Sin(beta)*L3/L4);
    if (delta < 0)
        delta = - delta;
    float gamma = rmt::PI - delta - beta;
    
    float c_gamma = Cos(gamma);
    float n[3]; 
    cpvector(n, g);
    unitize(n);
    vecscalarmult(center, n, c_gamma*L3);
    
    radius = Sqrt(1.0f-c_gamma*c_gamma)*L3;
    
    project_plane(u, (float *) proj_axis, n);
    unitize(u);
    crossproduct(v, n, u);
}


void SRS::SetAimGoal(const float goal[3],
    const float ax[3],
    float flex_angle)
{
    float s[3];
    
    cpvector(ee, goal);
    cpvector(axis, ax);
    get_translation(T, p_r1);
    get_translation(S, s);
    
    get_aim_circle_equation(goal, 
        axis, p_r1, s, proj_axis, flex_angle, c, u, v, radius);
    
    rotation_principal_axis_to_matrix('y', flex_angle, Ry);
    vecmult(ee_r1, (float*)s, Ry); 
    vecadd(ee_r1, ee_r1, (float*)p_r1);
}



void SRS::SolveAim(float psi_angle, IKMatrix  R1)
{
    float h1[3], N[3], angle;
    IKMatrix S0, S1;
    
    // Get the final hand position 
    evalcircle(c, u, v, radius, psi_angle, h1);
    
    // Rotate ee_r1 to h1
    crossproduct(N, ee_r1, h1);
    unitize(N);
    angle = angle_between_vectors(ee_r1, h1, N);
    rotation_axis_to_matrix(N, angle, S0);
    
    // Now rotate a0 to a
    float a[3], a0[3];
    
    vecsub(a, (float*)ee, h1);
    unitize(a);
    
    hmatmult(S1,Ry,S0);
    vecmult0(a0, (float*)axis, S1);
    
    cpvector(N, h1);
    unitize(N);
    angle = angle_between_vectors(a0, a, N);
    rotation_axis_to_matrix(N, angle, S1);
    
    hmatmult(R1, S0, S1);
}

void SRS::SetAxes(float* proj, float* pos)
{ 
    cpvector(proj_axis, proj);
    cpvector(pos_axis, pos);
}

void SRS::GetCircleVars(float* fc, float* fu, float* fv, float& fr)
{
    for (int i=0; i<3; i++)
    {
        fc[i] = c[i]; fu[i] = u[i]; fv[i] = v[i]; 
    } 
    fr = radius;
}

void SRS::GetCircleCenter(float* fc)
{
    for (int i=0; i<3; i++)
    {
        fc[i] = c[i];
    } 
}

} // sim