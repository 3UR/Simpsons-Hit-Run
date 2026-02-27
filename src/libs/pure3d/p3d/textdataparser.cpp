/*===========================================================================
    textdataparser.cpp
    12-Nov-01 Created by Liberty... Eric wrote the code.

    Copyright (c)2001 Radical Entertainment, Inc.
    All rights reserved.
===========================================================================*/
#include <p3d/textdataparser.hpp>
#include <p3d/file.hpp>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//******************************************************
//
// Class TextDataParser
// Constructor from memory
//
//
TextDataParser::TextDataParser(char *data, int len) :
    mData(NULL),
    mDataLen(len),
    mCurrentTokenStart(0),
    mCurrentTokenLen(0),
    mCurrentLine(1),
    mBracketLevel(0),
    mDataPushed(false)
{
    mData = new char[mDataLen];
    memcpy(mData, data, mDataLen);
}


//******************************************************
//
// Class TextDataParser
// Constructor from tFile
//
//
TextDataParser::TextDataParser(tFile *file) :
    mData(NULL),
    mDataLen(0),
    mBracketLevel(0),
    mDataPushed(false)
{

    mDataLen = file->GetSize();      
    mData = new char[mDataLen];
    file->GetData(mData, mDataLen, tFile::BYTE);

    Reset();
}


//******************************************************
//
// Class TextDataParser
// Default destructor
//
//
TextDataParser::~TextDataParser()
{
    if (mData != NULL) delete[] mData;
    mData = NULL;
    mDataLen = 0;
    mCurrentTokenStart = 0;
    mCurrentTokenLen = 0;
    mCurrentLine = 0;
    mBracketLevel = 0;
    mDataPushed = false;
}


//******************************************************
//
// Class TextDataParser
// reset
//
//
void TextDataParser::Reset(void)
{
    mCurrentTokenStart = -1;
    mCurrentTokenLen = 0;
    mCurrentLine = 1;
    mBracketLevel = 0;
}



//******************************************************
//
// Class TextDataParser
// Advance
// 
//
bool TextDataParser::Advance(void)
{

    if (mData == NULL) return false;

    if (mCurrentTokenStart >= 0) mCurrentTokenStart += (mCurrentTokenLen + 1);
    else mCurrentTokenStart = 0;

    // Start with a zero lenght token
    mCurrentTokenLen = 0;

    // Find the start of the next token
    while ((mCurrentTokenStart < mDataLen) && IsSeperator(mData[mCurrentTokenStart])) 
    {
        ++mCurrentTokenStart;
    }
    
    // Check for EOF
    if (Eof()) 
    {
        return false;
    }

    // Find the length of the new token
    int maxlen = mDataLen - mCurrentTokenStart;      
    while ((mCurrentTokenLen < maxlen) && !IsSeperator(mData[mCurrentTokenStart + mCurrentTokenLen])) 
    {
        ++mCurrentTokenLen;
    }

    // If the current token is a comment, advance until 
    // the next line and try advancing from there
    if (IsCurrentTokenComment())
    {
        mCurrentTokenLen = 0;
        while ((mCurrentTokenStart < mDataLen) && (mData[mCurrentTokenStart] != 0x0A))
        {
            ++mCurrentTokenStart;
        }

        // Check for EOF
        if (Eof()) 
        {
            return false;
        }

        // I did just hit a return, didn't I?
        ++mCurrentLine;

        return Advance();
    }

    // Check for special tokens
    if (mCurrentTokenLen == 1)
    {
        if (mData[mCurrentTokenStart] == '{') ++mBracketLevel;
        if (mData[mCurrentTokenStart] == '}') --mBracketLevel;
    }

    // All done, report success
    return true;
}





//******************************************************
//
// Class TextDataParser
// Advance To
// 
//
bool TextDataParser::AdvanceTo(const char *s)
{
    char token[128];

    while (!Eof())
    {
        CurrentToken(token, sizeof(token));
        if (!strcmp(token, s)) return true;
        Advance();
    }

    return false;
}



//******************************************************
//
// Class TextDataParser
// CurrentToken
// Returns data of the current token
//
int TextDataParser::CurrentToken(char *dest, int destlen)
{

    if (mData == NULL) return 0;
    if (dest == NULL || (destlen == 0))  return mCurrentTokenLen;
    if (mCurrentTokenStart < 0) return 0;

    dest[0] = 0;

    if (destlen < mCurrentTokenLen)
    {
        memcpy(dest, &mData[mCurrentTokenStart], destlen - 1);
        dest[destlen - 1] = 0;
    }
    else
    {
        memcpy(dest, &mData[mCurrentTokenStart], mCurrentTokenLen);
        dest[mCurrentTokenLen] = 0;
    }     

    return mCurrentTokenLen;
}



//*****************************************************
//
// Class TextDataParser
// Dup Current Token
//
char *TextDataParser::DupCurrentToken(void)
{

    if (mData == NULL) return NULL;
    if (Eof()) return NULL;
    if (mCurrentTokenLen == 0) return NULL;
    if (mCurrentTokenStart < 0) return NULL;

    char *s = new char[mCurrentTokenLen + 1];
        
    memcpy(s, &mData[mCurrentTokenStart], mCurrentTokenLen);
    s[mCurrentTokenLen] = 0;

    return s;
}

//******************************************************
//
// Class TextDataParser
// Token count at Current bracket level
//
//
int TextDataParser::TokenCountAtCurrentBracketLevel(const char *intoken)
{

    if (!PushPosition()) return 0;

    int count = 0;

    char token[256];
    int start_bracket_level = mBracketLevel;   

    while (mBracketLevel >= start_bracket_level)
    {
        if (Eof()) break;

        Advance();
        CurrentToken(token, sizeof(token));
        
        if (mBracketLevel != start_bracket_level) continue;

        if (!strcmp(intoken, token)) ++count;
    }

    PopPosition();
    return count;
}


//******************************************************
//
// Class TextDataParser
// Skip Bracketed section
// If current token =='{' will skip ahead to the matching close bracket
//
bool TextDataParser::SkipBracketedSection(void)
{
    char token[256];

    CurrentToken(token, sizeof(token));
    if (token[0] != '{') return false;

    int start_bracket_level = mBracketLevel;   

    while (mBracketLevel >= start_bracket_level)
    {
        if (Eof()) return false;

        Advance();
    }

    CurrentToken(token, sizeof(token));
    if (token[0] != '}') return false;
    return true;
}





