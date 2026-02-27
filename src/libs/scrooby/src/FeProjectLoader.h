//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FeProjectLoader
//
// Description: This class reads an entire Scrooby project from disk,
//              to build and populate the FE data hierarchy.
//
//              Because file access is asynchromous, this class is derived
//              from AsyncFileLoaderCallback and implements the 
//              OnFileLoadComplete() member function.  
//              
//
// Authors:     Darwin Chau
//
// Revisions        Date            Author        Revision
//                  2000/07/06        DChau        Created
//
//===========================================================================

#ifndef FEPROJECTLOADER_H
#define FEPROJECTLOADER_H

//===========================================================================
// Nested Includes
//===========================================================================
#include "Project.h"
#include "ResourceManager/FeResourceManager.h"
#include "tLinearTable.h"
#include "utility/AsyncFileLoader.h"

//===========================================================================
// Forward References
//===========================================================================
class FeProject;
class FeScreen;
class FePage;
class FeOwner;
class XMLTree;

//===========================================================================
// Interface Definitions
//===========================================================================
class FeProjectLoader 
: 
    public AsyncFileLoaderCallback,
    public Scrooby::ResourceManagerCallback,
    public Scrooby::GotoScreenCallback,
    public FeAllocatesMemory
{
    public:

        FeProjectLoader();
        virtual ~FeProjectLoader();

        // 
        //
        void LoadProjectFile( const char* fullFilePath );



        
        // This is where we figure out what to do next!
        //
        virtual void OnFileLoadComplete( const unsigned int fileSize,
                                         char* fileDataBuffer );

        virtual void OnResourceLoadComplete();

        virtual void OnGotoScreenComplete();
        

    protected:

        //---------------------------------------------------------------------
        // Protected Functions
        //---------------------------------------------------------------------

        // No copying. Declare but don't define.
        //
        FeProjectLoader( const FeProjectLoader& );

        // No assignment. Declare but don't define.
        //
        FeProjectLoader& operator= ( const FeProjectLoader& );

        
        void HandleProjectFilePending( const unsigned int fileSize,
                                       char* fileDataBuffer );

        //parse the project file using one of the file formats
        void HandleProjectParsingXML( char* fileDataBuffer, size_t fileSize );
        void HandleProjectParsingP3D( char* fileDataBuffer, size_t fileSize );

        void StartScreenLoading();

        void LoadScreen( FeScreen* pScreen );

        void HandleScreenFilePending( const unsigned int fileSize,
                                      char* fileDataBuffer );

        void StartPageLoading();
        
        void LoadPage( FePage* pPage );

        void HandlePageFilePending( const unsigned int fileSize,
                                    char* fileDataBuffer );

        
        void ParsePageResources( XMLTree* pXmlTree, FePage* pOwningPage );
        
        void ParsePageLayers( XMLTree* pXmlTree, FePage* pOwningPage );

        void ParseDrawables( XMLTree* pXmlTree, FeOwner* pOwner ); 

        void ParseGroup( XMLTree* pXmlTree, FeOwner* );
        void ParsePolygon( XMLTree* pXmlTree, FeOwner* );
        void ParseSprite( XMLTree* pXmlTree, FeOwner* );
        void ParseText( XMLTree* pXmlTree, FeOwner* );
        void ParseMovie( XMLTree* pXmlTree, FeOwner* );
        void ParsePure3dObject( XMLTree* pXmlTree, FeOwner* );
        


        //---------------------------------------------------------------------
        // Private Data
        //---------------------------------------------------------------------

        // These states allow us to figure out 
        //
        enum FeLoadStateEnum
        {
            
            eProjectFilePending,
            eScreenFilePending,
            ePageFilePending
        };

        enum ProjectFileType
        {
            SCROOBY_INVALID = -1,
            SCROOBY_XML,
            SCROOBY_P3D
        };

        FeLoadStateEnum mCurrentState;
        ProjectFileType m_ProjectType;

        AsyncFileLoader* mFileLoader;

        
        // Create an FeProject here and pass it back to FeApp when's it's
        // completed loading.  FeApp is responsible for deleting it when
        // finished.
        //
        FeProject* mpProject;

        tLinearTable::RawIterator* mpScreenIterator;
        tLinearTable::RawIterator* mpPageIterator;
};

#endif


// DARWIN TODO: How to handle the page reloading triggered by FeScreen::Reload()?
