#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {
#pragma optimize("t", on)
void daSoundRenderingManager::RunHomerSoundScripts( void )
{
    SetCurrentNameSpace( GetCharacterNamespace( SC_CHAR_HOMER ) );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_CHAR_HOMER );
    #include "homer.inl"
}
#pragma optimize("", on)
}
