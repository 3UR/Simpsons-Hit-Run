#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {
#pragma optimize("t", on)
void daSoundRenderingManager::RunLisaSoundScripts( void )
{
    SetCurrentNameSpace( GetCharacterNamespace( SC_CHAR_LISA ) );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_CHAR_LISA );
    #include "lisa.inl"
}
#pragma optimize("", on)
}
