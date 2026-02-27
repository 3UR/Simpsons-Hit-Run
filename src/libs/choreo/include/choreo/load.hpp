/*
 * choreo/load.hpp
 */

#ifndef CHOREO_LOAD_HPP
#define CHOREO_LOAD_HPP


#include <choreo/buildconfig.hpp>

#include <p3d/utility.hpp>


#ifdef CHOREO_USE_FILE_P3D
class tFile;
#endif

#ifdef CHOREO_USE_FILE_FTT
struct IRadFile;
#endif


namespace choreo
{


class FileReader;
class FileWriter;
class ScriptReader;
class ScriptWriter;
class BaseBank;


//---------------------------------------------------------------------------
// script read handler registration
//---------------------------------------------------------------------------

typedef bool (*ScriptReadHandler)(ScriptReader* script, BaseBank* bank);
bool RegisterScriptReadHandler(ScriptReadHandler scriptHandler);
void RegisterDefaultScriptReadHandlers();


//---------------------------------------------------------------------------
// script reading functions
//---------------------------------------------------------------------------

bool ReadFromScript(ScriptReader* script, tEntityStore* store = p3d::inventory);
bool ReadFromScriptFile(FileReader* scriptFile, tEntityStore* store = p3d::inventory);

bool ReadFromScriptString(const char* string, const char* fileName, tEntityStore* store = p3d::inventory);
bool ReadFromScriptString(const char* string, tEntityStore* store = p3d::inventory);

#ifdef CHOREO_USE_FILE_P3D
bool ReadFromScriptFileP3D(tFile* scriptFile, tEntityStore* store = p3d::inventory);
#endif

#ifdef CHOREO_USE_FILE_FTT
bool ReadFromScriptFileFTT(IRadFile* scriptFile, tEntityStore* store = p3d::inventory);
#endif

bool ReadFromScriptPath(const char* scriptPath, tEntityStore* store = p3d::inventory);


// only compile in writing if CHOREO_TOOL is defined
#ifdef CHOREO_TOOL
//---------------------------------------------------------------------------
// script write handler registration
//---------------------------------------------------------------------------

typedef bool (*ScriptWriteHandler)(ScriptWriter* script, BaseBank* bank);
bool RegisterScriptWriteHandler(ScriptWriteHandler scriptHandler);

#define CHOREO_SCRIPT_WRITE_HANDLER(type, script, bank) \
{ \
    radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP); \
    BaseBank::RawIterator* itor = bank->NewRawIterator(); \
    if (itor == 0) \
        return false; \
    itor->AddRef(); \
    IRefCount* entity = itor->First(); \
    while (entity != 0) \
    { \
        type* tPtr = dynamic_cast<type*>(entity); \
        if (tPtr != 0) \
        { \
            tPtr->WriteToScript(script); \
        } \
        entity = itor->Next(); \
    } \
    itor->Release(); \
    ::radMemorySetCurrentAllocator(oldAlloc); \
    return true; \
}

void RegisterDefaultScriptWriteHandlers();


//---------------------------------------------------------------------------
// script writing functions
//---------------------------------------------------------------------------

bool WriteToScript(ScriptWriter* script, tInventory* store = p3d::inventory);
bool WriteToScriptFile(FileWriter* scriptFile, tInventory* store = p3d::inventory);

#ifdef CHOREO_USE_FILE_FTT
bool WriteToScriptFileFTT(IRadFile* scriptFile, tInventory* store = p3d::inventory);
#endif

bool WriteToScriptPath(const char* scriptPath, tInventory* store = p3d::inventory);

#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// general script handler registration
//---------------------------------------------------------------------------

void RegisterDefaultScriptHandlers();


//---------------------------------------------------------------------------
// Pure3D loading system script loader integration
//---------------------------------------------------------------------------

#ifdef CHOREO_USE_FILE_P3D
void InstallDefaultLoaders(tLoadManager* loadManager = p3d::loadManager, bool useTxtExtension = true);
void InstallDefaultChunkHandlers(tLoadManager* loadManager = p3d::loadManager);
#endif


} // namespace choreo


#endif
