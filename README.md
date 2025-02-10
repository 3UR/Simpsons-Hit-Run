# SRR2

> [!WARNING]  
> THIS DOES NOT YET SUPPORT UWP

SRR2 is a cleaner version of *The Simpsons Hit & Run* with several changes, including C++ 17 support, x64 (WIP), and VCPkg for easier third party library installation. This is based on the source code that leaked in 2021, the end goal is to add more feature, improvements, and UWP support.

## Installation

> [!NOTE]
> Assets are too big to include in the repo, You can get them from (TODO) and drag and dropthem into the build folder.

### I dont want to mess around with code I just want a build

1. Download the latest build [Here](https://github.com/3UR/Simpsons-Hit-Run/releases/latest).
2. You will see a zip similar to `SRR2-Debug-x86-2025.02.09-15.21.49-07cbce9.zip` download it.
3. Open the zip and then open the file named the same with no extension (this is also a zip the workflow for releases is still WIP so its structured bad) extract it anywhere you want.
4. Go to where you extracted the zip and find `SRR2.exe`.
5. Download the assets from above and drag and drop them where `SRR2.exe` is.
6. You can run the game now. It may crash often though as it is unstable as of `2/11/2025`.


### I want to work on the code and create my own builds

#### Developer Environment

When working on the code you must have some things

- Visual Studio (Install from Microsoft if not installed)
- C++ Language Support (Check the `Desktop development with C++` and `Universal Windows Platform development` options in the Visual Studio installer)
- VCPkg support (When checking `Desktop development with C++` make sure `vcpkg package manager` on the right side is selected)

#### Setup

Once you have the dev environment you will have to

1. Open command prompt where you want to clone the source code and run `git clone https://github.com/3UR/Simpsons-Hit-Run`
2. Open `SRR2.sln` with Visual Studio
3. Once in Visual Studio press `Tools -> Command Line -> Developer Command Prompt`
4. A Developer Command Prompt will open enter the following `vcpkg.exe install` (Wait for this to finish it will install all required dependencies)
5. Once that is done you may need to run `vcpkg integrate install`

#### Building

After all of that if everything was successfull you should now be able to build any project in the SLN (If something went wrong above [Create an Issue](https://github.com/3UR/Simpsons-Hit-Run/issues/new?template=Blank+issue))

When building the main game project `SRR2` it will be outputted in the `build` folder `bin-Win32-Debug` for example, once it is built before running make sure you download the assets and drag and drop them into this folder (The same goes when using the visual studio debugger make sure the assets are there before running the build).

## Screenshots

**builds using this source**

### Windows

![Screenshot 2025-02-10 092453](https://github.com/user-attachments/assets/7b5c9c6a-259d-4e5d-bd07-e429bd2f54bb)
