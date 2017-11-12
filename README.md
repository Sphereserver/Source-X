# SphereServer-eXperimental
Game server for Ultima Online.


## Join SphereServer Discord channel!
https://discord.gg/ZrMTXrs


## Why a fork?

This is an experimental fork of SphereServer. Since so many (and sometimes radical) changes were done, it was impossible to work on the main repo.<br>
Most notable changes (right now) are:
* Bug fixes and heavy changing of some internal behaviours, when needed, to achieve better speed and stability.
* New features.
* Support for CMake, which is now the standard way to generate updated build and project files.
* Support for 64 bits architecture and TDM-GCC compiler for Windows.
* Added (and still adding) comments to the code to make it more understandable.
* Reorganization of directories and files, avoiding big files with thousands of lines.
* Refactoring of the code, updating to most recent programming standards and to the conventions described below.


## Building

You need to build makefiles (and project files if you wish) with CMake for both Linux (GCC) and Windows (MSVC and TDM-GCC).<br>
Both 32 and 64 bits compilation are supported.<br>
No pre-built project files included.<br>
When generating project files, if you don't specify a toolchain (setting the `CMAKE_TOOLCHAIN_FILE` variable in the GUI or passing the CLI parameter `-DCMAKE_TOOLCHAIN_FILE="..."`),
 the CMake script will pick the 32 bits one as default.<br>
When using Unix Makefiles, you can specify a build type by setting (also this via GUI or CLI) `CMAKE_BUILD_TYPE="build"`, where build is Nightly, Debug or Release. If the build type
 was not set, by default the makefiles for all of the three build types are generated.

### Ubuntu 12.x to 16.x
Install the following packages:
```
sudo apt-get install git
sudo apt-get install libmysql++ libmysql++-dev libmysqld-dev libmysqlclient-dev
```
If you are on a 64 bits architecture but you want to compile (or execute) a 32 bits binary, you will need to
 install the MySQL packages adding the postfix `:i386` to each package name.

### CentOS 6 / 7 - Red Hat 6 / 7 - Fedora 26
If you're using CentOS 7, Red Hat 7 or Fedora 26, the default package repository only have support to MariaDB instead MySQL. So you need to add the repo for MySQL:<br>
For CentOS and Red Hat: `sudo rpm -Uvh https://dev.mysql.com/get/mysql57-community-release-el7-9.noarch.rpm`<br>
For Fedora: `sudo rpm -Uvh https://dev.mysql.com/get/mysql57-community-release-fc26-10.noarch.rpm`<br>
Then install the required packages via yum (CentOS or RH) or dnf (Fedora): `git gcc-c++ glibc-devel mysql-community-devel mysql-community-libs`<br>
<br>If you are on a 64 bits architecture but you want to compile (or execute) a 32 bits binary, you will need to install the appropriate gcc package
 and to install the MySQL packages adding the postfix `.i686` to each package name.


## Running

### Required libraries (Windows):
`dbghelp.dll`: Newer OS versions have it by default on system folders so don't mess with it, but for some old OS you may need it, 
 so there is an old one (32 bits) in /dlls/dbghelp.dll.<br>
`libmysql.dll`: Placed on /dlls/x86/libmysql.dll for 32 bits builds or in /dlls/x86_64/libmysql.dll for 64 bits builds.


## Coding Notes (add as you wish to standardize the coding for new contributors)

* Make sure you can compile and run the program before pushing a commit.
* Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.
* Removing/Changing/Adding anything that was working in one way for years should be followed by an ini setting if the changes
  cannot be replicated from script to keep some backwards compatibility.
* Comment your code, add informations about its logic. It's very important since it helps the other to understand your work.
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
<br>
* Pointer variables should have as first prefix "p".
* Unsigned variables should have as first (or second to "p") prefix "u".
* Boolean variables should have the prefix "f".
* Classes need to have the first letter uppercase and the prefix "C".
* Internal (mostly private) variables of a class or struct need to have the prefix "m_".
* After the prefix, the descriptive name should begin with an upper letter.
<br>
<b>Variables meant to hold numerical values:</b>
* For char, short, int, long, llong, use the prefix: "i".
* For byte, word and dword use respectively the prefixes: "b", "w", "dw". Do not add the unsigned prefix.
* For float and double, use the prefix: "r".
<br>
<br>
<b>Variables meant to hold characters (also strings):</b>
* For char, wchar, tchar use respectively the prefixes "c", "wc", "tc".
* "lpstr", "lpcstr", "lpwstr", "lpcwstr", "lptstr", "lpctstr" data types are preferred aliases when handling strings.
<br>
<br>
Examples:
* Class or Struct: "CChar".
* Class internal variable, integer: "m_iAmount".
* Tchar pointer: "ptcName".
* Dword: "dwUID".

### Coding Style Conventions
* Indent with tabs of size 4.
* Use the Allman indentation style:<br>
while (x == y)<br>
{<br>
&nbsp;&nbsp;&nbsp;&nbsp;something();<br>
&nbsp;&nbsp;&nbsp;&nbsp;somethingelse();<br>
}
* When a single statement follows the if/else clauses, avoid the use of brackets:<br>
if (bTrue)<br>
&nbsp;&nbsp;&nbsp;&nbsp;g_Log.EventWarn("True!\n");


## Licensing

Copyright 2017 SphereServer development team.<br>

Licensed under the Apache License, Version 2.0 (the "License").<br>
You may not use any file of this project except in compliance with the License.<br>
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0