/*
 * choreo/scriptreader.cpp
 */

#include <choreo/scriptreader.hpp>
#include <choreo/utility.hpp>

#ifdef CHOREO_USE_FILE_P3D
#include <p3d/file.hpp>
#endif

#include <p3d/error.hpp>

#ifdef CHOREO_USE_FILE_FTT
#include <radfile.hpp>
#endif

#ifdef RAD_WIN32
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#endif

#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>


namespace choreo
{


//---------------------------------------------------------------------------
// class StringFileReader
//---------------------------------------------------------------------------

StringFileReader::StringFileReader(const char* string, const char* fileName):
    m_String(string),
    m_Filename(fileName),
    m_Position(0)
{
    m_Length = strlen(m_String);
}

StringFileReader::~StringFileReader()
{
}

void StringFileReader::SetPosition(unsigned pos)
{
    m_Position = pos;

    if (m_Position > m_Length)
    {
        m_Position = m_Length;
    }
}

char StringFileReader::ReadChar()
{
    P3DASSERT(m_Position < m_Length);
    if (m_Position >= m_Length)
        return 0;

    char c = m_String[m_Position];
    ++m_Position;
    return c;
}

bool StringFileReader::IsEndOfFile() const
{
    return (m_Position >= m_Length);
}


#ifdef CHOREO_USE_FILE_P3D
//----------------------------------------------------------------------
// class P3DFileReader
//----------------------------------------------------------------------

P3DFileReader::P3DFileReader(tFile* p3dFile)
{
    P3DASSERT(p3dFile != 0);
    m_File = p3dFile;
    m_File->AddRef();
}

P3DFileReader::~P3DFileReader()
{
    m_File->Release();
}

const char* P3DFileReader::GetFilename() const
{
    return m_File->GetFullFilename();
}

unsigned P3DFileReader::GetPosition() const
{
    return m_File->GetPosition();
}

void P3DFileReader::SetPosition(unsigned pos)
{
    m_File->SetPosition(pos);
}

char P3DFileReader::ReadChar()
{
    return m_File->GetByte();
}

bool P3DFileReader::IsEndOfFile() const
{
    return m_File->EndOfFile();
}

#endif // CHOREO_USE_FILE_P3D


#ifdef CHOREO_USE_FILE_FTT

//----------------------------------------------------------------------
// class FTTFileReader
//----------------------------------------------------------------------

FTTFileReader::FTTFileReader(IRadFile* fttFile)
{
    P3DASSERT(fttFile != 0);
    m_File = fttFile;
    m_File->AddRef();
}

FTTFileReader::~FTTFileReader()
{
    m_File->Release();
}

const char* FTTFileReader::GetFilename() const
{
    return m_File->GetFilename();
}

unsigned FTTFileReader::GetPosition() const
{
    // make sure we've finished any pending read/setpos requests
    m_File->WaitForCompletion();

    unsigned pos;
    m_File->GetPositionAsync(&pos);

    // make sure pos is filled before returning
    m_File->WaitForCompletion();

    return pos;
}

void FTTFileReader::SetPosition(unsigned pos)
{
    m_File->SetPositionAsync(pos);
}

char FTTFileReader::ReadChar()
{
    char data;
    m_File->ReadAsync(&data, sizeof(data));

    // make sure data is filled before returning
    m_File->WaitForCompletion();

    return data;
}

bool FTTFileReader::IsEndOfFile() const
{
    // make sure we've finished any pending read/setpos requests
    m_File->WaitForCompletion();

    unsigned filePos;
    m_File->GetPositionAsync(&filePos);

    unsigned fileSize;
    m_File->GetSizeAsync(&fileSize);

    // complete requests
    m_File->WaitForCompletion();

    return (filePos >= fileSize);
}

#endif // CHOREO_USE_FILE_FTT


//----------------------------------------------------------------------
// class ScriptReader
//----------------------------------------------------------------------

// constructors, destructors
//----------------------------------------------------------------------

ScriptReader::ScriptReader(FileReader* fileReader,
                           int lineNo):
    m_LineNumber(lineNo),

    m_IsError(false),
    m_ErrorLine(-1),

    m_Depth(0),
    m_IsInScope(true),

    m_DefaultFloatCount(0),
    m_DefaultStringCount(0),

    m_FloatCount(0),
    m_StringCount(0)
{
    P3DASSERT(fileReader != 0);
    m_FileReader = fileReader;
    m_FileReader->AddRef();

    m_ErrorString[0] = 0;
    m_Scope[0] = 0;

    NextToken();
}

ScriptReader::~ScriptReader()
{
    m_FileReader->Release();
}


// token stream advancement
//----------------------------------------------------------------------

ScriptReader::TokenType ScriptReader::LoadToken(FileReader* file,
                                                int& lineNo,
                                                char* token,
                                                int n)
{
    if (file->IsEndOfFile())
    {
        StrSet(token, n, "<EOF>");
        return TOKEN_EOF;
    }
    
    char c = file->ReadChar();

    // chew up whitespace & comments
    while (1)
    {
        // chew up whitespace
        while (isspace(c))
        {
            if (c == '\n')
            {
                ++lineNo;
            }
            
            if (file->IsEndOfFile())
            {
                StrSet(token, n, "<EOF>");
                return TOKEN_EOF;
            }
            
            c = file->ReadChar();
        }
        
        // chew up comments
        if (c == '/')
        {
            if (file->IsEndOfFile())
            {
                StrSet(token, n, "<EOF>");
                return TOKEN_EOF;
            }
            
            c = file->ReadChar();
            
            if (c != '/')
            {
                // syntax error, not a comment
                char workStr[2];
                workStr[0] = c;
                workStr[1] = 0;
                StrSet(token, n, workStr);
                return TOKEN_ERROR;
            }
            
            if (file->IsEndOfFile())
            {
                StrSet(token, n, "<EOF>");
                return TOKEN_EOF;
            }
            
            c = file->ReadChar();
            
            while (1)
            {
                if (c == '\n')
                {
                    break;
                }
                
                if (file->IsEndOfFile())
                {
                    StrSet(token, n, "<EOF>");
                    return TOKEN_EOF;
                }
                
                c = file->ReadChar();
            }
        }
        else
        {
            break;
        }
    }
    
    char workStr[256];
    int workIndex = 0;
    
    if (isalpha(c) ||
        (c == '_'))
    {
        // tokenize identifier
        workStr[0] = c;
        workIndex = 1;
        
        if (file->IsEndOfFile())
        {
            workStr[1] = 0;
            StrSet(token, n, workStr);
            return TOKEN_IDENTIFIER;
        }
        
        c = file->ReadChar();
        
        while (isalnum(c) ||
               (c == '_'))
        {
            if (workIndex >= 255)
            {
                StrSet(token, n, "<ERROR_OUT_OF_MEMORY>");
                return TOKEN_ERROR;
            }
            
            workStr[workIndex] = c;
            ++workIndex;
            
            if (file->IsEndOfFile())
            {
                break;
            }
            
            c = file->ReadChar();
        }
        
        // back up lookahead
        if (!(isalnum(c) ||
            (c == '_')))
        {
            file->SetPosition(file->GetPosition() - 1);
        }
        
        workStr[workIndex] = 0;
        StrSet(token, n, workStr);
        
        if (strcmp(workStr, "default") == 0)
        {
            // default keyword
            return TOKEN_DEFAULT;
        }
        else
        {
            return TOKEN_IDENTIFIER;
        }
    }
    else if (c == '"')
    {
        // tokenize string
        if (file->IsEndOfFile())
        {
            StrSet(token, n, "<ERROR_UNTERMINATED_STRING>");
            return TOKEN_ERROR;
        }
        
        c = file->ReadChar();
        
        while (c != '"')
        {
            if (c == '\n')
            {
                StrSet(token, n, "<ERROR_NEWLINE_IN_STRING>");
                return TOKEN_ERROR;
            }
            
            if (c == '\\')
            {
                if (file->IsEndOfFile())
                {
                    StrSet(token, n, "<ERROR_UNTERMINATED_STRING>");
                    return TOKEN_ERROR;
                }
                
                c = file->ReadChar();
            }
            
            if (workIndex >= 255)
            {
                StrSet(token, n, "<ERROR_OUT_OF_MEMORY>");
                return TOKEN_ERROR;
            }
            
            workStr[workIndex] = c;
            ++workIndex;
            
            if (file->IsEndOfFile())
            {
                StrSet(token, n, "<ERROR_UNTERMINATED_STRING>");
                return TOKEN_ERROR;
            }
            
            c = file->ReadChar();
        }
        
        workStr[workIndex] = 0;
        StrSet(token, n, workStr);
        return TOKEN_STRING;
    }
    else if (c == '.')
    {
        if (file->IsEndOfFile())
        {
            // tokenize dot operator
            StrSet(token, n, ".");
            return TOKEN_DOT;
        }
        
        c = file->ReadChar();
        
        if (isdigit(c))
        {
            // tokenize float
            workStr[0] = '.';
            workStr[1] = c;
            workIndex = 2;
            
            while (isdigit(c) ||
                   (c == 'e') ||
                   (c == 'E') ||
                   (c == '+') ||
                   (c == '-'))
            {
                if (workIndex >= 255)
                {
                    StrSet(token, n, "<ERROR_OUT_OF_MEMORY>");
                    return TOKEN_ERROR;
                }
                
                workStr[workIndex] = c;
                ++workIndex;
                
                if (file->IsEndOfFile())
                {
                    break;
                }
                
                c = file->ReadChar();
            }
            
            // back up lookahead
            if (!(isdigit(c) ||
                  (c == 'e') ||
                  (c == 'E') ||
                  (c == '+') ||
                  (c == '-')))
            {
                file->SetPosition(file->GetPosition() - 1);
            }
            
            workStr[workIndex] = 0;
            StrSet(token, n, workStr);
            return TOKEN_FLOAT;
        }
        else
        {
            // back up lookahead
            file->SetPosition(file->GetPosition() - 1);
            
            // tokenize dot operator
            StrSet(token, n, ".");
            return TOKEN_DOT;
        }
    }
    else if (isdigit(c) ||
             (c == '+') ||
             (c == '-'))
    {
        // tokenize float
        workStr[0] = c;
        workIndex = 1;
        
        if (file->IsEndOfFile())
        {
            // tokenize float
            workStr[1] = 0;
            StrSet(token, n, workStr);
            
            if ((c == '+') || (c == '-'))
            {
                // no digits!
                return TOKEN_ERROR;
            }
            else
            {
                return TOKEN_FLOAT;
            }
        }
        
        c = file->ReadChar();
        
        if (((workStr[0] == '+') || (workStr[0] == '-')) &&
            (!(isdigit(c) || (c == '.'))))
        {
            // no digits!
            file->SetPosition(file->GetPosition() - 1);
            
            workStr[1] = 0;
            StrSet(token, n, workStr);
            
            return TOKEN_ERROR;
        }
        
        while (isdigit(c) ||
               (c == 'e') ||
               (c == 'E') ||
               (c == '+') ||
               (c == '-') ||
               (c == '.'))
        {
            if (workIndex >= 255)
            {
                StrSet(token, n, "<ERROR_OUT_OF_MEMORY>");
                return TOKEN_ERROR;
            }
            
            workStr[workIndex] = c;
            ++workIndex;
            
            if (file->IsEndOfFile())
            {
                break;
            }
            
            c = file->ReadChar();
        }
        
        // back up lookahead
        if (!(isdigit(c) ||
              (c == 'e') ||
              (c == 'E') ||
              (c == '+') ||
              (c == '-') ||
              (c == '.')))
        {
            file->SetPosition(file->GetPosition() - 1);
        }
        
        workStr[workIndex] = 0;
        StrSet(token, n, workStr);
        return TOKEN_FLOAT;
    }
    else if (c == '{')
    {
        // tokenize opening brace
        StrSet(token, n, "{");
        return TOKEN_BRACE_OPEN;
    }
    else if (c == '}')
    {
        // tokenize closing brace
        StrSet(token, n, "}");
        return TOKEN_BRACE_CLOSE;
    }
    else if (c == ';')
    {
        // tokenize semicolon
        StrSet(token, n, ";");
        return TOKEN_SEMICOLON;
    }
    else
    {
        // tokenize syntax errors
        workStr[0] = c;
        workStr[1] = 0;
        StrSet(token, n, workStr);
        return TOKEN_ERROR;
    }
}

void ScriptReader::NextToken()
{
    m_TokenType = LoadToken(m_FileReader, m_LineNumber, m_Token, sizeof(m_Token));
}


// error handling
//----------------------------------------------------------------------

void ScriptReader::ClearError()
{
    m_ErrorString[0] = 0;
    m_IsError = false;
    m_ErrorLine = -1;
    m_IsInScope = true;
}

void ScriptReader::SetErrorString(const char* errorStr)
{
    if (m_IsError)
        return;
    
    StrSet(m_ErrorString, sizeof(m_ErrorString), errorStr);
    m_IsError = true;
    m_ErrorLine = m_LineNumber;
}

void ScriptReader::SetErrorFormat(const char* errorFmt, ...)
{
    if (m_IsError)
        return;
    
    va_list ap;
    va_start(ap, errorFmt);
    
    VStrPrintf(m_ErrorString, sizeof(m_ErrorString), errorFmt, ap);

    va_end(ap);
    
    m_IsError = true;
    m_ErrorLine = m_LineNumber;
}

void ScriptReader::SetErrorString(int lineNo, const char* errorStr)
{
    if (m_IsError)
        return;
    
    StrSet(m_ErrorString, sizeof(m_ErrorString), errorStr);
    m_IsError = true;
    m_ErrorLine = lineNo;
}

void ScriptReader::SetErrorFormat(int lineNo, const char* errorFmt, ...)
{
    if (m_IsError)
        return;
    
    va_list ap;
    va_start(ap, errorFmt);
    
    VStrPrintf(m_ErrorString, sizeof(m_ErrorString), errorFmt, ap);
    
    m_IsError = true;
    m_ErrorLine = lineNo;
}

void ScriptReader::GetErrorMessage(char* errorStr, int n)
{
    StrPrintf(errorStr, n, "choreo::ScriptReader ERROR\n%s(%d):  %s\n",
              m_FileReader->GetFilename(),
              m_ErrorLine,
              m_ErrorString);
}

bool ScriptReader::HandleError()
{
    if (!m_IsError)
        return true;

    if (m_ErrorHandler != 0)
    {
        bool rc = (*m_ErrorHandler)(this, m_ErrorHandlerContext);
        ClearError();
        return rc;
    }
    else
    {
        return false;
    }
}

// Halt the machine such that a it will break into the debugger if present
static void choreoBreakToDebugger()
{
#if defined(RAD_WIN32) || defined(RAD_UWP)
    __debugbreak();
#else
   while(1){/* nop */}; // if we don't have a trap instruction, just loop so that the debugger can connect
#endif
}

static void choreoMessageBox(const char* message)
{
#ifdef RAD_WIN32
    static char buffer[1024];
    
    StrPrintf(buffer, sizeof(buffer), "%s\nClick RETRY to debug", message);

    int retVal = MessageBox(NULL, buffer, "Script Error", MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION);

    if (retVal == IDABORT)
    {
        exit(-1);
    }
    else if (retVal == IDRETRY)
    {
        choreoBreakToDebugger();
    }
#else
    choreoBreakToDebugger();
#endif
}

// print a message to the debugging console
static void choreoPrintDebugMessage(const char* message)
{
#if defined(RAD_WIN32)
   OutputDebugString(message);
#else
   printf(message);
#endif
}

bool ScriptReader::DefaultErrorHandler(ScriptReader* script, void* context)
{
    char errorMsg[1024];
    script->GetErrorMessage(errorMsg, sizeof(errorMsg));

    choreoPrintDebugMessage(errorMsg);
    choreoMessageBox(errorMsg);

    return true;
}

ScriptReader::ErrorHandler ScriptReader::m_ErrorHandler = ScriptReader::DefaultErrorHandler;
void* ScriptReader::m_ErrorHandlerContext = 0;

void ScriptReader::InstallErrorHandler(ErrorHandler errorHandler, void* context)
{
    m_ErrorHandler = errorHandler;
    m_ErrorHandlerContext = context;
}


// default finding and loading
//----------------------------------------------------------------------

int ScriptReader::FindDefaultFloat(const char* identifier) const
{
    for (int i = (m_DefaultFloatCount - 1); i >= 0; --i)
    {
        if (strcmp(m_DefaultFloats[i].scope, identifier) == 0)
        {
            return i;
        }
    }
    
    return -1;
}

int ScriptReader::FindDefaultString(const char* identifier) const
{
    for (int i = (m_DefaultStringCount - 1); i >= 0; --i)
    {
        if (strcmp(m_DefaultStrings[i].scope, identifier) == 0)
        {
            return i;
        }
    }
    
    return -1;
}

void ScriptReader::LoadDefault()
{
    if (m_IsError)
        return;
    
    NextToken();
    
    if (m_TokenType != TOKEN_IDENTIFIER)
    {
        SetErrorFormat("Unexpected token \"%s\", expected identifier.", m_Token);
        return;
    }
    
    char scope[TOKEN_SIZE];
    StrSet(scope, sizeof(scope), m_Scope);
    
    if (strlen(scope) > 0)
    {
        StrCat(scope, sizeof(scope), ".");
    }
    StrCat(scope, sizeof(scope), m_Token);
    
    NextToken();
    while (m_TokenType == TOKEN_DOT)
    {
        // skip dot
        NextToken();
        
        // check to make sure we get an identifier
        if (m_TokenType != TOKEN_IDENTIFIER)
        {
            SetErrorFormat("Unexpected token \"%s\", expected identifier.", m_Token);
            return;
        }
        
        StrCat(scope, sizeof(scope), ".");
        StrCat(scope, sizeof(scope), m_Token);
        
        // skip identifier
        NextToken();
    }
    
    switch (m_TokenType)
    {
        case TOKEN_FLOAT:
        {
            int index = FindDefaultString(scope);
            if (index >= 0)
            {
                SetErrorFormat("Inconsistent type for default \"%s\"; see previous declaration on line %d.",
                    scope, m_DefaultStrings[index].lineNo);
                return;
            }
            
            index = FindDefaultFloat(scope);
            if (index < 0)
            {
                if (m_DefaultFloatCount >= MAX_DEFAULT_FLOAT_COUNT)
                {
                    SetErrorString("Ran out of space for default floats!");
                    return;
                }
                
                index = m_DefaultFloatCount;
                ++m_DefaultFloatCount;
            }
            
            DefaultFloatStruct* defaultFloat = (m_DefaultFloats + index);
            defaultFloat->depth = m_Depth;
            StrSet(defaultFloat->scope, sizeof(defaultFloat->scope), scope);
            defaultFloat->floatValue = (float)atof(m_Token);
            defaultFloat->lineNo = m_LineNumber;
            
            break;
        }
        
        case TOKEN_STRING:
        {
            int index = FindDefaultFloat(scope);
            if (index >= 0)
            {
                SetErrorFormat("Inconsistent type for default \"%s\"; see previous declaration on line %d.",
                    scope, m_DefaultFloats[index].lineNo);
                return;
            }
            
            index = FindDefaultString(scope);
            if (index < 0)
            {
                if (m_DefaultStringCount >= MAX_DEFAULT_STRING_COUNT)
                {
                    SetErrorString("Ran out of space for default strings!");
                    return;
                }
                
                index = m_DefaultStringCount;
                ++m_DefaultStringCount;
            }
            
            DefaultStringStruct* defaultStr = (m_DefaultStrings + index);
            defaultStr->depth = m_Depth;
            StrSet(defaultStr->scope, sizeof(defaultStr->scope), scope);
            StrSet(defaultStr->stringValue, sizeof(defaultStr->stringValue), m_Token);
            defaultStr->lineNo = m_LineNumber;
            
            break;
        }
        
        default:
            SetErrorFormat("Unexpected token \"%s\", expected value.", m_Token);
            return;
    }
    
    // skip value
    NextToken();
    
    if (m_TokenType != TOKEN_SEMICOLON)
    {
        SetErrorFormat("Unexpected token \"%s\", expected semicolon", m_Token);
        return;
    }
    
    // skip semicolon
    NextToken();
}


// local finding and loading
//----------------------------------------------------------------------

int ScriptReader::FindLocalFloat(const char* token) const
{
    for (int i = (m_FloatCount - 1); i >= 0; --i)
    {
        if (m_Floats[i].depth != m_Depth)
            return -1;
        if (strcmp(m_Floats[i].identifier, token) == 0)
            return i;
    }
    
    return -1;
}

bool ScriptReader::LoadLocalFloat()
{
    if (m_IsError)
        return false;
    
    int index = FindLocalFloat(m_Token);
    if (index < 0)
        return false;
    
    if (m_Floats[index].used)
    {
        SetErrorFormat("Float param \"%s\" already set to %g.",
                       m_Floats[index].identifier,
                       *(m_Floats[index].floatValue));
        return false;
    }
    
    NextToken();
    if (m_TokenType != TOKEN_FLOAT)
    {
        SetErrorFormat("Unexpected token \"%s\", expected float.", m_Token);
        return false;
    }
    *(m_Floats[index].floatValue) = (float)atof(m_Token);
    if (m_Floats[index].lineNo != 0)
    {
        *(m_Floats[index].lineNo) = m_LineNumber;
    }
    m_Floats[index].used = true;
    
    NextToken();
    if (m_TokenType != TOKEN_SEMICOLON)
    {
        SetErrorFormat("Unexpected token \"%s\", expected semicolon.", m_Token);
        return false;
    }
    
    NextToken();
    return true;
}

int ScriptReader::FindLocalString(const char* token) const
{
    for (int i = (m_StringCount - 1); i >= 0; --i)
    {
        if (m_Strings[i].depth != m_Depth)
            return -1;
        if (strcmp(m_Strings[i].identifier, token) == 0)
            return i;
    }
    
    return -1;
}

bool ScriptReader::LoadLocalString()
{
    if (m_IsError)
        return false;
    
    int index = FindLocalString(m_Token);
    if (index < 0)
        return false;
    
    if (m_Strings[index].used)
    {
        SetErrorFormat("String param \"%s\" already set to \"%s\".",
                       m_Strings[index].identifier,
                       m_Strings[index].stringValue);
        return false;
    }
    
    NextToken();
    if (m_TokenType != TOKEN_STRING)
    {
        SetErrorFormat("Unexpected token \"%s\", expected string.", m_Token);
        return false;
    }
    StrSet(m_Strings[index].stringValue, m_Strings[index].stringLength, m_Token);
    if (m_Strings[index].lineNo != 0)
    {
        *(m_Strings[index].lineNo) = m_LineNumber;
    }
    m_Strings[index].used = true;
    
    NextToken();
    if (m_TokenType != TOKEN_SEMICOLON)
    {
        SetErrorFormat("Unexpected token \"%s\", expected semicolon.", m_Token);
        return false;
    }
    
    NextToken();
    return true;
}

bool ScriptReader::LoadLocal()
{
    if (m_IsError)
        return false;
    
    if (LoadLocalString())
        return true;
    if (LoadLocalFloat())
        return true;
    
    return false;
}


// common loading tasks
//----------------------------------------------------------------------

void ScriptReader::LoadUntilStruct()
{
    if (m_IsError)
        return;
    
    if (!m_IsInScope)
    {
        switch (m_TokenType)
        {
            case TOKEN_SEMICOLON:
                return;
        
            case TOKEN_BRACE_OPEN:
                m_IsInScope = true;
                NextToken();
                break;
            
            default:
                SetErrorFormat("Unexpected token \"%s\", expected opening brace or semicolon.", m_Token);
                return;
        }
    }

    while (1)
    {
        if (m_IsError)
            return;
        
        switch (m_TokenType)
        {
            case TOKEN_IDENTIFIER:
                if (!LoadLocal())
                    return;
                break;
            
            case TOKEN_DEFAULT:
                LoadDefault();
                break;
            
            case TOKEN_BRACE_CLOSE:
                // end-of-scope
                return;
            
            default:
                SetErrorFormat("Unexpected token \"%s\".", m_Token);
                return;
        }
    }
}


// struct iteration
//----------------------------------------------------------------------

bool ScriptReader::IsEndOfFile() const
{
    return (m_TokenType == TOKEN_EOF);
}

int ScriptReader::GetStructCount(const char* identifier)
{
    if (m_IsError)
        return 0;
    
    // store current stream position
    unsigned tmpPos = m_FileReader->GetPosition();
    int tmpLine = m_LineNumber;
    char tmpToken[TOKEN_SIZE];
    StrSet(tmpToken, sizeof(tmpToken), m_Token);
    TokenType tmpTokenType = m_TokenType;
    
    int braceCount;
    
    if (!m_IsInScope)
    {
        while (1)
        {
            if (m_TokenType == TOKEN_SEMICOLON)
            {
                braceCount = 0;
                break;
            }
            
            if (m_TokenType == TOKEN_BRACE_OPEN)
            {
                braceCount = 1;
                NextToken();
                break;
            }
            
            NextToken();
        }
    }
    else
    {
        braceCount = 1;
    }
    
    int structCount = 0;
    
    // loop through tokens for this scope
    while (braceCount > 0)
    {
        switch (m_TokenType)
        {
            case TOKEN_BRACE_OPEN:
                ++braceCount;
                break;
            
            case TOKEN_BRACE_CLOSE:
                --braceCount;
                break;
            
            case TOKEN_DEFAULT:
                // need to handle this, since the searched identifier
                // might be listed in a default statement, and we don't
                // want to count it erroneously
                while ((m_TokenType != TOKEN_SEMICOLON) &&
                       (m_TokenType != TOKEN_EOF) &&
                       (m_TokenType != TOKEN_ERROR))
                {
                    NextToken();
                }
                break;
            
            case TOKEN_IDENTIFIER:
                if ((braceCount == 1) &&
                    (strcmp(m_Token, identifier) == 0))
                {
                    ++structCount;
                }
                break;
            
            case TOKEN_ERROR:
                structCount = 0;
                break;

            default:
                break;
        }
        
        if ((m_TokenType == TOKEN_EOF) ||
            (m_TokenType == TOKEN_ERROR))
            break;
        
        NextToken();
    }

    // restore stream state
    m_FileReader->SetPosition(tmpPos);
    m_LineNumber = tmpLine;
    StrSet(m_Token, sizeof(m_Token), tmpToken);
    m_TokenType = tmpTokenType;
    
    return structCount;
}

bool ScriptReader::IsMoreStructs()
{
    // make sure we're either at global scope or between braces
    LoadUntilStruct();
    if (m_IsError)
        return false;
    
    // stop when a non-local identifier is hit
    return (m_TokenType == TOKEN_IDENTIFIER);
}

const char* ScriptReader::GetNextStructIdentifier() const
{
    if (m_IsError ||
        (m_TokenType != TOKEN_IDENTIFIER))
    {
        return "";
    }
    else
    {
        return m_Token;
    }
}


// struct declarators
//----------------------------------------------------------------------

void ScriptReader::BeginStruct(const char* identifier)
{
    // make sure we're either in global scope or between braces
    LoadUntilStruct();
    if (m_IsError)
        return;
    
    // make sure found id == identifier
    if ((m_TokenType != TOKEN_IDENTIFIER) ||
        (strcmp(m_Token, identifier) != 0))
    {
        SetErrorFormat("Unexpected token \"%s\", expected \"%s\".", m_Token, identifier);
        return;
    }
    
    // push scope on stack -- reinitialize for new scope
    ++m_Depth;
    
    if (strlen(m_Scope) > 0)
    {
        StrCat(m_Scope, sizeof(m_Scope), ".");
    }
    StrCat(m_Scope, sizeof(m_Scope), identifier);
    
    m_IsInScope = false;
    NextToken();
    
    // error-check:  see if any defaults with this struct name have been applied
    int index = FindDefaultString(m_Scope);
    if (index >= 0)
    {
        SetErrorFormat(m_DefaultStrings[index].lineNo,
                       "Default specified for struct \"%s\".",
                       m_Scope);
        return;
    }
    
    index = FindDefaultFloat(m_Scope);
    if (index >= 0)
    {
        SetErrorFormat(m_DefaultFloats[index].lineNo,
                       "Default specified for struct \"%s\".",
                       m_Scope);
        return;
    }
}

void ScriptReader::EndStruct()
{
    // finish off remaining tokens in struct
    LoadUntilStruct();
    
    // finish struct
    if (m_IsError)
    {
        // if error, skip until closing brace/semicolon is reached
        if (m_IsInScope)
        {
            int braceCount = 1;
            while (braceCount > 0)
            {
                if (m_TokenType == TOKEN_BRACE_OPEN)
                {
                    ++braceCount;
                }
                else if (m_TokenType == TOKEN_BRACE_CLOSE)
                {
                    --braceCount;
                }
                else if (m_TokenType == TOKEN_EOF)
                {
                    break;
                }
                
                NextToken();
            }
        }
        
        int braceCount = 0;
        while ((m_TokenType != TOKEN_SEMICOLON) || (braceCount > 0))
        {
            if (m_TokenType == TOKEN_BRACE_OPEN)
            {
                ++braceCount;
            }
            else if (m_TokenType == TOKEN_BRACE_CLOSE)
            {
                --braceCount;
            }
            else if (m_TokenType == TOKEN_EOF)
            {
                break;
            }
            
            NextToken();
        }
    }
    else
    {
        // no error, so finish off scope
        if (m_IsInScope)
        {
            if (m_TokenType != TOKEN_BRACE_CLOSE)
            {
                SetErrorFormat("Unexpected token \"%s\", expected closing brace.", m_Token);
            }
            
            NextToken();
        }
        
        if (!m_IsError)
        {
            if (m_TokenType != TOKEN_SEMICOLON)
            {
                SetErrorFormat("Unexpected token \"%s\", expected closing brace.", m_Token);
            }
            
            NextToken();
            
            // TODO:
            // check all defaults whose depth & scope matches current and make sure they're
            // actually valid values -- otherwise ERROR
            // kinda weird, retroactive error...  not sure how we should handle it
        }
    }
    
    if (m_Depth <= 0)
    {
        if (!m_IsError)
        {
            SetErrorString("Unmatched BeginStruct()/EndStruct() pair.");
        }
    }
    else
    {
        // get rid of local default floats
        while ((m_DefaultFloatCount > 0) &&
               (m_DefaultFloats[m_DefaultFloatCount - 1].depth >= m_Depth))
        {
            --m_DefaultFloatCount;
        }
        
        // get rid of local default strings
        while ((m_DefaultStringCount > 0) &&
               (m_DefaultStrings[m_DefaultStringCount - 1].depth >= m_Depth))
        {
            --m_DefaultStringCount;
        }
        
        // get rid of local floats
        while ((m_FloatCount > 0) &&
               (m_Floats[m_FloatCount - 1].depth == m_Depth))
        {
            --m_FloatCount;
        }
        
        // get rid of local strings
        while ((m_StringCount > 0) &&
               (m_Strings[m_StringCount - 1].depth == m_Depth))
        {
            --m_StringCount;
        }
        
        // decrement depth
        --m_Depth;
        
        // decrement scope
        char* dotPtr = strrchr(m_Scope, '.');
        if (dotPtr == 0)
        {
            m_Scope[0] = 0;
        }
        else
        {
            *dotPtr = 0;
        }
        
        // reset scope flag
        m_IsInScope = true;
    }
}


// required param declarators
//----------------------------------------------------------------------

void ScriptReader::RequiredParamFloat(const char* identifier,
                                      float* floatVal,
                                      int* lineNo)
{
    // if error, ignore
    if (m_IsError)
        return;
    
    // create fully-scoped identifier
    char scopedIdentifier[TOKEN_SIZE];
    StrSet(scopedIdentifier, sizeof(scopedIdentifier), m_Scope);
    
    if (strlen(scopedIdentifier) > 0)
    {
        StrCat(scopedIdentifier, sizeof(scopedIdentifier), ".");
    }
    StrCat(scopedIdentifier, sizeof(scopedIdentifier), identifier);
    
    // check for any incorrect string defaults
    int index = FindDefaultString(scopedIdentifier);
    if (index >= 0)
    {
        SetErrorFormat(m_DefaultStrings[index].lineNo,
                       "Not allowed to specify default for required float parameter \"%s\".",
                       scopedIdentifier);
        return;
    }
    
    // check for any incorrect float defaults
    index = FindDefaultFloat(scopedIdentifier);
    if (index >= 0)
    {
        SetErrorFormat(m_DefaultFloats[index].lineNo,
                       "Not allowed to specify default for required float parameter \"%s\".",
                       scopedIdentifier);
        return;
    }
    
    // make sure we have room to append
    if (m_FloatCount >= MAX_FLOAT_COUNT)
    {
        SetErrorString("Ran out of space for required float!");
        return;
    }
    
    // set float
    FloatStruct* floatStruct = (m_Floats + m_FloatCount);
    floatStruct->depth = m_Depth;
    StrSet(floatStruct->identifier, sizeof(floatStruct->identifier), identifier);
    floatStruct->floatValue = floatVal;
    floatStruct->lineNo = lineNo;
    floatStruct->used = false;
    ++m_FloatCount;
    
    if (!m_IsInScope)
    {
        if (m_TokenType == TOKEN_FLOAT)
        {
            *floatVal = (float)atof(m_Token);
            if (floatStruct->lineNo != 0)
            {
                *(floatStruct->lineNo) = m_LineNumber;
            }
            floatStruct->used = true;
            NextToken();
            return;
        }
        
        if (m_TokenType != TOKEN_BRACE_OPEN)
        {
            SetErrorFormat("Unexpected token \"%s\", expected float or opening brace.", m_Token);
            return;
        }
        NextToken();
        
        m_IsInScope = true;
    }
    
    if ((m_TokenType != TOKEN_IDENTIFIER) ||
        (strcmp(m_Token, identifier) != 0))
    {
        SetErrorFormat("Unexpected token \"%s\", expected \"%s\".", m_Token, identifier);
        return;
    }
    NextToken();
    
    if (m_TokenType != TOKEN_FLOAT)
    {
        SetErrorFormat("Unexpected token \"%s\", expected float.", m_Token);
        return;
    }
    *floatVal = (float)atof(m_Token);
    if (floatStruct->lineNo != 0)
    {
        *(floatStruct->lineNo) = m_LineNumber;
    }
    floatStruct->used = true;
    NextToken();
    
    if (m_TokenType != TOKEN_SEMICOLON)
    {
        SetErrorFormat("Unexpected token \"%s\", expected semicolon.", m_Token);
        return;
    }
    NextToken();
}

void ScriptReader::RequiredParamString(const char* identifier,
                                       char* stringVal,
                                       int stringLen,
                                       int* lineNo)
{
    // if error, ignore
    if (m_IsError)
        return;
    
    // create fully-scoped identifier
    char scopedIdentifier[TOKEN_SIZE];
    StrSet(scopedIdentifier, sizeof(scopedIdentifier), m_Scope);
    
    if (strlen(scopedIdentifier) > 0)
    {
        StrCat(scopedIdentifier, sizeof(scopedIdentifier), ".");
    }
    StrCat(scopedIdentifier, sizeof(scopedIdentifier), identifier);
    
    // check for any incorrect string defaults
    int index = FindDefaultString(scopedIdentifier);
    if (index >= 0)
    {
        SetErrorFormat(m_DefaultStrings[index].lineNo,
                       "Not allowed to specify default for required string parameter \"%s\".",
                       scopedIdentifier);
        return;
    }
    
    // check for any incorrect float defaults
    index = FindDefaultFloat(scopedIdentifier);
    if (index >= 0)
    {
        SetErrorFormat(m_DefaultFloats[index].lineNo,
                       "Not allowed to specify default for required string parameter \"%s\".",
                       scopedIdentifier);
        return;
    }
    
    // make sure we have room to append
    if (m_StringCount >= MAX_STRING_COUNT)
    {
        SetErrorString("Ran out of space for required strings!");
        return;
    }
    
    // set string
    StringStruct* strStruct = (m_Strings + m_StringCount);
    strStruct->depth = m_Depth;
    StrSet(strStruct->identifier, sizeof(strStruct->identifier), identifier);
    strStruct->stringValue = stringVal;
    strStruct->stringLength = stringLen;
    strStruct->lineNo = lineNo;
    strStruct->used = false;
    ++m_StringCount;
    
    if (!m_IsInScope)
    {
        if (m_TokenType == TOKEN_STRING)
        {
            StrSet(stringVal, stringLen, m_Token);
            if (strStruct->lineNo != 0)
            {
                *(strStruct->lineNo) = m_LineNumber;
            }
            strStruct->used = true;
            NextToken();
            return;
        }
        
        if (m_TokenType != TOKEN_BRACE_OPEN)
        {
            SetErrorFormat("Unexpected token \"%s\", expected string or opening brace.", m_Token);
            return;
        }
        NextToken();
        
        m_IsInScope = true;
    }
    
    if ((m_TokenType != TOKEN_IDENTIFIER) ||
        (strcmp(m_Token, identifier) != 0))
    {
        SetErrorFormat("Unexpected token \"%s\", expected \"%s\".", m_Token, identifier);
        return;
    }
    NextToken();
    
    if (m_TokenType != TOKEN_STRING)
    {
        SetErrorFormat("Unexpected token \"%s\", expected string.", m_Token);
        return;
    }
    StrSet(stringVal, stringLen, m_Token);
    if (strStruct->lineNo != 0)
    {
        *(strStruct->lineNo) = m_LineNumber;
    }
    strStruct->used = true;
    NextToken();
    
    if (m_TokenType != TOKEN_SEMICOLON)
    {
        SetErrorFormat("Unexpected token \"%s\", expected semicolon.", m_Token);
        return;
    }
    NextToken();
}


// param declarators
//----------------------------------------------------------------------

void ScriptReader::ParamFloat(const char* identifier,
                              float* floatVal,
                              int* lineNo)
{
    // if error, ignore
    if (m_IsError)
        return;
    
    // make sure we have room to append
    if (m_FloatCount >= MAX_FLOAT_COUNT)
    {
        SetErrorString("Ran out of space for local floats!");
        return;
    }
    
    // set float
    FloatStruct* floatStruct = (m_Floats + m_FloatCount);
    floatStruct->depth = m_Depth;
    StrSet(floatStruct->identifier, sizeof(floatStruct->identifier), identifier);
    floatStruct->floatValue = floatVal;
    floatStruct->lineNo = lineNo;
    floatStruct->used = false;
    ++m_FloatCount;
    
    // create fully-scoped identifier
    char scopedIdentifier[TOKEN_SIZE];
    StrSet(scopedIdentifier, sizeof(scopedIdentifier), m_Scope);
    
    if (strlen(scopedIdentifier) > 0)
    {
        StrCat(scopedIdentifier, sizeof(scopedIdentifier), ".");
    }
    StrCat(scopedIdentifier, sizeof(scopedIdentifier), identifier);
    
    // check for any incorrect string defaults
    int index = FindDefaultString(scopedIdentifier);
    if (index >= 0)
    {
        SetErrorFormat(m_DefaultStrings[index].lineNo,
                       "Invalid type for default float \"%s\".",
                       scopedIdentifier);
        return;
    }
    
    // find float defaults, if any
    index = FindDefaultFloat(scopedIdentifier);
    if (index >= 0)
    {
        *(floatStruct->floatValue) = m_DefaultFloats[index].floatValue;
        
        if (floatStruct->lineNo != 0)
        {
            *(floatStruct->lineNo) = m_DefaultFloats[index].lineNo;
        }
    }
    
    if (!m_IsInScope)
    {
        // see if it has been supplied shorthand
        if (m_TokenType == TOKEN_FLOAT)
        {
            *(floatStruct->floatValue) = (float)atof(m_Token);
            if (floatStruct->lineNo != 0)
            {
                *(floatStruct->lineNo) = m_LineNumber;
            }
            floatStruct->used = true;
            NextToken();
        }
    }
}

void ScriptReader::ParamString(const char* identifier,
                               char* stringVal,
                               int stringLen,
                               int* lineNo)
{
    // if error, ignore
    if (m_IsError)
        return;
    
    // make sure we have room to append
    if (m_StringCount >= MAX_STRING_COUNT)
    {
        SetErrorString("Ran out of space for local strings!");
        return;
    }
    
    // set string
    StringStruct* strStruct = (m_Strings + m_StringCount);
    strStruct->depth = m_Depth;
    StrSet(strStruct->identifier, sizeof(strStruct->identifier), identifier);
    strStruct->stringValue = stringVal;
    strStruct->stringLength = stringLen;
    strStruct->lineNo = lineNo;
    strStruct->used = false;
    ++m_StringCount;
    
    // create fully-scoped identifier
    char scopedIdentifier[TOKEN_SIZE];
    StrSet(scopedIdentifier, sizeof(scopedIdentifier), m_Scope);
    
    if (strlen(scopedIdentifier) > 0)
    {
        StrCat(scopedIdentifier, sizeof(scopedIdentifier), ".");
    }
    StrCat(scopedIdentifier, sizeof(scopedIdentifier), identifier);
    
    // check for any incorrect float defaults
    int index = FindDefaultFloat(scopedIdentifier);
    if (index >= 0)
    {
        SetErrorFormat(m_DefaultFloats[index].lineNo,
                       "Invalid type for default string \"%s\".",
                       scopedIdentifier);
        return;
    }
    
    // find string defaults, if any
    index = FindDefaultString(scopedIdentifier);
    if (index >= 0)
    {
        StrSet(strStruct->stringValue,
               strStruct->stringLength,
               m_DefaultStrings[index].stringValue);
        
        if (strStruct->lineNo != 0)
        {
            *(strStruct->lineNo) = m_DefaultStrings[index].lineNo;
        }
    }
    
    if (!m_IsInScope)
    {
        // see if it has been supplied shorthand
        if (m_TokenType == TOKEN_STRING)
        {
            StrSet(strStruct->stringValue,
                   strStruct->stringLength,
                   m_Token);
            if (strStruct->lineNo != 0)
            {
                *(strStruct->lineNo) = m_LineNumber;
            }
            strStruct->used = true;
            NextToken();
        }
    }
}


} // namespace choreo


// End of file.
