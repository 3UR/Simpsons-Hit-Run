Create<globalSettings>("tuner")
    .SetMasterVolume ( 1.000000f )
#ifdef RAD_GAMECUBE
    .SetMasterVolume ( 0.670000f ) option GCN
#endif
    .SetSfxVolume ( 0.880000f )
    .SetMusicVolume ( 0.780000f )
#ifdef RAD_XBOX
    .SetMusicVolume ( 0.840000f )
#endif
#ifdef RAD_PC
    .SetMusicVolume ( 0.840000f )
#endif
    .SetDialogueVolume ( 1.000000f )
    .SetAmbienceVolume ( 0.750000f )
    .SetCarVolume ( 0.880000f )
    .SetPauseSfxVolume ( 0.000000f )
    .SetPauseCarVolume ( 0.000000f )
    .SetPauseMusicVolume ( 0.000000f )
    .SetPauseDialogueVolume ( 0.000000f )
    .SetPauseAmbienceVolume ( 0.000000f )
    .SetMissionScreenSfxVolume ( 0.000000f )
    .SetMissionScreenCarVolume ( 0.000000f )
    .SetMissionScreenMusicVolume ( 0.000000f )
    .SetMissionScreenDialogueVolume ( 1.000000f )
    .SetMissionScreenAmbienceVolume ( 0.000000f )
    .SetLetterboxSfxVolume ( 0.500000f )
    .SetLetterboxCarVolume ( 0.280000f )
    .SetLetterboxMusicVolume ( 0.290000f )
    .SetLetterboxDialogueVolume ( 1.000000f )
    .SetLetterboxAmbienceVolume ( 0.500000f )
    .SetDialogueSfxVolume ( 0.820000f )
    .SetDialogueCarVolume ( 0.680000f )
    .SetDialogueMusicVolume ( 0.780000f )
    .SetDialogueDialogueVolume ( 1.000000f )
    .SetDialogueAmbienceVolume ( 0.670000f )
    .SetStoreSfxVolume ( 0.800000f )
    .SetStoreCarVolume ( 0.000000f )
    .SetStoreMusicVolume ( 1.000000f )
    .SetStoreDialogueVolume ( 1.000000f )
    .SetStoreAmbienceVolume ( 0.000000f )
    .SetOnFootSfxVolume ( 1.000000f )
    .SetOnFootCarVolume ( 0.590000f )
    .SetOnFootMusicVolume ( 1.000000f )
    .SetOnFootDialogueVolume ( 0.900000f )
    .SetOnFootAmbienceVolume ( 1.000000f )
    .SetMinigameSfxVolume ( 0.700000f )
    .SetMinigameCarVolume ( 0.750000f )
    .SetMinigameMusicVolume ( 1.000000f )
    .SetMinigameDialogueVolume ( 1.000000f )
    .SetMinigameAmbienceVolume ( 1.000000f )
    .SetJustMusicSfxVolume ( 0.000000f )
    .SetJustMusicCarVolume ( 0.000000f )
    .SetJustMusicMusicVolume ( 1.000000f )
    .SetJustMusicDialogueVolume ( 0.000000f )
    .SetJustMusicAmbienceVolume ( 0.000000f )
    .SetCreditsSfxVolume ( 0.000000f )
    .SetCreditsCarVolume ( 0.000000f )
    .SetCreditsMusicVolume ( 1.000000f )
    .SetCreditsDialogueVolume ( 1.000000f )
    .SetCreditsAmbienceVolume ( 0.000000f )
    .SetSkidRoadClipName ( "skid" )
    .SetSkidDirtClipName ( "dirt_skid" )
    .SetFootstepRoadClipName ( "feet_concrete" )
    .SetFootstepMetalClipName ( "feet_metal" )
    .SetFootstepWoodClipName ( "feet_wood" )
    .SetPeeloutMin ( 0.100000f )
    .SetPeeloutMax ( 1.000000f )
    .SetPeeloutMaxTrim ( 0.800000f );
Create<reverbSettings>("pproom1")
    .SetGain ( 0.640000f )
    .SetFadeInTime ( 1780.000f )
    .SetFadeOutTime ( 1110.000f )
    .SetXboxRoom ( -500 )
    .SetXboxRoomHF ( -150 )
    .SetXboxRoomRolloffFactor ( 3.900000f )
    .SetXboxDecayTime ( 3.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.002000f )
    .SetXboxReverb ( 400 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 45.000000f )
    .SetXboxDensity ( 64.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 5 )
    .SetPS2Delay ( 0.550000f )
    .SetPS2Feedback ( 0.410000f )
    .SetGCPreDelay ( 19.000000f )
    .SetGCReverbTime ( 1330.000000f )
    .SetGCColoration ( 0.500000f )
    .SetGCDamping ( 0.500000f );
Create<reverbSettings>("pproom2")
    .SetGain ( 0.640000f )
    .SetFadeInTime ( 1560.000f )
    .SetFadeOutTime ( 500.0000f )
    .SetXboxRoom ( -500 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 3.900000f )
    .SetXboxDecayTime ( 4.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.080000f )
    .SetXboxReverb ( 100 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 67.000000f )
    .SetXboxDensity ( 55.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 5 )
    .SetPS2Delay ( 0.650000f )
    .SetPS2Feedback ( 0.430000f )
    .SetGCPreDelay ( 18.000000f )
    .SetGCReverbTime ( 3330.000000f )
    .SetGCColoration ( 0.270000f )
    .SetGCDamping ( 0.290000f );
Create<reverbSettings>("pproom3")
    .SetGain ( 0.670000f )
    .SetFadeInTime ( 500.0000f )
    .SetFadeOutTime ( 500.0000f )
    .SetXboxRoom ( -1000 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 1.800000f )
    .SetXboxDecayTime ( 3.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.002000f )
    .SetXboxReverb ( 53 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 37.000000f )
    .SetXboxDensity ( 57.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 5 )
    .SetPS2Delay ( 0.550000f )
    .SetPS2Feedback ( 0.470000f )
    .SetGCPreDelay ( 13.000000f )
    .SetGCReverbTime ( 3470.000000f )
    .SetGCColoration ( 0.400000f )
    .SetGCDamping ( 0.760000f );
Create<reverbSettings>("PP_tunnel_01")
    .SetGain ( 0.650000f )
    .SetFadeInTime ( 500.0000f )
    .SetFadeOutTime ( 500.0000f )
    .SetXboxRoom ( -1240 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 0.000000f )
    .SetXboxDecayTime ( 2.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.002000f )
    .SetXboxReverb ( 175 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 37.000000f )
    .SetXboxDensity ( 50.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 3 )
    .SetPS2Delay ( 0.470000f )
    .SetPS2Feedback ( 0.330000f )
    .SetGCPreDelay ( 40.000000f )
    .SetGCReverbTime ( 2670.000000f )
    .SetGCColoration ( 0.640000f )
    .SetGCDamping ( 0.500000f );
Create<reverbSettings>("PP_tunnel_02")
    .SetGain ( 0.660000f )
    .SetFadeInTime ( 500.0000f )
    .SetFadeOutTime ( 500.0000f )
    .SetXboxRoom ( -1000 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 0.000000f )
    .SetXboxDecayTime ( 3.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.002000f )
    .SetXboxReverb ( 53 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 100.000000f )
    .SetXboxDensity ( 100.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 1 )
    .SetPS2Delay ( 0.440000f )
    .SetPS2Feedback ( 0.330000f )
    .SetGCPreDelay ( 18.000000f )
    .SetGCReverbTime ( 2010.000000f )
    .SetGCColoration ( 0.390000f )
    .SetGCDamping ( 0.540000f );
Create<reverbSettings>("burns_tunnel")
    .SetGain ( 0.640000f )
    .SetFadeInTime ( 500.0000f )
    .SetFadeOutTime ( 500.0000f )
    .SetXboxRoom ( -1000 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 0.000000f )
    .SetXboxDecayTime ( 2.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.002000f )
    .SetXboxReverb ( 53 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 100.000000f )
    .SetXboxDensity ( 100.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 1 )
    .SetPS2Delay ( 0.370000f )
    .SetPS2Feedback ( 0.330000f )
    .SetGCPreDelay ( 0.500000f )
    .SetGCReverbTime ( 2080.000000f )
    .SetGCColoration ( 0.500000f )
    .SetGCDamping ( 0.500000f );
Create<reverbSettings>("mansion_interior")
    .SetGain ( 0.660000f )
    .SetFadeInTime ( 500.0000f )
    .SetFadeOutTime ( 500.0000f )
    .SetXboxRoom ( -1000 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 0.000000f )
    .SetXboxDecayTime ( 2.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.002000f )
    .SetXboxReverb ( 53 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 100.000000f )
    .SetXboxDensity ( 100.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 4 )
    .SetPS2Delay ( 0.940000f )
    .SetPS2Feedback ( 0.390000f )
    .SetGCPreDelay ( 0.500000f )
    .SetGCReverbTime ( 1050.000000f )
    .SetGCColoration ( 0.540000f )
    .SetGCDamping ( 0.500000f );
Create<reverbSettings>("sewers")
    .SetGain ( 0.670000f )
    .SetFadeInTime ( 500.0000f )
    .SetFadeOutTime ( 500.0000f )
    .SetXboxRoom ( -1000 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 0.000000f )
    .SetXboxDecayTime ( 3.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.110000f )
    .SetXboxReverb ( 53 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 100.000000f )
    .SetXboxDensity ( 100.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 5 )
    .SetPS2Delay ( 0.620000f )
    .SetPS2Feedback ( 0.330000f )
    .SetGCPreDelay ( 0.500000f )
    .SetGCReverbTime ( 3690.000000f )
    .SetGCColoration ( 0.500000f )
    .SetGCDamping ( 0.500000f );
Create<reverbSettings>("stonecuttertunnel")
    .SetGain ( 0.640000f )
    .SetFadeInTime ( 500.0000f )
    .SetFadeOutTime ( 500.0000f )
    .SetXboxRoom ( -1000 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 1.800000f )
    .SetXboxDecayTime ( 6.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.002000f )
    .SetXboxReverb ( 200 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 47.000000f )
    .SetXboxDensity ( 84.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 3 )
    .SetPS2Delay ( 0.410000f )
    .SetPS2Feedback ( 0.330000f )
    .SetGCPreDelay ( 21.000000f )
    .SetGCReverbTime ( 2590.000000f )
    .SetGCColoration ( 0.500000f )
    .SetGCDamping ( 0.500000f );
Create<reverbSettings>("stonecutterhall")
    .SetGain ( 0.670000f )
    .SetFadeInTime ( 500.0000f )
    .SetFadeOutTime ( 500.0000f )
    .SetXboxRoom ( -1000 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 3.600000f )
    .SetXboxDecayTime ( 3.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.002000f )
    .SetXboxReverb ( 53 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 100.000000f )
    .SetXboxDensity ( 100.000000f )
    .SetXboxHFReference ( 13930.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 5 )
    .SetPS2Delay ( 0.730000f )
    .SetPS2Feedback ( 0.330000f )
    .SetGCPreDelay ( 0.500000f )
    .SetGCReverbTime ( 3330.000000f )
    .SetGCColoration ( 0.500000f )
    .SetGCDamping ( 0.500000f );
Create<reverbSettings>("stadiumtunnel")
    .SetGain ( 0.690000f )
    .SetFadeInTime ( 1000.0000f )
    .SetFadeOutTime ( 6518.520020f )
    .SetXboxRoom ( -1000 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 0.000000f )
    .SetXboxDecayTime ( 4.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.002000f )
    .SetXboxReverb ( 53 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 100.000000f )
    .SetXboxDensity ( 100.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 1 )
    .SetPS2Delay ( 0.300000f )
    .SetPS2Feedback ( 0.330000f )
    .SetGCPreDelay ( 0.500000f )
    .SetGCReverbTime ( 1780.000000f )
    .SetGCColoration ( 0.500000f )
    .SetGCDamping ( 0.500000f );
Create<reverbSettings>("krustylu")
    .SetGain ( 0.550000f )
    .SetFadeInTime ( 500.0000f )
    .SetFadeOutTime ( 500.0000f )
    .SetXboxRoom ( -1000 )
    .SetXboxRoomHF ( -454 )
    .SetXboxRoomRolloffFactor ( 0.000000f )
    .SetXboxDecayTime ( 2.100000f )
    .SetXboxDecayHFRatio ( 0.830000f )
    .SetXboxReflections ( -1646 )
    .SetXboxReflectionsDelay ( 0.002000f )
    .SetXboxReverb ( 53 )
    .SetXboxReverbDelay ( 0.003000f )
    .SetXboxDiffusion ( 100.000000f )
    .SetXboxDensity ( 100.000000f )
    .SetXboxHFReference ( 1000.000000f )
    .SetWinEnvironmentDiffusion ( 1.000000f )
    .SetWinAirAbsorptionHF ( -5.000000f )
    .SetPS2ReverbMode ( 3 )
    .SetPS2Delay ( 0.310000f )
    .SetPS2Feedback ( 0.330000f )
    .SetGCPreDelay ( 10.000000f )
    .SetGCReverbTime ( 4300.000000f )
    .SetGCColoration ( 0.500000f )
    .SetGCDamping ( 0.500000f );
