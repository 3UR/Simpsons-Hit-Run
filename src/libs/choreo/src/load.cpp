/*
 * choreo/load.cpp
 */

#include <SDL2/SDL.h>

#include <choreo/load.hpp>
#include <choreo/scriptreader.hpp>
#include <choreo/scriptwriter.hpp>

// global script handlers
#include <choreo/partition.hpp>
#include <choreo/rig.hpp>
#include <choreo/bank.hpp>
#include <choreo/puppet.hpp>

#ifdef CHOREO_USE_FILE_P3D
#include <p3d/loadmanager.hpp>
#include <p3d/chunkfile.hpp>
#endif

#ifdef CHOREO_USE_FILE_FTT
#include <radfile.hpp>
#endif

#include <stdio.h>
#include <string.h>
#include <raddebug.hpp>


#ifdef CHOREO_USE_FILE_P3D

#ifdef RAD_LINUX
#define strcmpi strcasecmp
#endif // RAD_LINUX

#endif // CHOREO_USE_FILE_P3D


namespace choreo
{


//---------------------------------------------------------------------------
// script read registration variables
//---------------------------------------------------------------------------

static const int MAX_SCRIPT_READ_HANDLERS = 32;
static bool g_DefaultScriptReadHandlersRegistered = false;
static int g_ScriptReadHandlerCount = 0;
static ScriptReadHandler g_ScriptReadHandlers[MAX_SCRIPT_READ_HANDLERS];


//---------------------------------------------------------------------------
// script read handler registration
//---------------------------------------------------------------------------

bool RegisterScriptReadHandler(ScriptReadHandler scriptHandler)
{
    P3DASSERT(g_ScriptReadHandlerCount < MAX_SCRIPT_READ_HANDLERS);
    if (g_ScriptReadHandlerCount >= MAX_SCRIPT_READ_HANDLERS)
        return false;
    
    g_ScriptReadHandlers[g_ScriptReadHandlerCount] = scriptHandler;
    ++g_ScriptReadHandlerCount;
    
    return true;
}


//---------------------------------------------------------------------------
// global script read handlers
//---------------------------------------------------------------------------

void RegisterDefaultScriptReadHandlers()
{
    if (g_DefaultScriptReadHandlersRegistered)
        return;
    g_DefaultScriptReadHandlersRegistered = true;

    // global script handlers
    RegisterScriptReadHandler(CompletePartition::ReadFromScript);
    RegisterScriptReadHandler(InclusivePartition::ReadFromScript);
    RegisterScriptReadHandler(ExclusivePartition::ReadFromScript);
    RegisterScriptReadHandler(UnionPartition::ReadFromScript);
    RegisterScriptReadHandler(IntersectPartition::ReadFromScript);
    RegisterScriptReadHandler(Rig::ReadFromScript);
    RegisterScriptReadHandler(Bank::ReadFromScript);
    RegisterScriptReadHandler(Puppet::ReadFromScript);

    // register bank's handlers
    Bank::RegisterDefaultScriptReadHandlers();
}


//---------------------------------------------------------------------------
// script reading
//---------------------------------------------------------------------------

bool ReadFromScript(ScriptReader* script, tEntityStore* store)
{
	// temporarily allocate inv wrapper
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

    P3DEntityStoreWrapperBank* invWrapper = new P3DEntityStoreWrapperBank(store);
    invWrapper->AddRef();
    
	// revert
    ::radMemorySetCurrentAllocator(oldAlloc);

    while (!script->IsEndOfFile() &&
           !script->IsError())
    {
        bool rc = false;
        for (int i = 0; i < g_ScriptReadHandlerCount; ++i)
        {
            rc = g_ScriptReadHandlers[i](script, invWrapper);
            if (rc)
                break;
        }
        
        if (!rc && script->IsMoreStructs())
        {
            script->SetErrorFormat("Unknown token \"%s\".",
                script->GetNextStructIdentifier());
        }
    }

    invWrapper->Release();
    
    if (script->IsError())
    {
        return script->HandleError();
    }

    return true;
}

bool ReadFromScriptFile(FileReader* scriptFile, tEntityStore* store)
{
    ScriptReader* script = new ScriptReader(scriptFile, 1);
    script->AddRef();

    bool rc = ReadFromScript(script, store);
    
    script->Release();

    return rc;
}

bool ReadFromScriptString(const char* string, const char* fileName, tEntityStore* store)
{
	// temporarily allocate file reader
    radMemoryAllocator oldAlloc = radMemoryGetCurrentAllocator();
    rReleasePrintf( "Old allocator %d\n", oldAlloc );
    if( oldAlloc == RADMEMORY_ALLOC_PERSISTENT )
    {
        ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);
    }
    StringFileReader* fileReader = new StringFileReader(string, fileName);
    fileReader->AddRef();

  	// restore memory allocator
    if( oldAlloc == RADMEMORY_ALLOC_PERSISTENT )
    {
        ::radMemorySetCurrentAllocator(oldAlloc);
    }

    bool rc = ReadFromScriptFile(fileReader, store);

    fileReader->Release();

    return rc;
}

bool ReadFromScriptString(const char* string, tEntityStore* store)
{
    return ReadFromScriptString(string, "<STRING>", store);
}

#ifdef CHOREO_USE_FILE_P3D
bool ReadFromScriptFileP3D(tFile* scriptFile, tEntityStore* store)
{
	// temporarily allocate file reader
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

    P3DFileReader* fileReader = new P3DFileReader(scriptFile);
    fileReader->AddRef();

    bool rc = ReadFromScriptFile(fileReader, store);

    fileReader->Release();

	// restore memory allocator
    ::radMemorySetCurrentAllocator(oldAlloc);

    return rc;
}
#endif

#ifdef CHOREO_USE_FILE_FTT
bool ReadFromScriptFileFTT(IRadFile* scriptFile, tEntityStore* store)
{
	// temporarily allocate file reader
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

    FTTFileReader* fileReader = new FTTFileReader(scriptFile);
    fileReader->AddRef();

    bool rc = ReadFromScriptFile(fileReader, store);

    fileReader->Release();

	// restore memory allocator
    ::radMemorySetCurrentAllocator(oldAlloc);

    return rc;
}
#endif

bool ReadFromScriptPath(const char* scriptPath, tEntityStore* store)
{
    bool rc = false;

	// temporarily allocate file reader
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

#if defined(CHOREO_USE_FILE_FTT)

    // by default, use FTT IRadFiles
    IRadFile* scriptFile = 0;
    radFileOpen(&scriptFile,
                scriptPath,
                false,
                OpenExisting,
                NormalPriority);

    if (scriptFile == 0)
        return false;

    scriptFile->WaitForCompletion();
    if (!scriptFile->IsOpen())
    {
        scriptFile->Release();
        return false;
    }

    unsigned length = scriptFile->GetSize();
    char* scriptString = new char [length + 1];
    scriptFile->ReadAsync(scriptString, length);
    scriptFile->WaitForCompletion();
    scriptString[length] = '\0';

    rc = ReadFromScriptString(scriptString, scriptFile->GetFilename(), store);

    delete[] scriptString;
    scriptFile->Release();

#elif defined(CHOREO_USE_FILE_P3D)

    // otherwise, use P3D tFiles
    tFile* scriptFile = p3d::openFile(scriptPath);
    P3DASSERT(scriptFile != 0);
    if (scriptFile == 0)
    {
        return false;
    }
    scriptFile->AddRef();

    unsigned length = scriptFile->GetSize();
    char* scriptString = new char [length + 1];
    scriptFile->GetData(scriptString, length);
    scriptString[length] = '\0';

    rc = ReadFromScriptString(scriptString, scriptFile->GetFullFilename(), store);

    delete[] scriptString;
    scriptFile->Release();

#endif

	// restore memory allocator
    ::radMemorySetCurrentAllocator(oldAlloc);

    return rc;
}


// only compile in writing if CHOREO_TOOL is defined
#ifdef CHOREO_TOOL
//---------------------------------------------------------------------------
// script write registration variables
//---------------------------------------------------------------------------

static const int MAX_SCRIPT_WRITE_HANDLERS = 32;
static bool g_DefaultScriptWriteHandlersRegistered = false;
static int g_ScriptWriteHandlerCount = 0;
static ScriptWriteHandler g_ScriptWriteHandlers[MAX_SCRIPT_WRITE_HANDLERS];


//---------------------------------------------------------------------------
// script write handler registration
//---------------------------------------------------------------------------

bool RegisterScriptWriteHandler(ScriptWriteHandler scriptHandler)
{
    P3DASSERT(g_ScriptWriteHandlerCount < MAX_SCRIPT_WRITE_HANDLERS);
    if (g_ScriptWriteHandlerCount >= MAX_SCRIPT_WRITE_HANDLERS)
        return false;
    
    g_ScriptWriteHandlers[g_ScriptWriteHandlerCount] = scriptHandler;
    ++g_ScriptWriteHandlerCount;
    
    return true;
}


//---------------------------------------------------------------------------
// global script write handlers
//---------------------------------------------------------------------------

void RegisterDefaultScriptWriteHandlers()
{
    if (g_DefaultScriptWriteHandlersRegistered)
        return;
    g_DefaultScriptWriteHandlersRegistered = true;

    // global script handlers
    RegisterScriptWriteHandler(CompletePartition::WriteToScriptHandler);
    RegisterScriptWriteHandler(InclusivePartition::WriteToScriptHandler);
    RegisterScriptWriteHandler(ExclusivePartition::WriteToScriptHandler);
    RegisterScriptWriteHandler(UnionPartition::WriteToScriptHandler);
    RegisterScriptWriteHandler(IntersectPartition::WriteToScriptHandler);
    RegisterScriptWriteHandler(Rig::WriteToScriptHandler);
    RegisterScriptWriteHandler(Bank::WriteToScriptHandler);
    RegisterScriptWriteHandler(Puppet::WriteToScriptHandler);

    // register bank's handlers
    Bank::RegisterDefaultScriptWriteHandlers();
}


//---------------------------------------------------------------------------
// script writing functions
//---------------------------------------------------------------------------

bool WriteToScript(ScriptWriter* script, tInventory* store)
{
	// temporarily allocate inv wrapper
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

    P3DInventoryWrapperBank* invWrapper = new P3DInventoryWrapperBank(store);
    invWrapper->AddRef();

	bool rc = true;
    for (int i = 0; i < g_ScriptWriteHandlerCount; ++i)
    {
        rc = g_ScriptWriteHandlers[i](script, invWrapper);
		if (!rc)
			break;
    }

    invWrapper->Release();
    
	// restore memory allocator
    ::radMemorySetCurrentAllocator(oldAlloc);

    return rc;
}

bool WriteToScriptFile(FileWriter* scriptFile, tInventory* store)
{
	// temporarily allocate script
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

    ScriptWriter* script = new ScriptWriter(scriptFile);
    script->AddRef();

    bool rc = WriteToScript(script, store);

    script->Release();

	// restore memory allocator
    ::radMemorySetCurrentAllocator(oldAlloc);

    return rc;
}

#ifdef CHOREO_USE_FILE_FTT
bool WriteToScriptFileFTT(IRadFile* scriptFile, tInventory* store)
{
	// temporarily allocate file writer
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

    FTTFileWriter* fileWriter = new FTTFileWriter(scriptFile);
    fileWriter->AddRef();

    bool rc = WriteToScriptFile(fileWriter, store);

    fileWriter->Release();

	// restore memory allocator
    ::radMemorySetCurrentAllocator(oldAlloc);

    return rc;
}
#endif

bool WriteToScriptPath(const char* scriptPath, tInventory* store)
{
#if defined(CHOREO_USE_FILE_FTT)

    // by default use FTT IRadFiles
    IRadFile* scriptFile = 0;
    radFileOpen(&scriptFile,
                scriptPath,
                true,
                CreateAlways);

    if (scriptFile == 0)
        return false;

    scriptFile->WaitForCompletion();
    if (!scriptFile->IsOpen())
    {
        scriptFile->Release();
        return false;
    }

    bool rc = WriteToScriptFileFTT(scriptFile, store);

    scriptFile->Release();
    return rc;

#else

    // if nothing else, do nothing!
    return false;

#endif
}

#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// global script handlers
//---------------------------------------------------------------------------

void RegisterDefaultScriptHandlers()
{
	RegisterDefaultScriptReadHandlers();

#ifdef CHOREO_TOOL
	RegisterDefaultScriptWriteHandlers();
#endif // CHOREO_TOOL
}


//---------------------------------------------------------------------------
// Pure3D loading system script loader integration
//---------------------------------------------------------------------------

#ifdef CHOREO_USE_FILE_P3D

static const char* CHOREO_SCRIPT_EXTENSION = ".cho";
static const char* CHOREO_SCRIPT_EXTENSION_OLD = ".txt";

class ChoreoScriptHandler: public tFileHandler
{
public:

    ChoreoScriptHandler(bool useOldExtension):
        m_UseOldExtension(useOldExtension)
        { }

    virtual bool CheckExtension(char* fileName)
    {
        int len = strlen(fileName);

        int extLen = strlen(CHOREO_SCRIPT_EXTENSION);
        if (len >= extLen)
        {
            if (SDL_strcasecmp(CHOREO_SCRIPT_EXTENSION, &fileName[len - extLen]) == 0)
                return true;
        }

        if (m_UseOldExtension)
        {
            extLen = strlen(CHOREO_SCRIPT_EXTENSION_OLD);
            if (len >= extLen)
            {
                if (SDL_strcasecmp(CHOREO_SCRIPT_EXTENSION_OLD, &fileName[len - extLen]) == 0)
                    return true;
            }
        }

        return false;
    }

    virtual tLoadStatus Load(tFile* file, tEntityStore* store)
    {
        if (ReadFromScriptFileP3D(file, store))
        {
            return LOAD_OK;
        }

        return LOAD_ERROR;
    }

protected:

    virtual ~ChoreoScriptHandler()
        { }

private:

    bool m_UseOldExtension;
};

void InstallDefaultLoaders(tLoadManager* loadManager, bool useTxtExtension)
{
    RegisterDefaultScriptHandlers();

    loadManager->AddHandler(new ChoreoScriptHandler(useTxtExtension), "cho");
}

class ChoreoChunkHandler: public tChunkHandler
{
public:

    ChoreoChunkHandler()
        { }

    virtual tLoadStatus Load(tChunkFile* file, tEntityStore* store)
    {
        if (file->GetCurrentID() != SCRIPT_CHUNK_ID)
            return LOAD_ERROR;

	    // temporarily allocate script string memory
        radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

        // load script string
        int scriptLen = file->GetLong();
        char* scriptStr = new char [scriptLen + 1];
        file->GetData(scriptStr, scriptLen);
        scriptStr[scriptLen] = '\0';

        // parse script
        bool rc = ReadFromScriptString(scriptStr, file->GetFullFilename(), store);

        delete[] scriptStr;

	    // restore memory allocator
        ::radMemorySetCurrentAllocator(oldAlloc);

        if (!rc)
            return LOAD_ERROR;
        return LOAD_OK;
    }

    virtual bool CheckChunkID(unsigned id)
    {
        return (id == SCRIPT_CHUNK_ID);
    }

    virtual unsigned int GetChunkID()
    {
        return SCRIPT_CHUNK_ID;
    }

protected:

    virtual ~ChoreoChunkHandler()
        { }
};

void InstallDefaultChunkHandlers(tLoadManager* loadManager)
{
    RegisterDefaultScriptHandlers();

    loadManager->GetP3DHandler()->AddHandler(new ChoreoChunkHandler);
}

#endif // CHOREO_USE_FILE_P3D


} // namespace choreo


// End of file.
