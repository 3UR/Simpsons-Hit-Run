/*
 * choreo/partition.hpp
 */

#ifndef CHOREO_PARTITION_HPP
#define CHOREO_PARTITION_HPP


#include <p3d/entity.hpp>

#include <choreo/buildconfig.hpp>


class tSkeleton;
class tEntityStore;


namespace choreo
{


class Rig;
class ScriptReader;
class ScriptWriter;
class BaseBank;
class CompletePartition;


//---------------------------------------------------------------------------
// class Partition
//---------------------------------------------------------------------------

class Partition: public tEntity
{
public:

    Partition(Rig* rig);
    
    Rig* GetRig() const
        { return m_Rig; }
    
    virtual bool HasJoint(int jointIndex) const = 0;
    virtual float GetJointWeight(int jointIndex) const = 0;
    virtual int GetJointPriority(int jointIndex) const = 0;

    // bakes abstract partition contents into a newly allocated complete partition
    CompletePartition* Bake() const;
    
protected:
    
    virtual ~Partition();
    
private:
    
    Rig* m_Rig;
};


//---------------------------------------------------------------------------
// class InclusivePartition
//---------------------------------------------------------------------------

class InclusivePartition: public Partition
{
public:

    InclusivePartition(Rig* rig, int maxEntryCount, int maxRangeEntryCount = 0);
    
    bool AddEntry(int jointIndex, float weight = 1.0f, int priority = 0);
    bool AddRangeEntry(int jointMin, int jointMax, float weight = 1.0f, int priority = 0);
    
    virtual bool HasJoint(int jointIndex) const;
    virtual float GetJointWeight(int jointIndex) const;
    virtual int GetJointPriority(int jointIndex) const;
    
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);

#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~InclusivePartition();
    
private:

    struct JointAttrib
    {
        float weight;
        int priority;
    };
    
    int FindEntryIndex(int jointIndex) const;
    const JointAttrib* FindEntry(int jointIndex) const;
    
    struct Entry
    {
        int jointIndex;
        JointAttrib attrib;
    };
    
    int m_MaxEntryCount;
    int m_EntryCount;
    Entry* m_Entries;

    struct RangeEntry
    {
        int jointMin;
        int jointMax;
        JointAttrib attrib;
    };

    int m_MaxRangeEntryCount;
    int m_RangeEntryCount;
    RangeEntry* m_RangeEntries;
    
    int m_CachedJointIndex;
    int m_CachedEntryIndex;
    int m_CachedRangeEntryIndex;
};


//---------------------------------------------------------------------------
// class ExclusivePartition
//---------------------------------------------------------------------------

class ExclusivePartition: public Partition
{
public:

    ExclusivePartition(Rig* rig, int maxEntryCount, int maxRangeEntryCount = 0);
    
    bool AddEntry(int jointIndex);
    bool AddRangeEntry(int jointMin, int jointMax);
    
    float GetWeight() const
        { return m_Weight; }
    void SetWeight(float weight)
        { m_Weight = weight; }
    
    int GetPriority() const
        { return m_Priority; }
    void SetPriority(int priority)
        { m_Priority = priority; }
    
    virtual bool HasJoint(int jointIndex) const;
    
    virtual float GetJointWeight(int jointIndex) const
        { return m_Weight; }
    virtual int GetJointPriority(int jointIndex) const
        { return m_Priority; }
    
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);

#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~ExclusivePartition();
    
private:
    
    int FindEntryIndex(int jointIndex) const;
    int FindRangeEntryIndex(int jointIndex) const;
    
    int m_MaxEntryCount;
    int m_EntryCount;
    int* m_Entries;

    struct RangeEntry
    {
        int jointMin;
        int jointMax;
    };

    int m_MaxRangeEntryCount;
    int m_RangeEntryCount;
    RangeEntry* m_RangeEntries;
    
    float m_Weight;
    int m_Priority;
};


//---------------------------------------------------------------------------
// class UnionPartition
//---------------------------------------------------------------------------

class UnionPartition: public Partition
{
public:

    UnionPartition(Rig* rig, int maxPartitionCount);
    
    int GetMaxPartitionCount() const
        { return m_MaxEntryCount; }
    int GetPartitionCount() const
        { return m_EntryCount; }
    Partition* GetPartition(int index) const;
    int GetPartitionPriority(int index) const;
    float GetPartitionWeight(int index) const;
    int FindPartitionIndex(Partition* partition) const;
    
    bool AddPartition(Partition* partition, int priority = 0, float weight = 1.0f);
    bool RemovePartition(int index);
    
    virtual bool HasJoint(int jointIndex) const;
    virtual float GetJointWeight(int jointIndex) const;
    virtual int GetJointPriority(int jointIndex) const;
    
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);

#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~UnionPartition();
    
private:
    
    struct Entry
    {
        Partition* partition;
        int priority;
        float weight;
    };
    
    int m_MaxEntryCount;
    int m_EntryCount;
    Entry* m_Entries;
};


//---------------------------------------------------------------------------
// class IntersectPartition
//---------------------------------------------------------------------------

class IntersectPartition: public Partition
{
public:

    IntersectPartition(Rig* rig, int maxPartitionCount);
    
    int GetMaxPartitionCount() const
        { return m_MaxEntryCount; }
    int GetPartitionCount() const
        { return m_EntryCount; }
    Partition* GetPartition(int index) const;
    int GetPartitionPriority(int index) const;
    float GetPartitionWeight(int index) const;
    int FindPartitionIndex(Partition* partition) const;
    
    bool AddPartition(Partition* partition, int priority = 0, float weight = 1.0f);
    bool RemovePartition(int index);

    virtual bool HasJoint(int jointIndex) const;
    virtual float GetJointWeight(int jointIndex) const;
    virtual int GetJointPriority(int jointIndex) const;
    
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);

#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:
    
    virtual ~IntersectPartition();
    
private:
    
    struct Entry
    {
        Partition* partition;
        int priority;
        float weight;
    };
    
    int m_MaxEntryCount;
    int m_EntryCount;
    Entry* m_Entries;
};


//---------------------------------------------------------------------------
// class CompletePartition
//---------------------------------------------------------------------------

class CompletePartition: public Partition
{
public:

    CompletePartition(Rig* rig);

    virtual bool HasJoint(int jointIndex) const;
    bool SetHasJoint(int jointIndex, bool hasJoint);

    virtual float GetJointWeight(int jointIndex) const;
    bool SetJointWeight(int jointIndex, float weight);

    virtual int GetJointPriority(int jointIndex) const;
    bool SetJointPriority(int jointIndex, int priority);

    bool AddJoint(int jointIndex, float weight = 1.0f, int priority = 0);
    bool AddJoint(tUID jointUID, float weight = 1.0f, int priority = 0);
    bool AddJointRange(int jointMin, int jointMax, float weight = 1.0f, int priority = 0);
    bool AddJointRange(tUID jointMinUID, tUID jointMaxUID, float weight = 1.0f, int priority = 0);
    void Clear();

    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);

#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:

    virtual ~CompletePartition();

private:

    struct JointAttrib
    {
        JointAttrib():
            hasJoint(true),
            weight(1.0f),
            priority(0)
            { }

        bool hasJoint;
        float weight;
        int priority;
    };

    JointAttrib* m_Joints;
};


//---------------------------------------------------------------------------
// class LODPartition
//---------------------------------------------------------------------------

class LODPartition: public Partition
{
public:

    LODPartition(Rig* rig);

    virtual bool HasJoint(int jointIndex) const;
    virtual float GetJointWeight(int jointIndex) const;
    virtual int GetJointPriority(int jointIndex) const;

    int GetActiveJointCount() const
        { return m_ActiveJointCount; }
    void SetActiveJointCount(int activeJointCount);

    float GetLOD() const;
    void SetLOD(float lod);
    float GetLastActiveJointWeight() const
        { return m_LastActiveJointWeight; }

    int GetJointActiveIndex(int jointIndex) const;
    bool SetJointActiveIndex(int jointIndex, int activeIndex);

    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);

#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script) const;
#endif // CHOREO_TOOL

protected:

    virtual ~LODPartition();

private:

    struct JointAttrib
    {
        JointAttrib():
            activeIndex(-1),
            weight(1.0f),
            priority(0)
            { }

        int activeIndex;
        float weight;
        int priority;
    };

    JointAttrib* m_Joints;
    int m_ActiveJointCount;
    float m_LastActiveJointWeight;
};


} // namespace choreo


#endif
