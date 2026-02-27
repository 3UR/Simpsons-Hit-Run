/*
 * choreo/bank.hpp
 */

#ifndef CHOREO_BANK_HPP
#define CHOREO_BANK_HPP


#include <choreo/buildconfig.hpp>

#include <choreo/basebank.hpp>
#include <choreo/load.hpp>

// for convenience finds
#include <choreo/partition.hpp>
#include <choreo/animation.hpp>
#include <choreo/locomotion.hpp>


namespace choreo
{


//---------------------------------------------------------------------------
// class Bank
//---------------------------------------------------------------------------

class Bank: public BaseBank
{
public:

    enum
    {
        MAX_SCRIPT_READ_HANDLERS = 32,
        MAX_SCRIPT_WRITE_HANDLERS = 32
    };
    
    // constructor
    Bank(Rig* rig);
    
    // rig accessor
    virtual Rig* GetRig() const
        { return m_Rig; }
    
    // find routines
    virtual IRefCount* Find(radLoadInventory::SafeCastBase& safeCaster, tUID uid) const;

    // convenience find routines
    Partition* FindPartition(tUID uid) const
        { return choreo::find<Partition>(this, uid); }
    Partition* FindPartition(const char* name) const
        { return choreo::find<Partition>(this, name); }
    
    Animation* FindAnimation(tUID uid) const
        { return choreo::find<Animation>(this, uid); }
    Animation* FindAnimation(const char* name) const
        { return choreo::find<Animation>(this, name); }
    
    Locomotion* FindLocomotion(tUID uid) const
        { return choreo::find<Locomotion>(this, uid); }
    Locomotion* FindLocomotion(const char* name) const
        { return choreo::find<Locomotion>(this, name); }

    // store/remove routines
    virtual void Store(tEntity* entity);
    void Remove(tEntity* entity);
    void RemoveAll();

    // raw iterator
    class RawIterator: public BaseBank::RawIterator
    {
    public:

        RawIterator(Bank* bank);

        virtual IRefCount* First();
        virtual IRefCount* Last();
        virtual IRefCount* Next();
        virtual IRefCount* Prev();

    protected:

        virtual ~RawIterator()
            { }

    private:

        tEntityTable::RawIterator m_RawIterator;
    };

    virtual BaseBank::RawIterator* NewRawIterator();

    // load routines
    static bool ReadFromScript(ScriptReader* script, BaseBank* bank);
    
#ifdef CHOREO_TOOL
    static bool WriteToScriptHandler(ScriptWriter* script, BaseBank* bank);
    bool WriteToScript(ScriptWriter* script);
#endif // CHOREO_TOOL

    // handler registration
    static bool RegisterScriptReadHandler(ScriptReadHandler scriptHandler);
    static void RegisterDefaultScriptReadHandlers();

#ifdef CHOREO_TOOL
    static bool RegisterScriptWriteHandler(ScriptWriteHandler scriptHandler);
    static void RegisterDefaultScriptWriteHandlers();
#endif // CHOREO_TOOL

    static void RegisterDefaultScriptHandlers();
    
protected:
    
    virtual ~Bank();
    
private:

    friend class RawIterator;

    tEntityTable* m_EntityTable;
    Rig* m_Rig;

    static bool m_DefaultScriptReadHandlersRegistered;
    static int m_ScriptReadHandlerCount;
    static ScriptReadHandler m_ScriptReadHandlers[];

#ifdef CHOREO_TOOL
    static bool m_DefaultScriptWriteHandlersRegistered;
    static int m_ScriptWriteHandlerCount;
    static ScriptWriteHandler m_ScriptWriteHandlers[];
#endif // CHOREO_TOOL
};


} // namespace choreo


#endif
