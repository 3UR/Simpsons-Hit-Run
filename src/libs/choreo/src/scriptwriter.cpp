/*
 * choreo/scriptwriter.cpp
 */

#include <choreo/scriptwriter.hpp>


// only compile in writing if CHOREO_TOOL is defined
#ifdef CHOREO_TOOL


#include <choreo/utility.hpp>

#include <p3d/error.hpp>

#ifdef CHOREO_USE_FILE_FTT
#include <radfile.hpp>
#endif

#include <string.h>


#ifdef RAD_LINUX
#define CHOREO_WRITER_EOL "\n"
#else
#define CHOREO_WRITER_EOL "\r\n"
#endif


namespace choreo
{


//---------------------------------------------------------------------------
// class BufferFileWriter
//   - writes to a buffer maintained by instance
//---------------------------------------------------------------------------

BufferFileWriter::BufferFileWriter():
    m_MaxBufferSize(0),
    m_BufferLength(0),
    m_Buffer(0)
{
}

BufferFileWriter::~BufferFileWriter()
{
    delete[] m_Buffer;
}

void BufferFileWriter::Clear()
{
    m_BufferLength = 0;

    if (m_MaxBufferSize > 0)
    {
        m_Buffer[0] = '\0';
    }
}

void BufferFileWriter::WriteString(const char* str)
{
    int len = strlen(str);
    int postBufferLength = m_BufferLength + len;
    int postBufferSize = postBufferLength + 1;

    if (postBufferSize > m_MaxBufferSize)
    {
        if (m_MaxBufferSize <= 0)
        {
            m_MaxBufferSize = postBufferSize;
            m_Buffer = new char [m_MaxBufferSize];
        }
        else
        {
            while (m_MaxBufferSize < postBufferSize)
            {
                m_MaxBufferSize *= 2;
            }

            char* tmp = new char [m_MaxBufferSize];
            strcpy(tmp, m_Buffer);

            delete[] m_Buffer;
            m_Buffer = tmp;
        }
    }

    strcpy(m_Buffer + m_BufferLength, str);
    m_BufferLength = postBufferLength;
}

//---------------------------------------------------------------------------
// class CStreamFileWriter
//---------------------------------------------------------------------------

CStreamFileWriter::CStreamFileWriter(FILE* file):
	m_File(file)
{
}

CStreamFileWriter::~CStreamFileWriter()
{
}

void CStreamFileWriter::WriteString(const char* str)
{
	fwrite(str, strlen(str), 1, m_File);
}

#ifdef CHOREO_USE_FILE_FTT
//---------------------------------------------------------------------------
// class FTTFileWriter
//---------------------------------------------------------------------------

FTTFileWriter::FTTFileWriter(IRadFile* fttFile)
{
    P3DASSERT(fttFile != 0);
    m_File = fttFile;
    m_File->AddRef();
}

FTTFileWriter::~FTTFileWriter()
{
    m_File->Release();
}

void FTTFileWriter::WriteString(const char* str)
{
    int strLen = strlen(str);
    if (strLen > 0)
    {
        m_File->WriteAsync(str, strlen(str));

	    // FIXME:  currently, radFile keeps running out of request handles,
	    //         so we flush the buffer here
	    m_File->WaitForCompletion();
    }
}

#endif // CHOREO_USE_FILE_FTT


//----------------------------------------------------------------------
// class ScriptWriter
//----------------------------------------------------------------------

ScriptWriter::ScriptWriter(FileWriter* fileWriter,
                           int indentSize,
                           bool useTabs):
    m_IndentSize(indentSize),
    m_UseTabs(useTabs),

    m_Indent(0)
{
    P3DASSERT(fileWriter != 0);
    m_FileWriter = fileWriter;
    m_FileWriter->AddRef();
}

ScriptWriter::~ScriptWriter()
{
    m_FileWriter->Release();
}

void ScriptWriter::Indent()
{
    char buf[256];

    if (m_UseTabs)
    {
        P3DASSERT((sizeof(buf) - 1) >= m_Indent);

        for (int i = 0; i < m_Indent; ++i)
        {
            buf[i] = '\t';
        }

        buf[m_Indent] = '\0';
    }
    else
    {
        int bufLen = m_Indent * m_IndentSize;
        P3DASSERT((sizeof(buf) - 1) >= bufLen);

        for (int i = 0; i < bufLen; ++i)
        {
            buf[i] = ' ';
        }

        buf[bufLen] = '\0';
    }

    m_FileWriter->WriteString(buf);
}

void ScriptWriter::BeginStruct(const char* identifier)
{
    m_FileWriter->WriteString(CHOREO_WRITER_EOL);

    Indent();

    char buf[256];

    StrPrintf(buf, sizeof(buf), "%s" CHOREO_WRITER_EOL, identifier);
    m_FileWriter->WriteString(buf);

    Indent();

    m_FileWriter->WriteString("{" CHOREO_WRITER_EOL);

    ++m_Indent;
}

void ScriptWriter::EndStruct()
{
    P3DASSERT(m_Indent > 0);
    --m_Indent;

    Indent();

    m_FileWriter->WriteString("};" CHOREO_WRITER_EOL);
}

void ScriptWriter::RequiredParamFloat(const char* identifier, float floatVal)
{
    Indent();

    char buf[256];

    StrPrintf(buf, sizeof(buf), "%s %g;" CHOREO_WRITER_EOL, identifier, floatVal);
    m_FileWriter->WriteString(buf);
}

void ScriptWriter::RequiredParamString(const char* identifier, const char* stringVal)
{
    Indent();

    // convert escapes
    char tmp[1024];
    StrEscapify(tmp, sizeof(tmp), stringVal);

    // write string
    char buf[1024];
    StrPrintf(buf, sizeof(buf), "%s \"%s\";" CHOREO_WRITER_EOL, identifier, tmp);
    m_FileWriter->WriteString(buf);
}

void ScriptWriter::ParamFloat(const char* identifier, float floatVal)
{
    RequiredParamFloat(identifier, floatVal);
}

void ScriptWriter::ParamString(const char* identifier, const char* stringVal)
{
    RequiredParamString(identifier, stringVal);
}

void ScriptWriter::Comment(const char* commentStr)
{
    Indent();

    char buf[1024];

    StrPrintf(buf, sizeof(buf), "// %s" CHOREO_WRITER_EOL, commentStr);
    m_FileWriter->WriteString(buf);
}

void ScriptWriter::SectionComment(const char* commentStr)
{
    m_FileWriter->WriteString(CHOREO_WRITER_EOL);

    Indent();

    char buf[1024];

    StrPrintf(buf, sizeof(buf), "// %s" CHOREO_WRITER_EOL, commentStr);
    m_FileWriter->WriteString(buf);
}


} // namespace choreo


#endif // CHOREO_TOOL


// End of file.
