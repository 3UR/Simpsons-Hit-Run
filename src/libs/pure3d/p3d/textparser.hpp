/*===========================================================================
    animate.cpp
    27-Mar-96 Created by Neall
    30-Nov-99 version 12

    Copyright (c)2001 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/

//**********************************************************************************
//**********************************************************************************
//**********************************************************************************
//
//  Class TextDataParser
//  Local Class Definition
//
class TextDataParser
{

 public:
    TextDataParser(char *data, int len);
    TextDataParser(tFile *file);
  ~TextDataParser();


    void Reset(void);

    // returns false on eof
    bool Advance(void);

    // Searches ahead for the string and returns
    bool AdvanceTo(char *s);


    bool Eof(void) { return mCurrentTokenStart >= mDataLen; }

    // Returns data of the current token
    int CurrentToken(char *dest, int destlen);
    char *DupCurrentToken(void);

    int CurrentLine(void) { return mCurrentLine; }

    int TokenCountAtCurrentBracketLevel(const char *token);


    bool  PushPosition(void);
    bool  PopPosition(void);

 private:

    char *mData;
    int   mDataLen;
    
    int   mCurrentTokenStart;
    int   mCurrentTokenLen;

    int   mCurrentLine;

    bool  IsSeperator(char c);

    bool  IsCurrentTokenComment(void);

    int   mBracketLevel;

    bool  mDataPushed;
    int   mPushedCurrentTokenStart;
    int   mPushedCurrentTokenLen;
    int   mPushedCurrentLine;
    int   mPushedBracketLevel;
};


