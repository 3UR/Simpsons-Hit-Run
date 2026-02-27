/*
 * choreo/partition.cpp
 */

#include <choreo/partition.hpp>
#include <choreo/scriptreader.hpp>
#include <choreo/scriptwriter.hpp>
#include <choreo/basebank.hpp>
#include <choreo/rig.hpp>
#include <choreo/load.hpp>

#include <string.h>
#include <p3d/error.hpp>
#include <p3d/anim/skeleton.hpp>
#include <p3d/utility.hpp>


namespace choreo
{


//---------------------------------------------------------------------------
// script tokens
//---------------------------------------------------------------------------

static const char* TOKEN_INCLUSIVEPARTITION = "inclusivePartition";
static const char* TOKEN_EXCLUSIVEPARTITION = "exclusivePartition";
static const char* TOKEN_UNIONPARTITION     = "unionPartition";
static const char* TOKEN_INTERSECTPARTITION = "intersectPartition";
static const char* TOKEN_COMPLETEPARTITION  = "partition";
static const char* TOKEN_NAME               = "name";
static const char* TOKEN_RIG                = "rig";
static const char* TOKEN_JOINT              = "joint";
static const char* TOKEN_JOINTINDEX         = "jointIndex";
static const char* TOKEN_JOINTRANGE         = "jointRange";
static const char* TOKEN_JOINTINDEXRANGE    = "jointIndexRange";
static const char* TOKEN_INDEX              = "index";
static const char* TOKEN_WEIGHT             = "weight";
static const char* TOKEN_PRIORITY           = "priority";
static const char* TOKEN_MIN                = "min";
static const char* TOKEN_MAX                = "max";
static const char* TOKEN_PARTITION          = "partition";


//---------------------------------------------------------------------------
// class Partition
//---------------------------------------------------------------------------

Partition::Partition(Rig* rig)
{
    P3DASSERT(rig != 0);
    m_Rig = rig;
    m_Rig->AddRef();
}

Partition::~Partition()
{
    m_Rig->Release();
}

CompletePartition* Partition::Bake() const
{
    CompletePartition* partition = new CompletePartition(m_Rig);

    int jointCount = m_Rig->GetJointCount();
    for (int i = 0; i < jointCount; ++i)
    {
        if (HasJoint(i))
        {
            partition->SetHasJoint(i, true);
            partition->SetJointWeight(i, GetJointWeight(i));
            partition->SetJointPriority(i, GetJointPriority(i));
        }
    }

    return partition;
}


//---------------------------------------------------------------------------
// class InclusivePartition
//---------------------------------------------------------------------------

InclusivePartition::InclusivePartition(Rig* rig, int maxEntryCount, int maxRangeEntryCount):
    Partition(rig),
    m_MaxEntryCount(maxEntryCount),
    m_EntryCount(0),
    m_MaxRangeEntryCount(maxRangeEntryCount),
    m_RangeEntryCount(0),
    m_CachedJointIndex(-1),
    m_CachedEntryIndex(-1),
    m_CachedRangeEntryIndex(-1)
{
    if (m_MaxEntryCount > 0)
    {
        m_Entries = new Entry [m_MaxEntryCount];
    }
    else
    {
        m_Entries = 0;
    }

    if (m_MaxRangeEntryCount > 0)
    {
        m_RangeEntries = new RangeEntry [m_MaxRangeEntryCount];
    }
    else
    {
        m_RangeEntries = 0;
    }
}

InclusivePartition::~InclusivePartition()
{
    delete[] m_RangeEntries;
    delete[] m_Entries;
}

bool InclusivePartition::AddEntry(int jointIndex,
                                  float weight,
                                  int priority)
{
    P3DASSERT(m_EntryCount < m_MaxEntryCount);
    if (m_EntryCount >= m_MaxEntryCount)
        return false;
    
    int i;
    for (i = 0; i < m_EntryCount; ++i)
    {
        // for now, don't allow duplicate or replacement of entries
        P3DASSERT(jointIndex != m_Entries[i].jointIndex);
        if (jointIndex == m_Entries[i].jointIndex)
            return false;
        
        if (jointIndex < m_Entries[i].jointIndex)
        {
            break;
        }
    }
    
    memmove(m_Entries + i + 1,
            m_Entries + i,
            (m_EntryCount - i) * sizeof(Entry));
    
    Entry* entry = (m_Entries + i);
    entry->jointIndex = jointIndex;
    entry->attrib.weight = weight;
    entry->attrib.priority = priority;
    
    ++m_EntryCount;
    
    return true;
}

bool InclusivePartition::AddRangeEntry(int jointMin, int jointMax, float weight, int priority)
{
    P3DASSERT(m_RangeEntryCount < m_MaxRangeEntryCount);
    if (m_RangeEntryCount >= m_MaxRangeEntryCount)
        return false;

    // don't check duplicates or sort joint ranges

    RangeEntry* entry = (m_RangeEntries + m_RangeEntryCount);
    entry->jointMin = jointMin;
    entry->jointMax = jointMax;
    entry->attrib.weight = weight;
    entry->attrib.priority = priority;

    ++m_RangeEntryCount;

    return true;
}

int InclusivePartition::FindEntryIndex(int jointIndex) const
{
    // if we have no joints
    if (m_EntryCount == 0)
        return -1;
    
    // check to see if its in the cache
    if (jointIndex == m_CachedJointIndex)
    {
        return m_CachedEntryIndex;
    }
    
    // binary search for joint index
    int iLeft = 0;
    int iRight = m_EntryCount - 1;
    int iCurrent = iRight / 2;
    
    while (1)
    {
        if (jointIndex == m_Entries[iCurrent].jointIndex)
        {
            // found joint index, cache results
            InclusivePartition* cthis = const_cast<InclusivePartition*>(this);
            cthis->m_CachedJointIndex = jointIndex;
            cthis->m_CachedEntryIndex = iCurrent;
            
            return iCurrent;
        }
        
        if (iLeft >= iRight)
        {
            // didn't find joint index
            return -1;
        }

        if (jointIndex < m_Entries[iCurrent].jointIndex)
        {
            iRight = iCurrent - 1;
        }
        else
        {
            iLeft = iCurrent + 1;
        }

        iCurrent = (iLeft + iRight) / 2;
    }
    
    return -1;
}

const InclusivePartition::JointAttrib* InclusivePartition::FindEntry(int jointIndex) const
{
    // check joint entries
    int entryIndex = FindEntryIndex(jointIndex);
    if (entryIndex >= 0)
    {
        return &(m_Entries[entryIndex].attrib);
    }

    // check joint range cache
    if (jointIndex == m_CachedJointIndex)
    {
        P3DASSERT(m_CachedEntryIndex < 0);
        return &(m_RangeEntries[m_CachedRangeEntryIndex].attrib);
    }

    // check joint range entries
    for (int i = 0; i < m_RangeEntryCount; ++i)
    {
        const RangeEntry* entry = m_RangeEntries + i;

        if ((jointIndex >= entry->jointMin) &&
            (jointIndex <= entry->jointMax))
        {
            // found joint range, cache results
            InclusivePartition* cthis = const_cast<InclusivePartition*>(this);
            cthis->m_CachedJointIndex = jointIndex;
            cthis->m_CachedEntryIndex = -1;
            cthis->m_CachedRangeEntryIndex = i;

            return &(entry->attrib);
        }
    }

    return 0;
}

bool InclusivePartition::HasJoint(int jointIndex) const
{
    return (FindEntry(jointIndex) != 0);
}

float InclusivePartition::GetJointWeight(int jointIndex) const
{
    const JointAttrib* attrib = FindEntry(jointIndex);
    if (attrib == 0)
        return 0.0f;
    return attrib->weight;
}

int InclusivePartition::GetJointPriority(int jointIndex) const
{
    const JointAttrib* attrib = FindEntry(jointIndex);
    if (attrib == 0)
        return 0;
    return attrib->priority;
}


bool InclusivePartition::ReadFromScript(ScriptReader* script,
                                        BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_INCLUSIVEPARTITION) != 0)
        return false;
    
    script->BeginStruct(TOKEN_INCLUSIVEPARTITION);
    
    InclusivePartition* partition = 0;
    
    // required params
    char partitionName[256] = "";
    script->RequiredParamString(TOKEN_NAME, partitionName, sizeof(partitionName));
    
    // optional params
    char rigName[256] = "";
    script->ParamString(TOKEN_RIG, rigName, sizeof(rigName));
    
    // find rig
    Rig* rig = bank->GetRig();
    if (rig == 0)
    {
        if (strlen(rigName) == 0)
        {
            script->SetErrorString("No rig specified.");
        }
        else
        {
            rig = choreo::find<Rig>(bank, rigName);
            
            if (rig == 0)
            {
                script->SetErrorFormat("Could not find rig \"%s\".", rigName);
            }
        }
    }
    else
    {
        tUID rigUID = tEntity::MakeUID(rigName);

        if ((rigUID != static_cast< tUID >( 0 ) ) &&
            (rigUID != rig->GetUID()))
        {
            script->SetErrorString("Rig incorrectly specified for in-bank partition.");
        }
    }
    
    // struct (entry) counts
    int entryCount = script->GetStructCount(TOKEN_JOINT);
    entryCount += script->GetStructCount(TOKEN_JOINTINDEX);

    int rangeEntryCount = script->GetStructCount(TOKEN_JOINTRANGE);
    
    if (!script->IsError())
    {
        partition = new InclusivePartition(rig, entryCount, rangeEntryCount);
        partition->AddRef();
        partition->SetName(partitionName);
        
        while (script->IsMoreStructs())
        {
            if (strcmp(script->GetNextStructIdentifier(), TOKEN_JOINT) == 0)
            {
                char jointName[256];
                float jointWeight = 1.0f;
                float jointPriority = 0.0f;
                
                script->BeginStruct(TOKEN_JOINT);
                
                script->RequiredParamString(TOKEN_NAME, jointName, sizeof(jointName));
                script->ParamFloat(TOKEN_WEIGHT, &jointWeight);
                script->ParamFloat(TOKEN_PRIORITY, &jointPriority);
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    tSkeleton* skeleton = rig->GetSkeleton();
                    int jointIndex = skeleton->FindJointIndex(jointName);
                    
                    if (jointIndex < 0)
                    {
                        script->SetErrorFormat("Could not find joint named \"%s\" in skeleton.",
                            jointName);
                    }
                    else
                    {
                        if (!partition->AddEntry(jointIndex, jointWeight, (int)jointPriority))
                        {
                            script->SetErrorFormat("Failed to add joint named \"%s\", possible duplicate.", jointName);
                        }
                    }
                }
            }
            else if (strcmp(script->GetNextStructIdentifier(), TOKEN_JOINTINDEX) == 0)
            {
                float jointIndex;
                float jointWeight = 1.0f;
                float jointPriority = 0.0f;
                
                script->BeginStruct(TOKEN_JOINTINDEX);
                
                script->RequiredParamFloat(TOKEN_INDEX, &jointIndex);
                script->ParamFloat(TOKEN_WEIGHT, &jointWeight);
                script->ParamFloat(TOKEN_PRIORITY, &jointPriority);
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    int iJointIndex = (int)jointIndex;

                    if (iJointIndex < 0)
                    {
                        script->SetErrorFormat("Joint index %d out of range.", iJointIndex);
                    }
                    else if (!partition->AddEntry(iJointIndex, jointWeight, (int)jointPriority))
                    {
                        script->SetErrorFormat("Failed to add joint index %d, possible duplicate.", iJointIndex);
                    }
                }
            }
            else if (strcmp(script->GetNextStructIdentifier(), TOKEN_JOINTRANGE) == 0)
            {
                float jointMin;
                float jointMax;
                float jointWeight = 1.0f;
                float jointPriority = 0.0f;

                script->BeginStruct(TOKEN_JOINTRANGE);

                script->RequiredParamFloat(TOKEN_MIN, &jointMin);
                script->RequiredParamFloat(TOKEN_MAX, &jointMax);
                script->ParamFloat(TOKEN_WEIGHT, &jointWeight);
                script->ParamFloat(TOKEN_PRIORITY, &jointPriority);

                script->EndStruct();

                if (!script->IsError())
                {
                    int iMin = (int)jointMin;
                    int iMax = (int)jointMax;

                    if ((iMin < 0) || (iMax < iMin))
                    {
                        script->SetErrorFormat("Invalid index range values of %d to %d.", iMin, iMax);
                    }
                    else if (!partition->AddRangeEntry(iMin, iMax, jointWeight, (int)jointPriority))
                    {
                        script->SetErrorFormat("Failed to add joint range %d to %d.", iMin, iMax);
                    }
                }
            }
            else
            {
                script->SetErrorFormat("Unexpected token \"%s\".",
                                       script->GetNextStructIdentifier());
            }
        }
    }
    
    script->EndStruct();
    
    if (script->IsError())
    {
        tRefCounted::Release(partition);
        return script->HandleError();
    }
    else
    {
        bank->Store(partition);
        tRefCounted::Release(partition);
        return true;
    }
}

#ifdef CHOREO_TOOL
bool InclusivePartition::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(InclusivePartition, script, bank);
}

bool InclusivePartition::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_INCLUSIVEPARTITION);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());

    // optional params
    script->ParamString(TOKEN_RIG, GetRig()->GetName());

    // joints
    int i;
    for (i = 0; i < m_EntryCount; ++i)
    {
        script->BeginStruct(TOKEN_JOINTINDEX);

        // required params
        script->RequiredParamFloat(TOKEN_INDEX, (float)m_Entries[i].jointIndex);

        // optional params
        script->ParamFloat(TOKEN_WEIGHT, m_Entries[i].attrib.weight);
        script->ParamFloat(TOKEN_PRIORITY, (float)m_Entries[i].attrib.priority);

        script->EndStruct();
    }

    // joint ranges
    for (i = 0; i < m_RangeEntryCount; ++i)
    {
        script->BeginStruct(TOKEN_JOINTRANGE);

        // required params
        script->RequiredParamFloat(TOKEN_MIN, (float)m_RangeEntries[i].jointMin);
        script->RequiredParamFloat(TOKEN_MAX, (float)m_RangeEntries[i].jointMax);

        // optional params
        script->ParamFloat(TOKEN_WEIGHT, m_RangeEntries[i].attrib.weight);
        script->ParamFloat(TOKEN_PRIORITY, (float)m_RangeEntries[i].attrib.priority);

        script->EndStruct();
    }

    script->EndStruct();
    return true;
}
#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// class ExclusivePartition
//---------------------------------------------------------------------------

ExclusivePartition::ExclusivePartition(Rig* rig, int maxEntryCount, int maxRangeEntryCount):
    Partition(rig),

    m_MaxEntryCount(maxEntryCount),
    m_EntryCount(0),

    m_MaxRangeEntryCount(maxRangeEntryCount),
    m_RangeEntryCount(0),

    m_Weight(1.0f),
    m_Priority(0)
{
    if (m_MaxEntryCount > 0)
    {
        m_Entries = new int [m_MaxEntryCount];
    }
    else
    {
        m_Entries = 0;
    }

    if (m_MaxRangeEntryCount > 0)
    {
        m_RangeEntries = new RangeEntry [m_MaxRangeEntryCount];
    }
    else
    {
        m_RangeEntries = 0;
    }
}

ExclusivePartition::~ExclusivePartition()
{
    delete[] m_RangeEntries;
    delete[] m_Entries;
}

bool ExclusivePartition::AddEntry(int jointIndex)
{
    P3DASSERT(m_EntryCount < m_MaxEntryCount);
    if (m_EntryCount >= m_MaxEntryCount)
        return false;
    
    int i;
    for (i = 0; i < m_EntryCount; ++i)
    {
        // for now, don't allow duplicate or replacement of entries
        P3DASSERT(jointIndex != m_Entries[i]);
        if (jointIndex == m_Entries[i])
            return false;
        
        if (jointIndex < m_Entries[i])
        {
            break;
        }
    }
    
    memmove(m_Entries + i + 1,
            m_Entries + i,
            (m_EntryCount - i) * sizeof(int));
    
    m_Entries[i] = jointIndex;
    ++m_EntryCount;
    
    return true;
}

bool ExclusivePartition::AddRangeEntry(int jointMin, int jointMax)
{
    P3DASSERT(m_RangeEntryCount < m_MaxRangeEntryCount);
    if (m_RangeEntryCount >= m_MaxRangeEntryCount)
        return false;

    RangeEntry* entry = m_RangeEntries + m_RangeEntryCount;
    entry->jointMin = jointMin;
    entry->jointMax = jointMax;

    ++m_RangeEntryCount;

    return true;
}

int ExclusivePartition::FindEntryIndex(int jointIndex) const
{
    // if we have no joints
    if (m_EntryCount == 0)
        return -1;
    
    // binary search for joint index
    int iLeft = 0;
    int iRight = m_EntryCount - 1;
    int iCurrent = iRight / 2;
    
    while (1)
    {
        if (jointIndex == m_Entries[iCurrent])
        {
            // found joint index
            return iCurrent;
        }
        
        if (iLeft >= iRight)
        {
            // didn't find joint index
            return -1;
        }
        
        if (jointIndex < m_Entries[iCurrent])
        {
            iRight = iCurrent - 1;
        }
        else
        {
            iLeft = iCurrent + 1;
        }
        
        iCurrent = (iLeft + iRight) / 2;
    }
    
    return -1;
}

int ExclusivePartition::FindRangeEntryIndex(int jointIndex) const
{
    for (int i = 0; i < m_RangeEntryCount; ++i)
    {
        const RangeEntry* entry = m_RangeEntries + i;

        if ((jointIndex >= entry->jointMin) &&
            (jointIndex <= entry->jointMax))
        {
            return i;
        }
    }

    return -1;
}

bool ExclusivePartition::HasJoint(int jointIndex) const
{
    // return true if we HAVEN'T found it
    if (FindEntryIndex(jointIndex) >= 0)
        return false;
    if (FindRangeEntryIndex(jointIndex) >= 0)
        return false;
    return true;
}


//---------------------------------------------------------------------------
// class ExclusivePartition - loading
//---------------------------------------------------------------------------

bool ExclusivePartition::ReadFromScript(ScriptReader* script,
                                        BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_EXCLUSIVEPARTITION) != 0)
        return false;
    
    script->BeginStruct(TOKEN_EXCLUSIVEPARTITION);
    
    ExclusivePartition* partition = 0;
    
    // required params
    char partitionName[256] = "";
    script->RequiredParamString(TOKEN_NAME, partitionName, sizeof(partitionName));
    
    // optional params
    char rigName[256] = "";
    script->ParamString(TOKEN_RIG, rigName, sizeof(rigName));
    float weight = 1.0f;
    script->ParamFloat(TOKEN_WEIGHT, &weight);
    float priority = 0.0f;
    script->ParamFloat(TOKEN_PRIORITY, &priority);
    
    // find rig
    Rig* rig = bank->GetRig();
    if (rig == 0)
    {
        if (strlen(rigName) == 0)
        {
            script->SetErrorString("No rig specified.");
        }
        else
        {
            rig = choreo::find<Rig>(bank, rigName);
            
            if (rig == 0)
            {
                script->SetErrorFormat("Could not find rig \"%s\".", rigName);
            }
        }
    }
    else
    {
        tUID rigUID = tEntity::MakeUID(rigName);

        if ((rigUID != static_cast< tUID >( 0 ) ) &&
            (rigUID != rig->GetUID()))
        {
            script->SetErrorString("Rig incorrectly specified for in-bank partition.");
        }
    }
    
    // struct (entry) count
    int entryCount = script->GetStructCount(TOKEN_JOINT);
    entryCount += script->GetStructCount(TOKEN_JOINTINDEX);

    int rangeEntryCount = script->GetStructCount(TOKEN_JOINTRANGE);
    
    if (!script->IsError())
    {
        partition = new ExclusivePartition(rig, entryCount, rangeEntryCount);
        partition->AddRef();
        partition->SetName(partitionName);
        
        while (script->IsMoreStructs())
        {
            if (strcmp(script->GetNextStructIdentifier(), TOKEN_JOINT) == 0)
            {
                script->BeginStruct(TOKEN_JOINT);
                
                char jointName[256];
                script->RequiredParamString(TOKEN_NAME, jointName, sizeof(jointName));
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    tSkeleton* skeleton = rig->GetSkeleton();
                    int jointIndex = skeleton->FindJointIndex(jointName);
                    
                    if (jointIndex < 0)
                    {
                        script->SetErrorFormat("Could not find joint named \"%s\" in skeleton.",
                                               jointName);
                    }
                    else if (!partition->AddEntry(jointIndex))
                    {
                        script->SetErrorFormat("Failed to add joint \"%s\", possible duplicate.", jointName);
                    }
                }
            }
            else if (strcmp(script->GetNextStructIdentifier(), TOKEN_JOINTINDEX) == 0)
            {
                script->BeginStruct(TOKEN_JOINTINDEX);
                
                float jointIndex;
                script->RequiredParamFloat(TOKEN_INDEX, &jointIndex);
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    if (!partition->AddEntry((int)jointIndex))
                    {
                        script->SetErrorFormat("Failed to add joint %d, possible duplicate.", (int)jointIndex);
                    }
                }
            }
            else if (strcmp(script->GetNextStructIdentifier(), TOKEN_JOINTRANGE) == 0)
            {
                script->BeginStruct(TOKEN_JOINTRANGE);

                float jointMin;
                script->RequiredParamFloat(TOKEN_MIN, &jointMin);

                float jointMax;
                script->RequiredParamFloat(TOKEN_MAX, &jointMax);

                script->EndStruct();

                if (!script->IsError())
                {
                    int iMin = (int)jointMin;
                    int iMax = (int)jointMax;

                    if ((iMin < 0) || (iMax < iMin))
                    {
                        script->SetErrorFormat("Invalid index range values of %d to %d.", iMin, iMax);
                    }
                    else if (!partition->AddRangeEntry(iMin, iMax))
                    {
                        script->SetErrorFormat("Failed to add joint range %d to %d.", iMin, iMax);
                    }
                }
            }
            else
            {
                script->SetErrorFormat("Unexpected token \"%s\".",
                                       script->GetNextStructIdentifier());
            }
        }
    }
    
    script->EndStruct();
    
    if (script->IsError())
    {
        tRefCounted::Release(partition);
        return script->HandleError();
    }
    else
    {
        partition->SetWeight(weight);
        partition->SetPriority((int)priority);
        
        bank->Store(partition);
        tRefCounted::Release(partition);
        return true;
    }
    
    return false;
}

#ifdef CHOREO_TOOL
bool ExclusivePartition::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(ExclusivePartition, script, bank);
}

bool ExclusivePartition::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_EXCLUSIVEPARTITION);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());

    // optional params
    script->ParamString(TOKEN_RIG, GetRig()->GetName());
    script->ParamFloat(TOKEN_WEIGHT, m_Weight);
    script->ParamFloat(TOKEN_PRIORITY, (float)m_Priority);

    // joints
    for (int i = 0; i < m_EntryCount; ++i)
    {
        script->BeginStruct(TOKEN_JOINTINDEX);

        // required params
        script->RequiredParamFloat(TOKEN_INDEX, (float)m_Entries[i]);

        script->EndStruct();
    }

    script->EndStruct();
    return true;
}
#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// class UnionPartition
//---------------------------------------------------------------------------

UnionPartition::UnionPartition(Rig* rig, int maxPartitionCount):
    Partition(rig),
    m_MaxEntryCount(maxPartitionCount),
    m_EntryCount(0)
{
    m_Entries = new Entry [m_MaxEntryCount];
}

UnionPartition::~UnionPartition()
{
    for (int i = (m_EntryCount - 1); i >= 0; --i)
    {
        m_Entries[i].partition->Release();
    }
    
    delete[] m_Entries;
}

Partition* UnionPartition::GetPartition(int index) const
{
    P3DASSERT((index >= 0) && (index < m_EntryCount));
    if ((index < 0) || (index >= m_EntryCount))
        return 0;
    
    return m_Entries[index].partition;
}

int UnionPartition::GetPartitionPriority(int index) const
{
    P3DASSERT((index >= 0) && (index < m_EntryCount));
    if ((index < 0) || (index >= m_EntryCount))
        return -1;
    
    return m_Entries[index].priority;
}

float UnionPartition::GetPartitionWeight(int index) const
{
    P3DASSERT((index >= 0) && (index < m_EntryCount));
    if ((index < 0) || (index >= m_EntryCount))
        return -1;
    
    return m_Entries[index].weight;
}

int UnionPartition::FindPartitionIndex(Partition* partition) const
{
    for (int i = 0; i < m_EntryCount; ++i)
    {
        if (m_Entries[i].partition == partition)
            return i;
    }

    return -1;
}

bool UnionPartition::AddPartition(Partition* partition,
                                  int priority,
                                  float weight)
{
    P3DASSERT(m_EntryCount < m_MaxEntryCount);
    if (m_EntryCount >= m_MaxEntryCount)
        return false;
    
    P3DASSERT(partition != 0);
    if (partition == 0)
        return false;
    
    P3DASSERT(partition->GetRig() == GetRig());
    if (partition->GetRig() != GetRig())
        return false;
    
    Entry* entry = (m_Entries + m_EntryCount);
    entry->partition = partition;
    entry->partition->AddRef();
    entry->priority = priority;
    entry->weight = weight;
    
    ++m_EntryCount;
    
    return true;
}

bool UnionPartition::RemovePartition(int index)
{
    P3DASSERT((index >= 0) && (index < m_EntryCount));
    if ((index < 0) || (index >= m_EntryCount))
        return false;

    Entry* entry = (m_Entries + index);
    entry->partition->Release();

    --m_EntryCount;

    memmove(m_Entries + index,
            m_Entries + index + 1,
            (m_EntryCount - index) * sizeof(m_Entries[0]));

    return true;
}

bool UnionPartition::HasJoint(int jointIndex) const
{
    for (int i = 0; i < m_EntryCount; ++i)
    {
        if (m_Entries[i].partition->HasJoint(jointIndex))
        {
            return true;
        }
    }

    return false;
}

float UnionPartition::GetJointWeight(int jointIndex) const
{
    float weight = 1.0f;

    for (int i = 0; i < m_EntryCount; ++i)
    {
        if (m_Entries[i].partition->HasJoint(jointIndex))
        {
            weight *= m_Entries[i].weight;
            weight *= m_Entries[i].partition->GetJointWeight(jointIndex);
        }
    }

    return weight;
}

int UnionPartition::GetJointPriority(int jointIndex) const
{
    int priority = 0;

    for (int i = 0; i < m_EntryCount; ++i)
    {
        if (m_Entries[i].partition->HasJoint(jointIndex))
        {
            priority += m_Entries[i].priority;
            priority += m_Entries[i].partition->GetJointPriority(jointIndex);
        }
    }

    return priority;
}

bool UnionPartition::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_UNIONPARTITION) != 0)
        return false;
    
    script->BeginStruct(TOKEN_UNIONPARTITION);
    
    UnionPartition* unionPartition = 0;
    
    // required params
    char unionPartitionName[256] = "";
    script->RequiredParamString(TOKEN_NAME, unionPartitionName, sizeof(unionPartitionName));
    
    // optional params
    char rigName[256] = "";
    script->ParamString(TOKEN_RIG, rigName, sizeof(rigName));
    
    // find rig
    Rig* rig = bank->GetRig();
    if (rig == 0)
    {
        if (strlen(rigName) == 0)
        {
            script->SetErrorString("No rig specified.");
        }
        else
        {
            rig = choreo::find<Rig>(bank, rigName);
            
            if (rig == 0)
            {
                script->SetErrorFormat("Could not find rig \"%s\".", rigName);
            }
        }
    }
    else
    {
        tUID rigUID = tEntity::MakeUID(rigName);

        if ((rigUID != static_cast< tUID >( 0 )) &&
            (rigUID != rig->GetUID()))
        {
            script->SetErrorString("Rig incorrectly specified for in-bank partition.");
        }
    }
    
    // struct (entry) count
    int entryCount = script->GetStructCount(TOKEN_PARTITION);
    
    if (!script->IsError())
    {
        unionPartition = new UnionPartition(rig, entryCount);
        unionPartition->AddRef();
        unionPartition->SetName(unionPartitionName);
        
        while (script->IsMoreStructs())
        {
            if (strcmp(script->GetNextStructIdentifier(), TOKEN_PARTITION) == 0)
            {
                script->BeginStruct(TOKEN_PARTITION);
                
                char partitionName[256];
                script->RequiredParamString(TOKEN_NAME, partitionName, sizeof(partitionName));
                
                float priority = 0.0f;
                script->ParamFloat(TOKEN_PRIORITY, &priority);
                
                float weight = 1.0f;
                script->ParamFloat(TOKEN_WEIGHT, &weight);
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    Partition* partition = choreo::find<Partition>(bank, partitionName);
                    if (partition == 0)
                    {
                        script->SetErrorFormat("Could not find partition named \"%s\".",
                                               partitionName);
                    }
                    else
                    {
                        if (partition->GetRig() != unionPartition->GetRig())
                        {
                            script->SetErrorFormat("Partition \"%s\" has an incompatible rig.",
                                                   partitionName);
                        }
                        else
                        {
                            unionPartition->AddPartition(partition, (int)priority, weight);
                        }
                    }
                }
            }
            else
            {
                script->SetErrorFormat("Unexpected token \"%s\", expected \"%s\".",
                                       script->GetNextStructIdentifier(),
                                       TOKEN_PARTITION);
            }
        }
    }
    
    script->EndStruct();
    
    if (script->IsError())
    {
        tRefCounted::Release(unionPartition);
        return script->HandleError();
    }
    else
    {
        bank->Store(unionPartition);
        tRefCounted::Release(unionPartition);
        return true;
    }
    
    return false;
}

#ifdef CHOREO_TOOL
bool UnionPartition::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(UnionPartition, script, bank);
}

bool UnionPartition::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_UNIONPARTITION);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());

    // optional params
    script->ParamString(TOKEN_RIG, GetRig()->GetName());

    // sub-partitions
    for (int i = 0; i < m_EntryCount; ++i)
    {
        script->BeginStruct(TOKEN_PARTITION);

        // required params
        script->RequiredParamString(TOKEN_NAME, m_Entries[i].partition->GetName());

        // optional params
        script->ParamFloat(TOKEN_PRIORITY, (float)m_Entries[i].priority);
        script->ParamFloat(TOKEN_WEIGHT, m_Entries[i].weight);

        script->EndStruct();
    }

    script->EndStruct();
    return true;
}
#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// class IntersectPartition
//---------------------------------------------------------------------------

IntersectPartition::IntersectPartition(Rig* rig, int maxPartitionCount):
    Partition(rig),
    m_MaxEntryCount(maxPartitionCount),
    m_EntryCount(0)
{
    m_Entries = new Entry [m_MaxEntryCount];
}

IntersectPartition::~IntersectPartition()
{
    for (int i = (m_EntryCount - 1); i >= 0; --i)
    {
        m_Entries[i].partition->Release();
    }
    
    delete[] m_Entries;
}

Partition* IntersectPartition::GetPartition(int index) const
{
    P3DASSERT((index >= 0) && (index < m_EntryCount));
    if ((index < 0) || (index >= m_EntryCount))
        return 0;
    
    return m_Entries[index].partition;
}

int IntersectPartition::GetPartitionPriority(int index) const
{
    P3DASSERT((index >= 0) && (index < m_EntryCount));
    if ((index < 0) || (index >= m_EntryCount))
        return -1;
    
    return m_Entries[index].priority;
}

float IntersectPartition::GetPartitionWeight(int index) const
{
    P3DASSERT((index >= 0) && (index < m_EntryCount));
    if ((index < 0) || (index >= m_EntryCount))
        return -1;
    
    return m_Entries[index].weight;
}

int IntersectPartition::FindPartitionIndex(Partition* partition) const
{
    for (int i = 0; i < m_EntryCount; ++i)
    {
        if (m_Entries[i].partition == partition)
            return i;
    }

    return -1;
}

bool IntersectPartition::AddPartition(Partition* partition,
                                      int priority,
                                      float weight)
{
    P3DASSERT(m_EntryCount < m_MaxEntryCount);
    if (m_EntryCount >= m_MaxEntryCount)
        return false;
    
    P3DASSERT(partition != 0);
    if (partition == 0)
        return false;
    
    P3DASSERT(partition->GetRig() == GetRig());
    if (partition->GetRig() != GetRig())
        return false;
    
    Entry* entry = (m_Entries + m_EntryCount);
    entry->partition = partition;
    entry->partition->AddRef();
    entry->priority = priority;
    entry->weight = weight;
    
    ++m_EntryCount;
    
    return true;
}

bool IntersectPartition::RemovePartition(int index)
{
    P3DASSERT((index >= 0) && (index < m_EntryCount));
    if ((index < 0) || (index >= m_EntryCount))
        return false;

    Entry* entry = (m_Entries + index);
    entry->partition->Release();

    --m_EntryCount;

    memmove(m_Entries + index,
            m_Entries + index + 1,
            (m_EntryCount - index) * sizeof(m_Entries[0]));

    return true;
}

bool IntersectPartition::HasJoint(int jointIndex) const
{
    for (int i = 0; i < m_EntryCount; ++i)
    {
        if (!m_Entries[i].partition->HasJoint(jointIndex))
        {
            return false;
        }
    }

    return true;
}

float IntersectPartition::GetJointWeight(int jointIndex) const
{
    float weight = 1.0f;

    for (int i = 0; i < m_EntryCount; ++i)
    {
        P3DASSERT(m_Entries[i].partition->HasJoint(jointIndex));

        weight *= m_Entries[i].weight;
        weight *= m_Entries[i].partition->GetJointWeight(jointIndex);
    }

    return weight;
}

int IntersectPartition::GetJointPriority(int jointIndex) const
{
    int priority = 0;

    for (int i = 0; i < m_EntryCount; ++i)
    {
        P3DASSERT(m_Entries[i].partition->HasJoint(jointIndex));

        priority += m_Entries[i].priority;
        priority += m_Entries[i].partition->GetJointPriority(jointIndex);
    }

    return priority;
}

bool IntersectPartition::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_INTERSECTPARTITION) != 0)
        return false;
    
    script->BeginStruct(TOKEN_INTERSECTPARTITION);
    
    IntersectPartition* intersectPartition = 0;
    
    // required params
    char intersectPartitionName[256] = "";
    script->RequiredParamString(TOKEN_NAME, intersectPartitionName, sizeof(intersectPartitionName));
    
    // optional params
    char rigName[256] = "";
    script->ParamString(TOKEN_RIG, rigName, sizeof(rigName));
    
    // find rig
    Rig* rig = bank->GetRig();
    if (rig == 0)
    {
        if (strlen(rigName) == 0)
        {
            script->SetErrorString("No rig specified.");
        }
        else
        {
            rig = choreo::find<Rig>(bank, rigName);
            
            if (rig == 0)
            {
                script->SetErrorFormat("Could not find rig \"%s\".", rigName);
            }
        }
    }
    else
    {
        tUID rigUID = tEntity::MakeUID(rigName);

        if ((rigUID != static_cast< tUID >( 0 )) &&
            (rigUID != rig->GetUID()))
        {
            script->SetErrorString("Rig incorrectly specified for in-bank partition.");
        }
    }
    
    // struct (entry) count
    int entryCount = script->GetStructCount(TOKEN_PARTITION);
    
    if (!script->IsError())
    {
        intersectPartition = new IntersectPartition(rig, entryCount);
        intersectPartition->AddRef();
        intersectPartition->SetName(intersectPartitionName);
        
        while (script->IsMoreStructs())
        {
            if (strcmp(script->GetNextStructIdentifier(), TOKEN_PARTITION) == 0)
            {
                script->BeginStruct(TOKEN_PARTITION);
                
                char partitionName[256];
                script->RequiredParamString(TOKEN_NAME, partitionName, sizeof(partitionName));
                
                float priority = 0.0f;
                script->ParamFloat(TOKEN_PRIORITY, &priority);
                
                float weight = 1.0f;
                script->ParamFloat(TOKEN_WEIGHT, &weight);
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    Partition* partition = choreo::find<Partition>(bank, partitionName);
                    if (partition == 0)
                    {
                        script->SetErrorFormat("Could not find partition named \"%s\".",
                                               partitionName);
                    }
                    else
                    {
                        if (partition->GetRig() != intersectPartition->GetRig())
                        {
                            script->SetErrorFormat("Partition \"%s\" has an incompatible rig.",
                                                   partitionName);
                        }
                        else
                        {
                            intersectPartition->AddPartition(partition, (int)priority, weight);
                        }
                    }
                }
            }
            else
            {
                script->SetErrorFormat("Unexpected token \"%s\", expected \"%s\".",
                                       script->GetNextStructIdentifier(),
                                       TOKEN_PARTITION);
            }
        }
    }
    
    script->EndStruct();
    
    if (script->IsError())
    {
        tRefCounted::Release(intersectPartition);
        return script->HandleError();
    }
    else
    {
        bank->Store(intersectPartition);
        tRefCounted::Release(intersectPartition);
        return true;
    }
    
    return false;
}

#ifdef CHOREO_TOOL
bool IntersectPartition::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(IntersectPartition, script, bank);
}

bool IntersectPartition::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_INTERSECTPARTITION);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());

    // optional params
    script->ParamString(TOKEN_RIG, GetRig()->GetName());

    // sub-partitions
    for (int i = 0; i < m_EntryCount; ++i)
    {
        script->BeginStruct(TOKEN_PARTITION);

        // required params
        script->RequiredParamString(TOKEN_NAME, m_Entries[i].partition->GetName());

        // optional params
        script->ParamFloat(TOKEN_PRIORITY, (float)m_Entries[i].priority);
        script->ParamFloat(TOKEN_WEIGHT, m_Entries[i].weight);

        script->EndStruct();
    }

    script->EndStruct();
    return true;
}
#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// class CompletePartition
//---------------------------------------------------------------------------

CompletePartition::CompletePartition(Rig* rig):
    Partition(rig)
{
    m_Joints = new JointAttrib [GetRig()->GetJointCount()];
}

CompletePartition::~CompletePartition()
{
    delete[] m_Joints;
}

bool CompletePartition::HasJoint(int jointIndex) const
{
    int jointCount = GetRig()->GetJointCount();
    if (jointIndex >= jointCount)
        return false;
    return m_Joints[jointIndex].hasJoint;
}

bool CompletePartition::SetHasJoint(int jointIndex, bool hasJoint)
{
    int jointCount = GetRig()->GetJointCount();
    if (jointIndex >= jointCount)
        return false;
    m_Joints[jointIndex].hasJoint = hasJoint;
    return true;
}

float CompletePartition::GetJointWeight(int jointIndex) const
{
    return m_Joints[jointIndex].weight;
}

bool CompletePartition::SetJointWeight(int jointIndex, float weight)
{
    int jointCount = GetRig()->GetJointCount();
    if (jointIndex >= jointCount)
        return false;
    m_Joints[jointIndex].weight = weight;
    return true;
}

int CompletePartition::GetJointPriority(int jointIndex) const
{
    return m_Joints[jointIndex].priority;
}

bool CompletePartition::SetJointPriority(int jointIndex, int priority)
{
    int jointCount = GetRig()->GetJointCount();
    if (jointIndex >= jointCount)
        return false;
    m_Joints[jointIndex].priority = priority;
    return true;
}

bool CompletePartition::AddJoint(int jointIndex, float weight, int priority)
{
    if (jointIndex < 0)
        return false;
    int jointCount = GetRig()->GetJointCount();
    if (jointIndex >= jointCount)
        return false;

    JointAttrib* joint = (m_Joints + jointIndex);
    if (joint->hasJoint)
    {
        joint->weight *= weight;
        joint->priority += priority;
    }
    else
    {
        joint->hasJoint = true;
        joint->weight = weight;
        joint->priority = priority;
    }

    return true;
}

bool CompletePartition::AddJoint(tUID jointUID, float weight, int priority)
{
    int jointIndex = GetRig()->GetSkeleton()->FindJointIndex(jointUID);
    return AddJoint(jointIndex, weight, priority);
}

bool CompletePartition::AddJointRange(int jointMin, int jointMax, float weight, int priority)
{
    if (jointMin < 0)
    {
        jointMin = 0;
    }
    int jointCount = GetRig()->GetJointCount();
    if (jointMax >= jointCount)
    {
        jointMax = (jointCount - 1);
    }

    for (int i = jointMin; i <= jointMax; ++i)
    {
        if (!AddJoint(i, weight, priority))
            return false;
    }

    return true;
}

bool CompletePartition::AddJointRange(tUID jointMinUID, tUID jointMaxUID, float weight, int priority)
{
    tSkeleton* skeleton = GetRig()->GetSkeleton();
    int jointMinIndex = skeleton->FindJointIndex(jointMinUID);
    int jointMaxIndex = skeleton->FindJointIndex(jointMinUID);

    return AddJointRange(jointMinIndex, jointMaxIndex, weight, priority);
}

void CompletePartition::Clear()
{
    int jointCount = GetRig()->GetJointCount();
    for (int i = 0; i < jointCount; ++i)
    {
        m_Joints[i].hasJoint = false;
    }
}

bool CompletePartition::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    if (!script->IsMoreStructs())
        return false;
    if (strcmp(script->GetNextStructIdentifier(), TOKEN_COMPLETEPARTITION) != 0)
        return false;
    
    script->BeginStruct(TOKEN_COMPLETEPARTITION);
    
    CompletePartition* partition = 0;
    
    // required params
    char partitionName[256] = "";
    script->RequiredParamString(TOKEN_NAME, partitionName, sizeof(partitionName));
    
    // optional params
    char rigName[256] = "";
    script->ParamString(TOKEN_RIG, rigName, sizeof(rigName));
    
    // find rig
    Rig* rig = bank->GetRig();
    if (rig == 0)
    {
        if (strlen(rigName) == 0)
        {
            script->SetErrorString("No rig specified.");
        }
        else
        {
            rig = choreo::find<Rig>(bank, rigName);
            
            if (rig == 0)
            {
                script->SetErrorFormat("Could not find rig \"%s\".", rigName);
            }
        }
    }
    else
    {
        tUID rigUID = tEntity::MakeUID(rigName);

        if ((rigUID != static_cast< tUID >( 0 )) &&
            (rigUID != rig->GetUID()))
        {
            script->SetErrorString("Rig incorrectly specified for in-bank partition.");
        }
    }
    
    if (!script->IsError())
    {
        partition = new CompletePartition(rig);
        partition->AddRef();
        partition->SetName(partitionName);
        partition->Clear();
        
        while (script->IsMoreStructs())
        {
            if (strcmp(script->GetNextStructIdentifier(), TOKEN_JOINT) == 0)
            {
                char jointName[256];
                float jointWeight = 1.0f;
                float jointPriority = 0.0f;
                
                script->BeginStruct(TOKEN_JOINT);
                
                script->RequiredParamString(TOKEN_NAME, jointName, sizeof(jointName));
                script->ParamFloat(TOKEN_WEIGHT, &jointWeight);
                script->ParamFloat(TOKEN_PRIORITY, &jointPriority);
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    tSkeleton* skeleton = rig->GetSkeleton();
                    int jointIndex = skeleton->FindJointIndex(jointName);
                    
                    if (jointIndex < 0)
                    {
                        script->SetErrorFormat("Could not find joint named \"%s\" in skeleton.",
                            jointName);
                    }
                    else
                    {
                        if (!partition->AddJoint(jointIndex, jointWeight, (int)jointPriority))
                        {
                            script->SetErrorFormat("Failed to add joint named \"%s\".", jointName);
                        }
                    }
                }
            }
            else if (strcmp(script->GetNextStructIdentifier(), TOKEN_JOINTINDEX) == 0)
            {
                float jointIndex;
                float jointWeight = 1.0f;
                float jointPriority = 0.0f;
                
                script->BeginStruct(TOKEN_JOINTINDEX);
                
                script->RequiredParamFloat(TOKEN_INDEX, &jointIndex);
                script->ParamFloat(TOKEN_WEIGHT, &jointWeight);
                script->ParamFloat(TOKEN_PRIORITY, &jointPriority);
                
                script->EndStruct();
                
                if (!script->IsError())
                {
                    int iJointIndex = (int)jointIndex;

                    if ((iJointIndex < 0) || (iJointIndex >= rig->GetJointCount()))
                    {
                        script->SetErrorFormat("Joint index %d out of range.", iJointIndex);
                    }
                    else if (!partition->AddJoint(iJointIndex, jointWeight, (int)jointPriority))
                    {
                        script->SetErrorFormat("Failed to add joint index %d.", iJointIndex);
                    }
                }
            }
            else if (strcmp(script->GetNextStructIdentifier(), TOKEN_JOINTRANGE) == 0)
            {
                char jointMin[256];
                char jointMax[256];
                float jointWeight = 1.0f;
                float jointPriority = 0.0f;

                script->BeginStruct(TOKEN_JOINTRANGE);

                script->RequiredParamString(TOKEN_MIN, jointMin, sizeof(jointMin));
                script->RequiredParamString(TOKEN_MAX, jointMax, sizeof(jointMax));
                script->ParamFloat(TOKEN_WEIGHT, &jointWeight);
                script->ParamFloat(TOKEN_PRIORITY, &jointPriority);

                script->EndStruct();

                if (!script->IsError())
                {
                    tSkeleton* skeleton = rig->GetSkeleton();

                    int jointMinIndex = skeleton->FindJointIndex(jointMin);
                    if (jointMinIndex < 0)
                    {
                        script->SetErrorFormat("Could not find joint named \"%s\" in skeleton.",
                            jointMin);
                    }

                    int jointMaxIndex = skeleton->FindJointIndex(jointMax);
                    if (jointMaxIndex < 0)
                    {
                        script->SetErrorFormat("Could not find joint named \"%s\" in skeleton.",
                            jointMax);
                    }

                    if ((jointMinIndex < 0) || (jointMaxIndex < jointMinIndex) || (jointMaxIndex >= rig->GetJointCount()))
                    {
                        script->SetErrorFormat("Invalid joint range values of %s to %s.", jointMin, jointMax);
                    }
                    else if (!partition->AddJointRange(jointMinIndex, jointMaxIndex, jointWeight, (int)jointPriority))
                    {
                        script->SetErrorFormat("Failed to add joint range %s to %s.", jointMin, jointMax);
                    }
                }
            }
            else if (strcmp(script->GetNextStructIdentifier(), TOKEN_JOINTINDEXRANGE) == 0)
            {
                float jointMin;
                float jointMax;
                float jointWeight = 1.0f;
                float jointPriority = 0.0f;

                script->BeginStruct(TOKEN_JOINTINDEXRANGE);

                script->RequiredParamFloat(TOKEN_MIN, &jointMin);
                script->RequiredParamFloat(TOKEN_MAX, &jointMax);
                script->ParamFloat(TOKEN_WEIGHT, &jointWeight);
                script->ParamFloat(TOKEN_PRIORITY, &jointPriority);

                script->EndStruct();

                if (!script->IsError())
                {
                    int iMin = (int)jointMin;
                    int iMax = (int)jointMax;

                    if ((iMin < 0) || (iMax < iMin) || (iMax >= rig->GetJointCount()))
                    {
                        script->SetErrorFormat("Invalid index range values of %d to %d.", iMin, iMax);
                    }
                    else if (!partition->AddJointRange(iMin, iMax, jointWeight, (int)jointPriority))
                    {
                        script->SetErrorFormat("Failed to add joint range %d to %d.", iMin, iMax);
                    }
                }
            }
            else
            {
                script->SetErrorFormat("Unexpected token \"%s\".",
                                       script->GetNextStructIdentifier());
            }
        }
    }
    
    script->EndStruct();
    
    if (script->IsError())
    {
        tRefCounted::Release(partition);
        return script->HandleError();
    }
    else
    {
        bank->Store(partition);
        tRefCounted::Release(partition);
        return true;
    }

    return false;
}

#ifdef CHOREO_TOOL
bool CompletePartition::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    CHOREO_SCRIPT_WRITE_HANDLER(CompletePartition, script, bank);
}

bool CompletePartition::WriteToScript(ScriptWriter* script) const
{
    script->BeginStruct(TOKEN_COMPLETEPARTITION);

    // required params
    script->RequiredParamString(TOKEN_NAME, GetName());

    // optional params
    script->ParamString(TOKEN_RIG, GetRig()->GetName());

    // joints
    int i;
    for (i = 0; i < GetRig()->GetJointCount(); ++i)
    {
        if (m_Joints[i].hasJoint)
        {
            script->BeginStruct(TOKEN_JOINTINDEX);

            // required params
            script->RequiredParamFloat(TOKEN_INDEX, (float)i);

            // optional params
            script->ParamFloat(TOKEN_WEIGHT, m_Joints[i].weight);
            script->ParamFloat(TOKEN_PRIORITY, (float)m_Joints[i].priority);

            script->EndStruct();
        }
    }

    script->EndStruct();
    return true;
}
#endif // CHOREO_TOOL


//---------------------------------------------------------------------------
// class LODPartition
//---------------------------------------------------------------------------

LODPartition::LODPartition(Rig* rig):
    Partition(rig)
{
    int jointCount = GetRig()->GetJointCount();
    m_Joints = new JointAttrib [jointCount];
    m_ActiveJointCount = jointCount;
    m_LastActiveJointWeight = 1.0f;
}

LODPartition::~LODPartition()
{
    delete[] m_Joints;
}

bool LODPartition::HasJoint(int jointIndex) const
{
    int jointCount = GetRig()->GetJointCount();
    if (jointIndex >= jointCount)
        return false;
    return (m_Joints[jointIndex].activeIndex < m_ActiveJointCount);
}

float LODPartition::GetJointWeight(int jointIndex) const
{
    return m_Joints[jointIndex].weight;
}

int LODPartition::GetJointPriority(int jointIndex) const
{
    return m_Joints[jointIndex].priority;
}

void LODPartition::SetActiveJointCount(int activeJointCount)
{
    m_ActiveJointCount = activeJointCount;

    if (m_ActiveJointCount < 0)
    {
        m_ActiveJointCount = 0;
    }
    else
    {
        int jointCount = GetRig()->GetJointCount();
        if (m_ActiveJointCount > jointCount)
        {
            m_ActiveJointCount = jointCount;
        }
    }

    m_LastActiveJointWeight = 1.0f;
}

float LODPartition::GetLOD() const
{
    return (m_ActiveJointCount - 1.0f + m_LastActiveJointWeight) / (float)GetRig()->GetJointCount();
}

void LODPartition::SetLOD(float lod)
{
    int jointCount = GetRig()->GetJointCount();
    float activeJointCountf = lod * jointCount;
    if (activeJointCountf < 0.0f)
    {
        activeJointCountf = 0.0f;
    }

    m_ActiveJointCount = (int)activeJointCountf;
    if (m_ActiveJointCount >= jointCount)
    {
        m_ActiveJointCount = jointCount;
        m_LastActiveJointWeight = 1.0f;
        return;
    }

    m_LastActiveJointWeight = activeJointCountf - m_ActiveJointCount;
    if (rmt::Epsilon(m_LastActiveJointWeight, 0.0f))
    {
        m_LastActiveJointWeight = 1.0f;
    }
    else if (rmt::Epsilon(m_LastActiveJointWeight, 1.0f))
    {
        m_LastActiveJointWeight = 1.0f;
        ++m_ActiveJointCount;
    }
    else
    {
        ++m_ActiveJointCount;
    }
}

int LODPartition::GetJointActiveIndex(int jointIndex) const
{
    int jointCount = GetRig()->GetJointCount();
    if (jointIndex >= jointCount)
        return -1;
    return m_Joints[jointIndex].activeIndex;
}

bool LODPartition::SetJointActiveIndex(int jointIndex, int activeIndex)
{
    int jointCount = GetRig()->GetJointCount();
    if (jointIndex >= jointCount)
        return false;
    m_Joints[jointIndex].activeIndex = activeIndex;
    return true;
}

bool LODPartition::ReadFromScript(ScriptReader* script, BaseBank* bank)
{
    // TODO
    return false;
}

#ifdef CHOREO_TOOL
bool LODPartition::WriteToScriptHandler(ScriptWriter* script, BaseBank* bank)
{
    // TODO
    return false;
}

bool LODPartition::WriteToScript(ScriptWriter* script) const
{
    // TODO
    return false;
}
#endif // CHOREO_TOOL


} // namespace choreo


// End of file.
