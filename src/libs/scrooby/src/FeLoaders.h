//=============================================================================
// Copyright (C) 2001 Radical Entertainment Ltd.  All Rights Reserved
//
// Component   : Scrooby
//
// Description : Loading classes for use with P3D loading
//
// Author(s)   : Mike Perzel(10/31/01)
//
//=============================================================================

// Recompillation Protection
#ifndef FE_LOADERS_H
#define FE_LOADERS_H

//=============================================================================
// Nested Includes
//=============================================================================
#include <p3d/loadmanager.hpp>
//=============================================================================
// Forward References
//=============================================================================
class FeEntity;
class FeOwner;
class FePage;
class FeProject;
class FeScreen;
class FeTextBible;
class XMLTree;
class PascalCString;

//=============================================================================
// Constants, Typedefs, and Macro Definitions
//=============================================================================

//=============================================================================
// Interface Definitions
//=============================================================================

class FeProjectFileHandler : public tFileHandler
//=============================================================================
// Description : A project loader which will use XML for parsing, but use the
//               p3d stores for it's object management
//
// Constraints : this should only be used by internal scrooby classes
//
//=============================================================================
{
public:
    FeProjectFileHandler();
    virtual bool CheckExtension( char* filename );
    virtual tLoadStatus Load( tFile* file, tEntityStore* store );
protected:
    ~FeProjectFileHandler();
    const char** m_extensions;
    
    // These handle the parsing for the individual XML types supported.
    FePage* LoadPage( XMLTree* loadTree );
    FeScreen* LoadScreen( XMLTree* loadTree );
    FeProject* LoadProject( XMLTree* loadTree );
    FeEntity* LoadTextStyle( XMLTree* loadTree );
    
    // Page parsing helper functions
    void ParsePageResources( XMLTree* pXmlTree, FePage* pOwningPage );
    
    void ParseDrawables( XMLTree* pXmlTree, FeOwner* pOwner );
    
    void ParseGroup( XMLTree* pXmlTree, FeOwner* pOwner );
    void ParseMovie( XMLTree* pXmlTree, FeOwner* pOwner );
    void ParsePolygon( XMLTree* pXmlTree, FeOwner* pOwner );
    void ParsePure3dObject( XMLTree* pXmlTree, FeOwner* pOwner );
    void ParseSprite( XMLTree* pXmlTree, FeOwner* pOwner );
    void ParseText( XMLTree* pXmlTree, FeOwner* pOwner );
    
    // This just makes our extension array a little easier to read
    enum eFileTypes
    {
        FT_XML,
        FT_PRJ,
        FT_SCR,
        FT_PAG,
        FT_TS,
        FT_NO_TYPE // adding this value makes it possible to use the enum for counting
    };
    
    // The XML parsing makes alot of temporary objects
    // This is a quick fix for this, until I can revisit the XML parsing.  Mike.
    PascalCString* m_tempString;
    
private:
};

class FeTextBibleLoader : public tSimpleChunkHandler
//=============================================================================
// Description : Loads a text bible from a pure3d chunk
//
// Constraints : Only used by the pure3d loading system
//=============================================================================
{
public:
    FeTextBibleLoader();
    virtual tEntity* LoadObject( tChunkFile* file, tEntityStore* store );
    tEntity* LoadLanguage( tChunkFile* file, tEntityStore* store );

    static void SetOnlyLoadCurrentLanguage( bool isEnabled );
    static void SetUseLastLoadedTextBible( bool isEnabled );

protected:
    static bool s_onlyLoadCurrentLanguage;
    static bool s_useLastLoadedTextBible;

    FeTextBible* m_lastLoadedTextBible;

};

inline void FeTextBibleLoader::SetOnlyLoadCurrentLanguage( bool isEnabled )
{
    s_onlyLoadCurrentLanguage = isEnabled;
}

inline void FeTextBibleLoader::SetUseLastLoadedTextBible( bool isEnabled )
{
    s_useLastLoadedTextBible = isEnabled;
}

// end conditional define
#endif
