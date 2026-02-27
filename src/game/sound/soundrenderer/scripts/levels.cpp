#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {
#pragma optimize("t", on)
void daSoundRenderingManager::RunLevelSoundScripts( void )
{
    SoundLoader* loader = GetSoundManager()->GetSoundLoader();
    SetCurrentNameSpace( GetSoundNamespace() );
    loader->SetCurrentCluster( SC_LEVEL_SUBURBS );
    #include "suburbs.inl"
    loader->SetCurrentCluster( SC_LEVEL_DOWNTOWN );
    #include "downtown.inl"
    loader->SetCurrentCluster( SC_LEVEL_SEASIDE );
    #include "seaside.inl"
    loader->SetCurrentCluster( SC_LEVEL1 );
    #include "level1.inl"
    loader->SetCurrentCluster( SC_LEVEL2 );
    #include "level2.inl"
    loader->SetCurrentCluster( SC_LEVEL3 );
    #include "level3.inl"
    loader->SetCurrentCluster( SC_LEVEL4 );
    #include "level4.inl"
    loader->SetCurrentCluster( SC_LEVEL5 );
    #include "level5.inl"
    loader->SetCurrentCluster( SC_LEVEL6 );
    #include "level6.inl"
    loader->SetCurrentCluster( SC_LEVEL7 );
    #include "level7.inl"
    loader->SetCurrentCluster( SC_MINIGAME );
    #include "minigame.inl"
}
#pragma optimize("", on)
}
