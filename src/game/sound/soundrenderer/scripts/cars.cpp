#include <sound/soundrenderer/soundrenderingmanager.h>
#include <sound/avatar/carsoundparameters.h>

namespace Sound {
#pragma optimize("t", on)
void daSoundRenderingManager::RunCarSoundScripts( void )
{
    SoundLoader* loader = GetSoundManager()->GetSoundLoader();
    SetCurrentNameSpace( GetSoundNamespace() );
    loader->SetCurrentCluster( SC_BART_V );
    #include "bart_v.inl"
    loader->SetCurrentCluster( SC_APU_V );
    #include "apu_v.inl"
    loader->SetCurrentCluster( SC_SNAKE_V );
    #include "snake_v.inl"
    loader->SetCurrentCluster( SC_HOMER_V );
    #include "homer_v.inl"
    loader->SetCurrentCluster( SC_FAMIL_V );
    #include "famil_v.inl"
    loader->SetCurrentCluster( SC_GRAMP_V );
    #include "gramp_v.inl"
    loader->SetCurrentCluster( SC_CLETU_V );
    #include "cletu_v.inl"
    loader->SetCurrentCluster( SC_WIGGU_V );
    #include "wiggu_v.inl"
    loader->SetCurrentCluster( SC_MARGE_V );
    #include "marge_v.inl"
    loader->SetCurrentCluster( SC_SMITH_V );
    #include "smith_v.inl"
    loader->SetCurrentCluster( SC_ZOMBI_V );
    #include "zombi_v.inl"
    loader->SetCurrentCluster( SC_CVAN );
    #include "cvan.inl"
    loader->SetCurrentCluster( SC_COMPACTA );
    #include "compacta.inl"
    loader->SetCurrentCluster( SC_COMIC_V );
    #include "comic_v.inl"
    loader->SetCurrentCluster( SC_SKINN_V );
    #include "skinn_v.inl"
    loader->SetCurrentCluster( SC_CCOLA );
    #include "ccola.inl"
    loader->SetCurrentCluster( SC_CSEDAN );
    #include "csedan.inl"
    loader->SetCurrentCluster( SC_CPOLICE );
    #include "cpolice.inl"
    loader->SetCurrentCluster( SC_CCELLA );
    #include "ccella.inl"
    loader->SetCurrentCluster( SC_CCELLB );
    #include "ccellb.inl"
    loader->SetCurrentCluster( SC_CCELLC );
    #include "ccellc.inl"
    loader->SetCurrentCluster( SC_CCELLD );
    #include "ccelld.inl"
    loader->SetCurrentCluster( SC_MINIVANA );
    #include "minivana_v.inl"
    loader->SetCurrentCluster( SC_PICKUPA );
    #include "pickupa.inl"
    loader->SetCurrentCluster( SC_TAXIA );
    #include "taxia_v.inl"
    loader->SetCurrentCluster( SC_SPORTSA );
    #include "sportsa.inl"
    loader->SetCurrentCluster( SC_SPORTSB );
    #include "sportsb.inl"
    loader->SetCurrentCluster( SC_SUVA );
    #include "suva.inl"
    loader->SetCurrentCluster( SC_WAGONA );
    #include "wagona.inl"
    loader->SetCurrentCluster( SC_HBIKE_V );
    #include "hbike_v.inl"
    loader->SetCurrentCluster( SC_BURNS_V );
    #include "burns_v.inl"
    loader->SetCurrentCluster( SC_HONOR_V );
    #include "honor_v.inl"
    loader->SetCurrentCluster( SC_CARMOR );
    #include "carmor.inl"
    loader->SetCurrentCluster( SC_CCURATOR );
    #include "ccurator.inl"
    loader->SetCurrentCluster( SC_CHEARS );
    #include "chears.inl"
    loader->SetCurrentCluster( SC_CKLIMO );
    #include "cklimo.inl"
    loader->SetCurrentCluster( SC_CLIMO );
    #include "climo.inl"
    loader->SetCurrentCluster( SC_CNERD );
    #include "cnerd.inl"
    loader->SetCurrentCluster( SC_FRINK_V );
    #include "frink_v.inl"
    loader->SetCurrentCluster( SC_CMILK );
    #include "cmilk.inl"
    loader->SetCurrentCluster( SC_CDONUT );
    #include "cdonut.inl"
    loader->SetCurrentCluster( SC_BBMAN_V );
    #include "bbman_v.inl"
    loader->SetCurrentCluster( SC_BOOKB_V );
    #include "bookb_v.inl"
    loader->SetCurrentCluster( SC_CARHOM_V );
    #include "carhom_v.inl"
    loader->SetCurrentCluster( SC_ELECT_V );
    #include "elect_v.inl"
    loader->SetCurrentCluster( SC_FONE_V );
    #include "fone_v.inl"
    loader->SetCurrentCluster( SC_GRAMR_V );
    #include "gramr_v.inl"
    loader->SetCurrentCluster( SC_MOE_V );
    #include "moe_v.inl"
    loader->SetCurrentCluster( SC_MRPLO_V );
    #include "mrplo_v.inl"
    loader->SetCurrentCluster( SC_OTTO_V );
    #include "otto_v.inl"
    loader->SetCurrentCluster( SC_PLOWK_V );
    #include "plowk_v.inl"
    loader->SetCurrentCluster( SC_SCORP_V );
    #include "scorp_v.inl"
    loader->SetCurrentCluster( SC_WILLI_V );
    #include "willi_v.inl"
    loader->SetCurrentCluster( SC_SEDANA );
    #include "sedana.inl"
    loader->SetCurrentCluster( SC_SEDANB );
    #include "sedanb.inl"
    loader->SetCurrentCluster( SC_CBLBART );
    #include "cblbart.inl"
    loader->SetCurrentCluster( SC_CCUBE );
    #include "ccube.inl"
    loader->SetCurrentCluster( SC_CDUFF );
    #include "cduff.inl"
    loader->SetCurrentCluster( SC_CNONUP );
    #include "cnonup.inl"
    loader->SetCurrentCluster( SC_LISA_V );
    #include "lisa_v.inl"
    loader->SetCurrentCluster( SC_KRUST_V );
    #include "krust_v.inl"
    loader->SetCurrentCluster( SC_COFFIN );
    #include "coffin.inl"
    loader->SetCurrentCluster( SC_HALLO );
    #include "hallo.inl"
    loader->SetCurrentCluster( SC_SHIP );
    #include "ship.inl"
    loader->SetCurrentCluster( SC_WITCHCAR );
    #include "witchcar.inl"
    loader->SetCurrentCluster( SC_HUSKA );
    #include "huska.inl"
    loader->SetCurrentCluster( SC_ATV_V );
    #include "atv_v.inl"
    loader->SetCurrentCluster( SC_DUNE_V );
    #include "dune_v.inl"
    loader->SetCurrentCluster( SC_HYPE_V );
    #include "hype_v.inl"
    loader->SetCurrentCluster( SC_KNIGH_V );
    #include "knigh_v.inl"
    loader->SetCurrentCluster( SC_MONO_V );
    #include "mono_v.inl"
    loader->SetCurrentCluster( SC_OBLIT_V );
    #include "oblit_v.inl"
    loader->SetCurrentCluster( SC_ROCKE_V );
    #include "rocke_v.inl"
    loader->SetCurrentCluster( SC_AMBUL );
    #include "ambul.inl"
    loader->SetCurrentCluster( SC_BURNSARM );
    #include "burnsarm.inl"
    loader->SetCurrentCluster( SC_FISHTRUC );
    #include "fishtruc.inl"
    loader->SetCurrentCluster( SC_GARBAGE );
    #include "garbage.inl"
    loader->SetCurrentCluster( SC_ICECREAM );
    #include "icecream.inl"
    loader->SetCurrentCluster( SC_ISTRUCK );
    #include "istruck.inl"
    loader->SetCurrentCluster( SC_NUCTRUCK );
    #include "nuctruck.inl"
    loader->SetCurrentCluster( SC_PIZZA );
    #include "pizza.inl"
    loader->SetCurrentCluster( SC_SCHOOLBU );
    #include "schoolbu.inl"
    loader->SetCurrentCluster( SC_VOTETRUC );
    #include "votetruc.inl"
    loader->SetCurrentCluster( SC_GLASTRUC );
    #include "glastruc.inl"
    loader->SetCurrentCluster( SC_CFIRE_V );
    #include "cfire_v.inl"
    loader->SetCurrentCluster( SC_CBONE );
    #include "cbone.inl"
    loader->SetCurrentCluster( SC_REDBRICK );
    #include "redbrick.inl"
}
#pragma optimize("", on)

}
