# SRR2

> [!WARNING]  
> VERY UNSTABLE
> This is yet to support UWP, It still needs to be able to run normally on Windows first (Lots of bugs as of 09/02/2025)

# bigger todos

- when the game runs under visual studio debugger it fails to load assets even when their in the bin directory, this is most likely because of ancient file system code shouldnt be the hardest thing to fix just gotta find what function it is

# TODO
- Remove any references and code to things like RAB_XBOX, RAD_PS2 (These are for very old consols instead we will have RAD_UWP).
- Split SRR2 project to just be like a library maybe name it "game_code" then have the projects "game" (windows). and "game_uwp" (uwp) containing the entrypoint and init logic for the platform.
- Figure out how to bundle assets with the repo (They are too big to add to the repo) & right now assets have to be manually put in the build output dir we can do a post build script to copy them.
- README Explaining setup/installation.