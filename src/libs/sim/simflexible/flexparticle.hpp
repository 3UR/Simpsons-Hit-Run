
#ifndef _FLEXPARTICLE_HPP_
#define _FLEXPARTICLE_HPP_

#include "radmath/radmath.hpp"

namespace sim
{
    
    class FlexParticle 
    {
    public:
        FlexParticle(int i=0);
        ~FlexParticle();
        
        void Render();
        //void AddDisplacment(const float d[]);
        
        //Constraint:
        void MoveSelf(const float d[], float invTimeStep);
        void UnMoveSelf(const float d[], float invTimeStep);
        void ResetDirectionOfConstraint( );//Reset previous Direction of constraint set by AddDirectionOfConstraint
        bool AddDirectionOfConstraint( const rmt::Vector &inDir); //Add a new direction of constraint, return true effectively added.
        void CleanForConstraint(rmt::Vector *io_d) const; //Filter io_d regarding the current set of direction of constraints.
        
        float          m_x[3];      // Current position of this particle.
        float          m_v[3];      // Current velocity of this particle.
        rmt::Vector    mCollConstraint[3]; // The set of constraining directions. 
        int            mNbCollConstraint;  // Number of direction diffined in the set constraining directions.
        int            mIndex;
        bool           mFree;              // If false, an external contraint maintain this particel to a fix position.
        bool           mBroken;
        //float dxc[3];                    // position alteration due to collisions
        //bool bdxc;                       // indicates if dxc contains something
        
#ifdef RAD_DEBUG
        // these are for debug
        float m_fs[3];                     // applied force
        float m_fb[3];                     // applied force
        float m_nn[3];                     // usefull vector to display custom values
        float m_fc[3];
#endif
        
    protected:
        
    };
    
} // sim

#endif // _FLEXPARTICLE_HPP_
