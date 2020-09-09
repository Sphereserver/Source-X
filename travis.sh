#!/bin/bash

set -x # expand and echo commands
export SPHERE_TRAVIS=1

echo TRAVIS_BUILD_DIR is $TRAVIS_BUILD_DIR
echo TRAVIS_OS_NAME is $TRAVIS_OS_NAME

if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    export HOMEBREW_NO_AUTO_UPDATE=1
    brew install ninja
    brew install mysql-client
    gcc -v && g++ -v && cmake --version
    echo Building Release
    mkdir -p release && cd release && cmake -G Ninja ../src -DCMAKE_BUILD_TYPE=Release $EXTRA && cmake --build . || exit 1
    cd ..
    echo Building $BUILD package
    #zip -r sphereX64-osx-$BUILD.zip SphereSvrX64_release
fi
