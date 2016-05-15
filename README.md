# SphereServer
Game server for UO

## Join gitter chat
[![Join the chat at https://gitter.im/Sphereserver/Source](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Sphereserver/Source)

## Building
You need to build makefiles (and project files if you wish) with CMake for both Linux (GCC) and Windows (MSVC and TDM-GCC).
Both 32 and 64 bits compilation are supported.
No pre-built project files included.

### Ubuntu

Install mysql library:
* sudo apt-get install libmysqld-dev
* sudo apt-get install libmysql++ libmysql++-dev
* sudo apt-get install libmysqlclient-dev:i386
If you want to compile to a 64 bits architecture, you'll need also:
* sudo apt-get install libmysqlclient-dev:x86_64

## Running
Required libraries:
	dbghelp.dll Newer OS have it by default on system folders so don't mess with it, but for some old OS you may need it, there is an old one placed on /required libraries/dbghelp.dll.
	libmysql.dll placed on /required libraries/x86/libmysql.dll for 32 bits builds or in /required libraries/x86_64/libmysql.dll for 64 bits builds.

## Coding Notes (add as you wish to standardize the coding for new contributors)

* Make sure you can compile and run the program before pushing a commit.
* Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.
* Removing/Changing/Adding anything that was working in one way for years should be followed by an ini setting if the changes cannot be replicated from script to keep some backwards compatibility.
* Be sure to use Sphere's custom datatypes and the string formatting macros described in src/common/datatypes.h.
* When casting numeric data types, always prefer C-style casts, like (int), to C++ static_cast<int>().

## Licensing
Copyright 2016 SphereServer development team.

Licensed under the Apache License, Version 2.0 (the "License").<br>
You may not use any file of this project except in compliance with the License.<br>
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0