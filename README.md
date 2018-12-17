![Grin++](https://github.com/GrinPlusPlus/GrinPlusPlus/blob/master/Logo.png "Grin++")
## A Windows-Compatible C++ Implementation of the Grin(MimbleWimble) Protocol

This project is currently in progress. Although expected to be ready before mainnet (January 15th), this will be nowhere near as well tested as the official Grin Linux/MacOSX implementation. **USE AT YOUR OWN RISK.**

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

Replace <CONFIG> with `Debug` or `Release`
**Command Line**:
1. ```Open "Developer Command Prompt for Visual Studio 2017"```
2. ```cd C:/Choose/A/Path```
3. ```git clone https://github.com/GrinPlusPlus/GrinPlusPlus.git```
4. ```cd GrinPlusPlus```
5. ```mkdir build & cd build```
6. ```cmake -G "Visual Studio 15 2017 Win64" ..```
7. ```cmake --build . --config <CONFIG>```

Once your code is built, you can just open Server.exe from your bin folder.

### Project Status
#### Node
The following is a non-comprehensive list of items that still need addressing:

**MUST HAVE:**
* TxPool not implemented, & Dandelion not yet supported
* RangeProof & ProofOfWork Validation
* Finish TxHashSet Validation & Block/Tx Validation
* REST and/or RPC endpoints not implemented
* Compaction not yet implemented
* Need to memory map files for significant performance improvement
* Accept incoming connections
* Robust error handling. Crashing when peers send bad data seems like a less than stellar plan :grinning:
* Overhaul peer management (Peer database, preferred peers, request peers when low etc.)
* Better logging
* Document errything
* Tweak thread sleeps, socket options, timeouts, rocksdb options, etc.
* Functional tests for MMRs

**NICE TO HAVE:**
* CMake project setup not following best practices
* PMMR could still benefit from additional refactoring
* Batch DB commits not really implemented
* Large re-orgs not yet handled
* Track peer stats
* Support IPv6
* Determine appropriate peer banning guidelines
* Current lock situation not ideal - Distinguishing between read and write locks would be a huge performance gain. Clearly identify critical sections in the code.
* Logging - Use macros to skip building log string when log level lower than setting.

#### Wallet - Not Implemented Yet
This will be implemented. Not yet sure which features & communication methods will be supported initially. Hoping to have a nice UI for this, as well.

#### Miner - Not Implemented Yet
I'm willing to try integrating an existing miner. Who wants to send me a 1080ti so I can actually test my own code? :grinning: