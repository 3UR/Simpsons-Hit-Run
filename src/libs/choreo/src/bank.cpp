/*
 * choreo/bank.cpp
 */

#include <choreo/bank.hpp>
#include <choreo/rig.hpp>
#include <choreo/scriptreader.hpp>
#include <choreo/scriptwriter.hpp>

// for global script handlers
#include <choreo/replay.hpp>
#include <choreo/transition.hpp>

#include <string.h>


#include <radmemory.hpp>

namespace choreo
{


//---------------------------------------------------------------------------
// class Bank::RawIterator - implementation
//---------------------------------------------------------------------------

Bank::RawIterator::RawIterator(Bank* bank):
    m_RawIterator(bank->m_EntityTable)
{
}

IRefCount* Bank::RawIterator::First()
{
    return m_RawIterator.First();
}

IRefCount* Bank::RawIterator::Last()
{
    return m_RawIterator.Last();
}

IRefCount* Bank::RawIterator::Next()
{
    return m_RawIterator.Next();
}

IRefCount* Bank::RawIterator::Prev()
{
    return m_RawIterator.Prev();
}


//---------------------------------------------------------------------------
// class Bank - core routines
//---------------------------------------------------------------------------

Bank::Bank(Rig* rig)
{
    // FIXME:  right now, we can't control memory placement
    //         of the tEntityTable.  will need to fix this if
    //         pure3d provides a solution.
    m_EntityTable = new tEntityTable;
    m_EntityTable->AddRef();
    
    P3DASSERT(rig != 0);
    m_Rig = rig;
    m_Rig->AddRef();
}

Bank::~Bank()
{
    m_Rig->Release();
    m_EntityTable->Release();
}

IRefCount* Bank::Find(radLoadInventory::SafeCastBase& safeCaster, tUID uid) const
{
    tEntityTable::Iterator itor(m_EntityTable);
    IRefCount* entity = itor.First(uid);
    
    while (entity)
    {
        if (safeCaster.safe_cast(entity))
        {
            return entity;
        }
        
        entity = itor.Next();
    }
    
    return 0;
}

void Bank::Store(tEntity* entity)
{
    m_EntityTable->Store(entity->GetUID(), entity);
}

void Bank::Remove(tEntity* entity)
{
    m_EntityTable->Remove(entity);
}

void Bank::RemoveAll()
{
    m_EntityTable->RemoveAll();
}

BaseBank::RawIterator* Bank::NewRawIterator()
{
	return new RawIterator(this);
}


//---------------------------------------------------------------------------
// class Bank - script loading
//---------------------------------------------------------------------------

static const char* TOKEN_BANK = "bank";
static const char* TOKEN_NAME = "name";
static const char* TOKEN_RIG  = "rig";
    
bool Bank::m_DefaultScriptReadHandlersRegistered = false;
int Bank::m_ScriptReadHandlerCount = 0;
ScriptReadHandler Bank::m_ScriptReadHandlers[Bank::MAX_SCRIPT_READ_HANDLERS];

#ifdef CHOREO_TOOL
bool Bank::m_DefaultScriptWriteHandlersRegistered = false;
int Bank::m_ScriptWriteHandlerCount = 0;
ScriptWriteHandler Bank::m_ScriptWriteHandlers[Bank::MAX_SCRIPT_WRITE_HANDLERS];
#endif // CHOREO_TOOL

bool Bank::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_BANK) != 0)
        return false;
    
    script->BeginStruct(TOKEN_BANK);
    
    Bank* newBank = 0;
    
    // required params
    char bankName[256] = "";
    script->RequiredParamString(TOKEN_NAME, bankName, sizeof(bankName));
    
    char rigName[256] = "";
    script->RequiredParamString(TOKEN_RIG, rigName, sizeof(rigName));
    
    Rig* rig = choreo::find<Rig>(bank, rigName);
    if (rig == 0)
    {
        script->SetErrorFormat("Can't find Rig named \"%s\".", rigName);
    }
    
    if (!script->IsError())
    {
        newBank = new Bank(rig);
        newBank->AddRef();
        newBank->SetName(bankName);
        
		// temporarily allocate multibank
		radMemoryAllocator oldAlloc = ::radMemorySetCurrentAllocator(RADMEMORY_ALLOC_TEMP);

        MultiBank* multiBank = new MultiBank(newBank, bank);
        multiBank->AddRef();
        
		// use load memory allocator for loaded files
		::radMemorySetCurrentAllocator(oldAlloc);

        while (script->IsMoreStructs())
        {
            bool rc = false;
            for (int i = 0; i < m_ScriptReadHandlerCount; ++i)
            {
                rc = m_ScriptReadHandlers[i](script, multiBank);
                if (rc)
                    break;
            }
            
            if (!rc && script->IsMoreStructs())
            {
                script->SetErrorFormat("Unexpected token \"%s\".",
                                       script->GetNextStructIdentifier());
            }
        }

        multiBank->Release();
    }
    
    script->EndStruct();
    
    if (script->IsError())
    {
        tRefCounted::Release(newBank);
        return script->HandleError();
    }

    bank->Store(newBank);
    tRefCounted::Release(newBank);
    return true;
}

#ifdef CHOREO_TOOL
bool Bank::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(Bank, script, bank);
}

bool Bank::WriteToScript(ScriptWriter* script)
{
    script->BeginStruct(TOKEN_BANK);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());
    script->RequiredParamString(TOKEN_RIG, m_Rig->GetName());

    // entities in bank
	bool rc = true;
	for (int i = 0; i < m_ScriptWriteHandlerCount; ++i)
	{
		rc = m_ScriptWriteHandlers[i](script, this);
		if (!rc)
			break;
	}

    script->EndStruct();
	return rc;
}
#endif // CHOREO_TOOL

bool Bank::RegisterScriptReadHandler(ScriptReadHandler scriptHandler)
{
    P3DASSERT(m_ScriptReadHandlerCount < MAX_SCRIPT_READ_HANDLERS);
    if (m_ScriptReadHandlerCount >= MAX_SCRIPT_READ_HANDLERS)
        return false;
    
    m_ScriptReadHandlers[m_ScriptReadHandlerCount] = scriptHandler;
    ++m_ScriptReadHandlerCount;
    
    return true;
}

void Bank::RegisterDefaultScriptReadHandlers()
{
    if (m_DefaultScriptReadHandlersRegistered)
        return;
    m_DefaultScriptReadHandlersRegistered = true;
    
    // global script handlers
    RegisterScriptReadHandler(CompletePartition::ReadFromScript);
    RegisterScriptReadHandler(InclusivePartition::ReadFromScript);
    RegisterScriptReadHandler(ExclusivePartition::ReadFromScript);
    RegisterScriptReadHandler(UnionPartition::ReadFromScript);
    RegisterScriptReadHandler(IntersectPartition::ReadFromScript);
    
    // driver script handlers
    RegisterScriptReadHandler(Animation::ReadFromScript);
    RegisterScriptReadHandler(Transition::ReadFromScript);
    RegisterScriptReadHandler(Locomotion::ReadFromScript);
    RegisterScriptReadHandler(Locomotion::ReadFromScript4);
    RegisterScriptReadHandler(Locomotion::ReadFromScript8);
    RegisterScriptReadHandler(Replay::ReadFromScript);
}

#ifdef CHOREO_TOOL
bool Bank::RegisterScriptWriteHandler(ScriptWriteHandler scriptHandler)
{
    P3DASSERT(m_ScriptWriteHandlerCount < MAX_SCRIPT_WRITE_HANDLERS);
    if (m_ScriptWriteHandlerCount >= MAX_SCRIPT_WRITE_HANDLERS)
        return false;
    
    m_ScriptWriteHandlers[m_ScriptWriteHandlerCount] = scriptHandler;
    ++m_ScriptWriteHandlerCount;
    
    return true;
}

void Bank::RegisterDefaultScriptWriteHandlers()
{
    if (m_DefaultScriptWriteHandlersRegistered)
        return;
    m_DefaultScriptWriteHandlersRegistered = true;
    
    // global script handlers
    RegisterScriptWriteHandler(CompletePartition::WriteToScriptHandler);
    RegisterScriptWriteHandler(InclusivePartition::WriteToScriptHandler);
    RegisterScriptWriteHandler(ExclusivePartition::WriteToScriptHandler);
    RegisterScriptWriteHandler(UnionPartition::WriteToScriptHandler);
    RegisterScriptWriteHandler(IntersectPartition::WriteToScriptHandler);

    // driver script handlers
    RegisterScriptWriteHandler(Animation::WriteToScriptHandler);
    RegisterScriptWriteHandler(Transition::WriteToScriptHandler);
    RegisterScriptWriteHandler(Locomotion::WriteToScriptHandler);
    RegisterScriptWriteHandler(Replay::WriteToScriptHandler);
}
#endif // CHOREO_TOOL

void Bank::RegisterDefaultScriptHandlers()
{
	RegisterDefaultScriptReadHandlers();

#ifdef CHOREO_TOOL
	RegisterDefaultScriptWriteHandlers();
#endif // CHOREO_TOOL
}


} // namespace choreo


// End of file.
