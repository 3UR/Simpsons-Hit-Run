//=============================================================================
// Copyright (C) 2002 Radical Entertainment Ltd.  All rights reserved.
//
// File:        soundclusternameenum.h
//
// Description: Definition for SoundClusterName enum
//
// History:     11/18/2002 + Created -- Darren
//
//=============================================================================

#ifndef SOUNDCLUSTERNAMEENUM_H
#define SOUNDCLUSTERNAMEENUM_H

#include <constants/vehicleenum.h>

enum SoundClusterName
{
    SC_ALWAYS_LOADED,
    SC_FRONTEND,
    SC_INGAME,

    SC_LEVEL_SUBURBS,
    SC_LEVEL_DOWNTOWN,
    SC_LEVEL_SEASIDE,

    SC_LEVEL1,
    SC_LEVEL2,
    SC_LEVEL3,
    SC_LEVEL4,
    SC_LEVEL5,
    SC_LEVEL6,
    SC_LEVEL7,
    SC_MINIGAME,

    SC_NEVER_LOADED,

    SC_CHAR_APU,
    SC_CHAR_BART,
    SC_CHAR_HOMER,
    SC_CHAR_LISA,
    SC_CHAR_MARGE,

    SC_CAR_BASE,

    SC_BART_V = SC_CAR_BASE,
    SC_APU_V,
    SC_SNAKE_V,
    SC_HOMER_V,
    SC_FAMIL_V,
    SC_GRAMP_V,
    SC_CLETU_V,
    SC_WIGGU_V,
    SC_KRUSTYKAR_NOTYETIN,
    SC_MARGE_V,
    SC_OTTOBUS_NOTYETIN,
    SC_MOESDUFFTRUCK_NOTYETIN,
    SC_SMITH_V,
    SC_FLANDERSMOTORHOME_NOTYETIN,
    SC_MCBAINHUMMER_NOTYETIN,
    SC_ALIEN_NOTYETIN,
    SC_ZOMBI_V,
    SC_MUNSTERS_NOTYETIN,
    SC_HUMMER2_NOTYETIN,
    SC_CVAN,
    SC_COMPACTA,
    SC_COMIC_V,
    SC_SKINN_V,
    SC_CCOLA,
    SC_CSEDAN,
    SC_CPOLICE,
    SC_CCELLA,
    SC_CCELLB,
    SC_CCELLC,
    SC_CCELLD,
    SC_MINIVANA,
    SC_PICKUPA,
    SC_TAXIA,
    SC_SPORTSA,
    SC_SPORTSB,
    SC_SUVA,
    SC_WAGONA,
    SC_HBIKE_V,
    SC_BURNS_V,
    SC_HONOR_V,
    SC_CARMOR,
    SC_CCURATOR,
    SC_CHEARS,
    SC_CKLIMO,
    SC_CLIMO,
    SC_CNERD,
    SC_FRINK_V,
    SC_CMILK,
    SC_CDONUT,
    SC_BBMAN_V,
    SC_BOOKB_V,
    SC_CARHOM_V,
    SC_ELECT_V,
    SC_FONE_V,
    SC_GRAMR_V,
    SC_MOE_V,
    SC_MRPLO_V,
    SC_OTTO_V,
    SC_PLOWK_V,
    SC_SCORP_V,
    SC_WILLI_V,
    SC_SEDANA,
    SC_SEDANB,
    SC_CBLBART,
    SC_CCUBE,
    SC_CDUFF,
    SC_CNONUP,
    SC_LISA_V,
    SC_KRUST_V,
    SC_COFFIN,
    SC_HALLO,
    SC_SHIP,
    SC_WITCHCAR,
    SC_HUSKA,
    SC_ATV_V,
    SC_DUNE_V,
    SC_HYPE_V,
    SC_KNIGH_V,
    SC_MONO_V,
    SC_OBLIT_V,
    SC_ROCKE_V,
    SC_AMBUL,
    SC_BURNSARM,
    SC_FISHTRUC,
    SC_GARBAGE,
    SC_ICECREAM,
    SC_ISTRUCK,
    SC_NUCTRUCK,
    SC_PIZZA,
    SC_SCHOOLBU,
    SC_VOTETRUC,
    SC_GLASTRUC,
    SC_CFIRE_V,
    SC_CBONE,
    SC_REDBRICK,

    SC_MAX_CLUSTERS
};

static_assert(SC_MAX_CLUSTERS == SC_CAR_BASE + VehicleEnum::NUM_VEHICLES,
    "Not enough sound clusters for all vehicles");

#endif // SOUNDCLUSTERNAMEENUM_H

