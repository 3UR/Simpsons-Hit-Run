// lPath.h
// Created by wng on May 23, 2000 @ 12:22 PM.

#ifndef __lPath__
#define __lPath__

#ifdef WIN32
   #include <assert.h>     //IAN IMPROVE: remove this
    #include <stdlib.h>
#endif

#ifdef macintosh
   #include <stdlib.h>
    #define _MAX_DRIVE  3   /* max. length of drive component */
    #define _MAX_DIR    256 /* max. length of path component */
    #define _MAX_FNAME  256 /* max. length of file name component */
    #define _MAX_EXT    256 /* max. length of extension component */
#endif

#if !defined ( WIN32 ) && !defined( macintosh )
    #define _MAX_PATH   260 /* max. length of full pathname */
    #define _MAX_DRIVE  3   /* max. length of drive component */
    #define _MAX_DIR    256 /* max. length of path component */
    #define _MAX_FNAME  256 /* max. length of file name component */
    #define _MAX_EXT    256 /* max. length of extension component */
#else
#endif

class lPath
{
//    char mDrive[_MAX_DRIVE];
//    char mDir[_MAX_DIR];
    char mFullDir[_MAX_DIR+_MAX_DRIVE];
    char mFname[_MAX_FNAME];
    char mExt[_MAX_EXT];
    char mFullPath[ _MAX_PATH ];

public:
    lPath(const char *full_path);
    virtual ~lPath();
//    const char *Drive() {return mDrive;}
//    const char* Dir() {return mDir;}
    const char* FullDir() //IAN IMPROVE: remove this function entirely
    {
        return mFullDir;
    }            //there's no filename on this
    const char* FullPath() {return mFullPath;}
    const char* FileName() {return mFname;}             //there's no extension on this
    const char* Ext() {return mExt;}
};

#endif
