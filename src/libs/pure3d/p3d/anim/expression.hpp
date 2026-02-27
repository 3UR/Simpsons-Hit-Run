/*=============================================================================
  expression.hpp
  21-Mar-2000
  Created by: Katrina

  Description: Pure3D expression classes for parameterized
                    facial expressions
  Copyright (c) 1997-2002 Radical Entertainment Ltd
  All Rights Reserved

=============================================================================*/

#ifndef _EXPRESSION_HPP
#define _EXPRESSION_HPP

#include <p3d/entity.hpp>
#include <p3d/inventory.hpp>
#include <p3d/loadmanager.hpp>
#include <p3d/error.hpp>
#include <p3d/array.hpp>

class tExpression;

const int P3D_MAX_EXPRESSION = 20;
const float P3D_MAX_STATE_VALUE = 1.0f;

enum p3dExpressionStage
{
    P3D_EXPRESSION_STAGE_1 = 0,
    P3D_EXPRESSION_STAGE_2,
    P3D_EXPRESSION_STAGE_3
};

class tExpression : public tEntity
{
public:
    tExpression();
    unsigned int GetIndex(unsigned k) const { P3DASSERT(k < nKeys); return keyIndices[k]; }
    unsigned int GetNumKeys() const         { return nKeys; }
    float GetKey(int k) const               { return keys[k]; }
    const float* GetKeys() const            { return keys; }
    int GetKeyIndex(float keyVal);
    float GetUpperBoundKey(float keyVal);
    float GetLowerBoundKey(float keyVal);
    bool PosKeysExist() const               { return posKeysExist; }
    bool NegKeysExist() const               { return negKeysExist; }

    static const float INVALID_EXPRESSION_KEYVAL;

protected:
    friend class tExpressionLoader;
    virtual ~tExpression();

    unsigned int* keyIndices;  // key indices - which indicate the offsetlist for each key
    float* keys;               // the key values in this expression e.g. an expression may have 2 key shapes at 0.5 and 1.0
    float* posKeys;            // sorted small to large where posKeys[0] is always 0
    float* negKeys;            // sorted small to large where negKeys[nNegKeys] is always 0
    unsigned int nKeys;
    unsigned int nPosKeys;
    unsigned int nNegKeys;
    bool posKeysExist;
    bool negKeysExist;
};

//----------------------------------------------------------------------------
// Description: Defines a collection of expressions (tExpression)
// Each expression is associated with a blendStage (see enum p3dExpressionStage)
//
class tExpressionGroup : public tEntity
{
public:
    tExpressionGroup();
    tExpressionGroup(int nExp);
        
    int GetNumExpression() const { return nExpression; }

    tExpression* GetExpression(const int i) const
    {
        P3DASSERT(i < nExpression);
        return expression[i];
    }
    tExpression* FindExpression(const tName& name, int* expIndex) const;

    p3dExpressionStage GetExpressionStage(const int i) const
    {
        P3DASSERT(i < nExpression);
        return stage[i];
    }

    void SetExpressionStage(int i, p3dExpressionStage s) { stage[i] = s; }
    p3dExpressionStage FindExpressionStage(const tName& name) const;
    tUID GetTarget() const { return target; }

protected:
    virtual ~tExpressionGroup();

    int                 nExpression; // number of expressions for the target
    tPtrArray<tExpression*>  expression;  // expressions
    tArray<p3dExpressionStage> stage;       // blend stage of each expression
    tUID                target;      // target entity (defined in p3d file)

    // Note: the target UID cannot be safely used with asynchronous loading if
    // the target mesh can be in a separate inventory section. Should consider
    // removing this
    friend class tExpressionGroupLoader;
};

//----------------------------------------------------------------------------
// Description: Performs the calculation needed to convert expression animation
// data to vertex offsets to be applied during each frame.Updated during each
// tExpressionFrameController update call.
//
class tExpressionMixer : public tEntity
{
public:
    tExpressionMixer();

    virtual void Update() = 0;

    virtual void     SetTarget(tEntity* t) = 0;
    virtual tEntity* GetTarget() const = 0;

    int GetNumExpression() const
    {
        if (expressionGroup == NULL) return 0;
        return expressionGroup->GetNumExpression();
    }

    virtual void SetExpressionGroup(tExpressionGroup* eg);
    tExpressionGroup* GetExpressionGroup() const { return expressionGroup; }


    // accessors for current state of a given expression
    // Get() functions will return 0.0 (neutral) if exp could not be found
    // note that neutral is a valid state if expression could be found
    void SetExpressionState(const int i, const float s)
    {
        P3DASSERT(i < GetNumExpression());
        state[i] = s;
        mixerReady = false;
    }

    void SetExpressionState(const tName& name, const float s);
    float GetExpressionState(const int i) const
    {
        P3DASSERT(i < GetNumExpression());
        return state[i];
    }

    float GetExpressionState(const tName& name) const;

    virtual bool ValidateExpressionGroup(tExpressionGroup* eg) = 0;

    void SetMixerReady(const bool ready) { mixerReady = ready; }
    bool IsMixerReady() const            { return mixerReady; }

    virtual void ResetToNeutral();

protected:
    virtual ~tExpressionMixer();

    // check that whatever object the expression is supposed
    // to be applied to matches the object the mixer is updating
    virtual bool ValidateTarget(tEntity* e) = 0;

    tExpressionGroup* expressionGroup; // expressions for the target
    float*            state;           // current state for each exp
    bool              mixerReady;      // mixer is up to date flag
};

//----------------------------------------------------------------------------
// Loaders
//----------------------------------------------------------------------------
class tExpressionLoader : public tSimpleChunkHandler
{
public:
    tExpressionLoader();
protected:
    virtual ~tExpressionLoader() {}
    tEntity* LoadObject(tChunkFile*, tEntityStore* store);
};

class tExpressionGroupLoader : public tSimpleChunkHandler
{
public:
    tExpressionGroupLoader();

protected:
    ~tExpressionGroupLoader() {}
    tEntity* LoadObject(tChunkFile*, tEntityStore* store);

    class tExpressionStore : public tEntityStore
    {
    public:
        tExpressionStore();

        virtual IRefCount* Find(radLoadInventory::SafeCastBase& c, const tUID uid);
        virtual void Store(tEntity* obj);
        virtual void StoreHandlingCollisions( tEntity* obj );

        void SetExpStore(tExpression** se)    { storedExp = se; }
        void SetPrimaryStore(tEntityStore* s) { tRefCounted::Assign(primaryStore, s); }

    protected:
        ~tExpressionStore();

        tExpression**  storedExp;
        tEntityStore* primaryStore;
    };
};

class tExpressionMixerLoader : public tSimpleChunkHandler
{
public:
    // MUST match definition in tlExpressionAnim.hpp!
    enum MixerType { Undefined, Pose, HSplineOffset, VertexOffset };

    tExpressionMixerLoader();
protected:
    ~tExpressionMixerLoader() {};
    tEntity* LoadObject(tChunkFile*, tEntityStore* store);
};


//----------------------------------------------------------------------------
// Description: Iterates through the keys array for this expression to 
// find a key value larger than given argument. Note that keys[] is a sorted
// array in ascending order (sorted during export).
//
// Parameters: a key value.
//
// Returns: Returns the next value of the next larger key. Or, if no larger
// key is found, returns the largest key.
//
//----------------------------------------------------------------------------/
inline float tExpression::GetUpperBoundKey(float keyVal)
{
    int i;
    
    if(keyVal < 0)
    {
        for (i=0; i < static_cast<int>(nNegKeys); i++)
        {
            if(negKeys[i] >= keyVal) return negKeys[i];
        }
        return negKeys[nPosKeys - 1];
    }
    else if(keyVal > 0)
    {
        for (i=0; i < static_cast<int>(nPosKeys); i++)
        {
            if(posKeys[i] >= keyVal) return posKeys[i];
        }
        return posKeys[nPosKeys - 1];
    }
    return INVALID_EXPRESSION_KEYVAL;
}

//----------------------------------------------------------------------------
// Description: Iterates through the keys array for this expression to 
// find the next smaller key. 
//
// Parameters: a key value.
//
// Returns: Returns the value of the next smaller key. Or, if no smaller
// key is found, returns the smallest key in the expression.
//
//----------------------------------------------------------------------------/
inline float tExpression::GetLowerBoundKey(float keyVal)
{
    int i;
    if(keyVal < 0)
    {
        for (i=(nNegKeys -1); i >= 0; i--)
        {
            if(negKeys[i] <= keyVal) return negKeys[i];
        }
        return negKeys[0];
    }
    else if(keyVal > 0)
    {
        for (i=(nPosKeys -1); i >= 0; i--)
        {
            if(posKeys[i] <= keyVal) return posKeys[i];
        }
        return posKeys[0];
    }
    return INVALID_EXPRESSION_KEYVAL;
}

#endif
