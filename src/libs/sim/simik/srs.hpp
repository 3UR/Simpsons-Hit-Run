
/*
This source code is a part of IKAN.
Copyright (c) 2000 University of Pennsylvania
Center for Human Modeling and Simulation
All Rights Reserved.

IN NO EVENT SHALL THE UNIVERSITY OF PENNSYLVANIA BE LIABLE TO ANY
PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS
SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF PENNSYLVANIA
HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

Permission to use, copy, modify and distribute this software and its
documentation for educational, research and non-profit purposes,
without fee, and without a written agreement is hereby granted,
provided that the above copyright notice and the following three
paragraphs appear in all copies. For for-profit purposes, please
contact University of Pennsylvania
(http://hms.upenn.edu/software/ik/ik.html) for the software license
agreement.

  
    THE UNIVERSITY OF PENNSYLVANIA SPECIFICALLY DISCLAIM ANY
    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
    BASIS, AND THE UNIVERSITY OF PENNSYLVANIA HAS NO OBLIGATION
    TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
    MODIFICATIONS.
    
*/

#ifndef _SRSH
#define _SRSH

#include "simik/myvec.hpp"

namespace sim
{

//
// Given matrices G, S, T solve the equation 
//
// G = R2*S*Ry*T*R1
//	for R1,Ry,R2 
// where
//	R1 and R2 represent general rotation matrices
//      Ry represents a rotation about the y axis
// and
//      G is the desired goal IKMatrix
//      S, T are constant matrices 
// 

//
// In the case of the arm:
//	 R2 : Wrist joints 
//	 S  : Wrist to Elbow transformation
//       Ry : Elbow joint
//       T  : Elbow to Shoulder transformation
//       R1 : Shoulder joints 
//

class SRS
{
public:
    short project_to_workspace;
    //
    // Stores equation of circle for a given problem
    //
    float u[3];
    float v[3];
    float n[3];
    float c[3];
    float u0[3];
    float radius;
    
    rmt::Vector mU, mV, mN, mC, mU0;
    
    //
    // Stores projection axis for determining u and the positive 
    // direction axis for determining positive direction of angle
    float proj_axis[3];
    float pos_axis[3];
    rmt::Vector mProjAxis, mPosAxis;
    
    // Stores end effector position in world frame and R1 frame
    //
    float ee[3]; 
    float ee_r1[3];
    
    rmt::Vector mEE, mEE_r1;
    // Stores position of middle revolute joint in R1 frame
    // 
    float p_r1[3]; 
    rmt::Vector mp_r1;
    //
    // Stores angle of R joint
    //
    float upper_len;   // Len of T pos vector
    float lower_len;   // Len of S pos vector
    float reciprocal_upper_len; 
    
    float r_angle;
    
    // 
    // Stores goal transformation
    //
    IKMatrix G;
    
    //
    // Stores constant matrices and rotation of revolute joint
    // and their product S*Ry*T. 
    
    IKMatrix T, S, SRT;
    rmt::Matrix mT, mS, mSRT, mInvS;
    
    //
    // Ry = Rotation IKMatrix by flexion joint (only used by aiming routines)
    // axis = axis of aiming vector in hand coordinates
    IKMatrix Ry;
    rmt::Matrix mRmid;
    float axis[3];
    
    void evaluate_circle(float angle, float p[3]);
    void evaluate_circle(float angle, rmt::Vector p);
    
    // public:
    void ProjectOn() 
    { project_to_workspace = 1; }
    
    void ProjectOff() 
    { project_to_workspace = 0; }
    
    //
    // Given the position of the R joint find the corresponding 
    // swivel angle. Must call SetGoal or SetGoalPos first.
    // 
    
    float PosToAngle(const float p[3]);
    float PosToAngle(const rmt::Vector& p);
    //
    // Given the swivel angle calculate the pos of the R joint. 
    // Must call SetGoal or SetGoalPos first.
    // 
    void AngleToPos(float psi, float p[3]);
    
    // Sets the goal IKMatrix, the projection axis, and the 
    // positive direction axis
    // Returns 1 if the goal is feasible
    int  SetGoal(const IKMatrix  G, float &rangle);
    
    // Solve for both R1 and R2 given the pos or angle of the R joint
    // returns the angle of the R joint 
    
    void SolveR1R2(const float pos[3], IKMatrix  R1, IKMatrix  R2);
    void SolveR1R2(float angle, IKMatrix  R1, IKMatrix  R2);
    
    void Solve_R1(   const rmt::Vector p1, 
        const rmt::Vector q1, 
        const rmt::Vector p2, 
        const rmt::Vector q2,
        rmt::Matrix& inR1);
    
    // Must call SetGoal first 
    // Returns the psi equations of the rotation IKMatrix R1
    // ie: alpha[i][j]*cos(phi) + 
    //     beta[i][j]*sin(phi) + 
    //     xi[i][j] = R1[i][j] 
    
    int R1Psi(IKMatrix alpha, IKMatrix beta, IKMatrix xi);
    
    // Must call SetGoal first 
    // Returns the psi equations of the rotation IKMatrix R1 and R2 analogous
    // to R1Psi
    //
    
    int R1R2Psi(IKMatrix alpha,  IKMatrix beta, IKMatrix xi,
        IKMatrix alpha2, IKMatrix beta2, IKMatrix xi2); 
    
    
    // Sets the goal pos. EE is a constant IKMatrix that specifies
    // the value of R2*E where E is a IKMatrix that puts the base
    // of the last S joint to a desired EE site
    //
    // Returns 1 if the goal is feasible
    //
    // Thus the problem to solve is to find R1 and Ry st 
    //
    // g = [0,0,0,1]*EE*S*Ry*T*R1 
    
    int  SetGoalPos(const float g[3], const IKMatrix  EE, float &rangle);
    int  SetGoalPos(const float g[3], const IKMatrix  EE, const rmt::Vector& inRotateAxis, rmt::Matrix& inRmid);
    int  SetGoalPos(rmt::Vector inGoal, rmt::Matrix  inE, rmt::Matrix& inRmid);
    
    // Solves only for R1 after a call to SetGoalPos
    void SolveR1(const float pos[3],  IKMatrix  R1);
    void SolveR1(float angle,  IKMatrix  R);
    void SolveR1(float angle, rmt::Matrix R);
    
    // Constructor takes the T and S matrices
    void init(const IKMatrix  T, const IKMatrix  S, const float a[3], const float p[3]);
    void init(const rmt::Matrix& T, const rmt::Matrix& S, const rmt::Vector& a, const rmt::Vector& p);
    
    SRS(const IKMatrix  T1, const IKMatrix  S1, const float a[3], const float p[3]) 
    { 
        init(T1,S1,a,p); 
    }
    SRS(const rmt::Matrix T1, const rmt::Matrix S1, const rmt::Vector& a, const rmt::Vector& p)
    {  
        init(T1, S1, a, p);
    }
    
    SRS()  {}
    
    ~SRS() {}
    
    
    void Tmatrix(IKMatrix  TT)
    {
        cpmatrix(TT, T);
    }
    void Smatrix(IKMatrix  SS)
    {
        cpmatrix(SS, S);
    }
    
    
    
    // Sets the goal for an aiming problem
    // goal is the point we want to point to 
    // axis is the pointing axis in the hand frame
    // flex_angle is the amount of flexion in the elbow 
    /*
    void SRS::SetAimGoal(const float goal[3],
    */
    void SetAimGoal(const float goal[3],
        const float axis[3],
        float flex_angle);
    
    
    //
    // Solves the aiming problem for a given angle of the hand circle
    // (Must call SetAimGoal first)
    //
    /*
    void SRS::SolveAim(float psi_angle, IKMatrix  R1);
    */
    void SolveAim(float psi_angle, IKMatrix  R1);
    
    void SetAxes(float* proj, float* pos);
    void SetAxes(const rmt::Vector& proj, const rmt::Vector& pos);
    
    void GetCircleVars(float*, float*, float*, float&);
    void GetCircleCenter(float* fc);
};

} // sim

#endif
