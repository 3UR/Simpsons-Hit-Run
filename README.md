# The Simpsons: Hit & Run

This repository contains the source code for _The Simpsons: Hit & Run_ with quite a few enhancements done to it, Such as C++ 20 & x64 Builds, VCPkg for easier third-party 
library management, UWP Builds & more. 


## Issues

If you encounter issues, please [create an issue](https://github.com/3UR/Simpsons-Hit-Run/issues/new).

## Contributing

If you would like to contribute to this project please create a fork and then open a pull request.

## Known Issues

- Memory Leak (Should be easier to debug with the new Visual Studio 2026 profiling tools but sadly we can't build for VS 2026 as of right now.)
- Asset missing with the current released builds (I have assets that are good to go just need to some how slim them down and fit them into the repo & update the release)

## Fair Use Disclaimer

This repository and build distributations contains a modified version of _The Simpsons: Hit & Run_, using its original source code and assets strictly for non-commercial, educational, and archival purposes under fair use. There is no intent to cause harm to the original work or its market or make money of it.

- **Restrictions [If you are using this source or the builds]:** You are not allowed to sell or commercially distribute this in any form. Anyone attempting to do so will be blocked if found and reported to wherever it is distributed.
- **Purpose:** Enhancement, preservation, and research.
- **Use of Copyrighted Material:** Assets and source code are used strictly for modifications and improvements.
- **No Commercial Intent:** This project is not sold, monetized, or distributed for profit. Donations are not accepted.


## Installation

> [!WARNING]  
> There is a chance you will encounter memory leaks and (if you want to build the source) build issues with Visual Studio 2026 these will eventually be actively worked on but I currently don't have the time to work on this project.

### Quick Installation (Pre-built)

### Windows

1. Download the latest build from the [Releases page](https://github.com/3UR/Simpsons-Hit-Run/releases/latest).
2. Extract the contents of the zip file to your desired location.
3. Navigate to the extracted folder and locate `SRR2.exe`.
4. You can now run the game. (Note: The game will crash without the required assets)

### Xbox Series S|X

> [!NOTE]  
> Xbox One is currently not supported and will crash Xbox Series S|X is fine.

1. Navigate to the [Releases page](https://github.com/3UR/Simpsons-Hit-Run/releases/latest).
2. Download a file that looks like `SRR2_UWP_X.X.X.X_x64_XXX.appx`.
3. Now navigate to the Xbox dev mode portal (https://xbox:11443/ or the local IP and port the dev mode dashboard shows you).
4. Then press "Add" and drag and drop the AppX you downloaded.
5. Once done it should run.

### Developer Installation (Building from Source)

When working with the source you will need these installed:

- Visual Studio (2022 Preferred)
- C++ Language Support (In the Visual Studio installer, select the `Desktop development with C++` and `Universal Windows Platform development` options)
- VCPkg support (When selecting `Desktop development with C++`, make sure `vcpkg package manager` is selected on the right side)

#### Setup

1. Open command prompt and navigate to a directory where you want to store the source and run `git clone --recurse-submodules https://github.com/3UR/Simpsons-Hit-Run`
2. When done you can open `SRR2.sln` with Visual Studio.
3. Once in Visual Studio press `Tools -> Command Line -> Developer Command Prompt`
4. A Developer Command Prompt will open enter the following `vcpkg.exe install`
5. Once that is done you may need to run `vcpkg integrate install`

#### Building

If the setup was successful, you should now be able to build any project in the solution. When building for UWP don't forget to change the config! (example: if it's `ReleaseWindows` make it `ReleaseUwp`).

## Media

> [!NOTE]
> All builds previewed in the media are built using the source on this repository.

### Windows

![Screenshot 2025-02-10 092453](https://github.com/user-attachments/assets/7b5c9c6a-259d-4e5d-bd07-e429bd2f54bb)

### Xbox

![A](https://github.com/user-attachments/assets/9793dccf-5dd6-4bbf-beb6-a6db33521a0b)

![B](https://github.com/user-attachments/assets/ccfdb377-10ed-418b-a81b-932aad9938e1)

