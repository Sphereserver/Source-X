#!/usr/bin/sh
clang-tidy -p ../build/compile_commands.json -checks='-*,clang-analyzer*' ../src/**/*.cpp ../src/**/*.h
