# Compile **xmr-stak** for Windows

## Install Dependencies

### Preparation

- Open a command line (Windows key + r) and enter `cmd`
- Execute `mkdir C:\xmr-stak-dep`

### Visual Studio Community 2017

- Download and install [Visual Studio Community 2017](https://www.visualstudio.com/downloads/)
- During install choose following components:
  - `Desktop development with C++` (left side)
  - `VC++ 2015.3 v140 toolset for desktop` (right side - **NOT** needed for AMD GPU)
  - Since release of VS2017 15.5 (12/04/17), require `VC++ 2017 version 15.4 v14.11 toolset` (under tab `Individual Components`, section `Compilers, build tools, and runtimes`)

### CMake for Win64

- Download and install latest version from https://cmake.org/download/
- Tested version: [cmake 3.9](https://cmake.org/files/v3.9/cmake-3.9.0-rc3-win64-x64.msi)
- During install choose option: `Add CMake to the system PATH for all users`


### AMD APP SDK 3.0 (only needed for AMD GPUs)

- Download and install the latest version from https://www.dropbox.com/s/gq8vqhelq0m6gj4/AMD-APP-SDKInstaller-v3.0.130.135-GA-windows-F-x64.exe
  (do not wonder why it is a link to a dropbox but AMD has removed the SDK downloads, see https://community.amd.com/thread/222855)

### Dependencies OpenSSL and Microhttpd
- For AMD GPUs, CPU:
  - Download version 2 of the precompiled binary from https://github.com/fireice-uk/xmr-stak-dep/releases/download/v2/xmr-stak-dep.zip
  - Version 2 of the pre-compiled dependencies is not compatible with Visual Studio Toolset v140
- Extract archive to `C:\xmr-stak-dep`

### Validate the Dependency Folder

- Open a command line (Windows key + r) and enter `cmd`
- Execute
   ```
   cd c:\xmr-stak-dep
   tree .
   ```
- You should see something like this:
  ```
    C:\xmr-stak-dep>tree .
    Folder PATH listing for volume Windows
    Volume serial number is XX02-XXXX
    C:\XMR-STAK-DEP
    ├───libmicrohttpd
    │   ├───include
    │   └───lib
    └───openssl
        ├───bin
        ├───include
        │   └───openssl
        └───lib
  ```

## Compile

- Download xmr-stak [Source Code.zip](https://github.com/fireice-uk/xmr-stak/releases) and save to a location in your home folder (C:\Users\USERNAME\)
- Extract `Source Code.zip` (e.g. to `C:\Users\USERNAME\xmr-stak-<version>`)
- Open a command line (Windows key + r) and enter `cmd`
- Go to extracted source code directory (e.g. `cd C:\Users\USERNAME\xmr-stak-<version>`)
- Execute the following commands (NOTE: path to Visual Studio Community 2017 can be different)
  ```

  "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsMSBuildCmd.bat"
  ```
- Sometimes Windows will change the directory to `C:\Users\USERNAME\source\` instead of `C:\Users\USERNAME\xmr-stak-<version>\`. If that's the case execute `cd C:\Users\USERNAME\xmr-stak-<version>` followed by:
  ```
  mkdir build

  cd build

  set CMAKE_PREFIX_PATH=C:\xmr-stak-dep\libmicrohttpd;C:\xmr-stak-dep\openssl
  ```

### CMake

- See [build options](https://github.com/fireice-uk/xmr-stak/blob/master/doc/compile.md#build-system) to enable or disable dependencies.
- For AMD GPUs, CPU execute: `cmake -G "Visual Studio 15 2017 Win64" -T v141,host=x64 ..`
- Then execute
  ```
  cmake --build . --config Release --target install

  cd bin\Release

  copy C:\xmr-stak-dep\openssl\bin\* .
  ```
- Miner is by default compiled for AMD GPUs (if the AMD APP SDK is installed) and CPUs.
