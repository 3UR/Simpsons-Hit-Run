/*===========================================================================
    textdataparser.hpp
    12-Nov-01 Created by Liberty... Eric wrote the code.

    Copyright (c)2001 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#ifndef _TEXTDATAPARSER_HPP
#define _TEXTDATAPARSER_HPP

class tFile;
#include <ctype.h>

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
    bool AdvanceTo(const char *s);


    bool Eof(void) { return mCurrentTokenStart >= mDataLen; }

    // Returns data of the current token
    int CurrentToken(char *dest, int destlen);
    char *DupCurrentToken(void);

    int CurrentLine(void) { return mCurrentLine; }

    int TokenCountAtCurrentBracketLevel(const char *token);

    // If current token =='{' will skip ahead to the matching close bracket
    bool SkipBracketedSection(void);

    inline bool  PushPosition(void);
    inline bool  PopPosition(void);

 private:

    char *mData;
    int   mDataLen;
    
    int   mCurrentTokenStart;
    int   mCurrentTokenLen;

    int   mCurrentLine;

    inline bool IsSeperator(char c);

    inline bool IsCurrentTokenComment(void);

    int   mBracketLevel;

    bool  mDataPushed;
    int   mPushedCurrentTokenStart;
    int   mPushedCurrentTokenLen;
    int   mPushedCurrentLine;
    int   mPushedBracketLevel;
};

//******************************************************
//
// Class TextDataParser
// Push Position
//
//
inline bool TextDataParser::PushPosition(void)
{
    if (mDataPushed) return false;

    mDataPushed = true;
    mPushedCurrentTokenStart = mCurrentTokenStart;
    mPushedCurrentTokenLen   = mCurrentTokenLen;
    mPushedCurrentLine       = mCurrentLine;
    mPushedBracketLevel      = mBracketLevel;
    
    return true;
}
//******************************************************
//
// Class TextDataParser
// Pop Position
//
//
inline bool TextDataParser::PopPosition(void)
{
    if (!mDataPushed) return false;

    mDataPushed = false;
    mCurrentTokenStart = mPushedCurrentTokenStart;
    mCurrentTokenLen   = mPushedCurrentTokenLen;
    mCurrentLine       = mPushedCurrentLine;
    mBracketLevel      = mPushedBracketLevel;
    return true;
}

//******************************************************
//
// Class TextDataParser
// Is Seperator
//
//
inline bool TextDataParser::IsSeperator(char c)
{

    if (c == 0x0a) ++mCurrentLine;

    if (isalnum(c)) return false;
    if (c == '_')   return false;

    if (c == '+') return false;
    if (c == '-') return false;

    if (c == '{') return false;
    if (c == '}') return false;

    if (c == '/') return false;

    if (c == '.') return false;
    if (c == ':') return false;
    if (c == '\\') return false;


    return true;
}


//******************************************************
//
// Class TextDataParser
// Is Current Token Comment
//
//
inline bool TextDataParser::IsCurrentTokenComment(void)
{
        
    char *s = &mData[mCurrentTokenStart];
    if ((s[0] == '/') && (s[1] == '/')) return true;

    return false;
}


#endif

