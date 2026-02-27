choreo readme.txt
===========================================================================

choreo v02.00 - 2002/03/25
===========================================================================
  New release featuring new blended driver support, optimizations.
  Updated to use Pure3D v17.  Should still be compatible with v16.

  Requirements
  -------------------------------------------------------------------------
    - Pure3D v17      (required path:  ../pure3d)
    - RadMath         (required path:  ../radmath)
    - Poser SDK       (required path:  ../poser)
    - Sim Release     (required path:  ../sim)
    - RadCore*        (required path:  ../radcore)
        * Note that the RadCore dependency can be compiled out.

  Changes
  -------------------------------------------------------------------------
    - Speed optimizations.
    - Added new Blend and BlendDriver classes for creation of generic
      parameterized animation blending.  Now used by the Locomotion
      drivers.
    - Preserve bone length flag honoured.
    - Dropped Pure3D v14 support (in order to maintain sanity).

  Issues
  -------------------------------------------------------------------------
    - Elbow-joint IK unimplemented (still).
    - Locomotion phase initialization is currently expensive (still).


choreo v01.01 - 2002/02/06
===========================================================================
  Compatibility release for new FTT and Pure3D v16 libraries.

  Requirements
  -------------------------------------------------------------------------
    - Pure3D v16      (required path:  ../pure3d)
      or Pure3D v14*  (required path:  ../pure3d)
    - RadMath         (required path:  ../radmath)
    - Poser SDK       (required path:  ../poser)
    - Sim Release     (required path:  ../sim)
    - RadCore**       (required path:  ../radcore)
        *  Pure3D v14 supported if CHOREO_USE_P3D_V14 is defined.
	   See choreo/buildconfig.hpp.
        ** Note that the RadCore dependency can be compiled out.

  Changes
  -------------------------------------------------------------------------
    - Using new p3d v16 memory allocation scheme.
    - Support for n-ped locomotion (not fully tested on quadrupeds).
    - Supports writing inventory contents to choreo script (for future
      run-time editing).  Needs CHOREO_TOOL to be defined.
    - Interface change:  Puppet/Engine update loop now specified by
      Advance(), UpdateBegin(), UpdateRoot(), UpdatePose(), UpdateEnd().
    - Added replay driver.
    - Added Choreo Viewer.
    - Added PS2 build.
    - Full root y-motion supported.
    - Added choreoinvlist tool, prints list of objects referenced by
      choreo scripts (useful for pipeline animation extraction).
    - Added choreochunk tool, which chunkifies choreo scripts.

  Issues
  -------------------------------------------------------------------------
    - Elbow-joint IK unimplemented (still).
    - Locomotion phase initialization is currently expensive (still).
    - Preserve bone length flag currently ignored (still).
    - FK leg joints still driven when IK is on (still).


choreo v01.00 - 2002/01/22
===========================================================================
  First official release of the Choreographer module.

  Requirements
  -------------------------------------------------------------------------
    - Pure3D v14     (required path:  ../pure3d)
    - Poser SDK      (required path:  ../poser)
    - Sim PreRelease (required path:  ../sim)

  Changes
  -------------------------------------------------------------------------
    - Initial drop.

  Issues
  -------------------------------------------------------------------------
    - Missing PS2 build.
    - Elbow-joint IK unimplemented.
    - AI root y-motion partially implemented.
    - Non-biped locomotion not reasonably supported.
    - Locomotion phase initialization is currently expensive.
    - Contained physics and p3d objects don't have same memory semantics.
    - Preserve bone length flag currently ignored.
    - FK leg joints still driven when IK is on.

--
End of file.
