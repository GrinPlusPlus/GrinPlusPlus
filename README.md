![Grin++](https://github.com/GrinPlusPlus/GrinPlusPlus/blob/master/Logo.png "Grin++")
## A Windows-Compatible C++ Implementation of the Grin(MimbleWimble) Protocol

### Project Status
#### Node
The node has been released on 64-bit Windows for beta-testing! Since the wallet is not yet released, running Grin++ should not put any of your grins at risk.
However, since it is only a beta release, it is not yet recommended for production environments. Additionally, expect that future releases could have backward-incompatible changes that may require deleting chain data and re-syncing.

#### Wallet - In Progress
The wallet is currently in progress, but is not expected to be ready for beta testing until mid-March.

### Build Instructions

Although CMake was chosen for future portability, the focus so far has been on Windows-only. Visual Studio 2017 with "Visual C++ Tools for CMake" is required to build.

#### Prerequisites
* A recent version of CMake
* Visual Studio 2017 64-bit with C++ 2017 Support
* OPTIONAL: Visual C++ Tools for CMake

**With Visual C++ Tools for CMake**
1. Clone `https://github.com/GrinPlusPlus/GrinPlusPlus.git` to location of your choice
2. Open Visual Studio 2017
3. File>Open>CMake...
4. Choose CMakeLists.txt from GrinPlusPlus folder
5. Choose x64-Debug or x64-Release from build configurations drop-down
6. Generate CMake Cache and Build All from the CMake menu

Replace `<CONFIG>` with `Debug` or `Release`
**Command Line**:
1. ```Open "Developer Command Prompt for Visual Studio 2017"```
2. ```cd C:/Choose/A/Path```
3. ```git clone https://github.com/GrinPlusPlus/GrinPlusPlus.git```
4. ```cd GrinPlusPlus```
5. ```mkdir build & cd build```
6. ```cmake -G "Visual Studio 15 2017 Win64" ..```
7. ```cmake --build . --config <CONFIG>```

Once your code is built, you can just open Server.exe from your bin folder.