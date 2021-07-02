# SphereServer X version
Game server for Ultima Online.
<br>
[![Build status](https://ci.appveyor.com/api/projects/status/ab152o83mipjojin?svg=true)](https://ci.appveyor.com/project/cbnolok/sphereserver-x)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/20225/badge.svg)](https://scan.coverity.com/projects/sphereserver-source-x)
<br><a href="https://forum.spherecommunity.net/sshare.php?srt=4">Get the automated builds here!</a>


### Join SphereServer Discord channel!
https://discord.gg/ZrMTXrs


### Use the specific script pack!
The official script pack is fully compatible with X new syntax, has all the new X features and preserves legacy/classic systems, which can be activated back in place
of the new ones.<br>
Beware, it's still not 100% complete!<br>
https://github.com/cbnolok/SphereX-Scriptpack


### Coming from a different SphereServer version?
* From 0.56d? <a href="https://github.com/Sphereserver/Source-X/blob/master/docs/Porting%20from%200.56%20to%20X.txt">Here</a> a list of major scripting changes!
* From an older 0.56 version? <a href="https://github.com/Sphereserver/Source-X/blob/master/docs/Porting%20from%200.55%20to%200.56.txt">This</a> might help resuming major changes until 0.56d.


## Why a fork?
This branch started in 2016 from a slow and radical rework of SphereServer 0.56d, while trying to preserve script compatibility with the starting branch.<br>
Though, something has changed script-wise, so we suggest to take a look <a href="https://github.com/Sphereserver/Source-X/blob/master/docs/Porting%20from%200.56%20to%20X.txt">here</a>.<br>
Most notable changes (right now) are:
* Bug fixes and heavy changing of some internal behaviours, with the aim to achieve truly better **speed** and **stability**;
* Support for 64 bits architecture and MinGW compiler for Windows;
* CMake is now the standard way to generate updated build and project files;
* Added (and still adding) comments to the code to make it more understandable;
* Reorganization of directories and files, avoiding big files with thousands of lines;
* Code refactoring, updating to most recent programming standards and to the conventions described below.


## Running

### Required libraries (Windows):
* `libmysql.dll`: Placed on /DLLs/32/libmysql.dll for 32 bits builds or in /DLLs/64/libmysql.dll for 64 bits builds.
* `dbghelp.dll`: Newer OS versions have it by default on system folders so don't mess with it, but for some old OS you may need it, 
 so there is an old one (32 bits) in /dlls/dbghelp.dll.

### Required libraries (Linux):

#### Ubuntu 14.x to 16.x
Install MySQL client: `sudo apt-get install libmysql++`

#### Debian 9
Install MySQL client (the default repository supports only the MariaDB client, so we need to install MySQL manually):<br>
For 64 bits builds:<br>
```
wget http://security.debian.org/debian-security/pool/updates/main/m/mysql-5.5/libmysqlclient18_5.5.62-0+deb8u1_amd64.deb
sudo dpkg -i libmysqlclient18_5.5.62-0+deb8u1_amd64.deb
```
For 32 bits builds:<br>
```
wget http://security.debian.org/debian-security/pool/updates/main/m/mysql-5.5/libmysqlclient18_5.5.62-0+deb8u1_i386.deb
sudo dpkg -i libmysqlclient18_5.5.62-0+deb8u1_i386.deb
```

#### CentOS 6 / 7 - Red Hat 6 / 7 - Fedora 26+
If you're using CentOS, Red Hat or Fedora, nowadays the default package repository only supports MariaDB instead of MySQL, so we need to add the repo for MySQL:<br>
* For CentOS 6 and Red Hat 6: `sudo rpm -Uvh https://dev.mysql.com/get/mysql57-community-release-el6-9.noarch.rpm`<br>
* For CentOS 7 and Red Hat 7: `sudo rpm -Uvh https://dev.mysql.com/get/mysql57-community-release-el7-9.noarch.rpm`<br>
* For Fedora 26: `sudo rpm -Uvh https://dev.mysql.com/get/mysql57-community-release-fc26-10.noarch.rpm`<br>
* For Fedora 27: `sudo rpm -Uvh https://dev.mysql.com/get/mysql57-community-release-fc27-10.noarch.rpm`<br>
Then install MySQL client via yum (CentOS or RH) or dnf (Fedora): `mysql-community-libs`<br>

## Building

### Generating the project files
The compilation of the code is possible only using recent compilers, since C++17 features are used: Visual Studio 2017 or 2015 Update 3, GCC 7.1 and later (even if GCC 6 can work, 7 is reccomended), MinGW distributions using GCC 7.1 and later (like nuwen's).<br>
You need to build makefiles (and project files if you wish) with CMake for both Linux (GCC) and Windows (MSVC and MinGW).<br>
Both 32 and 64 bits compilation are supported.<br>
No pre-built project files included.<br>
Does CMake give you an error? Ensure that you have Git installed, and if you are on Windows ensure also that the Git executable was added to the PATH environmental variable
 (you'll need to add it manually if you are using Git Desktop,
 <a href="https://stackoverflow.com/questions/26620312/installing-git-in-path-with-github-client-for-windows?answertab=votes#tab-top">here's a quick guide</a>).<br>

#### Toolchains and custom CMake variables
When generating project files, if you don't specify a toolchain, the CMake script will pick the 32 bits one as default.<br>
How to set a toolchain:
* Via CMake GUI: when configuring for the first time the project, choose "Specify toolchain file for cross-compiling", then on the next step you'll be allowed to select the toolchain file
* Via CMake CLI (command line interface): pass the parameter `-DCMAKE_TOOLCHAIN_FILE="..."`
<br>When using Unix Makefiles, you can specify a build type by setting (also this via GUI or CLI) `CMAKE_BUILD_TYPE="build"`, where build is Nightly, Debug or Release. If the build type
 was not set, by default the makefiles for all of the three build types are generated.<br>
<br />
You can also add other compiler flags, like optimization flags, with the custom variables C_FLAGS_EXTRA and CXX_FLAGS_EXTRA.<br>

Example of CMake CLI additional parameters:<br>
```
-DC_FLAGS_EXTRA="-mtune=native" -DCXX_FLAGS_EXTRA="-mtune=native"
```
(Use the -mtune=native flag only if you are compiling on the same machine on which you will execute Sphere!)

Example to build makefiles on Linux for a 64 bits Nightly version, inside the "build" directory (run it inside the project's root folder, not inside "src"):<br>
```
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../src/cmake/toolchains/Linux-GNU-x86_64.cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="Nightly" --build ./ ../src
```

### Compiling

#### Installing the required packages on Linux
Building will require more packages than the ones needed to run Sphere.
 
##### Ubuntu 14.x to 16.x and Debian 9
Install these additional packages: `sudo apt-get install git libmysql++-dev libmysqld-dev libmysqlclient-dev`<br>
If you are on a 64 bits architecture but you want to compile (or execute) a 32 bits binary, you will need to
 install the MySQL packages adding the postfix `:i386` to each package name.

##### CentOS 6 / 7 - Red Hat 6 / 7 - Fedora 26
Then install these additional packages via yum (CentOS or RH) or dnf (Fedora): `git gcc-c++ glibc-devel mysql-community-devel`<br>
<br>If you are on a 64 bits architecture but you want to compile (or execute) a 32 bits binary, you will need to install the appropriate gcc package
 and to install the MySQL packages adding the postfix `.i686` to each package name.

#### Compiling on Linux
Just run the `make` command inside the `build` folder. You can pass the -jX argument (`make -jX`, where X is a number) to speed up the compilation and split the work between X threads.
 
#### Extra: compiling with Clang (if you wish to try)
At the moment, Clang was tested only on Windows and in combination with Visual Studio 2017.<br>

##### On Windows
1. Install Clang for LLVM 6.0 (<a href="http://releases.llvm.org/download.html">here</a>) and select the option to add the bin folder to the PATH environmental variable.
2. Install via the Visual Studio installer the package "Clang/C2".
3. Install the 3rd party Visual Studio 2017 toolset for Clang from <a href="https://github.com/arves100/llvm-vs2017-integration">here</a>.
4. Run CMake using the Visual Studio 15 2017 (Win64) generator, "Windows-clang-MSVC-*.cmake" toolchain and toolset "LLVM-vs2017".

###### Address Sanitizer and Undefined Behaviour Sanitizer
You can enable Address Sanitizer (ASan) and Undefined Behaviour Sanitizer (UBSan) with the ENABLE_SANITIZERS checkbox via the GUI, or via the CLI flag `-DENABLE_SANITIZERS=true`.
 Due to limitations of Clang's ASan and UBSan on Windows, it doesn't work with the Debug build. This repository ships only the 64 bits libraries
 for LLVM 6.0's ASan and UBSan.<br>
Since ASan redirects the error output to stderr, you can retrieve its output by launching sphere from cmd (Command Prompt) with the following command: 
`SphereSvrX64_nightly > Sphere_ASan_log.txt 2>&1`


## Coding Notes (add as you wish to standardize the coding for new contributors)

* Make sure you can compile and run the program before pushing a commit.
* Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.
* Removing/Changing/Adding anything that was working in one way for years should be followed by an ini setting if the changes
  cannot be replicated from script to keep some backwards compatibility.
* Comment your code, add informations about its logic. It's very important since it helps others to understand your work.
* Be sure to use Sphere's custom datatypes and the string formatting macros described in src/common/datatypes.h.
* When casting numeric data types, always prefer C-style casts, like (int), to C++ static_cast&lt;int&gt;().
* Be wary that in SphereScript unsigned values does not exist, all numbers are considered signed, and that 64 bits integers meant
  to be printed to or retrieved by scripts should always be signed.
* Don't use "long" except if you know why do you actually need it. Always prefer "int" or "llong".
  Use fixed width variables only for values that need to fit a limited range.
* For strings, use pointers:<br>
  to "char" for strings that should always have ASCII encoding;<br>
  to "tchar" for strings that may be ASCII or Unicode, depending from compilation settings (more info in "datatypes.h");<br>
  to "wchar" for string that should always have Unicode encoding.


### Naming Conventions
These are meant to be applied to new code and, if there's some old code not following them, it would be nice to update it.
* Pointer variables should have as first prefix "p".
* Unsigned variables should have as first (or second to "p") prefix "u".
* Boolean variables should have the prefix "f" (it stands for flag).
* Classes need to have the first letter uppercase and the prefix "C".
* Private or protected methods (functions) and members (variables) of a class or struct need to have the prefix "\_". This is a new convention, the old one used the "m\_" prefix for the members.
* Constants (static const class members, to be preferred to preprocessor macros) should have the prefix "k".
* After the prefix, the descriptive name should begin with an upper letter.

**Variables meant to hold numerical values:**
* For char, short, int, long, llong, use the prefix: "i" (stands for integer).
* For byte, word and dword use respectively the prefixes: "b", "w", "dw". Do not add the unsigned prefix.
* For float and double, use the prefix: "r" (stands for real number).

**Variables meant to hold characters (also strings):**
* For char, wchar, tchar use respectively the prefixes "c", "wc", "tc".
* When handling strings, "lpstr", "lpcstr", "lpwstr", "lpcwstr", "lptstr", "lpctstr" data types are preferred aliases.<br>
  You'll find a lot of "psz" prefixes for strings: the reason is that in the past Sphere coders wanted to be consistent with Microsoft's Hungarian Notation.<br>
  The correct and up to date notation is "pc" for lpstr/lpcstr (which are respectively char* and const char*), "pwc" (wchar* and const wchar*),
  "ptc" for lptstr/lpctstr (tchar* and const tchar*).<br>
  Use the "s" or "ps" (if pointer) when using CString or std::string. Always prefer CString over std::string, unless in your case there are obvious advantages for using the latter.

<br />Examples:
* Class or Struct: "CChar".
* Class internal variable, signed integer: "_iAmount".
* Tchar pointer: "ptcName".
* Dword: "dwUID".

### Coding Style Conventions
* Indent with **spaces** of size 4.
* Use the Allman indentation style:<br>
while (x == y)<br>
{<br>
&nbsp;&nbsp;&nbsp;&nbsp;something();<br>
&nbsp;&nbsp;&nbsp;&nbsp;somethingelse();<br>
}
* Even if a single statement follows the if/else/while... clauses, use the brackets:<br>
if (fTrue)<br>
{<br>
&nbsp;&nbsp;&nbsp;&nbsp;g_Log.EventWarn("True!\n");<br>
}


## Licensing

Copyright 2021 SphereServer development team.<br>

Licensed under the Apache License, Version 2.0 (the "License").<br>
You may not use any file of this project except in compliance with the License.<br>
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
