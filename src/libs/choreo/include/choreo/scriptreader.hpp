/*
 * choreo/scriptreader.hpp
 */

#ifndef CHOREO_SCRIPTREADER_HPP
#define CHOREO_SCRIPTREADER_HPP


#include <choreo/buildconfig.hpp>
#include <p3d/refcounted.hpp>


#ifdef CHOREO_USE_FILE_P3D
class tFile;
#endif

#ifdef CHOREO_USE_FILE_FTT
struct IRadFile;
#endif


namespace choreo
{


//---------------------------------------------------------------------------
// class FileReader
//   - abstract file reader interface
//---------------------------------------------------------------------------

class FileReader: public tRefCounted
{
public:

    virtual const char* GetFilename() const = 0;

    virtual unsigned GetPosition() const = 0;
    virtual void SetPosition(unsigned pos) = 0;

    virtual char ReadChar() = 0;

    virtual bool IsEndOfFile() const = 0;
};


//---------------------------------------------------------------------------
// class StringFileReader
//   - file reader interface to a c-style string
//---------------------------------------------------------------------------

class StringFileReader: public FileReader
{
public:

    StringFileReader(const char* string, const char* fileName = 0);

    virtual const char* GetFilename() const
        { return m_Filename; }

    virtual unsigned GetPosition() const
        { return m_Position; }
    virtual void SetPosition(unsigned pos);

    virtual char ReadChar();

    virtual bool IsEndOfFile() const;

protected:

    virtual ~StringFileReader();

private:

    const char* m_String;
    const char* m_Filename;
    unsigned m_Length;
    unsigned m_Position;
};


#ifdef CHOREO_USE_FILE_P3D
//---------------------------------------------------------------------------
// class P3DFileReader
//   - file reader interface to Pure3D's tFile
//---------------------------------------------------------------------------

class P3DFileReader: public FileReader
{
public:

    P3DFileReader(tFile* p3dFile);

    virtual const char* GetFilename() const;

    virtual unsigned GetPosition() const;
    virtual void SetPosition(unsigned pos);

    virtual char ReadChar();

    virtual bool IsEndOfFile() const;

protected:

    virtual ~P3DFileReader();

private:

    tFile* m_File;
};
#endif  // CHOREO_USE_FILE_P3D


#ifdef CHOREO_USE_FILE_FTT
//---------------------------------------------------------------------------
// class FTTFileReader
//   - file reader interface to FTT's IRadFile
//---------------------------------------------------------------------------

class FTTFileReader: public FileReader
{
public:

    FTTFileReader(IRadFile* fttFile);

    virtual const char* GetFilename() const;

    virtual unsigned GetPosition() const;
    virtual void SetPosition(unsigned pos);

    virtual char ReadChar();

    virtual bool IsEndOfFile() const;

protected:

    virtual ~FTTFileReader();

private:

    IRadFile* m_File;
};
#endif  // CHOREO_USE_FILE_FTT


//---------------------------------------------------------------------------
// class ScriptReader
//   - high-level script reader interface used by script loaders
//---------------------------------------------------------------------------

class ScriptReader: public tRefCounted
{
public:

    enum
    {
        TOKEN_SIZE = 256,
        ERROR_SIZE = 256,
        
        MAX_DEFAULT_FLOAT_COUNT = 128,
        MAX_DEFAULT_STRING_COUNT = 128,
        
        MAX_FLOAT_COUNT = 64,
        MAX_STRING_COUNT = 64
    };
    
    ScriptReader(FileReader* fileReader, int lineNo = 1);
    
    // line number accessors
    int GetLineNumber() const
        { return m_LineNumber; }
    
    // struct iterators
    bool IsEndOfFile() const;
    int GetStructCount(const char* identifier);
    bool IsMoreStructs();
    const char* GetNextStructIdentifier() const;
    
    // struct declarators
    void BeginStruct(const char* identifier);
    void EndStruct();
    
    // required param declarators
    void RequiredParamFloat(const char* identifier, float* floatVal, int* lineNo = 0);
    void RequiredParamString(const char* identifier, char* stringVal, int stringLen, int* lineNo = 0);
    
    // param (local) declarators
    void ParamFloat(const char* identifier, float* floatVal, int* lineNo = 0);
    void ParamString(const char* identifier, char* stringVal, int stringLen, int* lineNo = 0);
    
    // errors
    bool IsError() const
        { return m_IsError; }
    const char* GetErrorString() const
        { return m_ErrorString; }
    int GetErrorLine() const
        { return m_ErrorLine; }
    void ClearError();
    void SetErrorString(const char* errorStr);
    void SetErrorFormat(const char* errorFmt, ...);
    void SetErrorString(int lineNo, const char* errorStr);
    void SetErrorFormat(int lineNo, const char* errorFmt, ...);
    void GetErrorMessage(char* errorStr, int n);
    
    // shows error and clears error flag
    // return true if there was no error
    bool HandleError();

    // error handler registration
    typedef bool (*ErrorHandler)(ScriptReader* script, void* context);
    static void InstallErrorHandler(ErrorHandler errorHandler, void* context);
    static ErrorHandler GetErrorHandler()
        { return m_ErrorHandler; }
    static void* GetErrorHandlerContext()
        { return m_ErrorHandlerContext; }
    
protected:
    
    virtual ~ScriptReader();
    
private:
    
    enum TokenType
    {
        TOKEN_NONE,
        TOKEN_ERROR,
        TOKEN_IDENTIFIER,
        TOKEN_FLOAT,
        TOKEN_STRING,
        TOKEN_BRACE_OPEN,
        TOKEN_BRACE_CLOSE,
        TOKEN_DEFAULT,
        TOKEN_DOT,
        TOKEN_SEMICOLON,
        TOKEN_EOF
    };
    
    // token advancement
    static TokenType LoadToken(FileReader* file, int& lineNo, char* token, int n);
    void NextToken();
    
    // defaults
    int FindDefaultFloat(const char* identifier) const;
    int FindDefaultString(const char* identifier) const;
    void LoadDefault();
    
    // local params
    int FindLocalFloat(const char* token) const;
    bool LoadLocalFloat();
    int FindLocalString(const char* token) const;
    bool LoadLocalString();
    bool LoadLocal();
    
    // struct advancement
    void LoadUntilStruct();

    // default error handler
    static bool DefaultErrorHandler(ScriptReader* script, void* context);

    // error handler
    static ErrorHandler m_ErrorHandler;
    static void* m_ErrorHandlerContext;
    
    // stream
    FileReader* m_FileReader;
    int m_LineNumber;
    
    // error
    bool m_IsError;
    char m_ErrorString[ERROR_SIZE];
    int m_ErrorLine;
    
    // token
    TokenType m_TokenType;
    char m_Token[TOKEN_SIZE];
    
    // scope
    int m_Depth;
    char m_Scope[TOKEN_SIZE];
    bool m_IsInScope;
    
    // default floats
    struct DefaultFloatStruct
    {
        int depth;
        char scope[TOKEN_SIZE];
        float floatValue;
        int lineNo;
    };
    
    int m_DefaultFloatCount;
    DefaultFloatStruct m_DefaultFloats[MAX_DEFAULT_FLOAT_COUNT];
    
    // default strings
    struct DefaultStringStruct
    {
        int depth;
        char scope[TOKEN_SIZE];
        char stringValue[TOKEN_SIZE];
        int lineNo;
    };
    
    int m_DefaultStringCount;
    DefaultStringStruct m_DefaultStrings[MAX_DEFAULT_STRING_COUNT];
    
    // local floats
    struct FloatStruct
    {
        int depth;
        char identifier[TOKEN_SIZE];
        float* floatValue;
        int* lineNo;
        bool used;
    };
    
    int m_FloatCount;
    FloatStruct m_Floats[MAX_FLOAT_COUNT];
    
    // local strings
    struct StringStruct
    {
        int depth;
        char identifier[TOKEN_SIZE];
        char* stringValue;
        int stringLength;
        int* lineNo;
        bool used;
    };
    
    int m_StringCount;
    StringStruct m_Strings[MAX_STRING_COUNT];
};


} // namespace choreo


#endif
