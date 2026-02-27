#ifndef _SIMCONSTRAINT_HPP_
#define _SIMCONSTRAINT_HPP_

#include <p3d/refcounted.hpp>
#include <radmath/radmath.hpp>
#include "simcommon/simtarget.hpp"

namespace sim
{

class SimState;

class LocalTarget {
    
public:
    LocalTarget():
      mIndex(0),
          mOffset(0.0f,0.0f,0.0f)
      {}
      int  GetIndex() const {return mIndex;}
      const rmt::Vector &GetOffset() const { return mOffset; }
      void SetIndex(int in_index) { mIndex=in_index; }
      void SetOffset ( const rmt::Vector in_Offset ) { mOffset = in_Offset; }
      
      bool operator == ( const LocalTarget &inTarget )const
      {
          if (mIndex != inTarget.mIndex)
              return false;
          if (mOffset.x != inTarget.mOffset.x)
              return false;
          if (mOffset.y != inTarget.mOffset.y)
              return false;
          if (mOffset.z != inTarget.mOffset.z)
              return false;
          
          return true;
      }
      bool operator != ( const LocalTarget &inTarget )const { return !(*this==inTarget);}
      
      
protected:
    int         mIndex;
    rmt::Vector mOffset;
    
};

class SimConstraint: public tRefCounted
{
public:
    
    SimConstraint( SimState *in_constrainingObject, LocalTarget &in_constrainingLacalTarget,
        SimState *in_constrainedObject,  LocalTarget &in_constrainedLacalTarget  );
    void Update( float in_dt);
    bool operator == ( const SimConstraint &in_cns )const
    {
        if ( mState1 != in_cns.mState1 )
            return false;
        if ( mState2 != in_cns.mState2 )
            return false;
        if ( mLocTarget1 == in_cns.mLocTarget1 )
            return false;
        if ( mLocTarget2 == in_cns.mLocTarget2 )
            return false;
        
        return true;
    }
    
protected:
    ~SimConstraint();
    
    SimState    *mState1;      //The simstate of the constraining object.
    SimTarget   *mTarget1;     //The constraining target. A world position.
    LocalTarget  mLocTarget1;  //Tartet as specified in the local ref frame of the constraining object. Usually a constant set at definition.
    SimState    *mState2;      //The simstate of the constrained object.
    //SimTarget   *mTarget2;     //The constrained target. A world position.
    LocalTarget  mLocTarget2;  //Tartet as specified in the local ref frame of the constrained object. Usually a constant set at definition.
    
};

} // sim

#endif //_SimConstraint_HPP_