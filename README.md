# NOT MAINTAINED

This is currently not maintained due to the amount of issues and just how jank it is to work with I plan to sometime in the future rework everything and fix all the issues but do not know when for now.

# SRR2

SRR2 is a modified fork of _The Simpsons: Hit & Run_ with lots of enhancements, including C++20 support, x64, and VCPkg for easier third-party library management. 

## Fair Use Disclaimer

This repository and or build distributations contains a modified version of _The Simpsons: Hit & Run_, using its original source code and assets strictly for non-commercial, educational, and archival purposes under fair use. There is no intent to cause harm to the original work or its market.

- **End-User Restrictions: (YOU)** You are not permitted to sell or commercially distribute this project in any form. Anyone attempting to do so will be blocked if found and reported to wherever it is distributed.
- **Purpose:** Enhancement, preservation, and research.
- **Use of Copyrighted Material:** Assets and source code are used strictly for modifications and improvements.
- **No Commercial Intent:** This project is not sold, monetized, or distributed for profit. Donations are not accepted.

## Issues

If you encounter issues, please [create an issue](https://github.com/3UR/Simpsons-Hit-Run/issues/new).

## Installation

### Quick Installation (Pre-built)

### Windows

1. Download the latest build from the [Releases page](https://github.com/3UR/Simpsons-Hit-Run/releases/latest).
2. Extract the contents of the zip file to your desired location.
3. Navigate to the extracted folder and locate `SRR2.exe`.
4. You can now run the game. (Note: The game will crash without the required assets)

### Xbox (Will work on Windows too but requires controller for input; note - it will also act as the console version because it is)

**NOTE:** Sadly Xbox One is not supported right now so it will just crash Xbox Series is okay and another thing to note is on UWP no matter PC or Xbox there is a memory leak and issues with the `srrmemory.cpp` file which end up causing a crash.

1. Navigate to the [Releases page](https://github.com/3UR/Simpsons-Hit-Run/releases/latest).
2. Download a file that looks like `SRR2_UWP_X.X.X.X_x64_XXX.appx`.
3. Now navigate to the Xbox dev mode portal (https://xbox:11443/ or the local IP and port the dev mode dashboard shows you) or if on PC just double click the AppX and install then run and make sure a controller is connected.
4. Then press "Add" and drag and drop the AppX you downloaded.
5. Once done it should run.

### Developer Installation (Building from Source)

To work with the source code, ensure you have the following:

When working on the code you must have some things

- Visual Studio (Install from Microsoft if not already present)
- C++ Language Support (In the Visual Studio installer, select the `Desktop development with C++` and `Universal Windows Platform development` options)
- VCPkg support (When selecting `Desktop development with C++`, ensure `vcpkg package manager` is checked on the right side)

#### Setup

1. Open command prompt and navigate to a directory where you want to store the source and run `git clone --recurse-submodules https://github.com/3UR/Simpsons-Hit-Run`
2. When done you can open `SRR2.sln` with Visual Studio.
3. Once in Visual Studio press `Tools -> Command Line -> Developer Command Prompt`
4. A Developer Command Prompt will open enter the following `vcpkg.exe install`
5. Once that is done you may need to run `vcpkg integrate install`

#### Building

If the setup was successful, you should now be able to build any project in the solution. When building for UWP don't forget to change the config! e.g if it's `ReleaseWindows` make it `ReleaseUwp`.

## Media

> [!NOTE]
> All builds previewed in the media are built using the source on this repository.

### Windows

![Screenshot 2025-02-10 092453](https://github.com/user-attachments/assets/7b5c9c6a-259d-4e5d-bd07-e429bd2f54bb)

### Xbox

[_Watch HD on YouTube_](https://www.youtube.com/watch?v=qxqnziUVz9c)

https://github.com/user-attachments/assets/9793dccf-5dd6-4bbf-beb6-a6db33521a0b

[_Watch HD on YouTube_](https://www.youtube.com/watch?v=l_Ii-4Wygn8)

https://github.com/user-attachments/assets/ccfdb377-10ed-418b-a81b-932aad9938e1

