# SphereServer X

Ultima Online game server, developed in C++.
<br><br>
[![GitHub License](https://img.shields.io/github/license/Sphereserver/Source-X?color=blue)](https://github.com/Sphereserver/Source-X/blob/master/LICENSE)
&nbsp; &nbsp; [![GitHub Repo size](https://img.shields.io/github/repo-size/Sphereserver/Source-X.svg)](https://github.com/Sphereserver/Source-X/)
&nbsp; &nbsp; [![GitHub Stars](https://img.shields.io/github/stars/Sphereserver/Source-X?logo=github)](https://github.com/Sphereserver/Source-X/stargazers)
<br>
[![Coverity Scan Build Status](https://scan.coverity.com/projects/20225/badge.svg)](https://scan.coverity.com/projects/sphereserver-source-x)
&nbsp; &nbsp; [![GitHub Issues](https://img.shields.io/github/issues/Sphereserver/Source-X.svg)](https://github.com/Sphereserver/Source-X/issues)

| Join the SphereServer Discord channel! |
| :---: |
| [![Discord Shield](https://discordapp.com/api/guilds/354358315373035542/widget.png?style=shield)](https://discord.gg/ZrMTXrs) |

## Getting Started tutorial
If you're new to the Sphere server and want to setup your first shard, [this is your go-to guide](docs/Getting-started.md)!

## Releases

### **Core**

| Branch: Master <br> (most stable pre-releases) | Branch: Dev <br> (most recent, potentially unstable) |
| :--- | :--- |
| [![GitHub last commit on Master branch](https://img.shields.io/github/last-commit/Sphereserver/Source-X/master.svg)](https://github.com/Sphereserver/Source-X/) &nbsp; <a href="https://github.com/Sphereserver/Source-X/blob/master/Changelog.txt">Changelog</a> | [![GitHub last commit on Dev branch](https://img.shields.io/github/last-commit/Sphereserver/Source-X/dev.svg)](https://github.com/Sphereserver/Source-X/tree/dev) &nbsp; <a href="https://github.com/Sphereserver/Source-X/blob/dev/Changelog.txt">Changelog</a> |
| **Nightly builds:** | **Nightly builds:** |
| [![Build status: Windows x86_64](https://github.com/Sphereserver/Source-X/actions/workflows/build_win_x86_64.yml/badge.svg?branch=master)](https://github.com/Sphereserver/Source-X/actions/workflows/build_win_x86_64.yml?query=branch%3Amaster) [![Build status: Windows x86](https://github.com/Sphereserver/Source-X/actions/workflows/build_win_x86.yml/badge.svg?branch=master)](https://github.com/Sphereserver/Source-X/actions/workflows/build_win_x86.yml?query=branch%3Amaster) | [![Build status: Windows x86_64](https://github.com/Sphereserver/Source-X/actions/workflows/build_win_x86_64.yml/badge.svg?branch=dev)](https://github.com/Sphereserver/Source-X/actions/workflows/build_win_x86_64.yml?query=branch%3Adev) [![Build status: Windows x86](https://github.com/Sphereserver/Source-X/actions/workflows/build_win_x86.yml/badge.svg?branch=dev)](https://github.com/Sphereserver/Source-X/actions/workflows/build_win_x86.yml?query=branch%3Adev)|
| [![Build status: Linux x86_64](https://github.com/Sphereserver/Source-X/actions/workflows/build_linux_x86_64.yml/badge.svg?branch=master)](https://github.com/Sphereserver/Source-X/actions/workflows/build_linux_x86_64.yml) [![Build status: Linux x86](https://github.com/Sphereserver/Source-X/actions/workflows/build_linux_x86.yml/badge.svg?branch=master)](https://github.com/Sphereserver/Source-X/actions/workflows/build_linux_x86.yml?query=branch%3Amaster) | [![Build status: Linux x86_64](https://github.com/Sphereserver/Source-X/actions/workflows/build_linux_x86_64.yml/badge.svg?branch=dev)](https://github.com/Sphereserver/Source-X/actions/workflows/build_linux_x86_64.yml?query=branch%3Adev) [![Build status: Linux x86](https://github.com/Sphereserver/Source-X/actions/workflows/build_linux_x86.yml/badge.svg?branch=dev)](https://github.com/Sphereserver/Source-X/actions/workflows/build_linux_x86.yml?query=branch%3Adev) |
| [![Build status: MacOS x86_64](https://github.com/Sphereserver/Source-X/actions/workflows/build_osx_x86_64.yml/badge.svg?branch=master)](https://github.com/Sphereserver/Source-X/actions/workflows/build_osx_x86_64.yml?query=branch%3Amaster) [![Build status: MacOS ARM](https://github.com/Sphereserver/Source-X/actions/workflows/build_osx_arm.yml/badge.svg?branch=master)](https://github.com/Sphereserver/Source-X/actions/workflows/build_osx_arm.yml?query=branch%3Amaster) | [![Build status: MacOS x86_64](https://github.com/Sphereserver/Source-X/actions/workflows/build_osx_x86_64.yml/badge.svg?branch=dev)](https://github.com/Sphereserver/Source-X/actions/workflows/build_osx_x86_64.yml?query=branch%3Adev) [![Build status: MacOS ARM](https://github.com/Sphereserver/Source-X/actions/workflows/build_osx_arm.yml/badge.svg?branch=dev)](https://github.com/Sphereserver/Source-X/actions/workflows/build_osx_arm.yml?query=branch%3Adev) |

**Click the badges or follow the links:**

+ <a href="https://github.com/Sphereserver/Source-X/releases">Github Nightly builds</a>
+ <a href="https://forum.spherecommunity.net/sshare.php?srt=4">SphereServer Website</a>

### **ScriptPack**

The official script pack is fully compatible with X new syntax, has all the new X features and preserves legacy/classic systems, which can be activated back in place
of the new ones.
<br>It is currently being revamped to add original OSI features.
<br>**Beware, it's still not 100% complete!**

+ <a href="https://github.com/Sphereserver/Scripts-X">Current (most updated)</a>
+ <a href="https://github.com/Sphereserver/Scripts-X/releases">Milestone releases</a>

## Resources

+ <a href="https://wiki.spherecommunity.net/">Scripting guide</a>
+ <a href="https://forum.spherecommunity.net/sshare.php">ScriptShare</a>
+ <a href="https://www.sphereserver.net/">SphereCommunity website</a>

### Coming from a different SphereServer version?

+ From 0.56d? <a href="docs/Porting%20from%200.56%20to%20X.txt">Here</a> a list of major scripting changes!
+ From an older 0.55 or 0.56 version? <a href="docs/Porting%20from%200.55%20to%200.56.txt">This</a> might help resuming major changes until 0.56d.

## Why the fork?

This branch started in 2016 from a slow and radical rework of SphereServer 0.56d, while trying to preserve script compatibility with the starting branch.
Though, something has changed script-wise, so we suggest to take a look <a href="docs/Porting%20from%200.56%20to%20X.txt">here</a>.
Most notable changes (right now) are:

+ Bug fixes and heavy changing of some internal behaviours, with the aim to achieve truly better **speed** and **stability**;
+ Support for x86_64 (64 bits) and ARM architectures, Mac OSX builds, Clang and GCC Linux builds, MinGW compiler for Windows;
+ CMake is now the standard way to generate updated build and project files;
+ Switched from MySQL 5.x to MariaDB client;
+ Added (and still adding) comments to the code to make it more understandable;
+ Reorganization of directories and files, avoiding big files with thousands of lines;
+ Code refactoring, updating to most recent programming standards and to the conventions described below.

## Running

### Required libraries (Windows)

+ `libmariadb.dll` (MariaDB Client v10.\* package), found in `lib/bin/*cpu_architecture*/mariadb/libmariadb.dll`

### Required libraries (Linux)

+ MariaDB Client library. Get it from the following sources.
  + From MariaDB website  
    See <https://mariadb.com/docs/skysql/connect/clients/mariadb-client/>
  + Ubuntu and Debian repositories  
    Ubuntu: Enable "universe" repository: `sudo add-apt-repository universe`  
    Install MariaDB client: `sudo apt-get install mariadb-client` or `sudo apt-get install libmariadb3` (depends on the OS version)
  + CentOS - Red Hat Enterprise Linux - Fedora repositories  
    Then install MariaDB client via yum (CentOS or RH) or dnf (Fedora): `mariadb-connector-c`

### Required libraries (MacOS)

+ Install MariaDB Client library via `brew install mariadb-connector-c`

## Building the server from the source

### Generating the project files

Only recent compilers are supported, since Sphere uses C++20 features: the newer the compiler, the better. Oldest compiler versions supporting C++20: Visual Studio 2019 version 16.11, GCC 8, MinGW distributions using GCC 8, Clang version 10.  
No pre-built project files are included. You need to build Visual Studio solution (.sln), Makefiles or Ninja Build files (or project files for other IDE or build systems) with CMake.  
Below there's is a small guide.  
You can use CMake GUI (graphical interface) or its CLI (command line interface).  
To pass a flag to using the CLI, just prepend `-D` to the name of the variable, then assign the value with the equal sign, eg: `-DFOO=TRUE`.  
  
Before starting: does CMake give you an error? Ensure that you have Git installed, and if you are on Windows ensure also that the Git executable was added to the PATH environmental variable
 (you'll need to add it manually if you are using Git Desktop,
 <a href="https://stackoverflow.com/questions/26620312/installing-git-in-path-with-github-client-for-windows?answertab=votes#tab-top">here's a quick guide</a>).

+ Select a **Generator**  
  Tells CMake which kind of project file to generate (Visual Studio, makefile, ninja build file...)
  + GUI: it's the first thing CMake will ask you (well, after Sphere source directory)
  + CLI: use -G flag, example: `-G "Visual Studio 17 2022"` or `-G Ninja`.

+ Select a **Toolchain**  
  When generating project files, the easiest way is to use the provided OS-specific CMake toolchain file to automatically pass the right options to the selected compiler.  
  You can find them in the `cmake/toolchains/` folder.  
  If you don't specify a toolchain, CMake will pick the native one as default.  
  How to set a toolchain:
  + Via CMake GUI: when configuring for the first time the project, choose "Specify toolchain file for cross-compiling", then on the next step you'll be allowed to select the toolchain file.
  + Via CMake CLI (command line interface): pass the parameter `-DCMAKE_TOOLCHAIN_FILE="..."`
  
+ Build **Configuration** (type)  
  When using Makefiles or Ninja, you can specify a build type by setting (also this via GUI or CLI) `CMAKE_BUILD_TYPE="build"`, where build is **Nightly**, **Debug** or **Release**. If the build type  was not set, by default the makefiles for all of the three build types are generated.  
  **Debug** build is expected to be slow and it's to be used, you guessed it, for debugging purposes (best coupled with a debugger or with sanitizers enabled, more on them right below), so don't use it for a live shard!  

Other useful CMake flags:

+ You can add other compiler flags with the custom variables `C_FLAGS_EXTRA`, `CXX_FLAGS_EXTRA`, `CMAKE_EXE_LINKER_FLAGS_EXTRA`.
+ Enable Sanitizers: `USE_ASAN[=ON]`, `USE_UBSAN`, `USE_LSAN`, etc.
  If you are using Address Sanitizer, Undefined Behaviour Sanitizer, Leak Sanitizer on Windows, it might be useful to redirect stderr to stdout to correctly show the output: `SphereSvrX64_nightly > Sphere_ASan_log.txt 2>&1`.
  Remember to setup Sanitizers settings by setting the respective shell variables. To ease this, there are some batch/shell scripts doing that for you inside `utilities/`.
+ `CROSSCOMPILING_ARCH`: set this to TRUE to tell the compiler you are building binary files for a different architecture (not from x86_64 to x86, but for example from x86 to ARM).

Example to build makefiles on Linux for a 64 bits Nightly version, inside the "build" directory (run the command inside the project's root folder):  

```bash
mkdir build
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/Linux-GNU-x86_64.cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Nightly" -B ./build -S ./
```

### Compiling

#### Installing the required packages on Linux

Building will require more packages than the ones needed to run Sphere.

+ Ubuntu and Debian  
  Install these additional packages:
  + Build tools (other than the compiler): `sudo apt-get install git cmake`.
  + MariaDB client: `sudo apt-get install libmariadb-dev` and  `libmariadb3` or `mariadb-client` (depends on the OS version)  
  If you are on a 64 bits architecture but you want to compile (or execute) a 32 bits binary, you will need to
  install MariaDB packages adding the postfix `:i386` to each package name.

+ CentOS - Red Hat Enterprise Linux - Fedora
  Install these additional packages via yum (CentOS or RH) or dnf (Fedora).
  + Build tools (other than the compiler): `git cmake glibc-devel`
  + MariaDB client: `mariadb-connector-c mariadb-connector-c-devel`  
  If you are on a 64 bits architecture but you want to compile (or execute) a 32 bits binary, you will need to install MariaDB packages adding the postfix `.i686` to each package name.

#### Compiling on Linux

Just run `cmake --build .` in the build directory (the one where you have asked CMake to create its files).

## Coding Notes (add as you wish to standardize the coding for new contributors)

+ Make sure you can compile and run the program before pushing a commit.
+ Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.
+ Removing/Changing/Adding anything that was working in one way for years should be followed by an ini setting if the changes
  cannot be replicated from script to keep some backwards compatibility.
+ Comment your code, add informations about its logic. It's very important since it helps others to understand your work.
+ Be sure to use Sphere's custom datatypes and the string formatting macros described in src/common/datatypes.h.
+ When casting numeric data types, always prefer C-style casts, like (int), to C++ static_cast&lt;int&gt;(). It's way more concise.
+ Be wary that in SphereScript unsigned values does not exist, all numbers are considered signed, and that 64 bits integers meant
  to be printed to or retrieved by scripts should always be signed.
+ Don't use "long" except if you know why do you actually need it. Always prefer "int" or "llong".
  Use fixed width variables only for values that need to fit a limited range.
+ For strings, use pointers:  
  to "char" for strings that should always have ASCII encoding;  
  to "tchar" for strings that may be ASCII or Unicode, depending from compilation settings (more info in "datatypes.h");  
  to "wchar" for string that should always have Unicode encoding.

### Naming Conventions

These are meant to be applied to new code and, if there's some old code not following them, it would be nice to update it.

+ Pointer variables should have as first prefix "p".
+ Unsigned variables should have as first (or second to "p") prefix "u".
+ Boolean variables should have the prefix "f" (it stands for flag).
+ Classes need to have the first letter uppercase and the prefix "C".
+ Private or protected methods (functions) and members (variables) of a class or struct need to have the prefix "\_". This is a new convention, the old one used the "m\_" prefix for the members.
+ Constants (static const class members, to be preferred to preprocessor macros) should have the prefix "k".
+ After the prefix, the descriptive name should begin with an upper letter.

**Variables meant to hold numerical values:**

+ For char, short, int, long, llong, use the prefix: "i" (stands for integer).
+ For byte, word and dword use respectively the prefixes: "b", "w", "dw". Do not add the unsigned prefix.
+ For float and double, use the prefix: "r" (stands for real number).

**Variables meant to hold characters (also strings):**

+ For char, wchar, tchar use respectively the prefixes "c", "wc", "tc".
+ When handling strings, "lpstr", "lpcstr", "lpwstr", "lpcwstr", "lptstr", "lpctstr" data types are preferred aliases.  
  You'll find a lot of "psz" prefixes for strings: the reason is that in the past Sphere coders wanted to be consistent with Microsoft's Hungarian Notation.  
  The correct and up to date notation is "pc" for lpstr/lpcstr (which are respectively char*and const char*), "pwc" (wchar*and const wchar*),
  "ptc" for lptstr/lpctstr (tchar*and const tchar*).  
  Use the "s" or "ps" (if pointer) when using CString or std::string. Always prefer CString over std::string, unless in your case there are obvious advantages for using the latter.

Examples:

+ Class or Struct: "CChar".
+ Class internal variable, signed integer: "_iAmount".
+ Tchar pointer: "ptcName".
+ Dword: "dwUID".

### Coding Style Conventions

+ Indent with **spaces** of size 4.
+ Use the Allman indentation style:  
while (x == y)  
{  
&nbsp;&nbsp;&nbsp;&nbsp;something();  
&nbsp;&nbsp;&nbsp;&nbsp;somethingelse();  
}

+ Even if a single statement follows the if/else/while... clauses, use the brackets:  
if (fTrue)  
{  
&nbsp;&nbsp;&nbsp;&nbsp;g_Log.EventWarn("True!\n");  
}

## Licensing

Copyright 2025 SphereServer development team.  

Licensed under the Apache License, Version 2.0 (the "License").  
You may not use any file of this project except in compliance with the License.  
You may obtain a copy of the License at <http://www.apache.org/licenses/LICENSE-2.0>
