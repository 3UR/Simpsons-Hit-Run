//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#ifndef _GAMEATTR_HPP_
#define _GAMEATTR_HPP_

#include <p3d/entity.hpp>
#include <p3d/loadmanager.hpp>
#include <string.h>

class tGameAttr : public tEntity
{
public:
    enum Type
    {
        INT,
        FLOAT,
        VECTOR,
        MATRIX,
        COLOUR,
        UNKNOWN
    };

    tGameAttr(const char* name, unsigned count);

    bool HasParam(const char* name, Type type) { return HasParam(tEntity::MakeUID(name), type); }
    bool HasParam(tUID name, Type type);

    int                GetInt(tUID paramName);
    float              GetFloat(tUID paramName);
    const rmt::Vector& GetVector(tUID paramName);
    const rmt::Matrix& GetMatrix(tUID paramName);
    tColour            GetColour(tUID paramName);

    int                GetInt(const char* paramName) { return GetInt(tEntity::MakeUID(paramName)); }
    float              GetFloat(const char* paramName) { return GetFloat(tEntity::MakeUID(paramName)); }
    const rmt::Vector& GetVector(const char* paramName) { return GetVector(tEntity::MakeUID(paramName)); }
    const rmt::Matrix& GetMatrix(const char* paramName) { return GetMatrix(tEntity::MakeUID(paramName)); }
    tColour            GetColour(const char* paramName) { return GetColour(tEntity::MakeUID(paramName)); }


protected:
    friend class tGameAttrLoader;
    ~tGameAttr();

    class BaseMap
    {
    public:
    
        BaseMap() : value(NULL) {}
        ~BaseMap();
        void SetName(char* name);
        void SetParam(void* param, int size);
        void SetType(Type t) { type = t; }
        
        tName name;
        Type type;
        char* value;
    };

    BaseMap* Get(tUID paramName, Type type);

    void  Set(char* paramName, Type t, void* data, unsigned size);
    void  SetInt(char* paramName, int v);
    void  SetFloat(char* paramName, float v);
    void  SetColour(char* paramName, tColour v);
    void  SetVector(char* paramName, const rmt::Vector& v);
    void  SetMatrix(char* paramName, const rmt::Matrix& v);
    
    void  SetNumParams(unsigned n) {numParams = n;}

    //the basic info
    unsigned numParams;       //how many attributes there are
    BaseMap* params;
};

class tGameAttrLoader : public tSimpleChunkHandler
{
public:
    tGameAttrLoader();
    tEntity* LoadObject(tChunkFile* file, tEntityStore* store);

protected:
    ~tGameAttrLoader() {}

};

#endif

