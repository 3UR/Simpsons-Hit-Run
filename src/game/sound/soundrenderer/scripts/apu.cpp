#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {
#pragma optimize("t", on)
void daSoundRenderingManager::RunApuSoundScripts( void )
{
    SetCurrentNameSpace( GetCharacterNamespace( SC_CHAR_APU ) );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_CHAR_APU );
    #include "apu.inl"
}
#pragma optimize("", on)
}
