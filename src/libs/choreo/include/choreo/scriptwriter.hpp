/*
 * choreo/scriptwriter.hpp
 */

#ifndef CHOREO_SCRIPTWRITER_HPP
#define CHOREO_SCRIPTWRITER_HPP


#include <choreo/buildconfig.hpp>


// only compile in writing if CHOREO_TOOL is defined
#ifdef CHOREO_TOOL


#include <p3d/refcounted.hpp>

#include <stdio.h>


#ifdef CHOREO_USE_FILE_FTT
struct IRadFile;
#endif


namespace choreo
{


//---------------------------------------------------------------------------
// class FileWriter
//   - abstract file writer interface
//---------------------------------------------------------------------------

class FileWriter: public tRefCounted
{
public:

    virtual void WriteString(const char* str) = 0;
};


//---------------------------------------------------------------------------
// class BufferFileWriter
//   - writes to a buffer maintained by instance
//---------------------------------------------------------------------------

class BufferFileWriter: public FileWriter
{
public:

    BufferFileWriter();

    int GetBufferLength() const
        { return m_BufferLength; }
    int GetBufferSize() const
        { return m_BufferLength + 1; }
    const char* GetBuffer() const
        { return m_Buffer; }

    void Clear();

    virtual void WriteString(const char* str);

protected:

    virtual ~BufferFileWriter();

private:

    int m_MaxBufferSize;
    int m_BufferLength;
    char* m_Buffer;
};


//---------------------------------------------------------------------------
// class CStreamFileWriter
//   - file writer interface to ANSI C file streams
//---------------------------------------------------------------------------

class CStreamFileWriter: public FileWriter
{
public:

    CStreamFileWriter(FILE* file);

    virtual void WriteString(const char* str);

protected:

    virtual ~CStreamFileWriter();

private:

    FILE* m_File;
};


#ifdef CHOREO_USE_FILE_FTT
//---------------------------------------------------------------------------
// class FTTFileWriter
//   - file writer interface to FTT's IRadFile
//---------------------------------------------------------------------------

class FTTFileWriter: public FileWriter
{
public:

    FTTFileWriter(IRadFile* fttFile);

    virtual void WriteString(const char* str);

protected:

    virtual ~FTTFileWriter();

private:

    IRadFile* m_File;
};
#endif  // CHOREO_USE_FILE_FTT


//---------------------------------------------------------------------------
// class ScriptWriter
//   - high-level script writer interface used by script writers
//---------------------------------------------------------------------------

class ScriptWriter: public tRefCounted
{
public:

    enum
    {
        DEFAULT_INDENT_SIZE = 4
    };

    ScriptWriter(FileWriter* fileWriter,
                 int indentSize = DEFAULT_INDENT_SIZE,
                 bool useTabs = false);
    
    // struct declarators
    void BeginStruct(const char* identifier);
    void EndStruct();
    
    // required param declarators
    void RequiredParamFloat(const char* identifier, float floatVal);
    void RequiredParamString(const char* identifier, const char* stringVal);
    
    // param (local) declarators
    void ParamFloat(const char* identifier, float floatVal);
    void ParamString(const char* identifier, const char* stringVal);

    // comments
    void Comment(const char* commentStr);
    void SectionComment(const char* commentStr);

protected:
    
    virtual ~ScriptWriter();
    
private:

    void Indent();
    
    // stream
    FileWriter* m_FileWriter;

    // formatting
    int m_IndentSize;
    bool m_UseTabs;

    // indentation
    int m_Indent;
};


} // namespace choreo


#endif // CHOREO_TOOL


#endif
