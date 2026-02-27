Create<daSoundResourceData>("cHears_car")
    .AddFilename ( "sound/carsound/hearse.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.620000f );
Create<daSoundResourceData>("cHears_horn")
    .AddFilename ( "sound/carsound/horn-hearse.rsd", 1.000000f )
    .SetTrim ( 0.870000f )
    .SetLooping ( false );
Create<daSoundResourceData>("cHears_overlay")
    .AddFilename ( "sound/carsound/siren.rsd", 1.000000f )
    .SetTrim ( 0.870000f )
    .SetLooping ( false );
